#!/bin/bash

if [ ! -d ".travis/boost/lib" ]; then
    rm -rf ".travis/boost"
    mkdir -p ".travis" && cd ".travis"
    wget 'http://downloads.sourceforge.net/project/boost/boost/1.67.0/boost_1_67_0.tar.gz'
    tar xzf boost_1_67_0.tar.gz
    cd ./boost_1_67_0
    chmod +x bootstrap.sh
    ./bootstrap.sh --prefix="$(pwd)/../boost"
    ./b2 toolset=gcc-7 variant=release link=static "$@" install
    cd ../boost
else
    cd ".travis/boost"
    echo "Using cached boost at $(pwd)"
fi
export BOOST_ROOT="$(pwd)"
