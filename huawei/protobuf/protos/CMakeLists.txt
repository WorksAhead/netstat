cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

project(protos)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})

FILE(GLOB ALL_PROTOS *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${ALL_PROTOS})

add_library(protos ${PROTO_SRCS} ${PROTO_HDRS})
