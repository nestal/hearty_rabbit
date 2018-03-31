# HeartyRabbit

Branch  | Status
--------|--------
[Develop](https://github.com/nestal/hearty_rabbit/tree/develop) | ![status](https://travis-ci.org/nestal/hearty_rabbit.svg?branch=develop)
[Master](https://github.com/nestal/hearty_rabbit/tree/master)  | ![status](https://travis-ci.org/nestal/hearty_rabbit.svg?branch=master)

It's a web server develop using [boost/beast](https://github.com/boostorg/beast). It's
currently an image album that powered [nestal.net](https://www.nestal.net).

# Build Environment

In order maintain a stable build environment to product predictable software
builds, HeartyRabbit uses docker containers to build. The `hearty_rabbit_dev` image 
contain a full snapshot of the build environment. It can be found in [docker hub](https://hub.docker.com/r/nestal/hearty_rabbit_dev/).

The docker `hearty_rabbit_dev` image can be built by the dockerfile [`automation/Dockerfile`](automation/Dockerfile).

Run

	docker build -t hearty_rabbit_dev automation
	
in the source directory to build the docker image.

The `hearty_rabbit_dev` is a big image base on CentOS 7. The complete development
tool-chain, e.g. `devtoolset-7`, `cmake` and `boost` have been installed to
make sure HeartyRabbit can be built successfully.

## Details About the Build Environment

The [hearty_rabbit_dev](https://hub.docker.com/r/nestal/hearty_rabbit_dev/)
image is base on latest [CentOS 7](https://hub.docker.com/_/centos/).
[devtoolset-7](https://www.softwarecollections.org/en/scls/rhscl/devtoolset-7/),
which contains [GCC 7.2.1](https://gcc.gnu.org/gcc-7/), provides the whole tool
chain that builds HeartyRabbit. In addition, the following libraries are included
from official CentOS repository to build HeartyRabbit:

- RapidJSON
- openssl-devel
- hiredis
- libunwind
- libmagic (file-devel)
- libicu-devel/zlib-devel/bzip2-devel/xz-devel (optional dependencies for Boost)
- autoconf/automake/libtool (for building libb2)
- nasm (for building turbe-jpeg)

In addition, HeartyRabbit also requires [Boost libraries](http://boost.org),
[CMake](https://cmake.org) [turbo-jpeg](https://libjpeg-turbo.org/Documentation/Documentation�)
and [libb2](https://github.com/BLAKE2/libb2) to
build. However, the official packages of Boost, turbe-jpeg and CMake from CentOS 7.4 is
too old to build HeartyRabbit, and there's no official packages for libb2.
These three packages are built from source.

# Building Hearty Rabbit

Run

	docker build -t hearty_rabbit --network=host .
	
in the source directory to build Hearty Rabbit. The resultant image is a
complete installation of Hearty Rabbit encapsulated in a docker container.
The `--network=host` option is required because the unit tests need to
connect to the redis server in the host.

# Travis Automation

The [`.travis.yaml`](.travis.yml) script basically calls
`docker build -t hearty_rabbit --network=host .` so the
travis build is basically the same as local builds.

# Notes/Reminder

Need a "ROOT" search index base on the date of the images taken

year -> month -> day level of containers

Host quota for invalid session IDs and incorrect logins to prevent brute-force
attacks.
