cmake_minimum_required(VERSION 3.21)

# Project information / metadata.
project(ryce VERSION 0.1 LANGUAGES C)

if(MSVC)
    message(WARNING "MSVC does not support C23. Falling back to C17.")
    set(CMAKE_C_STANDARD 17)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /Od")
else()
    set(CMAKE_C_STANDARD 23)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O -Wall -Wextra -Werror -Wpedantic -Wundef -Wshadow -fsanitize=address -fsanitize=undefined -pthread -g")
endif()

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

