#!/bin/bash

if [ "$1" == "debug" ]
then
    BUILD_TYPE="Debug"
else
    BUILD_TYPE="Release"
fi

if [ ! -d "curl" ]; then
    git clone https://github.com/curl/curl.git
    cd curl
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCURL_STATICLIB=1 -DBUILD_CURL_EXE=0 -DBUILD_TESTING=0 ..
    make
    cd ../../
fi

rm -rf ./build
mkdir -p ./build
cd build
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCURL_INCLUDE_DIR="$pwd\..\curl\include" -DCURL_LIBRARY="$pwd\..\curl\build\lib" ..

