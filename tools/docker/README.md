# Natron Linux SDK

This directory contains tools to build Natron using docker images (requires [Docker]).

These docker images are based on CentOS6 for a maximum compatibility accross all Linux distributions (CentOS6 will receive maintenance updates until November 30th, 2020). This can be modified by changing the value of `DOCKER_BASE` in `tools/jenkins/insclude/scripts/build-Linux-sdk.sh`.

## Downloading the pre-built Natron SDK images

The `natrongithub/natron-sdk:latest` and `natrongithub/natron-sdk-i386:latest` docker images are available from [Docker Hub].

To download these images, for the [64-bit SDK] execute:
```
docker pull natrongithub/natron-sdk
```
and for the [32-bit SDK]:
```
docker pull natrongithub/natron-sdk-i386
```

## Launching a build

To launch a build:

- create a directory when the build artifacts should be put (`$(pwd)/artifacts` in the following), another one for the builds (`$(pwd)/builds` in the following), and launch a build that binds these to directories to `/home/jenkins_artifacts` and `/home/builds_archive`:
```
mkdir artifacts; mkdir builds; docker run -it --rm --mount src="$(pwd)/artifacts",target=/home/jenkins_artifacts,type=bind --mount src="$(pwd)/builds",target=/home/builds_archive,type=bind natrongithub/natron-sdk:latest
```
(for the 32-bit version, use `natrongithub/natron-sdk-i386:latest` instead of `natrongithub/natron-sdk:latest`)

By default, this launches a snapshot build, but you may want to customize build variables, which appear when the build starts and are described at the top of `tools/jenkins/launchBuildMain.sh`. This can be done by adding the following options to the `docker run`command: `--env VAR1=value1 --env VAR2=value2 ...`.

For example, to limit the number of parallel build jobs to 2:
```
docker run -it --rm --mount src="$(pwd)/artifacts",target=/home/jenkins_artifacts,type=bind --mount src="$(pwd)/builds",target=/home/builds_archive,type=bind --env MKJOBS=2 natrongithub/natron-sdk:latest
```


## Debugging a build

If something goes bad during a build, you may want to launch it from an interactive shell instead.
```
docker run -it --rm --mount src="$(pwd)/artifacts",target=/home/jenkins_artifacts,type=bind --mount src="$(pwd)/builds",target=/home/builds_archive,type=bind  natrongithub/natron-sdk:latest bash
```
Then you can execute manually `launchBuildMain.sh` from the shell.


## Building the SDK images

If you change anything in the Natron SDK, including package versions (as found in `tools/jenkins/insclude/scripts/pkg`) or the build script itself (`tools/jenkins/include/scripts/build-Linux-SDK.sh`), you may want to re-build the SDK images.

- `tools/docker/natron-sdk/build.sh` builds the 64-bit Natron SDK docker image, and gives instructions to tag it and push it to the docker hub.
- `tools/docker/natron-sdk-i386/build.sh` builds the 32-bit Natron SDK docker image.



[Docker]: https://docs.docker.com/
[Docker Hub]: https://hub.docker.com
[64-bit SDK]: https://hub.docker.com/r/natrongithub/natron-sdk
[32-bit SDK]: https://hub.docker.com/r/natrongithub/natron-sdk-i386
