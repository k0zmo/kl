#!/bin/bash

if [ ! -d ".travis/cmake/bin" ]; then
    rm -rf ".travis/cmake"
    mkdir -p ".travis" && cd ".travis"
    curl -O https://cmake.org/files/v3.9/cmake-3.9.1-Linux-x86_64.tar.gz
    tar xzf cmake-3.9.1-Linux-x86_64.tar.gz
    mv cmake-3.9.1-Linux-x86_64 cmake
    cd cmake/bin
else
    cd ".travis/cmake/bin"
    echo "Using cached cmake at $(pwd)"
fi
export PATH="$(pwd):$PATH"
