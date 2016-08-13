mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE="$VARIANT" ..

if [ "$VARIANT" = "Coverage" ]; then 
    make kl-coverage
else 
    make
fi

if [ "$VARIANT" = "Coverage" ]; then 
    echo "Sending code coverage data"
    ~/.local/bin/codecov -X gcov
fi
