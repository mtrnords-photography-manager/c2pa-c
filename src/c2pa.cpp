// Copyright 2024 Adobe. All rights reserved.
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

/// @file   c2pa.cpp
/// @brief  C++ wrapper for the C2PA C library.
/// @details This is used for creating and verifying C2PA manifests.
///          This is an early version, and has not been fully tested.
///          Thread safety is not guaranteed due to the use of errno and etc.

#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem> // C++17
#include <fstream>
#include <ios>
#include <istream>
#include <optional> // C++17
#include <ostream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "c2pa.h"
#include "c2pa.hpp"

using path = std::filesystem::path;

using namespace std;
using namespace c2pa;

namespace {
intptr_t signer_passthrough(const void *context, const unsigned char *data,
                            const uintptr_t len, unsigned char *signature,
                            const uintptr_t sig_max_len) {
  try {
    // the context is a pointer to the C++ callback function
    auto *callback =
        reinterpret_cast<SignerFunc *>(const_cast<void *>(context));
    const std::vector<uint8_t> data_vec(data, data + len);
    std::vector<uint8_t> signature_vec = (callback)(data_vec);
    if (signature_vec.size() > sig_max_len) {
      return -1;
    }
    std::copy(signature_vec.begin(), signature_vec.end(), signature);
    return static_cast<intptr_t>(signature_vec.size());
  } catch (std::exception const &e) {
    // todo pass exceptions to Rust error handling
    (void)e;
    // printf("Error: signer_passthrough - %s\n", e.what());
    return -1;
  } catch (...) {
    // printf("Error: signer_passthrough - unknown exception\n");
    return -1;
  }
}
} // namespace

