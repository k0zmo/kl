#!/bin/bash
# Builds lcov 1.11

if [ ! -d ".travis/lcov-install" ]; then
    rm -rf ".travis/lcov-install"
    mkdir -p ".travis/lcov-install" && cd ".travis/lcov-install"
    wget http://ftp.de.debian.org/debian/pool/main/l/lcov/lcov_1.11.orig.tar.gz
    tar xf lcov_1.11.orig.tar.gz
    cd ./lcov-1.11/bin
else
    cd ".travis/lcov-install/lcov-1.11/bin"
fi
LCOVPATH=$(pwd)
export PATH="$LCOVPATH:$PATH"
