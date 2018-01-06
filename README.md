# HeartyRabbit ![status](https://travis-ci.org/nestal/hearty_rabbit.svg?branch=master)
Not sure yet

# Prepare Build Environment using Docker
The dockerfile that prepares the build environment is in `automation/Dockerfile`.
Run

	docker build -t nestal/hearty_rabbit_dev .
	
In the `automation` directory to build the docker image. Then run the image

	docker run -it --entrypoint /build/build.sh -e "BUILD_NUMBER=9" nestal/hearty_rabbit_dev
	
To launch a build. The output RPM will be put in `/build/build/hearty-rabbit-0.1-9.el7.centos.x64_64.rpm`
You can use `docker cp` to copy the RPM to target box.