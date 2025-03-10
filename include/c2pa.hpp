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

/// @file   c2pa.hpp
/// @brief  C++ wrapper for the C2PA C library.
/// @details This is used for creating and verifying C2PA manifests.
///          This is an early version, and has not been fully tested.
///          Thread safety is not guaranteed due to the use of errno and etc.

#ifndef C2PA_H
#define C2PA_H

// Suppress unused function warning for GCC/Clang
#include <cstdint>
#include <exception>
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

// Suppress unused function warning for MSVC
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4505)
#endif

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "c2pa.h"

#if defined(_WIN32) || defined(_WIN64) && !defined(C2PA_DYNAMIC_LOADING)
#ifdef BUILDING_C2PA_DLL
#define C2PA_EXPORT __declspec(dllexport)
#else
#define C2PA_EXPORT __declspec(dllimport)
#endif
#else
#define C2PA_EXPORT __attribute__((visibility("default")))
#endif

using path = std::filesystem::path;

namespace c2pa {
using namespace std;

using SignerInfo = C2paSignerInfo;

/// Exception class for C2pa errors.
/// This class is used to throw exceptions for errors encountered by the C2pa
/// library via c2pa_error().
class C2PA_EXPORT Exception final : public exception {
private:
  string message;

public:
  Exception();

  explicit Exception(string what);

  [[nodiscard]] const char *what() const noexcept override;
};

/// Returns the version of the C2pa library.
string C2PA_EXPORT version();

/// Loads C2PA settings from a string in a given format.
/// @param format the mime format of the string.
/// @param data the string to load.
void C2PA_EXPORT load_settings(string format, string data);

/// Reads a file and returns the manifest json as a C2pa::String.
/// @param source_path the path to the file to read.
/// @param data_dir the directory to store binary resources (optional).
/// @return a string containing the manifest json if a manifest was found.
/// @throws a C2pa::Exception for errors encountered by the C2PA library.
optional<string>
    C2PA_EXPORT read_file(const filesystem::path &source_path,
                          const optional<path> &data_dir = nullopt);

/// Reads a file and returns an ingredient JSON as a C2pa::String.
/// @param source_path the path to the file to read.
/// @param data_dir the directory to store binary resources.
/// @return a string containing the ingredient json.
/// @throws a C2pa::Exception for errors encountered by the C2PA library.
std::string C2PA_EXPORT read_ingredient_file(const path &source_path,
                                             const path &data_dir);

/// Adds the manifest and signs a file.
/// @param source_path the path to the asset to be signed.
/// @param dest_path the path to write the signed file to.
/// @param manifest the manifest json to add to the file.
/// @param signer_info the signer info to use for signing.
/// @param data_dir the directory to store binary resources (optional).
/// @throws a C2pa::Exception for errors encountered by the C2PA library.
void C2PA_EXPORT sign_file(const path &source_path, const path &dest_path,
                           const char *manifest, const SignerInfo *signer_info,
                           const std::optional<path> &data_dir = std::nullopt);

/// @brief Istream Class wrapper for CStream.
/// @details This class is used to wrap an input stream for use with the C2PA
/// library.
class C2PA_EXPORT CppIStream : public CStream {
public:
  CStream *c_stream;
  template <typename IStream> explicit CppIStream(IStream &istream);

  CppIStream(const CppIStream &) = delete;
  CppIStream &operator=(const CppIStream &) = delete;
  CppIStream(CppIStream &&) = delete;
  CppIStream &operator=(CppIStream &&) = delete;

  ~CppIStream();

private:
  static intptr_t reader(StreamContext *context, uint8_t *buffer,
                         intptr_t size);
  static intptr_t writer(StreamContext *context, const uint8_t *buffer,
                         intptr_t size);
  static intptr_t seeker(StreamContext *context, intptr_t offset,
                         C2paSeekMode whence);
  static intptr_t flusher(StreamContext *context);

  friend class Reader;
};

/// @brief Ostream Class wrapper for CStream.
/// @details This class is used to wrap an output stream for use with the C2PA
/// library.
class C2PA_EXPORT CppOStream : public CStream {
public:
  CStream *c_stream;
  template <typename OStream> explicit CppOStream(OStream &ostream);

  CppOStream(const CppOStream &) = default;
  CppOStream(CppOStream &&) = default;
  CppOStream &operator=(const CppOStream &) = default;
  CppOStream &operator=(CppOStream &&) = default;

  ~CppOStream();

private:
  static intptr_t reader(StreamContext *context, uint8_t *buffer,
                         intptr_t size);
  static intptr_t writer(StreamContext *context, const uint8_t *buffer,
                         intptr_t size);
  static intptr_t seeker(StreamContext *context, intptr_t offset,
                         C2paSeekMode whence);
  static intptr_t flusher(StreamContext *context);
};

/// @brief IOStream Class wrapper for CStream.
/// @details This class is used to wrap an input/output stream for use with the
/// C2PA library.
class C2PA_EXPORT CppIOStream : public CStream {
public:
  CStream *c_stream;
  template <typename IOStream> explicit CppIOStream(IOStream &iostream);

