// Copyright 2023 Adobe. All rights reserved.
// This file is licensed to you under the Apache License,
// Version 2.0 (http://www.apache.org/licenses/LICENSE-2.0)
// or the MIT license (http://opensource.org/licenses/MIT),
// at your option.

// Unless required by applicable law or agreed to in writing,
// this software is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR REPRESENTATIONS OF ANY KIND, either express or
// implied. See the LICENSE-MIT and LICENSE-APACHE files for the
// specific language governing permissions and limitations under
// each license.

use std::{
    io::{Cursor, Read, Seek, SeekFrom, Write},
    slice,
};

use crate::Error;

#[repr(C)]
#[derive(Debug)]
/// An Opaque struct to hold a context value for the stream callbacks
pub struct StreamContext {
    _priv: (),
}

#[repr(C)]
#[derive(Debug)]
/// An enum to define the seek mode for the seek callback
/// Start - seek from the start of the stream
/// Current - seek from the current position in the stream
/// End - seek from the end of the stream
pub enum C2paSeekMode {
    Start = 0,
    Current = 1,
    End = 2,
}

/// Defines a callback to read from a stream
/// The return value is the number of bytes read, or a negative number for an error
type ReadCallback =
    unsafe extern "C" fn(context: *mut StreamContext, data: *mut u8, len: isize) -> isize;

/// Defines a callback to seek to an offset in a stream
/// The return value is the new position in the stream, or a negative number for an error
type SeekCallback =
    unsafe extern "C" fn(context: *mut StreamContext, offset: isize, mode: C2paSeekMode) -> isize;

/// Defines a callback to write to a stream
/// The return value is the number of bytes written, or a negative number for an error
type WriteCallback =
    unsafe extern "C" fn(context: *mut StreamContext, data: *const u8, len: isize) -> isize;

/// Defines a callback to flush a stream
/// The return value is 0 for success, or a negative number for an error
type FlushCallback = unsafe extern "C" fn(context: *mut StreamContext) -> isize;

#[repr(C)]
/// A CStream is a Rust Read/Write/Seek stream that can be created in C
#[derive(Debug)]
pub struct CStream {
    context: Box<StreamContext>,
    reader: ReadCallback,
    seeker: SeekCallback,
    writer: WriteCallback,
    flusher: FlushCallback,
}

impl CStream {
    /// Creates a new CStream from context with callbacks
    /// # Arguments
    /// * `context` - a pointer to a StreamContext
    /// * `read` - a ReadCallback to read from the stream
    /// * `seek` - a SeekCallback to seek in the stream
    /// * `write` - a WriteCallback to write to the stream
    /// * `flush` - a FlushCallback to flush the stream
    /// # Safety
    /// The context must remain valid for the lifetime of the C2paStream
    /// The read, seek, and write callbacks must be valid for the lifetime of the C2paStream
    /// The resulting C2paStream must be released by calling c2pa_release_stream
    pub unsafe fn new(
        context: *mut StreamContext,
        reader: ReadCallback,
        seeker: SeekCallback,
        writer: WriteCallback,
        flusher: FlushCallback,
    ) -> Self {
        Self {
            context: unsafe { Box::from_raw(context) },
            reader,
            seeker,
            writer,
            flusher,
        }
    }

    /// Extracts the context from the CStream (used for testing in Rust)
    pub fn extract_context(&mut self) -> Box<StreamContext> {
        std::mem::replace(&mut self.context, Box::new(StreamContext { _priv: () }))
    }
}

impl Read for CStream {
    fn read(&mut self, buf: &mut [u8]) -> std::io::Result<usize> {
        if buf.len() > isize::MAX as usize {
            return Err(std::io::Error::new(
                std::io::ErrorKind::InvalidInput,
                "Read buffer is too large",
            ));
        }
        let bytes_read =
            unsafe { (self.reader)(&mut (*self.context), buf.as_mut_ptr(), buf.len() as isize) };
        // returns a negative number for errors
        if bytes_read < 0 {
            return Err(std::io::Error::last_os_error());
        }
        Ok(bytes_read as usize)
    }
}

