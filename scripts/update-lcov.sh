#!/bin/bash

if [ ! -d ".travis/lcov/bin" ]; then
    rm -rf ".travis/lcov"
    mkdir -p ".travis" && cd ".travis"
    curl -O http://ftp.de.debian.org/debian/pool/main/l/lcov/lcov_1.11.orig.tar.gz
    tar xzf lcov_1.11.orig.tar.gz
    mv lcov-1.11 lcov
    cd ./lcov/bin
else
    cd ".travis/lcov/bin"
    echo "Using cached lcov at $(pwd)"
fi
LCOVPATH=$(pwd)
export PATH="$LCOVPATH:$PATH"
