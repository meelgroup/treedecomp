#!/bin/bash

rm -rf .cmake CMake* src cmake* Make*
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j12 VERBOSE=1
