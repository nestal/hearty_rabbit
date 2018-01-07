# HeartyRabbit ![status](https://travis-ci.org/nestal/hearty_rabbit.svg?branch=develop)
It's not doing anything useful yet.

# Build Environment

In order maintain a stable build environment to product predictable software
builds, HeartyRabbit uses docker containers to build. The docker image of the
build environment can be found in [docker hub](https://hub.docker.com/r/nestal/hearty_rabbit_dev/).

The docker image can be built by the dockerfile is [`automatic/Dockerfile`](automation/Dockerfile).
Run

	docker build -t hearty_rabbit_dev .
	
In the `automation` directory to build the docker image. The command to run the image
and start the build process in encapsulated in a [Makefile](automation/Makefile).
It can be triggered by:

	make -f automation/Makefile
	
To launch a build. The output RPM will be put in `/build/build/hearty-rabbit-0.1-9.el7.centos.x64_64.rpm`
You can use `docker cp` to copy the RPM to target box.

## Details About the Build Environment

The [hearty_rabbit_dev](https://hub.docker.com/r/nestal/hearty_rabbit_dev/)
image is base on latest [CentOS 7](https://hub.docker.com/_/centos/).
[devtoolset-7](https://www.softwarecollections.org/en/scls/rhscl/devtoolset-7/),
which contains [GCC 7.2.1](https://gcc.gnu.org/gcc-7/). In addition, the
following libraries are included from official CentOS repository to build
HeartyRabbit:

- systemd-devel
- RapidJSON
- openssl-devel
- postgresql-libs

In addition, HeartyRabbit also requires [Boost libraries](http://boost.org) and
[CMake](https://cmake.org) to build. However, the official packages from
CentOS 7.4 is too old to build HeartyRabbit. These two packages are built
from source.

# Travis Automation

The [`.travis.yaml`](.travis.yml) script basically calls `make -f automation/Makefile` so the
travis build is basically the same as local builds.