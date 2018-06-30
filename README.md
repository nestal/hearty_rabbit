# HeartyRabbit

Branch  | Status
--------|--------
[Develop](https://github.com/nestal/hearty_rabbit/tree/develop) | ![status](https://travis-ci.org/nestal/hearty_rabbit.svg?branch=develop)
[Master](https://github.com/nestal/hearty_rabbit/tree/master)  | ![status](https://travis-ci.org/nestal/hearty_rabbit.svg?branch=master)

It's a web server develop using [boost/beast](https://github.com/boostorg/beast). It's
currently an image album that powered [nestal.net](https://www.nestal.net).

# Installation

Use docker:

	docker pull nestal/hearty_rabbit

See the [systemd unit file](systemd/hearty_rabbit.service) for
a complete command line for `docker run`. The extra options are mostly for mounting volumes from the host.

There are three directories need to be mounted to the container:

-   The directory that contain the configuration file (and the certificate/private key files specified by the
    configuration file)
-   The directory that contain all the uploaded images.
-   `/dev/log` for `syslog()`, which is used for logging.

HeartyRabbit also require [Redis](https://redis.io/).

# Configuration

HeartyRabbit uses a JSON configuration file. By default it is in `/etc/hearty_rabbit/hearty_rabbit.json`.
It can be specified by the `HEART_RABBIT_CONFIG` environment variable or using command line options `--cfg`.

The configuration file specifies the location of other files used by HeartyRabbit. If you use relative
paths for these configuration files, they will be relative to the location of the configuration file,
not the current directory of the HeartyRabbit process.

-   `web_root`: the directory that contains the HTML, CSS and javascript files served by HeartyRabbit
    as a web server. This directory is typically provided by the docker image, so there is little
    reason to change this. By default, it points to `../../lib`. Relative to
    `/etc/hearty_rabbit/hearty_rabbit.json`, it actually resolves to `/lib,` which is the actual
    directory inside the docker image that contains the [lib](lib) directory of the source.
    This directory must be readable by the user that runs HeartyRabbit (by default UID 65535).
-   `blob_path`: the directory that store uploaded images, or _blobs_ (Binary Large OBjects).
    Images or and uploaded files are stored in subdirectories that named by their Blake2 hash
    under `blob_path`.
-   `cert_chain` and `private_key`: both files are used for SSL/TLS to support HTTPS. Normally
    they are provided by a certificate authority. For testing purpose the
    [HeartyRabbit source](etc/hearty_rabbit) include a self-signed certificate for
    automated testing. Please do not use them for production.
-   `redis`: Optional. IP address and port number of the Redis server. The default setting
	 is `127.0.0.1/6379`. We need to pass `--network=host` to let HeartyRabbit if Redis
	 is running in the host for this to work. 
-   `https`: Local IP address and port number HeartyRabbit listens to for HTTPS.
	 Normally it should be `0.0.0.0/443`. For testing we use port `4433` just in
	 case that HeartyRabbit is not run by root.
-   `http`: Local IP address and port number for HTTP. Normally it should be
	`0.0.0.0/80`. HeartyRabbit will redirect all requests to HTTPS.
-   `uid`/`gid`: Optional. Effective user ID and group ID of the HeartyRabbit
	process after it drops privileges. Note that `blob_path` must be readable
	and writeable by this user. Currently only numerical values are accepted.
	User names and group names are not. Default value is `65535`. 

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

- openssl-devel
- hiredis
- libunwind
- libmagic (file-devel)
- libicu-devel/zlib-devel/bzip2-devel/xz-devel (optional dependencies for Boost)
- autoconf/automake/libtool (for building libb2)
- OpenCV
- libexif

In addition, HeartyRabbit also requires [Boost libraries](http://boost.org),
[CMake](https://cmake.org), [OpenCV](https://opencv.org/)
and [libb2](https://github.com/BLAKE2/libb2) to
build. However, the official packages of Boost, OpenCV and CMake from CentOS 7.4 is
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
