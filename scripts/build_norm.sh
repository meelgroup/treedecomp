#!/usr/bin/env bash
set -euo pipefail

rm -rf .cmake CMake* src cmake* Make*
cmake ..
make -j12
