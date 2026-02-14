rm -rf .cmake CMake* src cmake* Make*
cmake -DCMAKE_BUILD_TYPE=Release -DSTATICCOMPILE=ON ..
make -j4
