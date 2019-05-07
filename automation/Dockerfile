FROM centos:7 as builder
MAINTAINER [Nestal Wan <me@nestal.net>]
ENV container docker

USER 0

# install development tools
RUN yum install -y deltarpm epel-release centos-release-scl \
	&& rpm --import /etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-7 \
	&& yum update -y \
	&& yum install -y --setopt=tsflags=nodocs openssl-devel git make hiredis-devel \
		file-devel libicu-devel zlib-devel bzip2-devel xz-devel libunwind-devel which \
		autoconf automake libtool libexif-devel msgpack-devel libpng-devel libcurl-devel \
		cmake3 libb2-devel gperftools-devel devtoolset-8 \
	&& yum clean all -y

# add required packages after installing packages
#	https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.gz \
#	https://github.com/opencv/opencv/archive/3.4.1.tar.gz \
ADD \
	boost_1_70_0.tar.gz 3.4.1.tar.gz \
	/build/

WORKDIR /build

ENV \
	PATH="/opt/rh/devtoolset-8/root/usr/bin:/opt/bin:${PATH}" \
	LD_LIBARY_PATH="/opt/rh/devtoolset-8/root/usr/lib64"

# build boost (>1.65 to support boost beast)
RUN \
	cd boost_1_70_0 \
	&& ./bootstrap.sh \
		--with-libraries=program_options,filesystem,system \
		--prefix=/opt/boost_1_70 \
	&& (./b2 -j8 install; exit 0) \
	&& rm -rf /build/boost_1_70_0

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
ADD https://github.com/nlohmann/json/releases/download/v3.6.1/json.hpp /opt/include/nlohmann/
