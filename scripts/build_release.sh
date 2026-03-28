#!/bin/bash

rm -rf .cmake CMake* src cmake* Make*
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) VERBOSE=1
