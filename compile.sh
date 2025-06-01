mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j8
cp -r ../samples/. bin/
cp -r ../build/src/gen bin/gen
