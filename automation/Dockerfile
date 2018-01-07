FROM centos:7
MAINTAINER [Nestal Wan <me@nestal.net>]
ENV container docker

# install development tools
RUN yum install -y centos-release-scl \
	&& rpm --import /etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-SIG-SCLo \
	&& yum install -y wget devtoolset-7 systemd-devel git openssl-devel rpm-build rapidjson-devel \
	&& scl enable devtoolset-7 bash \
	&& mkdir build

ENV PATH $PATH:/opt/rh/devtoolset-7/root/usr/bin:/opt/cmake-3.10.1/bin

WORKDIR /build

# build cmake
RUN wget --quiet https://cmake.org/files/v3.10/cmake-3.10.1.tar.gz \
	&& tar zxf cmake-3.10.1.tar.gz \
	&& cd cmake-3.10.1 \
	&& ./configure --prefix=/opt/cmake-3.10.1 \
	&& make -j8 install

# build boost
RUN wget --quiet https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz \
	&& tar zxf boost_1_66_0.tar.gz \
	&& cd boost_1_66_0 \
	&& ./bootstrap.sh \
	&& (./b2 -j8; exit 0)

# build script as entrypoint
COPY build.sh /build
