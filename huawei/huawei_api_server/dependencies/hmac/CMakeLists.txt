# Build hmac.

cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

project(hmac)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# add_compile_options(-Wall -DTEST_VECTORS)

add_library(hmac
            hmac_sha2.c
            hmac_sha2.h
            sha2.c
            sha2.h)