namespace c2pa {
/// Exception class for C2PA errors.
/// This class is used to throw exceptions for errors encountered by the C2PA
/// library via c2pa_error().

Exception::Exception() {
  auto *const result = c2pa_error();
  message = string(result);
  c2pa_release_string(result);
}

Exception::Exception(string what) : message(std::move(what)) {}

const char *Exception::what() const noexcept { return message.c_str(); }

/// Returns the version of the C2PA library.
string version() {
  auto *const result = c2pa_version();
  auto str = string(result);
  c2pa_release_string(result);
  return str;
}

/// Loads C2PA settings from a string in a given format.
/// @param format the mime format of the string.
/// @param data the string to load.
/// @throws a C2pa::Exception for errors encountered by the C2PA library.
void load_settings(const string &format, const string &data) {
  if (const auto result = c2pa_load_settings(format.c_str(), data.c_str());
      result != 0) {
    throw c2pa::Exception();
  }
}

/// converts a filesystem::path to a string in utf-8 format
inline std::string path_to_string(const filesystem::path &source_path) {
  return source_path.u8string().c_str();
}

/// Reads a file and returns the manifest json as a C2pa::String.
/// @param source_path the path to the file to read.
/// @param data_dir the directory to store binary resources (optional).
/// @return a string containing the manifest json if a manifest was found.
/// @throws a C2pa::Exception for errors encountered by the C2PA library.
optional<string> read_file(const filesystem::path &source_path,
                           const optional<path> &data_dir) {
  const auto dir =
      data_dir.has_value() ? path_to_string(data_dir.value()) : string();

  char *result =
      c2pa_read_file(path_to_string(source_path).c_str(), dir.c_str());

  if (result == nullptr) {
    // Check if "ManifestNotFound" is contained (possibly as a substring) in the
    // error message
    if (const auto exception = c2pa::Exception();
        std::string(exception.what()).find("ManifestNotFound") !=
        std::string::npos) {
      std::cout << "Error reading file: " << exception.what() << '\n';
      return std::nullopt;
    }
    throw c2pa::Exception();
  }
  auto str = string(result);
  c2pa_release_string(result);
  return str;
}

/// Reads a file and returns an ingredient JSON as a C2pa::String.
/// @param source_path the path to the file to read.
/// @param data_dir the directory to store binary resources.
/// @return a string containing the ingredient json.
/// @throws a C2pa::Exception for errors encountered by the C2PA library.
string read_ingredient_file(const path &source_path, const path &data_dir) {
  char *result = c2pa_read_ingredient_file(path_to_string(source_path).c_str(),
                                           path_to_string(data_dir).c_str());
  if (result == nullptr) {
    throw c2pa::Exception();
  }
  auto str = string(result);
  c2pa_release_string(result);
  return str;
}

/// Adds the manifest and signs a file.
// source_path: path to the asset to be signed
// dest_path: the path to write the signed file to
// manifest: the manifest json to add to the file
// signer_info: the signer info to use for signing
// data_dir: the directory to store binary resources (optional)
// Throws a C2pa::Exception for errors encountered by the C2PA library
void sign_file(const path &source_path, const path &dest_path,
               const char *manifest, const c2pa::SignerInfo *signer_info,
               const std::optional<path> &data_dir) {
  const auto dir =
      data_dir.has_value() ? path_to_string(data_dir.value()) : string();

  char *result = c2pa_sign_file(path_to_string(source_path).c_str(),
                                path_to_string(dest_path).c_str(), manifest,
                                signer_info, dir.c_str());
  if (result == nullptr) {
    throw c2pa::Exception();
  }
  c2pa_release_string(result);
}

/// IStream Class wrapper for CStream.
template <typename IStream>
CppIStream::CppIStream(IStream &istream)
    : CStream(),
      c_stream(c2pa_create_stream(reinterpret_cast<StreamContext *>(&istream),
                                  reader, seeker, writer, flusher)) {
  static_assert(std::is_base_of_v<std::istream, IStream>,
                "Stream must be derived from std::istream");
}

CppIStream::~CppIStream() { c2pa_release_stream(c_stream); }

intptr_t CppIStream::reader(StreamContext *context, uint8_t *buffer,
                            const intptr_t size) {
  auto *istream = reinterpret_cast<std::istream *>(context);
  istream->read(reinterpret_cast<char *>(buffer), size);
  if (istream->fail()) {
    if (!istream->eof()) {
      // do not report eof as an error
      errno = EINVAL; // Invalid argument
      // std::perror("Error: Invalid argument");
      return -1;
    }
  }
  if (istream->bad()) {
    errno = EIO; // Input/output error
    // std::perror("Error: Input/output error");
    return -1;
  }
  return istream->gcount();
}

intptr_t CppIStream::seeker(StreamContext *context, const intptr_t offset,
                            const C2paSeekMode whence) {
  auto *istream = reinterpret_cast<std::istream *>(context);

  std::ios_base::seekdir dir = std::ios_base::beg;
  switch (whence) {
  case C2paSeekMode::Start:
    dir = std::ios_base::beg;
    break;
  case C2paSeekMode::Current:
    dir = std::ios_base::cur;
    break;
  case C2paSeekMode::End:
    dir = std::ios_base::end;
    break;
  };

  istream->clear(); // important: clear any flags
  istream->seekg(offset, dir);
  if (istream->fail()) {
    errno = EINVAL;
    return -1;
  }
  if (istream->bad()) {
    errno = EIO;
    return -1;
  }
  const long pos = (long)istream->tellg();
  if (pos < 0) {
    errno = EIO;
    return -1;
  }
  // printf("seeker offset= %ld pos = %d whence = %d\n", offset, pos, dir);
  return pos;
}

intptr_t CppIStream::writer(StreamContext *context, const uint8_t *buffer,
                            intptr_t size) {
  auto *iostream = reinterpret_cast<std::iostream *>(context);
  iostream->write(reinterpret_cast<const char *>(buffer), size);
  if (iostream->fail()) {
    errno = EINVAL; // Invalid argument
    return -1;
  }
  if (iostream->bad()) {
    errno = EIO; // I/O error
    return -1;
  }
  return size;
}

intptr_t CppIStream::flusher(StreamContext *context) {
  auto *iostream = reinterpret_cast<std::iostream *>(context);
  iostream->flush();
  return 0;
}

/// Ostream Class wrapper for CStream implementation.

template <typename OStream>
CppOStream::CppOStream(OStream &ostream)
    : CStream(),
      c_stream(c2pa_create_stream(reinterpret_cast<StreamContext *>(&ostream),
                                  reader, seeker, writer, flusher)) {
  static_assert(std::is_base_of_v<std::ostream, OStream>,
                "Stream must be derived from std::ostream");
}

CppOStream::~CppOStream() { c2pa_release_stream(c_stream); }

intptr_t CppOStream::reader(StreamContext * /*context*/, uint8_t * /*buffer*/,
                            intptr_t /*size*/) {
  errno = EINVAL; // Invalid argument
  return -1;
}

intptr_t CppOStream::seeker(StreamContext *context, const intptr_t offset,
                            const C2paSeekMode whence) {
  auto *ostream = reinterpret_cast<std::ostream *>(context);
  // printf("seeker ofstream = %p\n", ostream);

  if (ostream == nullptr) {
    errno = EINVAL; // Invalid argument
    return -1;
  }

  std::ios_base::seekdir dir = ios_base::beg;
  switch (whence) {
  case SEEK_SET:
    dir = ios_base::beg;
    break;
  case SEEK_CUR:
    dir = ios_base::cur;
    break;
  case SEEK_END:
    dir = ios_base::end;
    break;
  };

  ostream->clear(); // Clear any error flags
  ostream->seekp(offset, dir);
  if (ostream->fail()) {
    errno = EINVAL; // Invalid argument
    return -1;
  }
  if (ostream->bad()) {
    errno = EIO; // Input/output error
    return -1;
  }
  const long pos = ostream->tellp();
  if (pos < 0) {
    errno = EIO; // Input/output error
    return -1;
  }
  return pos;
}

intptr_t CppOStream::writer(StreamContext *context, const uint8_t *buffer,
                            const intptr_t size) {
  auto *ofstream = reinterpret_cast<std::ostream *>(context);
  // printf("writer ofstream = %p\n", ofstream);
  ofstream->write(reinterpret_cast<const char *>(buffer), size);
  if (ofstream->fail()) {
    errno = EINVAL; // Invalid argument
    return -1;
  }
  if (ofstream->bad()) {
    errno = EIO; // I/O error
    return -1;
  }
  return size;
}

intptr_t CppOStream::flusher(StreamContext *context) {
  auto *ofstream = reinterpret_cast<std::ofstream *>(context);
  ofstream->flush();
  return 0;
}

/// IOStream Class wrapper for CStream implementation.
template <typename IOStream>
CppIOStream::CppIOStream(IOStream &iostream)
    : CStream(),
      c_stream(c2pa_create_stream(reinterpret_cast<StreamContext *>(&iostream),
                                  reader, seeker, writer, flusher)) {
  static_assert(std::is_base_of_v<std::iostream, IOStream>,
                "Stream must be derived from std::iostream");
}

CppIOStream::~CppIOStream() { c2pa_release_stream(c_stream); }

intptr_t CppIOStream::reader(StreamContext *context, uint8_t *buffer,
                             intptr_t size) {
  auto *iostream = reinterpret_cast<std::iostream *>(context);
  iostream->read(reinterpret_cast<char *>(buffer), size);
  if (iostream->fail()) {
    if (!iostream->eof()) {
      // do not report eof as an error
      errno = EINVAL; // Invalid argument
      // std::perror("Error: Invalid argument");
      return -1;
    }
  }
  if (iostream->bad()) {
    errno = EIO; // Input/output error
    // std::perror("Error: Input/output error");
    return -1;
  }
  return iostream->gcount();
}

intptr_t CppIOStream::seeker(StreamContext *context, intptr_t offset,
                             C2paSeekMode whence) {
  auto *iostream = reinterpret_cast<std::iostream *>(context);

  if (iostream == nullptr) {
    errno = EINVAL; // Invalid argument
    return -1;
  }

  std::ios_base::seekdir dir = std::ios_base::beg;
  switch (whence) {
  case SEEK_SET:
    dir = std::ios_base::beg;
    break;
  case SEEK_CUR:
    dir = std::ios_base::cur;
    break;
  case SEEK_END:
    dir = std::ios_base::end;
    break;
  };
  // seek for both read and write since rust does it that way

  iostream->clear(); // Clear any error flags
  iostream->seekg(offset, dir);
  if (iostream->fail()) {
    errno = EINVAL; // Invalid argument
    return -1;
  }
  if (iostream->bad()) {
    errno = EIO; // Input/output error
    return -1;
  }
  long pos = (long)iostream->tellg();
  if (pos < 0) {
    errno = EIO; // Input/output error
    return -1;
  }

  iostream->seekp(offset, dir);
  if (iostream->fail()) {
    errno = EINVAL; // Invalid argument
    return -1;
  }
  if (iostream->bad()) {
    errno = EIO; // Input/output error
    return -1;
  }
  pos = static_cast<long>(iostream->tellp());
  if (pos < 0) {
    errno = EIO; // Input/output error
    return -1;
  }
  return pos;
}

intptr_t CppIOStream::writer(StreamContext *context, const uint8_t *buffer,
                             const intptr_t size) {
  auto *iostream = reinterpret_cast<std::iostream *>(context);
  iostream->write(reinterpret_cast<const char *>(buffer), size);
  if (iostream->fail()) {
    errno = EINVAL; // Invalid argument
    return -1;
  }
  if (iostream->bad()) {
    errno = EIO; // I/O error
    return -1;
  }
  return size;
}

intptr_t CppIOStream::flusher(StreamContext *context) {
  auto *iostream = reinterpret_cast<std::iostream *>(context);
  iostream->flush();
  return 0;
}

/// Reader class for reading a manifest implementation.
Reader::Reader(const string &format, std::istream &stream)
    : cpp_stream(new CppIStream(stream)) {
  // keep this allocated for life of Reader
  c2pa_reader = c2pa_reader_from_stream(format.c_str(), cpp_stream->c_stream);
  if (c2pa_reader == nullptr) {
    throw Exception();
  }
}

Reader::Reader(const std::filesystem::path &source_path) {
  std::ifstream file_stream(source_path, std::ios::binary);
  if (!file_stream.is_open()) {
    throw Exception("Failed to open file: " + source_path.string() + " - " +
                    std::strerror(errno));
  }
  string extension = source_path.extension().string();
  if (!extension.empty()) {
    extension = extension.substr(1); // Skip the dot
  }

  cpp_stream =
      new CppIStream(file_stream); // keep this allocated for life of Reader
  c2pa_reader =
      c2pa_reader_from_stream(extension.c_str(), cpp_stream->c_stream);
  if (c2pa_reader == nullptr) {
    throw Exception();
  }
}

Reader::~Reader() {
  c2pa_reader_free(c2pa_reader);
  delete cpp_stream;
}

string Reader::json() const {
  char *result = c2pa_reader_json(c2pa_reader);
  if (result == nullptr) {
    throw Exception();
  }
  auto str = string(result);
  c2pa_release_string(result);
  return str;
}

int Reader::get_resource(const string &uri,
                         const std::filesystem::path &path) const {
  std::ofstream file_stream(path, std::ios::binary);
  if (!file_stream.is_open()) {
    throw Exception(); // Handle file open error appropriately
  }
  return get_resource(uri, file_stream);
}

int Reader::get_resource(const string &uri, std::ostream &stream) const {
  const CppOStream cpp_stream_(stream);
  const int result = c2pa_reader_resource_to_stream(c2pa_reader, uri.c_str(),
                                                    cpp_stream_.c_stream);
  if (result < 0) {
    throw Exception();
  }
  return result;
}

Signer::Signer(SignerFunc *callback, const C2paSigningAlg alg,
               const string &sign_cert,
               const std::optional<std::string> &tsa_uri)
    : signer_(c2pa_signer_create(reinterpret_cast<const void *>(callback),
                                 &signer_passthrough, alg, sign_cert.c_str(),
                                 tsa_uri.has_value() ? tsa_uri.value().c_str()
                                                     : nullptr)) {}

Signer::~Signer() { c2pa_signer_free(signer_); }

/// @brief  Get the C2paSigner
C2paSigner *Signer::c2pa_signer() const { return signer_; }

/// @brief  Get the size to reserve for a signature for this signer.
uintptr_t Signer::reserve_size() const {
  return c2pa_signer_reserve_size(signer_);
}

/// @brief  Builder class for creating a manifest implementation.
Builder::Builder(const string &manifest_json)
    : builder(c2pa_builder_from_json(manifest_json.c_str())) {

  if (builder == nullptr) {
    throw Exception();
  }
}

/// @brief Create a Builder from an archive.
/// @param archive  The input stream to read the archive from.
/// @throws C2pa::Exception for errors encountered by the C2PA library.
Builder::Builder(istream &archive) {
  const auto c_archive = CppIStream(archive);
  builder = c2pa_builder_from_archive(c_archive.c_stream);
  if (builder == nullptr) {
    throw Exception();
  }
}

void Builder::set_no_embed() const { c2pa_builder_set_no_embed(builder); }

void Builder::set_remote_url(const string &remote_url) const {
  if (const int result =
          c2pa_builder_set_remote_url(builder, remote_url.c_str());
      result < 0) {
    throw Exception();
  }
}

void Builder::add_resource(const string &uri, istream &source) const {
  const auto c_source = CppIStream(source);
  if (const int result =
          c2pa_builder_add_resource(builder, uri.c_str(), c_source.c_stream);
      result < 0) {
    throw Exception();
  }
}

void Builder::add_resource(const string &uri,
                           const std::filesystem::path &source_path) const {
  auto stream = ifstream(source_path);
  if (!stream.is_open()) {
    throw std::runtime_error("Failed to open source file: " +
                             source_path.string());
  }
  add_resource(uri, stream);
}

void Builder::add_ingredient(const string &ingredient_json,
                             const string &format, istream &source) const {
  const auto c_source = CppIStream(source);
  if (const int result = c2pa_builder_add_ingredient_from_stream(
          builder, ingredient_json.c_str(), format.c_str(), c_source.c_stream);
      result < 0) {
    throw Exception();
  }
}

void Builder::add_ingredient(const string &ingredient_json,
                             const std::filesystem::path &source_path) const {
  auto stream = ifstream(source_path);
  if (!stream.is_open()) {
    throw std::runtime_error("Failed to open source file: " +
                             source_path.string());
  }
  auto format = source_path.extension().string();
  if (!format.empty()) {
    format = format.substr(1); // Skip the dot.
  }
  add_ingredient(ingredient_json, format, stream);
}

std::vector<unsigned char> Builder::sign(const string &format, istream &source,
                                         ostream &dest,
                                         const Signer &signer) const {
  const auto c_source = CppIStream(source);
  const auto c_dest = CppOStream(dest);
  const unsigned char *c2pa_manifest_bytes = nullptr;
  const auto result = c2pa_builder_sign(
      builder, format.c_str(), c_source.c_stream, c_dest.c_stream,
      signer.c2pa_signer(), &c2pa_manifest_bytes);
  if (result < 0 || c2pa_manifest_bytes == nullptr) {
    throw Exception();
  }

  auto manifest_bytes = std::vector<unsigned char>(
      c2pa_manifest_bytes, c2pa_manifest_bytes + result);
  c2pa_manifest_bytes_free(c2pa_manifest_bytes);
  return manifest_bytes;
}

/// @brief Sign a file and write the signed data to an output file.
/// @param source_path The path to the file to sign.
/// @param dest_path The path to write the signed file to.
/// @param signer A signer object to use when signing.
/// @return A vector containing the signed manifest bytes.
/// @throws C2pa::Exception for errors encountered by the C2PA library.
std::vector<unsigned char> Builder::sign(const path &source_path,
                                         const path &dest_path,
                                         Signer &signer) const {
  std::ifstream source(source_path, std::ios::binary);
  if (!source.is_open()) {
    throw std::runtime_error("Failed to open source file: " +
                             source_path.string());
  }
  // Ensure the destination directory exists
  if (const std::filesystem::path dest_dir = dest_path.parent_path();
      !std::filesystem::exists(dest_dir)) {
    std::filesystem::create_directories(dest_dir);
  }
  std::ofstream dest(dest_path, std::ios::binary);
  if (!dest.is_open()) {
    throw std::runtime_error("Failed to open destination file: " +
                             dest_path.string());
  }
  auto format = dest_path.extension().string();
  if (!format.empty()) {
    format = format.substr(1); // Skip the dot
  }
  auto result = sign(format, source, dest, signer);
  return result;
}

/// @brief Create a Builder from an archive stream.
/// @param archive The input stream to read the archive from.
/// @throws C2pa::Exception for errors encountered by the C2PA library.
Builder Builder::from_archive(istream &archive) { return Builder(archive); }

/// @brief Create a Builder from an archive file.
/// @param archive_path The input path to read the archive from.
/// @throws C2pa::Exception for errors encountered by the C2PA library.
Builder Builder::from_archive(const path &archive_path) {
  std::ifstream path(archive_path, std::ios::binary);
  if (!path.is_open()) {
    throw std::runtime_error("Failed to open archive file: " +
                             archive_path.string());
  }
  return from_archive(path);
}

/// @brief Write the builder to an archive stream.
/// @param dest The output stream to write the archive to.
/// @throws C2pa::Exception for errors encountered by the C2PA library.
void Builder::to_archive(ostream &dest) const {
  const auto c_dest = CppOStream(dest);
  if (const int result = c2pa_builder_to_archive(builder, c_dest.c_stream);
      result < 0) {
    throw Exception();
  }
}

/// @brief Write the builder to an archive file.
/// @param dest_path The path to write the archive file to.
/// @throws C2pa::Exception for errors encountered by the C2PA library.
void Builder::to_archive(const path &dest_path) const {
  std::ofstream dest(dest_path, std::ios::binary);
  if (!dest.is_open()) {
    throw std::runtime_error("Failed to open destination file: " +
                             dest_path.string());
  }
  to_archive(dest);
}

std::vector<unsigned char>
Builder::data_hashed_placeholder(uintptr_t reserve_size,
                                 const string &format) const {
  const unsigned char *c2pa_manifest_bytes = nullptr;
  const auto result = c2pa_builder_data_hashed_placeholder(
      builder, reserve_size, format.c_str(), &c2pa_manifest_bytes);
  if (result < 0 || c2pa_manifest_bytes == nullptr) {
    throw(Exception());
  }

  auto data = std::vector<unsigned char>(c2pa_manifest_bytes,
                                         c2pa_manifest_bytes + result);
  c2pa_manifest_bytes_free(c2pa_manifest_bytes);
  return data;
}

std::vector<unsigned char> Builder::sign_data_hashed_embeddable(
    const Signer &signer, const string &data_hash, const string &format,
    istream *asset) const {
  int result = 0;
  const unsigned char *c2pa_manifest_bytes = nullptr;
  if (asset != nullptr) {
    const CppIStream c_asset(*asset);
    result = c2pa_builder_sign_data_hashed_embeddable(
        builder, signer.c2pa_signer(), data_hash.c_str(), format.c_str(),
        c_asset.c_stream, &c2pa_manifest_bytes);
  } else {
    result = c2pa_builder_sign_data_hashed_embeddable(
        builder, signer.c2pa_signer(), data_hash.c_str(), format.c_str(),
        nullptr, &c2pa_manifest_bytes);
  }
  if (result < 0 || c2pa_manifest_bytes == nullptr) {
    throw(Exception());
  }

  auto data = std::vector<unsigned char>(c2pa_manifest_bytes,
                                         c2pa_manifest_bytes + result);
  c2pa_manifest_bytes_free(c2pa_manifest_bytes);
  return data;
}

std::vector<unsigned char>
Builder::format_embeddable(const string &format,
                           const std::vector<unsigned char> &data) {
  const unsigned char *c2pa_manifest_bytes = nullptr;
  const auto result = c2pa_format_embeddable(format.c_str(), data.data(),
                                             data.size(), &c2pa_manifest_bytes);
  if (result < 0 || c2pa_manifest_bytes == nullptr) {
    throw(Exception());
  }

  auto formatted_data = std::vector<unsigned char>(
      c2pa_manifest_bytes, c2pa_manifest_bytes + result);
  c2pa_manifest_bytes_free(c2pa_manifest_bytes);
  return formatted_data;
}
} // namespace c2pa
