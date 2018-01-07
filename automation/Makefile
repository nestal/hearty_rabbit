docker-build:
	@sudo docker run -it \
		--name build_hearty_rabbit \
		--entrypoint /build/hearty_rabbit/automation/build.sh \
		-e "BUILD_NUMBER=$BUILD_NUMBER" \
		--mount type=bind,source=$(shell pwd),target=/build/hearty_rabbit \
		nestal/hearty_rabbit_dev
	@sudo docker cp build_hearty_rabbit:/build/build/hearty-rabbit-0.1-$BUILD_NUMBER.el7.centos.x86_64.rpm .
	