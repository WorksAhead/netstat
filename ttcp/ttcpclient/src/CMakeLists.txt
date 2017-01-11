# Build ttcpclient

cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

project(ttcpclient)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Lib)

if(UNIX)
    find_package(Boost
                 1.6.2
                 REQUIRED
                 COMPONENTS log filesystem thread system)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

add_library(ttcpclient
            ttcpclient.cpp
            ttcpclient.h)

target_compile_features(ttcpclient PRIVATE cxx_generalized_initializers)
