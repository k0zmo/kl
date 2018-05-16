#!/bin/bash
set -e

# Override gcc and his friends version to gcc-6
# Put an appropriate symlink at the front of the path.
if [ "$COMPILER" = "gcc-6" ]; then
    pushd .
    rm -rf ".travis/bin"
    mkdir -p ".travis/bin"
    for g in gcc g++ gcov gcc-ar gcc-nm gcc-ranlib
    do
        test -x $(type -p ${g}-6)
        ln -sv $(type -p ${g}-6) ".travis/bin/${g}"
    done
    cd ".travis/bin"
    export PATH="$(pwd):$PATH"
    popd
fi

mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE="$VARIANT" ..

if [ "$VARIANT" = "Coverage" ]; then 
    make kl-coverage
else 
    make
    ctest
fi

if [ "$VARIANT" = "Coverage" ]; then 
    echo "Sending code coverage data"
    ~/.local/bin/codecov -X gcov
fi
