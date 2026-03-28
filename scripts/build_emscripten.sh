#!/bin/bash
set -e

rm -rf .cmake CMake* src cmake* Make*
emcmake cmake -DSTATICCOMPILE=ON -DCMAKE_INSTALL_PREFIX=$EMINSTALL -DENABLE_TESTING=OFF ..
emmake make -j$(nproc)
emmake make install
