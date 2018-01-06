FROM centos:7
MAINTAINER [Nestal Wan <me@nestal.net>]
ENV container docker

# install development tools
RUN yum install -y centos-release-scl \
	&& rpm --import /etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-SIG-SCLo \
	&& yum install -y wget devtoolset-7 systemd-devel git openssl-devel rpm-build \
	&& scl enable devtoolset-7 bash

#ENV PATH $PATH:/opt/rh/devtoolset-7/root/usr/bin
ENV PATH $PATH:/opt/rh/devtoolset-7/root/usr/bin:/opt/cmake-3.10.1/bin

RUN mkdir build
WORKDIR /build

# build cmake
RUN wget https://cmake.org/files/v3.10/cmake-3.10.1.tar.gz \
	&& tar zxvf cmake-3.10.1.tar.gz \
	&& cd cmake-3.10.1 \
	&& ./configure --prefix=/opt/cmake-3.10.1 \
	&& make \
	&& make install

# build boost
RUN wget https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz \
	&& tar zxvf boost_1_66_0.tar.gz \
	&& cd boost_1_66_0 \
	&& ./bootstrap.sh

RUN cd boost_1_66_0; ./b2; exit 0

# get hearty_rabbit source code
COPY build.sh /build

