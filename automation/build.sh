#!/usr/bin/env bash

echo Building hearty_rabbit $BUILD_NUMBER

#git clone --recursive https://github.com/nestal/hearty_rabbit.git
mkdir build
cd build
cmake -DBOOST_ROOT=/build/boost_1_66_0 -DCMAKE_BUILD_TYPE=Release ../hearty_rabbit
make -j4 package
