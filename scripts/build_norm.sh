#!/bin/bash

rm -rf .cmake CMake* src cmake* Make*
cmake ..
make -j12
