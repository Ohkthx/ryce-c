cmake_minimum_required(VERSION 3.21)

# Project information / metadata.
project(ryce VERSION 0.1 LANGUAGES C)

# Target Standard.
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Destination for the compiled binary.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

# Source files.
file(GLOB_RECURSE SRC_FILES
    "${PROJECT_SOURCE_DIR}/src/*.c"
    "${PROJECT_SOURCE_DIR}/src/*.h"
)

# Executable.
add_executable(${PROJECT_NAME} ${SRC_FILES})

