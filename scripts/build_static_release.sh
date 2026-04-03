#!/bin/bash

rm -rf .cmake
rm -rf CMake*
rm -rf src
rm -rf cmake*
rm -rf Make*
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF ..
make -j$(nproc)
