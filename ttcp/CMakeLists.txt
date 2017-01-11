# Build ttcp.

cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

project(ttcp)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

add_subdirectory(./ttcpclient)
add_subdirectory(./ttcpserver)