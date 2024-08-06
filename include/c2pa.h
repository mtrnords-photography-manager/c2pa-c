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

// This file is generated by cbindgen. Do not edit by hand.

#ifndef c2pa_bindings_h
#define c2pa_bindings_h

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(_WIN32) || defined(_WIN64)
    #if defined(_STATIC_C2PA) 
        #define IMPORT  __declspec(dllexport)
    #else 
        #if __GNUC__
            #define IMPORT __attribute__ ((dllimport))
        #else
            #define IMPORT __declspec(dllimport)
        #endif
    #endif
#else
    #define IMPORT

#endif



/**
 * List of supported signing algorithms.
 */
typedef enum C2paSigningAlg {
  Es256,
  Es384,
  Es512,
  Ps256,
  Ps384,
  Ps512,
  Ed25519,
} C2paSigningAlg;

typedef struct C2paSigner C2paSigner;

/**
 * Defines the configuration for a Signer.
 *
 * The signer is created from the sign_cert and private_key fields.
 * an optional url to an RFC 3161 compliant time server will ensure the signature is timestamped.
 *
 */
typedef struct C2paSignerInfo {
  /**
   * The signing algorithm.
   */
  const char *alg;
  /**
   * The public certificate chain in PEM format.
   */
  const char *sign_cert;
  /**
   * The private key in PEM format.
   */
  const char *private_key;
  /**
   * The timestamp authority URL or NULL.
   */
  const char *ta_url;
} C2paSignerInfo;

typedef struct C2paReader {

} C2paReader;

/**
 * An Opaque struct to hold a context value for the stream callbacks
 */
typedef struct StreamContext {

} StreamContext;

/**
 * Defines a callback to read from a stream
 */
typedef intptr_t (*ReadCallback)(const struct StreamContext *context, uint8_t *data, uintptr_t len);

/**
 * Defines a callback to seek to an offset in a stream
 */
typedef int (*SeekCallback)(const struct StreamContext *context, long offset, int mode);

/**
 * Defines a callback to write to a stream
 */
typedef intptr_t (*WriteCallback)(const struct StreamContext *context,
                                  const uint8_t *data,
                                  uintptr_t len);

typedef intptr_t (*FlushCallback)(const struct StreamContext *context);

/**
 * A CStream is a Rust Read/Write/Seek stream that can be created in C
 */
typedef struct CStream {
  struct StreamContext *context;
  ReadCallback reader;
  SeekCallback seeker;
  WriteCallback writer;
  FlushCallback flusher;
} CStream;

typedef struct C2paBuilder {

} C2paBuilder;

/**
 * Defines a callback to read from a stream.
 */
typedef intptr_t (*SignerCallback)(const void *context,
                                   const unsigned char *data,
                                   uintptr_t len,
                                   unsigned char *signed_bytes,
                                   uintptr_t signed_len);

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * Returns a version string for logging.
 *
 * # Safety
 * The returned value MUST be released by calling release_string
 * and it is no longer valid after that call.
 */
IMPORT extern char *c2pa_version(void);

/**
 * Returns the last error message.
 *
 * # Safety
 * The returned value MUST be released by calling release_string
 * and it is no longer valid after that call.
 */
IMPORT extern char *c2pa_error(void);

/**
 * Load Settings from a string.
 * # Errors
 * Returns -1 if there were errors, otherwise returns 0.
 * The error string can be retrieved by calling c2pa_error.
 * # Safety
 * Reads from NULL-terminated C strings.
 */
IMPORT extern int c2pa_load_settings(const char *settings, const char *format);

/**
 * Returns a ManifestStore JSON string from a file path.
 * Any thumbnails or other binary resources will be written to data_dir if provided.
 *
 * # Errors
 * Returns NULL if there were errors, otherwise returns a JSON string.
 * The error string can be retrieved by calling c2pa_error.
 *
 * # Safety
 * Reads from NULL-terminated C strings.
 * The returned value MUST be released by calling release_string
 * and it is no longer valid after that call.
 */
IMPORT extern char *c2pa_read_file(const char *path, const char *data_dir);

/**
 * Returns an Ingredient JSON string from a file path.
 * Any thumbnail or C2PA data will be written to data_dir if provided.
 *
 * # Errors
 * Returns NULL if there were errors, otherwise returns a JSON string
 * containing the Ingredient.
 * The error string can be retrieved by calling c2pa_error.
 *
 * # Safety
 * Reads from NULL-terminated C strings.
 * The returned value MUST be released by calling release_string
 * and it is no longer valid after that call.
 */
IMPORT extern char *c2pa_read_ingredient_file(const char *path, const char *data_dir);

/**
 * Add a signed manifest to the file at path using auth_token.
 * If cloud is true, upload the manifest to the cloud.
 *
 * # Errors
 * Returns an error field if there were errors.
 *
 * # Safety
 * Reads from NULL-terminated C strings.
 * The returned value MUST be released by calling release_string
 * and it is no longer valid after that call.
 */
