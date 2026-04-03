#!/bin/bash

rm -rf .cmake
rm -rf CMake*
rm -rf src
rm -rf cmake*
rm -rf Make*
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc) VERBOSE=1
