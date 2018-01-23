FROM centos:7 as builder
MAINTAINER [Nestal Wan <me@nestal.net>]
ENV container docker

# install development tools
RUN yum install -y centos-release-scl epel-release \
	&& rpm --import /etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-SIG-SCLo \
	&& rpm --import /etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-7 \
	&& yum install -y wget devtoolset-7 systemd-devel openssl-devel \
		rpm-build rapidjson-devel hiredis-devel file-devel \
		libicu-devel zlib-devel bzip2-devel xz-devel \
	&& source scl_source enable devtoolset-7 \
	&& mkdir /build

# Set PATH, because "scl enable" does not have any effects to "docker build"
# https://github.com/bit3/jsass/blob/master/src/main/docker/build-linux-x64/Dockerfile
ENV PATH $PATH:/opt/rh/devtoolset-7/root/usr/bin:/opt/cmake-3.10.1/bin

WORKDIR /build

# build cmake 3.10.1 (>= 3.9 to support building RPM with default filename)
ADD https://cmake.org/files/v3.10/cmake-3.10.1.tar.gz /build
RUN tar zxf cmake-3.10.1.tar.gz \
	&& cd cmake-3.10.1 \
	&& ./configure --prefix=/opt/cmake-3.10.1 \
	&& make -j8 install

# build boost (>1.65 to support boost beast)
ADD https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz /build
RUN tar zxf boost_1_66_0.tar.gz \
	&& cd boost_1_66_0 \
	&& ./bootstrap.sh \
	&& (./b2 -j8; exit 0)

# Copy source code. Not using git clone because the code may not be committed
# yet in development builds
COPY . /build/hearty_rabbit
RUN mkdir /build/docker-build \
	&& /build/hearty_rabbit/automation/build.sh

FROM scratch
COPY --from=builder /build/docker-build/hearty_rabbit /
CMD /hearty_rabbit
