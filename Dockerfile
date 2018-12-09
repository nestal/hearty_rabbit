FROM nestal/hearty_rabbit_dev as builder
MAINTAINER [Nestal Wan <me@nestal.net>]
ARG BUILD_NUMBER
ENV container docker

ENV PATH="$PATH:/opt/bin" \
	PKG_CONFIG_PATH=/opt/lib/pkgconfig/ \
	LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/opt/lib:/opt/gcc-8.2.0-multilib/lib64"

# Copy source code. Not using git clone because the code may not be committed
# yet in development builds
COPY . /build/src
RUN mkdir /build/docker-build \
	&& chmod -R a+rX /build/src \
	&& cd docker-build \
	&& cmake3 \
		-DBOOST_ROOT=/opt/boost_1_68 \
		-DBUILD_NUMBER=$BUILD_NUMBER \
		-DCMAKE_PREFIX_PATH=/opt \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX=/build/hearty_rabbit \
		-DHAARCASCADE_PATH="/build/opencv-3.4.1/data/haarcascades/" \
			../src \
	&& make -j8 install

# chmod before copy
RUN chmod a+rX -R /build/opencv-3.4.1/data/haarcascades/

# Copy products to runtime docker image
FROM scratch
COPY --from=builder /build/hearty_rabbit/ /

# Libraries required for running hearty_rabbit
COPY --from=builder \
	/lib64/libmagic.so.1 \
	/lib64/libhiredis.so.0.12  \
	/lib64/libssl.so.10  \
	/lib64/libcrypto.so.10  \
	/lib64/libpthread.so.0  \
	/lib64/libm.so.6  \
	/lib64/libgcc_s.so.1  \
	/lib64/libstdc++.so.6  \
	/lib64/libc.so.6  \
	/lib64/libz.so.1  \
	/lib64/librt.so.1 \
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
	/lib64/libpng15.so.15 \
		/lib64/

# Copy the magic cookie file for libmagic
COPY --from=builder /usr/share/misc/magic /usr/share/misc/magic

# Copy HAAR-cascade models
COPY --from=builder \
	/build/opencv-3.4.1/data/haarcascades/haarcascade_frontalface_default.xml \
	/build/opencv-3.4.1/data/haarcascades/haarcascade_eye_tree_eyeglasses.xml \
		/build/opencv-3.4.1/data/haarcascades/

# Use environment to specify config file location.
# It works even when we run --add-user
ENV HEARTY_RABBIT_CONFIG /etc/hearty_rabbit/hearty_rabbit.json
ENTRYPOINT ["/bin/hearty_rabbit"]
