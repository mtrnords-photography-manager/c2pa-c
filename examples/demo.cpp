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

#include "c2pa.h"
#include "c2pa.hpp"
#include "test_signer.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

// this example uses nlohmann json for parsing the manifest
using json = nlohmann::json;
using namespace std;
namespace fs = std::filesystem;
using namespace c2pa;

namespace {
/// @brief Read a text file into a string
string read_text_file(const fs::path &path) {
  ifstream file(path);
  if (!file.is_open()) {
    throw runtime_error("Could not open file " + string(path));
  }
  const string contents((istreambuf_iterator<char>(file)),
                        istreambuf_iterator<char>());
  file.close();
  return contents;
}

// Helper function to get the directory of the current file
fs::path get_current_directory(const char *file_path) {
  return fs::path(file_path).parent_path();
}
} // namespace

/// @brief Example of signing a file with a manifest and reading the manifest
/// back
/// @details This shows how to write a do not train assertion and read the
/// status back
/// @return 0 on success, 1 on failure
int main() {
  // Get the current directory of this file
  const fs::path current_dir = get_current_directory(__FILE__);

  // Construct the paths relative to the current directory
  const fs::path manifest_path =
      current_dir / "../tests/fixtures/training.json";
  const fs::path certs_path = current_dir / "../tests/fixtures/es256_certs.pem";
  const fs::path image_path = current_dir / "../tests/fixtures/A.jpg";
  const fs::path output_path = current_dir / "../target/example/training.jpg";
  const fs::path thumbnail_path =
      current_dir / "../target/example/thumbnail.jpg";

  try {
    // load the manifest, certs, and private key
    const string manifest_json = read_text_file(manifest_path);
    const string certs = read_text_file(certs_path);

    // create a signer
    auto signer =
        Signer(&test_signer, Es256, certs, "http://timestamp.digicert.com");

    auto builder = Builder(manifest_json);
    auto manifest_data = builder.sign(image_path, output_path, signer);

    // read the new manifest and display the JSON
    auto reader = Reader(output_path);

    auto manifest_store_json = reader.json();
    cout << "The new manifest is " << manifest_store_json << '\n';

    // get the active manifest
    if (json manifest_store = json::parse(manifest_store_json);
        manifest_store.contains("active_manifest")) {
      const string active_manifest = manifest_store["active_manifest"];
      json &manifest = manifest_store["manifests"][active_manifest];

      // scan the assertions for the training-mining assertion
      const string identifer = manifest["thumbnail"]["identifier"];

      // ReSharper disable once CppExpressionWithoutSideEffects
      reader.get_resource(identifer, thumbnail_path);

      cout << "thumbnail written to" << thumbnail_path << '\n';
    }
  } catch (c2pa::Exception const &e) {
    cout << "C2PA Error: " << e.what() << '\n';
  } catch (runtime_error const &e) {
    cout << "setup error" << e.what() << '\n';
  } catch (json::parse_error const &e) {
    cout << "parse error " << e.what() << '\n';
  }
}
