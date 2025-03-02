cmake_minimum_required(VERSION 3.10)
project(c2pa_cpp)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# LTO
include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)


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


# Default list of compile options applying to all compilers
set(COMPILE_OPTIONS
        -Wshadow
        -Wconversion
        -Wpedantic
        -Wuninitialized
        -Wall
        -Wextra
        -Werror
)

# If clang we have to ignore the unknown-warning-option check since it gets confused with gcc and werror
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    list(APPEND COMPILE_OPTIONS
            -Wno-error=unknown-warning-option)
endif ()

# Set warning levels and other compiler options
target_compile_options(c2pa_cpp PRIVATE
        ${COMPILE_OPTIONS}
)

# if gcc we add -fhardened
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(c2pa_cpp PRIVATE
            -fhardened
            -Wno-error=hardened
    )
else ()
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(c2pa_cpp PRIVATE
                -D_FORTIFY_SOURCE=3
        )
    endif ()

    target_compile_options(c2pa_cpp PRIVATE
            -D_GLIBCXX_ASSERTIONS
            -ftrivial-auto-var-init=zero
            -fPIE
            -fstack-protector-strong
            -fstack-clash-protection
    )
    # if x86 gnu/linux we add -fcf-protection=full
    if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_compile_options(c2pa_cpp PRIVATE
                -fcf-protection=full
        )
    endif ()
endif ()

# LTO
if (supported)
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET c2pa_cpp PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else ()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif ()

# Define the installation rules
install(TARGETS c2pa_cpp
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)

install(FILES ${INCLUDES}/c2pa.hpp
        DESTINATION include)