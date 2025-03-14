# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Find OpenSSL
find_package(OpenSSL 3.2 REQUIRED)

# install json
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)


# Set the project name
project(Examples)

# Include the directory containing the main project headers
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/tests)

# Add example1 executable
add_executable(training training.cpp)
target_link_libraries(training nlohmann_json::nlohmann_json)
target_link_libraries(training OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(training c2pa_cpp test_signer)

add_executable(demo demo.cpp)
target_link_libraries(demo nlohmann_json::nlohmann_json)
target_link_libraries(demo OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(demo c2pa_cpp test_signer)

# if debug building
if (SANITIZERS_ENABLED)
    target_compile_options(demo PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
    )
    target_link_options(
            demo PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
    )


    target_compile_options(training PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
    )
    target_link_options(
            training PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
    )
endif ()