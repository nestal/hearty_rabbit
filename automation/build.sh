#!/usr/bin/env bash

echo Building hearty_rabbit $BUILD_NUMBER

# Try enabling devtoolset-7
source scl_source enable devtoolset-7

cd docker-build
cmake -DBOOST_ROOT=/build/boost_1_66_0 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/opt/hearty_rabbit ../hearty_rabbit
make -j8 package
