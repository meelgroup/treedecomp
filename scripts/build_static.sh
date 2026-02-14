#!/bin/bash

rm -rf .cmake CMake* src cmake* Make*
cmake -DSTATICCOMPILE=ON ..
make -j14