  CppIOStream(const CppIOStream &) = default;
  CppIOStream(CppIOStream &&) = default;
  CppIOStream &operator=(const CppIOStream &) = default;
  CppIOStream &operator=(CppIOStream &&) = default;
  ~CppIOStream();

private:
  static intptr_t reader(StreamContext *context, uint8_t *buffer,
                         intptr_t size);
  static intptr_t writer(StreamContext *context, const uint8_t *buffer,
                         intptr_t size);
  static intptr_t seeker(StreamContext *context, intptr_t offset,
                         C2paSeekMode whence);
  static intptr_t flusher(StreamContext *context);
};

/// @brief Reader class for reading a manifest.
/// @details This class is used to read and validate a manifest from a stream or
/// file.
class C2PA_EXPORT Reader {
private:
  C2paReader *c2pa_reader;
  CppIStream *cpp_stream = nullptr;

public:
  /// @brief Create a Reader from a stream.
  /// @details The validation_status field in the json contains validation
  /// results.
  /// @param format The mime format of the stream.
  /// @param stream The input stream to read from.
  /// @throws C2pa::Exception for errors encountered by the C2PA library.
  Reader(const std::string &format, std::istream &stream);

  /// @brief Create a Reader from a file path.
  /// @param source_path  the path to the file to read.
  /// @throws C2pa::Exception for errors encountered by the C2PA library.
  explicit Reader(const std::filesystem::path &source_path);

  Reader(const Reader &) = default;
  Reader(Reader &&) = default;
  Reader &operator=(const Reader &) = default;
  Reader &operator=(Reader &&) = default;
  ~Reader();

  /// @brief Get the manifest as a json string.
  /// @return The manifest as a json string.
  /// @throws C2pa::Exception for errors encountered by the C2PA library.
  [[nodiscard]] string json() const;

  /// @brief  Get a resource from the reader and write it to a file.
  /// @param uri The uri of the resource.
  /// @param path The path to write the resource to.
  /// @return The number of bytes written.
  /// @throws C2pa::Exception for errors encountered by the C2PA library.
  [[nodiscard]] int get_resource(const string &uri,
                                 const std::filesystem::path &path) const;

  /// @brief  Get a resource from the reader  and write it to an output stream.
  /// @param uri The uri of the resource.
  /// @param stream The output stream to write the resource to.
  /// @return The number of bytes written.
  /// @throws C2pa::Exception for errors encountered by the C2PA library.
  int get_resource(const string &uri, std::ostream &stream) const;
};

/// @brief  Signer Callback function type.
/// @param  data the data to sign.
/// @return the signature as a vector of bytes.
/// @details This function type is used to create a callback function for
/// signing.
using SignerFunc =
    std::vector<unsigned char>(const std::vector<unsigned char> &);

/// @brief  Signer class for creating a signer.
/// @details This class is used to create a signer from a signing algorithm,
/// certificate, and TSA URI.
class C2PA_EXPORT Signer {
private:
  C2paSigner *signer_{};

public:
  /// @brief Create a Signer from a callback function, signing algorithm,
  /// certificate, and TSA URI.
  /// @param callback  the callback function to use for signing.
  /// @param alg  The signing algorithm to use.
  /// @param sign_cert The certificate to use for signing.
  /// @param tsa_uri  The TSA URI to use for time-stamping.
  Signer(SignerFunc *callback, C2paSigningAlg alg, const string &sign_cert,
         const std::optional<std::string> &tsa_uri);

  explicit Signer(C2paSigner *signer) : signer_(signer) {}

  Signer(const Signer &) = default;
  Signer(Signer &&) = default;
  Signer &operator=(const Signer &) = default;
  Signer &operator=(Signer &&) = default;
  ~Signer();

  /// @brief  Get the size to reserve for a signature for this signer.
  /// @return Reserved size for the signature.
  [[nodiscard]] uintptr_t reserve_size() const;

  /// @brief  Get the C2paSigner
  [[nodiscard]] C2paSigner *c2pa_signer() const;
};

/// @brief Builder class for creating a manifest.
/// @details This class is used to create a manifest from a json string and add
/// resources and ingredients to the manifest.
class C2PA_EXPORT Builder final {
private:
  C2paBuilder *builder;

public:
  /// @brief  Create a Builder from a manifest JSON string.
  /// @param manifest_json  The manifest JSON string.
  /// @throws C2pa::Exception for errors encountered by the C2PA library.
  explicit Builder(const std::string &manifest_json);
  Builder(const Builder &) = delete;
  Builder(Builder &&) = delete;
  Builder &operator=(const Builder &) = delete;
  Builder &operator=(Builder &&) = delete;

  virtual ~Builder() { c2pa_builder_free(builder); }

  /// @brief  Set the no embed flag.
  void set_no_embed() const;

