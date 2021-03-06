# Build latency_server.

cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

project(latency_server)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(UNIX)
    find_package(Boost
                 1.6.2
                 REQUIRED
                 COMPONENTS program_options log filesystem thread system)
    include_directories(${Boost_INCLUDE_DIRS})

    set(CMAKE_CXX_FLAGS -DBOOST_LOG_DYN_LINK)
    set(EXTRA_LIBS pthread)
endif()

if(WIN32)
    # warning C4819: The file contains a character that cannot be represented 
	# in the current code page (936). Save the file in Unicode format to prevent data loss
    add_compile_options(/wd"4819")

	add_definitions(-DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00)
endif()

add_executable(latency_server
               main.cpp)

target_link_libraries(latency_server ${Boost_LIBRARIES} ${EXTRA_LIBS})
target_compile_features(latency_server PRIVATE cxx_generalized_initializers)
