#!/bin/bash

rm -rf .cmake CMake* src cmake* Make*
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF ..
make -j$(nproc)
