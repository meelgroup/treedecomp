#!/bin/bash

rm -rf .cmake
rm -rf CMake*
rm -rf src
rm -rf cmake*
rm -rf Make*
cmake -DBUILD_SHARED_LIBS=OFF ..
make -j$(nproc)
