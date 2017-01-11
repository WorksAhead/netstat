# Build ttcpserver.

cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

project(ttcpserver)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin)

if(UNIX)
    find_package(Boost
                 1.6.2
                 REQUIRED
                 COMPONENTS program_options log filesystem thread system)
    include_directories(${Boost_INCLUDE_DIRS})

    set(CMAKE_CXX_FLAGS -DBOOST_LOG_DYN_LINK)
endif()

add_executable(ttcpserver
               ttcpserver_main.cpp
               ttcpserver.cpp
               ttcpserver.h)

target_link_libraries(ttcpserver ${Boost_LIBRARIES})
target_compile_features(ttcpserver PRIVATE cxx_generalized_initializers)
