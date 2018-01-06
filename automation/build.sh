#!/usr/bin/env bash

# assume source code is in /build/hearty_rabbit
cd /build
mkdir build
cd build
cmake -DBOOST_ROOT=/build/boost_1_66_0 -DCMAKE_BUILD_TYPE=Release ../hearty_rabbit
make package
