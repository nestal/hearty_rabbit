#!/usr/bin/env bash

# Parent directory of the script
dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Have this script to exit if any of the step below fails
set -e

docker run -t --detach --rm \
  -v $dir/../etc:/etc \
  -v $dir/../lib:/lib \
  --network=host --name hb_test \
    nestal/hearty_rabbit

curl -k https://localhost:4433/login.html
docker stop hb_test
