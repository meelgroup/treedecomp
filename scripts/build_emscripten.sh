#!/bin/bash
set -e

rm -rf .cmake CMake* src cmake* Make*
emcmake cmake -DCMAKE_INSTALL_PREFIX=$EMINSTALL -DENABLE_TESTING=OFF ..
emmake make -j26
emmake make install
cp ganak.wasm ../html
cp $EMINSTALL/bin/ganak.js ../html
