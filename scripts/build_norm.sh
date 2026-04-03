#!/usr/bin/env bash
set -euo pipefail

rm -rf .cmake
rm -rf CMake*
rm -rf src
rm -rf cmake*
rm -rf Make*
cmake ..
make -j$(nproc)
