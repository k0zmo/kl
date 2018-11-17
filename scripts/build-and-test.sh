#!/bin/bash
set -e

# Override gcc and his friends version to gcc-7
# Put an appropriate symlink at the front of the path.
if [[ "$COMPILER" =~ gcc-([[:digit:]]+) ]]; then
    pushd .
    ver=${BASH_REMATCH[1]};
    rm -rf ".travis/bin"
    mkdir -p ".travis/bin"
    for g in gcc g++ gcov gcc-ar gcc-nm gcc-ranlib
    do
        test -x $(type -p ${g}-${ver})
        ln -sv $(type -p ${g}-${ver}) ".travis/bin/${g}"
    done
    cd ".travis/bin"
    export PATH="$(pwd):$PATH"
    popd
fi

mkdir -p build && cd build
cmake -E env LDFLAGS="-fuse-ld=gold" \
    cmake -DCMAKE_BUILD_TYPE="$VARIANT" ..

if [ "$VARIANT" = "Coverage" ]; then
    make kl-coverage
else
    make
    CTEST_OUTPUT_ON_FAILURE=1 ctest
fi

if [ "$VARIANT" = "Coverage" ]; then
    echo "Sending code coverage data"
    ~/.local/bin/codecov -X gcov
fi