impl Seek for CStream {
    fn seek(&mut self, from: std::io::SeekFrom) -> std::io::Result<u64> {
        let (pos, mode) = match from {
            std::io::SeekFrom::Current(pos) => (pos, C2paSeekMode::Current),
            std::io::SeekFrom::Start(pos) => (pos as i64, C2paSeekMode::Start),
            std::io::SeekFrom::End(pos) => (pos, C2paSeekMode::End),
        };

        let new_pos = unsafe { (self.seeker)(&mut (*self.context), pos as isize, mode) };
        Ok(new_pos as u64)
    }
}

impl Write for CStream {
    fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
        if buf.len() > isize::MAX as usize {
            return Err(std::io::Error::new(
                std::io::ErrorKind::InvalidInput,
                "Write buffer is too large",
            ));
        }
        let bytes_written =
            unsafe { (self.writer)(&mut (*self.context), buf.as_ptr(), buf.len() as isize) };
        if bytes_written < 0 {
            return Err(std::io::Error::last_os_error());
        }
        Ok(bytes_written as usize)
    }

    fn flush(&mut self) -> std::io::Result<()> {
        let err = unsafe { (self.flusher)(&mut (*self.context)) };
        if err < 0 {
            return Err(std::io::Error::last_os_error());
        }
        Ok(())
    }
}

/// Creates a new C2paStream from context with callbacks
///
/// This allows implementing streams in other languages
///
/// # Arguments
/// * `context` - a pointer to a StreamContext
/// * `read` - a ReadCallback to read from the stream
/// * `seek` - a SeekCallback to seek in the stream
/// * `write` - a WriteCallback to write to the stream
///
/// # Safety
/// The context must remain valid for the lifetime of the C2paStream
/// The resulting C2paStream must be released by calling c2pa_release_stream
///
#[no_mangle]
pub unsafe extern "C" fn c2pa_create_stream(
    context: *mut StreamContext,
    reader: ReadCallback,
    seeker: SeekCallback,
    writer: WriteCallback,
    flusher: FlushCallback,
) -> *mut CStream {
    Box::into_raw(Box::new(CStream::new(
        context, reader, seeker, writer, flusher,
    )))
}

/// Releases a CStream allocated by Rust
///
/// # Safety
/// can only be released once and is invalid after this call
#[no_mangle]
pub unsafe extern "C" fn c2pa_release_stream(stream: *mut CStream) {
    if !stream.is_null() {
        drop(Box::from_raw(stream));
    }
}

/// This struct is used to test the CStream implementation
/// It is a wrapper around a Cursor<Vec<u8>>
/// It is exported in Rust so that it may be used externally
pub struct TestCStream {
    cursor: Cursor<Vec<u8>>,
}

impl TestCStream {
    fn new(data: Vec<u8>) -> Self {
        Self {
            cursor: Cursor::new(data),
        }
    }

    #[no_mangle]
    unsafe extern "C" fn reader(context: *mut StreamContext, data: *mut u8, len: isize) -> isize {
        let stream: &mut TestCStream = &mut *(context as *mut TestCStream);
        let data: &mut [u8] = slice::from_raw_parts_mut(data, len as usize);
        match stream.cursor.read(data) {
            Ok(bytes) => bytes as isize,
            Err(e) => {
                crate::Error::set_last(Error::Io(e.to_string()));
                -1
            }
        }
    }

