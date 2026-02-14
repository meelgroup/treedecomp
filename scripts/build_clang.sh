rm -rf .cmake CMake* src cmake* Make*
cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang ..
make -j14
