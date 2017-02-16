# Build huawei.

cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

project(huawei)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

#add_subdirectory(./huawei_api_client)
add_subdirectory(./huawei_api_server/src)