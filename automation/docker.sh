mkdir -p docker-build-release
sudo docker run -it \
	--name hearty_rabbit_builder \
	--entrypoint /build/hearty_rabbit/automation/build.sh \
	-e "BUILD_NUMBER=$BUILD_NUMBER" \
	--mount type=bind,source=$(pwd),target=/build/hearty_rabbit,readonly \
	--mount type=bind,source=$(pwd)/docker-build-release,target=/build/docker-build \
	--net=host \
	--rm \
	nestal/hearty_rabbit_dev
