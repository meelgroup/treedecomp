#!/bin/bash

rm -rf .cmake CMake* src cmake* Make*
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j12 VERBOSE=1
