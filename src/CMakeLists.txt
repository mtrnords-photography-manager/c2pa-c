cmake_minimum_required(VERSION 3.10)
project(c2pa_cpp)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
set(INCLUDES ${C2PA_C_ROOT_DIR}/include)
include_directories(${INCLUDES})

# Source files
set(SOURCES c2pa.cpp)

# Create the static library
add_library(c2pa_cpp STATIC ${SOURCES})

# Add the Rust library
# If macos, use .dylib, otherwise use .so unless Windows, then use .dll
if (APPLE)
    message("Building for MacOS")
    set(RUST_LIB "${C2PA_C_ROOT_DIR}/target/release/libc2pa_c.dylib")
elseif (WIN32)
    message("Building for Windows")
    set(RUST_LIB "${C2PA_C_ROOT_DIR}/target/release/c2pa_c.dll")
else ()
    message("Building for Unix")
    set(RUST_LIB "${C2PA_C_ROOT_DIR}/target/release/libc2pa_c.so")
endif ()

# Check if the rust library is available and if not we will build it after checking if cargo is available
if (NOT EXISTS RUST_LIB)
    find_program(CARGO cargo)
    if (CARGO)
        execute_process(COMMAND cargo build --release WORKING_DIRECTORY ${C2PA_C_ROOT_DIR})
    else ()
        message(FATAL_ERROR "Cargo is required to build the Rust library")
    endif ()
endif ()

message("Using Rust library: ${RUST_LIB}")

target_link_libraries(c2pa_cpp ${RUST_LIB})

target_include_directories(c2pa_cpp PUBLIC ${INCLUDES})


option(SANITIZERS_ENABLED "Enable sanitizers" OFF)

# if debug building
if (SANITIZERS_ENABLED)
    target_compile_options(c2pa_cpp PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
    )
    target_link_options(
            c2pa_cpp PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
    )
endif ()

# Define the installation rules
install(TARGETS c2pa_cpp
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)

install(FILES ${INCLUDES}/c2pa.hpp
        DESTINATION include)