FROM nestal/hearty_rabbit_dev as builder
MAINTAINER [Nestal Wan <me@nestal.net>]
ENV container docker

ENV PATH=$PATH:/opt/rh/devtoolset-7/root/usr/bin:/build/cmake-3.10.2/bin \
	PKG_CONFIG_PATH=/opt/libjpeg-turbo/lib64/pkgconfig/ \
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/libjpeg-turbo/lib64

# Copy source code. Not using git clone because the code may not be committed
# yet in development builds
COPY . /build/hearty_rabbit
RUN mkdir /build/docker-build \
	&& cd docker-build \
	&& cmake \
		-DBOOST_ROOT=/build/boost_1_66_0 \
		-DCMAKE_PREFIX_PATH=/opt/libb2 \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX=/opt/hearty_rabbit \
			../hearty_rabbit \
	&& make -j8 install

# Copy products to runtime docker image
FROM scratch
COPY --from=builder /opt/hearty_rabbit/ /

# Libraries required for running hearty_rabbit
COPY --from=builder \
	/lib64/libmagic.so.1 \
	/lib64/libhiredis.so.0.12  \
	/lib64/libssl.so.10  \
	/lib64/libcrypto.so.10  \
	/lib64/libpthread.so.0  \
	/lib64/libstdc++.so.6  \
	/lib64/libm.so.6  \
	/lib64/libgcc_s.so.1  \
	/lib64/libc.so.6  \
	/lib64/libz.so.1  \
	/lib64/libgssapi_krb5.so.2  \
	/lib64/libkrb5.so.3  \
	/lib64/libcom_err.so.2  \
	/lib64/libk5crypto.so.3  \
	/lib64/libdl.so.2  \
	/lib64/ld-linux-x86-64.so.2  \
	/lib64/libkrb5support.so.0  \
	/lib64/libkeyutils.so.1  \
	/lib64/libresolv.so.2  \
	/lib64/libselinux.so.1  \
	/lib64/libpcre.so.1  \
	/lib64/libunwind.so.8 \
	/lib64/libtinfo.so.5  \
	/opt/libjpeg-turbo/lib64/libturbojpeg.so.0  \
		/lib64/

# Copy the magic cookie file for libmagic
COPY --from=builder /usr/share/misc/magic /usr/share/misc/magic

# Use environment to specify config file location.
# It works even when we run --add-user
ENV HEART_RABBIT_CONFIG /etc/hearty_rabbit/hearty_rabbit.json
ENTRYPOINT ["/bin/hearty_rabbit"]