IMPORT extern
char *c2pa_sign_file(const char *source_path,
                     const char *dest_path,
                     const char *manifest,
                     const struct C2paSignerInfo *signer_info,
                     const char *data_dir);

/**
 * Frees a string allocated by Rust.
 *
 * # Safety
 * Reads from NULL-terminated C strings.
 * The string must not have been modified in C.
 * The string can only be freed once and is invalid after this call.
 */
IMPORT extern void c2pa_string_free(char *s);

/**
 * Frees a string allocated by Rust.
 *
 * Deprecated: for backward api compatibility only.
 * # Safety
 * Reads from NULL-terminated C strings.
 * The string must not have been modified in C.
 * The string can only be freed once and is invalid after this call.
 */
IMPORT extern void c2pa_release_string(char *s);

/**
 * Creates a C2paReader from an asset stream with the given format.
 * # Errors
 * Returns NULL if there were errors, otherwise returns a pointer to a ManifestStore.
 * The error string can be retrieved by calling c2pa_error.
 * # Safety
 * Reads from NULL-terminated C strings.
 * The returned value MUST be released by calling c2pa_reader_free
 * and it is no longer valid after that call.
 * # Example
 * ```c
 * auto result = c2pa_reader_from_stream("image/jpeg", stream);
 * if (result == NULL) {
 *     printf("Error: %s\n", c2pa_error());
 * }
 * ```
 */
IMPORT extern
struct C2paReader *c2pa_reader_from_stream(const char *format,
                                           struct CStream *stream);

/**
 * Frees a C2paReader allocated by Rust.
 * # Safety
 * The C2paReader can only be freed once and is invalid after this call.
 */
IMPORT extern void c2pa_reader_free(struct C2paReader *reader_ptr);

/**
 * Returns a JSON string generated from a C2paReader.
 *
 * # Safety
 * The returned value MUST be released by calling c2pa_string_free
 * and it is no longer valid after that call.
 */
IMPORT extern char *c2pa_reader_json(struct C2paReader *reader_ptr);

/**
 * Writes a C2paReader resource to a stream given a URI.
 * # Errors
 * Returns -1 if there were errors, otherwise returns size of stream written.
 *
 * # Safety
 * Reads from NULL-terminated C strings.
 *
 * # Example
 * ```c
 * result c2pa_reader_resource_to_stream(store, "uri", stream);
 * if (result < 0) {
 *     printf("Error: %s\n", c2pa_error());
 * }
 * ```
 */
IMPORT extern
int c2pa_reader_resource_to_stream(struct C2paReader *reader_ptr,
                                   const char *uri,
                                   struct CStream *stream);

/**
 * Creates a C2paBuilder from a JSON manifest definition string.
 * # Errors
 * Returns NULL if there were errors, otherwise returns a pointer to a Builder.
 * The error string can be retrieved by calling c2pa_error.
 * # Safety
 * Reads from NULL-terminated C strings.
 * The returned value MUST be released by calling c2pa_builder_free
 * and it is no longer valid after that call.
 * # Example
 * ```c
 * auto result = c2pa_builder_from_json(manifest_json);
 * if (result == NULL) {
 *     printf("Error: %s\n", c2pa_error());
 * }
 * ```
 *
 */
IMPORT extern struct C2paBuilder *c2pa_builder_from_json(const char *manifest_json);

/**
 * Create a C2paBuilder from an archive stream.
 * # Errors
 * Returns NULL if there were errors, otherwise returns a pointer to a Builder.
 * The error string can be retrieved by calling c2pa_error.
 * # Safety
 * Reads from NULL-terminated C strings.
 * The returned value MUST be released by calling c2pa_builder_free
 * and it is no longer valid after that call.
 * # Example
 * ```c
 * auto result = c2pa_builder_from_archive(stream);
 * if (result == NULL) {
 *     printf("Error: %s\n", c2pa_error());
 * }
 * ```
 */
IMPORT extern struct C2paBuilder *c2pa_builder_from_archive(struct CStream *stream);

/**
 * Frees a C2paBuilder allocated by Rust.
 * # Safety
 * The C2paBuilder can only be freed once and is invalid after this call.
 */
IMPORT extern void c2pa_builder_free(struct C2paBuilder *builder_ptr);

/**
 * Adds a resource to the C2paBuilder.
 * # Errors
 * Returns -1 if there were errors, otherwise returns 0.
 * The error string can be retrieved by calling c2pa_error.
 * # Safety
 * Reads from NULL-terminated C strings
 *
 */
IMPORT extern
int c2pa_builder_add_resource(struct C2paBuilder *builder_ptr,
                              const char *uri,
                              struct CStream *stream);

