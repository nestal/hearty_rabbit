FROM centos:7 as builder
MAINTAINER [Nestal Wan <me@nestal.net>]
ENV container docker

USER 0

# add devtoolset rpms
ADD \
	devtoolset-8-gcc-8.2.1-3.el7.x86_64.rpm \
	devtoolset-8-gcc-c++-8.2.1-3.el7.x86_64.rpm \
	devtoolset-8-binutils-2.30-47.el7.x86_64.rpm \
	devtoolset-8-libstdc++-devel-8.2.1-3.el7.x86_64.rpm \
	devtoolset-8-runtime-8.0-2.el7.x86_64.rpm \
	/tmp/

# install development tools
RUN yum install -y deltarpm epel-release centos-release-scl \
	&& rpm --import /etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-7 \
	&& yum update -y \
	&& cd /tmp && yum install -y *.rpm \
	&& yum install -y --setopt=tsflags=nodocs openssl-devel git make hiredis-devel \
		file-devel libicu-devel zlib-devel bzip2-devel xz-devel libunwind-devel which \
		autoconf automake libtool libexif-devel msgpack-devel libpng-devel nasm libcurl-devel \
		gmp-devel mpfr-devel libmpc-devel cmake3 \
	&& yum clean all -y \
	&& mkdir /build

WORKDIR /build

# download packages
ADD \
	boost_1_68_0.tar.gz 3.4.1.tar.gz \
#	https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.gz \
#	https://cmake.org/files/v3.11/cmake-3.11.1-Linux-x86_64.tar.gz \
#	https://github.com/opencv/opencv/archive/3.4.1.tar.gz \
#   http://mirror-hk.koddos.net/gnu/gcc/gcc-8.2.0/gcc-8.2.0.tar.xz \
		/build/

# build gcc 8.2.0
#RUN \
#	mkdir gcc-build \
#	&& cd gcc-build \
#	&& ../gcc-8.2.0/configure \
#		--prefix=/opt/gcc-8.2.0-multilib \
#		--enable-languages=c,c++ \
#		--enable-lto \
#		--enable-multilib \
#		--with-multilib-list=m64 \
#		--enable-ld \
#		--enable-gold \
#		--with-system-zlib \
#		--build=x86_64-linux-gnu \
#	&& make -j8 \
#	&& make install

ENV \
	PATH="/opt/rh/devtoolset-8/root/usr/bin:/opt/bin:${PATH}" \
	LD_LIBARY_PATH="/opt/rh/devtoolset-8/root/usr/lib64"

# remove the system gcc so that our gcc 8.2 will always be used
#RUN rpm -e --nodeps gcc gcc-c++

# build boost (>1.65 to support boost beast)
RUN \
	cd boost_1_68_0 \
	&& ./bootstrap.sh \
		--with-libraries=program_options,filesystem,system \
		--prefix=/opt/boost_1_68 \
	&& (./b2 -j8 install; exit 0) \
	&& rm -rf /build/boost_1_68_0

# build libb2
# Don't enable native CPU optimization because the machine that builds this image may be different from
# the machine that builds and run the code.
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
	&& cmake3 \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX=/opt \
		-DBUILD_SHARED_LIBS=OFF \
		-DBUILD_LIST=core,imgproc,imgcodecs,objdetect,highgui \
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

# AWS libraries: for future requirements to support S3 backup
#RUN git clone --depth 1 -b 1.4.60 https://github.com/aws/aws-sdk-cpp.git
#RUN cd aws-sdk-cpp && mkdir build && cd build && cmake -DBUILD_SHARED_LIBS=OFF .. && make -j8

# nlohmann JSON library
ADD https://github.com/nlohmann/json/releases/download/v3.1.2/json.hpp /opt/include/nlohmann/
