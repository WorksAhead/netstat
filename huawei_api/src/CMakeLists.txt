# Build huawei_api.

cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

project(huawei_api)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(UNIX)
    find_package(Boost
                 1.6.2
                 REQUIRED
                 COMPONENTS thread system)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

if(WIN32)
    # Disable the ompile tips: Please define _WIN32_WINNT or _WIN32_WINDOWS appropriately.
    add_definitions(-DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00)

    # Disable warning C4003 for nlohmann/json: not enough actual parameters for macro 'max'.
    add_definitions(-DNOMINMAX)

    # Libs thast were depended by libcurl.
    set(EXTRA_LIBS ws2_32 wldap32 crypt32)
endif()

#libcurl
find_package(CURL
             7.53.0
             REQUIRED
             COMPONENTS
             curl)
include_directories(${CURL_INCLUDE_DIRS})
include_directories("${CURL_INCLUDE_DIRS}/../build/include/curl")
add_definitions(-DCURL_STATICLIB)

#nlohmann/json
include_directories("${JSON_INCLUDE_DIR}")

#ogay/hmac
include_directories("${HMAC_INCLUDE_DIR}")

#littlstar/b64.c
include_directories("${B64C_INCLUDE_DIR}")

add_library(huawei_api
            huawei_api.cpp
            huawei_api.h)

target_compile_features(huawei_api PRIVATE cxx_generalized_initializers)
