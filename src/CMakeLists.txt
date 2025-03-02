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

# Check if the rust library is available and if not we will build it after checking if cargo is available
if (NOT EXISTS ${C2PA_C_ROOT_DIR}/target/release/libc2pa_c.a)
    find_program(CARGO cargo)
    if (CARGO)
        execute_process(COMMAND cargo build --release WORKING_DIRECTORY ${C2PA_C_ROOT_DIR})
    else()
        message(FATAL_ERROR "Cargo is required to build the Rust library")
    endif()
endif()

# Add the Rust library
set(RUST_LIB ${C2PA_C_ROOT_DIR}/target/release/libc2pa_c.a)

target_link_libraries(c2pa_cpp ${RUST_LIB})

target_include_directories(c2pa_cpp PUBLIC ${INCLUDES})

# Define the installation rules
install(TARGETS c2pa_cpp
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)

install(FILES ${INCLUDES}/c2pa.hpp
        DESTINATION include)