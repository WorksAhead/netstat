#!/bin/bash

DEP_DIR="$PWD/dependencies"

CURL_DIR="$DEP_DIR/curl"
JSON_DIR="$DEP_DIR/json"
HMAC_DIR="$DEP_DIR/hmac"
B64C_DIR="$DEP_DIR/b64.c"

CURL_GIT="https://github.com/curl/curl.git"
JSON_GIT="https://github.com/nlohmann/json.git"

if [ "$1" == "debug" ]
then
    BUILD_TYPE="Debug"
else
    BUILD_TYPE="Release"
fi

PUSHD $DEP_DIR

# clone curl
if [ ! -d "$CURL_DIR" ]; then
    git clone "$CURL_GIT"

    PUSHD $CURL_DIR
    
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCURL_STATICLIB=1 -DBUILD_CURL_EXE=0 -DBUILD_TESTING=0 ..
    make

    POPD
fi

# clone json
if [ ! -d "$JSON_DIR" ]; then
    git clone "$JSON_GIT"
fi

# Build hmac
PUSHD $HMAC_DIR

if [ ! -d "build" ]; then
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
    make
fi

POPD

# Build b64.c
PUSHD $B64C_DIR

if [ ! -d "build" ]; then
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
    make
fi

POPD

POPD

rm -rf ./build
mkdir -p ./build
cd build
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCURL_INCLUDE_DIR="$CURL_DIR/include" -DCURL_LIBRARY="$CURL_DIR/build/lib" -DJSON_INCLUDE_DIR="$JSON_DIR/src" -DHMAC_INCLUDE_DIR="$HMAC_DIR" -DHMAC_LIBRARY="$HMAC_DIR/build/lib" -DB64C_INCLUDE_DIR="$B64C_DIR" -DB64C_LIBRARY="$B64C_DIR/build/lib" ..

