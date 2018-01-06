FROM centos:7
MAINTAINER [Nestal Wan <me@nestal.net>]
ENV container docker

# install development tools
RUN yum install -y centos-release-scl epel-release \
	&& rpm --import /etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-SIG-SCLo \
	&& rpm --import /etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-7 \
	&& yum install -y wget devtoolset-7 systemd-devel git cmake3 \
	&& scl enable devtoolset-7 bash

ENV PATH $PATH:/opt/rh/devtoolset-7/root/usr/bin

# build boost
RUN mkdir build \
	&& cd build \
	&& wget https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz \
	&& tar zxvf boost_1_66_0.tar.gz \
	&& cd boost_1_66_0 \
	&& ./bootstrap.sh \
	&& ./b2 \
	&& cd ..
	
RUN git clone https://github.com/nestal/hearty_rabbit \
	&& cd hearty_rabbit \
	&& mkdir build \
	&& cmake -DBOOST_ROOT=/build/boost_1_66_0 -DCMAKE_BUILD_TYPE=Release .. \
	&& make package
	