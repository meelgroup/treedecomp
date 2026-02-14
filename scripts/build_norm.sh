rm -rf .cmake CMake* src cmake* Make*
cmake -DENABLE_TESTING=ON ..
make -j12
make test
