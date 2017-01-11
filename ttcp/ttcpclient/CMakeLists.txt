# Build ttcpclient_test.

cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

project(ttcpclient_test)

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

add_subdirectory(./src)

add_executable(ttcpclient_test ttcpclient_test.cpp)

target_link_libraries(ttcpclient_test ttcpclient ${Boost_LIBRARIES})
target_compile_features(ttcpclient_test PRIVATE cxx_generalized_initializers)
