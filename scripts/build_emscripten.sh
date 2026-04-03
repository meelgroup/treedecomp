#!/bin/bash
set -e

rm -rf .cmake
rm -rf CMake*
rm -rf src
rm -rf cmake*
rm -rf Make*
emcmake cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=$EMINSTALL -DENABLE_TESTING=OFF ..
emmake make -j$(nproc)
emmake make install