/**
 * Adds an ingredient to the C2paBuilder.
 *
 * # Parameters
 * * builder_ptr: pointer to a Builder.
 * * ingredient_json: pointer to a C string with the JSON ingredient definition.
 * * format: pointer to a C string with the mime type or extension.
 * * source: pointer to a CStream.
 * # Errors
 * Returns -1 if there were errors, otherwise returns 0.
 * The error string can be retrieved by calling c2pa_error.
 * # Safety
 * Reads from NULL-terminated C strings.
 *
 */
IMPORT extern
int c2pa_builder_add_ingredient(struct C2paBuilder *builder_ptr,
                                const char *ingredient_json,
                                const char *format,
                                struct CStream *source);

/**
 * Writes an Archive of the Builder to the destination stream.
 * # Parameters
 * * builder_ptr: pointer to a Builder.
 * * stream: pointer to a writable CStream.
 * # Errors
 * Returns -1 if there were errors, otherwise returns 0.
 * The error string can be retrieved by calling c2pa_error.
 * # Safety
 * Reads from NULL-terminated C strings.
 * # Example
 * ```c
 * auto result = c2pa_builder_to_archive(builder, stream);
 * if (result < 0) {
 *     printf("Error: %s\n", c2pa_error());
 * }
 * ```
 */
IMPORT extern int c2pa_builder_to_archive(struct C2paBuilder *builder_ptr, struct CStream *stream);

/**
 * Creates and writes signed manifest from the C2paBuilder to the destination stream.
 * # Parameters
 * * builder_ptr: pointer to a Builder.
 * * format: pointer to a C string with the mime type or extension.
 * * source: pointer to a CStream.
 * * dest: pointer to a writable CStream.
 * * signer: pointer to a C2paSigner.
 * * c2pa_bytes_ptr: pointer to a pointer to a c_uchar to return manifest_bytes (optional, can be NULL).
 * # Errors
 * Returns -1 if there were errors, otherwise returns the size of the c2pa data.
 * The error string can be retrieved by calling c2pa_error.
 * # Safety
 * Reads from NULL-terminated C strings
 * If c2pa_data_ptr is not NULL, the returned value MUST be released by calling c2pa_release_string
 * and it is no longer valid after that call.
 */
IMPORT extern
int c2pa_builder_sign(struct C2paBuilder *builder_ptr,
                      const char *format,
                      struct CStream *source,
                      struct CStream *dest,
                      struct C2paSigner *signer,
                      const unsigned char **manifest_bytes_ptr);

/**
 * Frees a C2PA manifest returned by c2pa_builder_sign.
 * # Safety
 * The bytes can only be freed once and are invalid after this call.
 */
IMPORT extern void c2pa_manifest_bytes_free(const unsigned char *manifest_bytes_ptr);

/**
 * Creates a C2paSigner from a callback and configuration.
 * # Parameters
 * * callback: a callback function to sign data.
 * * alg: the signing algorithm.
 * * certs: a pointer to a NULL-terminated string containing the certificate chain in PEM format.
 * * tsa_url: a pointer to a NULL-terminated string containing the RFC 3161 compliant timestamp authority URL.
 * # Errors
 * Returns NULL if there were errors, otherwise returns a pointer to a C2paSigner.
 * The error string can be retrieved by calling c2pa_error.
 * # Safety
 * Reads from NULL-terminated C strings
 * The returned value MUST be released by calling c2pa_signer_free
 * and it is no longer valid after that call.
 * # Example
 * ```c
 * auto result = c2pa_signer_create(callback, alg, certs, tsa_url);
 * if (result == NULL) {
 *     printf("Error: %s\n", c2pa_error());
 * }
 * ```
 */
IMPORT extern
struct C2paSigner *c2pa_signer_create(const void *context,
                                      SignerCallback callback,
                                      enum C2paSigningAlg alg,
                                      const char *certs,
                                      const char *tsa_url);

/**
 * Frees a C2paSigner allocated by Rust.
 * # Safety
 * The C2paSigner can only be freed once and is invalid after this call.
 */
IMPORT extern void c2pa_signer_free(const struct C2paSigner *signer_ptr);

/**
 * Creates a new C2paStream from context with callbacks
 *
 * This allows implementing streams in other languages
 *
 * # Arguments
 * * `context` - a pointer to a StreamContext
 * * `read` - a ReadCallback to read from the stream
 * * `seek` - a SeekCallback to seek in the stream
 * * `write` - a WriteCallback to write to the stream
 *
 * # Safety
 * The context must remain valid for the lifetime of the C2paStream
 * The resulting C2paStream must be released by calling c2pa_release_stream
 *
 */
IMPORT extern
struct CStream *c2pa_create_stream(struct StreamContext *context,
                                   ReadCallback reader,
                                   SeekCallback seeker,
                                   WriteCallback writer,
                                   FlushCallback flusher);

/**
 * Releases a CStream allocated by Rust
 *
 * # Safety
 * can only be released once and is invalid after this call
 */
IMPORT extern void c2pa_release_stream(struct CStream *stream);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif /* c2pa_bindings_h */