  /// @brief  Set the remote URL.
  /// @param remote_url  The remote URL to set.
  /// @throws C2pa::Exception for errors encountered by the C2PA library.
  void set_remote_url(const string &remote_url) const;

  /// @brief  Add a resource to the builder.
  /// @param uri  The uri of the resource.
  /// @param source  The input stream to read the resource from.
  /// @throws C2pa::Exception for errors encountered by the C2PA library.
  void add_resource(const string &uri, istream &source) const;

  /// @brief  Add a resource to the builder.
  /// @param uri  The uri of the resource.
  /// @param source_path  The path to the resource file.
  /// @throws C2pa::Exception for errors encountered by the C2PA library.
  void add_resource(const string &uri,
                    const std::filesystem::path &source_path) const;

  /// @brief Add an ingredient to the builder.
  /// @param ingredient_json  Any fields of the ingredient you want to define.
  /// @param format  The format of the ingredient file.
  /// @param source  The input stream to read the ingredient from.
  /// @throws C2pa::Exception for errors encountered by the C2pa library.
  void add_ingredient(const string &ingredient_json, const string &format,
                      istream &source) const;

  /// @brief Add an ingredient to the builder.
  /// @param ingredient_json  Any fields of the ingredient you want to define.
  /// @param source_path  The path to the ingredient file.
  /// @throws C2pa::Exception for errors encountered by the C2PA library.
  void add_ingredient(const string &ingredient_json,
                      const std::filesystem::path &source_path) const;

  /// @brief Sign an input stream and write the signed data to an output stream.
  /// @param format The format of the output stream.
  /// @param source The input stream to sign.
  /// @param dest The output stream to write the signed data to.
  /// @param signer
  /// @return A vector containing the signed manifest bytes.
  /// @throws C2pa::Exception for errors encountered by the C2PA library.
  std::vector<unsigned char> sign(const string &format, istream &source,
                                  iostream &dest, const Signer &signer) const;

  /// @brief Sign a file and write the signed data to an output file.
  /// @param source_path The path to the file to sign.
  /// @param dest_path The path to write the signed file to.
  /// @param signer A signer object to use when signing.
  /// @return A vector containing the signed manifest bytes.
  /// @throws C2pa::Exception for errors encountered by the C2PA library.
  std::vector<unsigned char> sign(const path &source_path,
                                  const path &dest_path, Signer &signer) const;

  /// @brief Create a Builder from an archive.
  /// @param archive  The input stream to read the archive from.
  /// @throws C2pa::Exception for errors encountered by the C2PA library.
  static Builder from_archive(istream &archive);

  /// @brief Create a Builder from an archive
  /// @param archive_path  the path to the archive file
  /// @throws C2pa::Exception for errors encountered by the C2PA library
  static Builder from_archive(const std::filesystem::path &archive_path);

  /// @brief Write the builder to an archive stream.
  /// @param dest The output stream to write the archive to.
  /// @throws C2pa::Exception for errors encountered by the C2PA library.
  void to_archive(ostream &dest) const;

  /// @brief Write the builder to an archive file.
  /// @param dest_path The path to write the archive file to.
  /// @throws C2pa::Exception for errors encountered by the C2PA library.
  void to_archive(const path &dest_path) const;

  /// @brief Create a hashed placeholder from the builder.
  /// @param reserved_size  The size required for a signature from the intended
  /// signer.
  /// @param format  The format of the mime type or extension of the asset.
  /// @return A vector containing the hashed placeholder.
  /// @throws C2pa::Exception for errors encountered by the C2PA library.
  [[nodiscard]] std::vector<unsigned char>
  data_hashed_placeholder(uintptr_t reserved_size, const string &format) const;

  /// @brief Sign a Builder using the specified signer and data hash.
  /// @param signer  The signer to use for signing.
  /// @param data_hash  The data hash ranges to sign. This must contain hashes
  /// unless and asset is provided.
  /// @param format  The mime format for embedding into.  Use "c2pa" for an
  /// unformatted result.
  /// @param asset  An optional asset to hash according to the data_hash
  /// information.
  /// @return A vector containing the signed data.
  /// @throws C2pa::Exception for errors encountered by the C2PA library.
  std::vector<unsigned char>
  sign_data_hashed_embeddable(const Signer &signer, const string &data_hash,
                              const string &format,
                              istream *asset = nullptr) const;

  /// @brief convert an unformatted manifest data to an embeddable format.
  /// @param format The format for embedding into.
  /// @param data An unformatted manifest data block from
  /// sign_data_hashed_embeddable using "c2pa" format.
  /// @return A formatted copy of the data.
  static std::vector<unsigned char>
  format_embeddable(const string &format,
                    const std::vector<unsigned char> &data);

private:
  // Private constructor for Builder from an archive (todo: find a better way to
  // handle this)
  explicit Builder(istream &archive);
};
} // namespace c2pa

// Restore warnings
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // C2PA_H
