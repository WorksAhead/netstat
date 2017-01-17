#!/bin/bash

if [ "$1" == "debug" ]
then
    BUILD_TYPE="Debug"
else
    BUILD_TYPE="Release"
fi

rm -rf ./build
mkdir -p ./build

cp ./StartServer.sh ./build
chmod +x ./build/StartServer.sh
cp ./StopServer.sh ./build
chmod +x ./build/StopServer.sh

cd build
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
