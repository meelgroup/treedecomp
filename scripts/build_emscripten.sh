#!/bin/bash
set -e

rm -rf .cmake
rm -rf CMake*
rm -rf src
rm -rf cmake*
rm -rf Make*
emcmake cmake -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_INSTALL_PREFIX=$EMINSTALL \
    -DENABLE_TESTING=OFF ..
emmake make -j$(nproc)
emmake make install
