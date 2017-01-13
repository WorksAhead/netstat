# Build ttcpclient_test.

cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

project(ttcpclient_test)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(UNIX)
    find_package(Boost
                 1.6.2
                 REQUIRED
                 COMPONENTS program_options log filesystem thread system)
    include_directories(${Boost_INCLUDE_DIRS})

    set(CMAKE_CXX_FLAGS -DBOOST_LOG_DYN_LINK)
endif()

if(WIN32)
	# Disable the ompile tips: Please define _WIN32_WINNT or _WIN32_WINDOWS appropriately.
	add_definitions(-DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00)

	# Disable warning C4996 for Boost.Signals2: 'std::copy': Function call with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct.
    # It is nothing to worry about. In the last few releases of MSVC, they've gone into full security-paranoia mode. 
    # std::copy issues this warning when it is used with raw pointers, because when used incorrectly, it can result in buffer overflows.
    # Their iterator implementation performs bounds checking to ensure this doesn't happen, at a significant performance cost.
    # So feel free to ignore the warning. It doesn't mean there's anything wrong with your code. 
    # It is just saying that if there is something wrong with your code, then bad things will happen. Which is an odd thing to issue warnings about
    add_definitions(-D_SCL_SECURE_NO_WARNINGS)
endif()

add_subdirectory(./src)
include_directories(./src)

add_executable(ttcpclient_test ttcpclient_test.cpp)

target_link_libraries(ttcpclient_test ttcpclient ${Boost_LIBRARIES} pthread)
target_compile_features(ttcpclient_test PRIVATE cxx_generalized_initializers)
