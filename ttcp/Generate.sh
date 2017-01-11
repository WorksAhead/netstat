#!/bin/bash

if [ "$1" == "debug" ]
then
    BUILD_TYPE="Debug"
else
    BUILD_TYPE="Release"
fi

rm -rf ./cos-cpp-sdk-v4
git clone https://github.com/tencentyun/cos-cpp-sdk-v4.git

rm -rf ./Build
mkdir -p ./Build/Bin
cp ./cosconfig.json ./Build/Bin

cd Build
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
