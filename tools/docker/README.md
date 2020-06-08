# Natron Linux SDK

This directory contains tools to build Natron using docker images (requires [Docker]).

These docker images are based on CentOS6 for a maximum compatibility accross all Linux distributions (CentOS6 will receive maintenance updates until November 30th, 2020). This can be modified by changing the value of `DOCKER_BASE` in `tools/jenkins/insclude/scripts/build-Linux-sdk.sh`.

## Downloading the pre-built Natron SDK images

The `natrongithub/natron-sdk-centos6-dts8:latest` and `natrongithub/natron-sdk-centos6:latest` docker images are available from [Docker Hub].

To download these images, for the [64-bit SDK] execute:
```
docker pull natrongithub/natron-sdk-centos6-dts8
```
and for the [32-bit SDK]:
```
docker pull natrongithub/natron-sdk-i386-centos6
```

## Launching a build

To launch a build:

- create a directory for the builds (`$(pwd)/builds` in the following), and launch a build that binds this directory to `/home/builds_archive`:
```
mkdir builds
docker run -it --rm --mount src="$(pwd)/builds",target=/home/builds_archive,type=bind natrongithub/natron-sdk-centos6-dts8:latest
```
(for the 32-bit version, use `natrongithub/natron-sdk-i386-centos6:latest` instead of `natrongithub/natron-sdk-centos6-dts8:latest`)

By default, this launches a snapshot build, but you may want to customize build variables, which appear when the build starts and are described at the top of `tools/jenkins/launchBuildMain.sh`. This can be done by adding the following options to the `docker run`command: `--env VAR1=value1 --env VAR2=value2 ...`.

For example, to limit the number of parallel build jobs to 2:
```
docker run -it --rm --mount src="$(pwd)/builds",target=/home/builds_archive,type=bind --env MKJOBS=2 natrongithub/natron-sdk-centos6-dts8:latest
```


### Launching a build from a specific fork, branch or commit

`GIT_URL`, `GIT_BRANCH`, and `GIT_COMMIT` can be set to launch a build from a specific git repository, branch, and/or commit.

If `GIT_URL` is not an official Natron repository (as listed in [gitRepositories.sh](https://github.com/NatronGitHub/Natron/blob/master/tools/jenkins/gitRepositories.sh)), `GIT_URL_IS_NATRON=1` can be used to force a Natron build, as in the following example, which launches a build from branch `SetDefaultProjectFormat` of repository `https://github.com/rodlie/Natron.git`.

```
docker run -it --rm --mount src="$(pwd)/builds",target=/home/builds_archive,type=bind --env GIT_URL_IS_NATRON=1 --env GIT_URL=https://github.com/rodlie/Natron.git --env GIT_BRANCH=SetDefaultProjectFormat natrongithub/natron-sdk-centos6-dts8:latest
```


### Launching a release build

To launch a release, the Natron and plugins repositories must have the appropriate tags. for example, for release 2.3.15, the Natron repository must have tag `v2.3.15`, and the plugin repositories (openfx-misc, openfx-io, openfx-arena, openfx-gmic) must all have the tag `Natron-2.3.15`.

When launching the build, `RELEASE_TAG` must be set to the version number (eg "2.3.15") and `NATRON_BUILD_NUMBER` must be set to an integer value (typically 1 for the first build, and increment for each new build after fixing issues). See [launchBuildMain.sh](https://github.com/NatronGitHub/Natron/blob/master/tools/jenkins/launchBuildMain.sh#L340) for more details.

```
docker run -it --rm --env RELEASE_TAG=2.3.15 --env NATRON_BUILD_NUMBER=1 --env NATRON_DEV_STATUS=STABLE --mount src="$(pwd)/builds",target=/home/builds_archive,type=bind natrongithub/natron-sdk-centos6-dts8:latest
```


### More build options

Other build options are documented at the beginning of the [launchBuildMain.sh](https://github.com/NatronGitHub/Natron/blob/master/tools/jenkins/launchBuildMain.sh) script.


## Debugging a build

If something goes bad during a build, you may want to launch it from an interactive shell instead.
```
docker run -it --rm --mount src="$(pwd)/artifacts",target=/home/jenkins_artifacts,type=bind --mount src="$(pwd)/builds",target=/home/builds_archive,type=bind  natrongithub/natron-sdk-centos6-dts8:latest bash
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
