FROM centos:7 as builder
MAINTAINER [Nestal Wan <me@nestal.net>]
ENV container docker

# install development tools
RUN yum install -y centos-release-scl epel-release \
	&& rpm --import /etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-SIG-SCLo \
	&& rpm --import /etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-7 \
	&& yum install -y devtoolset-7 openssl-devel git rapidjson-devel \
		hiredis-devel file-devel libicu-devel zlib-devel bzip2-devel xz-devel \
		libunwind-devel autoconf automake libtool libexif-devel turbojpeg-devel \
	&& source scl_source enable devtoolset-7 \
	&& yum update -y \
	&& mkdir /build

# Set PATH, because "scl enable" does not have any effects to "docker build"
# https://github.com/bit3/jsass/blob/master/src/main/docker/build-linux-x64/Dockerfile
ENV PATH $PATH:/opt/rh/devtoolset-7/root/usr/bin:/build/cmake-3.10.2/bin

WORKDIR /build

# build boost (>1.65 to support boost beast)
ADD https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz /build
RUN tar zxf boost_1_66_0.tar.gz \
	&& cd boost_1_66_0 \
	&& ./bootstrap.sh --with-libraries=program_options,filesystem,system \
	&& (./b2 -j8; exit 0)

# build cmake 3.10.2 (>= 3.8 to support C++17)
ADD https://cmake.org/files/v3.10/cmake-3.10.2.tar.gz /build
RUN tar zxf cmake-3.10.2.tar.gz \
	&& cd cmake-3.10.2 \
	&& ./configure --prefix=/opt/cmake-3.10.2 \
	&& make -j8 install

# build libb2
# Don't enable native CPU optimization because the build machine may be different from
# the current one.
RUN git clone https://github.com/BLAKE2/libb2.git
RUN cd libb2 \
	&& ./autogen.sh \
	&& ./configure --prefix=/opt/libb2 --enable-native=no \
	&& make install
