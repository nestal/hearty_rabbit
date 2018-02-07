#!/usr/bin/env bash

# Find the root directory of the source code base on the location of this script
pushd "$( dirname "${BASH_SOURCE[0]}" )"/..
pwd

# Have this script to exit if any of the step below fails
set -e

docker run -t --detach --rm \
  -v $(pwd)/etc/hearty_rabbit:/etc/hearty_rabbit \
  -v $(pwd)/lib:/lib \
  --network=host --name hb_test \
    nestal/hearty_rabbit

curl -k https://localhost:4433/login.html
docker stop hb_test
