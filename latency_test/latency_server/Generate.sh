#!/bin/bash

if [ "$1" == "debug" ]
then
    BUILD_TYPE="Debug"
else
    BUILD_TYPE="Release"
fi

rm -rf ./build
mkdir -p ./build

cd build
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..

