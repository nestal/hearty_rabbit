docker-build:
	mkdir -p docker-build-release
	@sudo docker run -it \
		--name hearty_rabbit_builder \
		--entrypoint /build/hearty_rabbit/automation/build.sh \
		-e "BUILD_NUMBER=$(BUILD_NUMBER)" \
		--mount type=bind,source=$(shell pwd),target=/build/hearty_rabbit,readonly \
		--mount type=bind,source=$(shell pwd)/docker-build-release,target=/build/docker-build \
		--rm \
		nestal/hearty_rabbit_dev
