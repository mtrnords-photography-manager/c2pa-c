# Copyright 2024 Adobe. All rights reserved.
# This file is licensed to you under the Apache License,
# Version 2.0 (http://www.apache.org/licenses/LICENSE-2.0)
# or the MIT license (http://opensource.org/licenses/MIT),
# at your option.
#
# Unless required by applicable law or agreed to in writing,
# this software is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR REPRESENTATIONS OF ANY KIND, either express or
# implied. See the LICENSE-MIT and LICENSE-APACHE files for the
# specific language governing permissions and limitations under
# each license.

cmake_minimum_required(VERSION 3.27)

project(c2pa-c)

set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.3")

INCLUDE_DIRECTORIES(include)

# Test if we are within FetchContent and set the CMAKE_SOURCE_DIR as a custom variable for the subdirectories to use
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(C2PA_C_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    set(C2PA_C_IS_FETCH_CONTENT FALSE)

    # Add tests and examples
    ADD_SUBDIRECTORY(tests)
    ADD_SUBDIRECTORY(examples)
else()
    set(C2PA_C_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    set(C2PA_C_IS_FETCH_CONTENT TRUE)
endif()

message("C2PA_C_ROOT_DIR: ${C2PA_C_ROOT_DIR}")
message("C2PA_C_IS_FETCH_CONTENT: ${C2PA_C_IS_FETCH_CONTENT}")

ADD_SUBDIRECTORY(src)