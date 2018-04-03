FROM centos/devtoolset-7-toolchain-centos7:7 as builder
MAINTAINER [Nestal Wan <me@nestal.net>]
ENV container docker

USER 0

# install development tools
RUN yum install -y deltarpm epel-release \
	&& rpm --import /etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-7 \
	&& yum install -y --setopt=tsflags=nodocs openssl-devel git make rapidjson-devel hiredis-devel \
		file-devel libicu-devel zlib-devel bzip2-devel xz-devel libunwind-devel which \
		autoconf automake libtool exiv2-devel nasm \
	&& source scl_source enable devtoolset-7 \
	&& yum clean all -y \
	&& mkdir /build

ADD https://github.com/libjpeg-turbo/libjpeg-turbo/archive/1.5.3.tar.gz \
	https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz \
	https://cmake.org/files/v3.11/cmake-3.11.0.tar.gz /build/

WORKDIR /build

# libjpeg-turbo
# The libjpeg-turbo package provided by CentOS 7 uses a different API version than
# Fedora. For consistency, build the latest version from source and use that.
RUN tar zxf 1.5.3.tar.gz \
	&& cd libjpeg-turbo-1.5.3 \
	&& autoreconf -fiv \
	&& ./configure --prefix=/opt/libjpeg-turbo \
	&& make -j8 install

# build boost (>1.65 to support boost beast)
RUN tar zxf boost_1_66_0.tar.gz \
	&& cd boost_1_66_0 \
	&& ./bootstrap.sh \
		--with-libraries=program_options,filesystem,system \
		--prefix=/opt/boost_1_66_0 \
	&& (./b2 -j8 install; exit 0) \
	&& rm -rf /build/boost_1_66_0

# build cmake 3.11.0 (>= 3.8 to support C++17)
RUN tar zxf cmake-3.11.0.tar.gz \
	&& cd cmake-3.11.0 \
	&& ./configure --prefix=/opt/cmake-3.11.0 \
	&& make -j8 install

# build libb2
# Don't enable native CPU optimization because the build machine may be different from
# the current one.
RUN git clone https://github.com/BLAKE2/libb2.git
RUN cd libb2 \
	&& git checkout v0.98 \
	&& ./autogen.sh \
	&& ./configure --prefix=/opt/libb2 --enable-native=no \
	&& make install

ADD https://github.com/nlohmann/json/releases/download/v3.1.2/json.hpp /opt/nlohmann/include/
