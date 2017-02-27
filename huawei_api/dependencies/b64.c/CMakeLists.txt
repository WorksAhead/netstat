# Build b64c.

cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

project(b64c)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# add_compile_options(-Wall -std=c99)

add_library(b64c
            decode.c
            encode.c
            b64.h)
