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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/c2pa.h"
#include "file_stream.h"
#include "unit_test.h"

int main(void)
{
    char *version = c2pa_version();
    assert_contains("version", version, "c2pa-c/0.");

    char *result1 = c2pa_read_file("tests/fixtures/C.jpg", NULL);
    assert_str_not_null("c2pa_read_file_no_data_dir", result1);

    char *result = c2pa_read_file("tests/fixtures/C.jpg", "target/tmp");
    assert_str_not_null("c2pa_read_file", result);

    result = c2pa_read_ingredient_file("tests/fixtures/C.jpg", "target/ingredient");
    assert_str_not_null("c2pa_ingredient_from_file", result);

    CStream* input_stream = open_file_stream("tests/fixtures/C.jpg", "rb");
    assert_not_null("open_file_stream", input_stream);

    C2paReader* reader = c2pa_reader_from_stream("image/jpeg", input_stream);
    assert_not_null("c2pa_reader_from_stream", reader);

    close_file_stream(input_stream);

    char* json = c2pa_reader_json(reader);
    assert_not_null("c2pa_reader_json", json);
    // printf("manifest json = %s\n", json);

    // we should fetch the active manifest and retrieve the identifier from the thumbnail in that manifest 
    char *uri = findValueByKey(json, "identifier");
    if (uri == NULL) {
        fprintf(stderr, "FAILED: unable to find identifier in manifest json\n");
        exit(1);
    }

	//Open a file to write the thumbnail into
    CStream* thumb_stream = open_file_stream("target/thumb_c.jpg", "wb");
    assert_not_null("open_file_stream thumbnail", thumb_stream);

    // write the thumbnail resource to the stream
    int res = c2pa_reader_resource_to_stream(reader, uri, thumb_stream);
    free(uri);
    assert_int("c2pa_reader_resource", res);

    c2pa_reader_free(reader);
 
    char *certs = load_file("tests/fixtures/es256_certs.pem");
    char *private_key = load_file("tests/fixtures/es256_private.key");

    char *manifest = load_file("tests/fixtures/training.json");

    // create a sign_info struct
    C2paSignerInfo sign_info = {.alg = "es256", .sign_cert = certs, .private_key = private_key, .ta_url = "http://timestamp.digicert.com"};

    result = c2pa_sign_file("tests/fixtures/C.jpg", "target/tmp/earth.jpg", manifest, &sign_info, "tests/fixtures");
    assert_not_null("c2pa_sign_file_ok", result);

    result = c2pa_sign_file("tests/fixtures/foo.jpg", "target/tmp/earth.jpg", manifest, &sign_info, "tests/fixtures");
    assert_null("c2pa_sign_file_not_found", result, "FileNotFound");

    result = c2pa_sign_file("tests/fixtures/es256_certs.pem", "target/tmp/earth.jpg", manifest, &sign_info, "tests/fixtures");
    assert_null("c2pa_sign_file_not_supported", result, "NotSupported");

    C2paBuilder *builder = c2pa_builder_from_json(manifest);
    assert_not_null("c2pa_builder_from_json", builder);

    CStream *archive = open_file_stream("target/tmp/archive.zip", "wb");
    int arch_result = c2pa_builder_to_archive(builder, archive);
    assert_int("c2pa_builder_to_archive", arch_result);
    close_file_stream(archive);

    CStream *archive2 = open_file_stream("target/tmp/archive.zip", "rb");
    C2paBuilder *builder2 = c2pa_builder_from_archive(archive2);
    assert_not_null("c2pa_builder_from_archive", builder2);
    close_file_stream(archive2);

    C2paSigner *signer = c2pa_signer_create((const void *)"testing context", &signer_callback, Es256, certs, "http://timestamp.digicert.com");
    assert_not_null("c2pa_signer_create", signer);

    CStream *source = open_file_stream("tests/fixtures/C.jpg", "rb");
    CStream *dest = open_file_stream("target/tmp/earth.jpg", "wb");

    int result2 = c2pa_builder_sign(builder2, "image/jpeg", source, dest, signer, NULL);
    assert_int("c2pa_builder_sign", result2);

    close_file_stream(source);
    close_file_stream(dest);

    c2pa_builder_free(builder2);
    c2pa_builder_free(builder);
    c2pa_signer_free(signer);

    free(certs);
    free(private_key);
}