    #[no_mangle]
    unsafe extern "C" fn seeker(
        context: *mut StreamContext,
        offset: isize,
        mode: C2paSeekMode,
    ) -> isize {
        let stream: &mut TestCStream = &mut *(context as *mut TestCStream);

        match mode {
            C2paSeekMode::Start => {
                stream.cursor.set_position(offset as u64);
            }
            C2paSeekMode::Current => match stream.cursor.seek(SeekFrom::Current(offset as i64)) {
                Ok(_) => {}
                Err(e) => {
                    crate::Error::set_last(Error::Io(e.to_string()));
                    return -1;
                }
            },
            C2paSeekMode::End => match stream.cursor.seek(SeekFrom::End(offset as i64)) {
                Ok(_) => {}
                Err(e) => {
                    crate::Error::set_last(Error::Io(e.to_string()));
                    return -1;
                }
            },
        }

        stream.cursor.position() as isize
    }

    #[no_mangle]
    unsafe extern "C" fn flusher(_context: *mut StreamContext) -> isize {
        0
    }

    #[no_mangle]
    unsafe extern "C" fn writer(context: *mut StreamContext, data: *const u8, len: isize) -> isize {
        let stream: &mut TestCStream = &mut *(context as *mut TestCStream);
        let data: &[u8] = slice::from_raw_parts(data, len as usize);
        match stream.cursor.write(data) {
            Ok(bytes) => bytes as isize,
            Err(e) => {
                crate::Error::set_last(Error::Io(e.to_string()));
                -1
            }
        }
    }

    fn into_c_stream(self) -> CStream {
        unsafe {
            CStream::new(
                Box::into_raw(Box::new(self)) as *mut StreamContext,
                Self::reader,
                Self::seeker,
                Self::writer,
                Self::flusher,
            )
        }
    }

    pub fn from_bytes(data: Vec<u8>) -> CStream {
        let test_stream = Self::new(data);
        test_stream.into_c_stream()
    }

    pub fn from_c_stream(mut c_stream: CStream) -> Self {
        let original_context = c_stream.extract_context();
        unsafe { *Box::from_raw(Box::into_raw(original_context) as *mut TestCStream) }
    }

    pub fn drop_c_stream(c_stream: CStream) {
        drop(Self::from_c_stream(c_stream));
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_cstream_read() {
        let data = vec![1, 2, 3, 4, 5];
        let mut c_stream = TestCStream::from_bytes(data);

        let mut buf = [0u8; 3];
        assert_eq!(c_stream.read(&mut buf).unwrap(), 3);
        assert_eq!(buf, [1, 2, 3]);

        let mut buf = [0u8; 3];
        assert_eq!(c_stream.read(&mut buf).unwrap(), 2);
        assert_eq!(buf, [4, 5, 0]);

        let _test_stream = TestCStream::from_c_stream(c_stream);
    }

    #[test]
    fn test_cstream_seek() {
        let data = vec![1, 2, 3, 4, 5];
        let mut c_stream = TestCStream::from_bytes(data);

        c_stream.seek(SeekFrom::Start(2)).unwrap();
        let mut buf = [0u8; 3];
        assert_eq!(c_stream.read(&mut buf).unwrap(), 3);
        assert_eq!(buf, [3, 4, 5]);

        c_stream.seek(SeekFrom::End(-2)).unwrap();
        let mut buf = [0u8; 2];
        assert_eq!(c_stream.read(&mut buf).unwrap(), 2);
        assert_eq!(buf, [4, 5]);

        c_stream.seek(SeekFrom::Current(-4)).unwrap();
        let mut buf = [0u8; 3];
        assert_eq!(c_stream.read(&mut buf).unwrap(), 3);
        assert_eq!(buf, [2, 3, 4]);

        TestCStream::drop_c_stream(c_stream);
    }

    #[test]
    fn test_cstream_write() {
        let data = vec![1, 2, 3, 4, 5];
        let mut c_stream = TestCStream::from_bytes(data);
        c_stream.seek(SeekFrom::End(0)).unwrap();
        let buf = [6, 7, 8];
        let bytes_written = c_stream.write(&buf).unwrap();
        assert_eq!(bytes_written, 3);
        assert_eq!(c_stream.seek(SeekFrom::End(0)).unwrap(), 8);
        TestCStream::drop_c_stream(c_stream);
    }
}
