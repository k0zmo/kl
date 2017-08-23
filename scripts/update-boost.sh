#!/bin/bash

if [ ! -d ".travis/boost/lib" ]; then
    rm -rf ".travis/boost"
    mkdir -p ".travis" && cd ".travis"
    wget 'http://downloads.sourceforge.net/project/boost/boost/1.63.0/boost_1_63_0.tar.gz'
    tar xzf boost_1_63_0.tar.gz
    cd ./boost_1_63_0
    chmod +x bootstrap.sh
    ./bootstrap.sh --prefix="$(pwd)/../boost"
    ./b2 toolset=gcc-6 variant=release link=static "$@" install
    cd ../boost
else
    cd ".travis/boost"
    echo "Using cached boost at $(pwd)"
fi
export BOOST_ROOT="$(pwd)"
