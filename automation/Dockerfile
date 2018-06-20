FROM centos/devtoolset-7-toolchain-centos7:7 as builder
MAINTAINER [Nestal Wan <me@nestal.net>]
ENV container docker

USER 0

# install development tools
RUN yum install -y deltarpm epel-release \
	&& rpm --import /etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-7 \
	&& yum update -y \
	&& yum install -y --setopt=tsflags=nodocs openssl-devel git make hiredis-devel \
		file-devel libicu-devel zlib-devel bzip2-devel xz-devel libunwind-devel which \
		autoconf automake libtool exiv2-devel msgpack-devel libpng-devel nasm libcurl-devel \
	&& source scl_source enable devtoolset-7 \
	&& yum clean all -y \
	&& mkdir /build

WORKDIR /build

ENV PATH="/build/cmake-3.11.1-Linux-x86_64/bin:/opt/bin:/opt/bin:${PATH}"

# download packages
ADD \
	1.5.3.tar.gz boost_1_67_0.tar.gz cmake-3.11.1-Linux-x86_64.tar.gz 3.4.1.tar.gz \
#	https://github.com/libjpeg-turbo/libjpeg-turbo/archive/1.5.3.tar.gz \
#	https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.gz \
#	https://cmake.org/files/v3.11/cmake-3.11.1-Linux-x86_64.tar.gz \
#	https://github.com/opencv/opencv/archive/3.4.1.tar.gz \
		/build/

# libjpeg-turbo
# The libjpeg-turbo package provided by CentOS 7 uses a different API version than
# Fedora. For consistency, build the latest version from source and use that.
RUN  \
	cd libjpeg-turbo-1.5.3 \
	&& autoreconf -fiv \
	&& ./configure --prefix=/opt \
	&& make -j8 install

# build boost (>1.65 to support boost beast)
RUN \
	cd boost_1_67_0 \
	&& ./bootstrap.sh \
		--with-libraries=program_options,filesystem,system \
		--prefix=/opt/boost_1_67 \
	&& (./b2 -j8 install; exit 0) \
	&& rm -rf /build/boost_1_67_0

# build libb2
# Don't enable native CPU optimization because the build machine may be different from
# the current one.
RUN git clone --depth 1 -b v0.98 https://github.com/BLAKE2/libb2.git
RUN cd libb2 \
	&& ./autogen.sh \
	&& ./configure --prefix=/opt --enable-native=no \
	&& make install

# OpenCV
RUN \
	cd opencv-3.4.1 \
	&& mkdir build \
	&& cd build \
	&& cmake \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX=/opt \
		-DBUILD_SHARED_LIBS=OFF \
		-DBUILD_LIST=core,imgproc,imgcodecs \
		-DBUILD_opencv_legacy=OFF \
		-DBUILD_JASPER=OFF \
		-DBUILD_WEBP=OFF \
		-DBUILD_TBB=OFF \
		-DBUILD_IPP_IW=OFF \
		-DBUILD_ITT=OFF \
		-DBUILD_OPENEXR=OFF \
		-DWITH_FFMPEG=OFF \BUILD_OPENEXR \
		-DWITH_GSTREAMER=OFF \
		-DWITH_GTK=OFF \
		-DWITH_IPP=OFF \
		-DWITH_JASPER=OFF \
		-DWITH_WEBP=OFF \
		-DWITH_OPENEXR=OFF \
			.. \
	&& make -j8 install \
	&& cd .. && rm -rf build

RUN git clone --depth 1 -b 1.4.60 https://github.com/aws/aws-sdk-cpp.git
RUN cd aws-sdk-cpp; mkdir build; cd build; cmake -DBUILD_SHARED_LIBS=OFF ..; make -j8

ADD https://github.com/nlohmann/json/releases/download/v3.1.2/json.hpp /opt/include/
