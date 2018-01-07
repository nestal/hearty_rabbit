docker-build:
	@sudo docker run -it \
		--name hearty_rabbit_builder \
		--entrypoint /build/hearty_rabbit/automation/build.sh \
		-e "BUILD_NUMBER=$BUILD_NUMBER" \
		--mount type=bind,source=$(shell pwd),target=/build/hearty_rabbit,readonly \
		nestal:hearty_rabbit_dev
	@sudo docker cp hearty_rabbit_builder:/build/build/hearty-rabbit-0.1-$BUILD_NUMBER.el7.centos.x86_64.rpm .
	@sudo docker rm /hearty_rabbit_builder
