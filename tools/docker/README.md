# Natron Linux SDK

This directory contains tools to build Natron using docker images (requires [Docker]).

The Natron 2.6 Linux SDK image is based on Rocky Linux 9 (EL9, glibc 2.34, as in the VFX Reference Platform CY2027), so that the binaries built with it run on all the Linux distributions that are at least as recent. The scripts of the older images (CentOS 6, 7 and 8, Ubuntu 18.04 and 20.04, i386) were removed: they are in the RB-2.5 branch. The base image is chosen by `DOCKER_BASE` in `tools/jenkins/include/scripts/build-Linux-sdk.sh`.

## Debugging a docker build that fails

See [this github issue comment](https://github.com/moby/buildkit/issues/1472#issuecomment-941628522): just add `FROM intermediate` before the RUN line that fails, and relaunch the build with `docker builtx build --load --target itermediate -t arch .`. Now `docker run --rm -it arch sh` gives you a shell before the fail.

## Building the Rocky Linux SDK image (amd64 and arm64)

`natron-sdk-rockylinux/build.sh` builds `natrongithub/natron-sdk-rockylinux9` (or `natrongithub/natron-sdk-rockylinux8` with `ROCKY=8`) for one architecture, natively. Rocky Linux 9 is the baseline of the Natron 2.6 Linux binaries.
```
tools/docker/natron-sdk-rockylinux/build.sh arm64   # or amd64; default: the docker host's architecture
DOCKER=podman tools/docker/natron-sdk-rockylinux/build.sh arm64   # with podman
ROCKY=8 tools/docker/natron-sdk-rockylinux/build.sh   # the older Rocky Linux 8 SDK
```
The image is tagged `:latest-amd64` or `:latest-arm64`. Nothing is pushed: the script prints the commands that push the image and create the multi-arch `:latest` manifest once both architectures are pushed. Building for the other architecture works through emulation, but takes much longer. The build needs about 30 GB of disk space for the image and its layers.

The source tarballs are extracted with `tar --no-same-owner`, since some of them have owners that rootless docker/podman can't map. The arm64 image was built with a rootful podman connection (`podman --connection <machine>-root`).

## The SDK image and the packages on GitHub Actions

The `Linux packages` workflow (`.github/workflows/linux_packages.yml`) runs weekly (Sunday), on demand (`workflow_dispatch`) and on pre-release tags (`v2.6.0-rc1`, ...), for x86_64 (`ubuntu-24.04` runners) and aarch64 (`ubuntu-24.04-arm`):

1. **SDK image**, only when its definition changed, i.e. the hash of `tools/jenkins/include/scripts`, `tools/jenkins/include/patches`, `tools/jenkins/common.sh`, `tools/jenkins/compiler-common.sh` and this directory, or on demand (the `rebuild_sdk` input: no layer cache, the newest `rockylinux/rockylinux:9` base image and system packages). The whole SDK takes longer than the 6 hours a GitHub-hosted job may run, so `split-stages.sh` splits the Dockerfile made by `build.sh` (`PREPARE_ONLY=1`) into stages, each built by one job (`linux_sdk_stage.yml`) and pushed as `ghcr.io/natrongithub/natron-sdk-rockylinux9:<hash>-<arch>-stage<N>`, with the layers of the previous builds as cache (a change to one `pkg/*.sh` script only rebuilds the steps from that one on). The SDK image is `:<hash>-amd64` and `:<hash>-arm64`, and the multi-arch `:<hash>` and, on the default branch, `:latest`. Rebuild it on demand before each pre-release, to get the security updates of the base image.
2. **Package:** `launchBuildMain.sh` in the SDK image (CMake builds of Natron and the plug-ins, the bundled unit tests, the portable archive and its system-library checks, `checkBundle` in `build-Linux-installer.sh`). The archives are kept 14 days as workflow artifacts, and attached to the GitHub pre-release on pre-release tags.
3. **Tests:** `natron-test.sh` runs Natron-Tests (master, with its `known_failures/` lists) on the archive in a clean `rockylinux/rockylinux:9` container, with only the system libraries that the archive doesn't bundle (see INSTALL_LINUX.md). The results are named as in the releases: `<name>-tests.txt`, `<name>-tests-summary.txt`, `<name>-tests.log` and `<name>-unit_tests_failures.zip`. To run it locally: `tools/docker/natron-test.sh [-n Natron-Tests] Natron-...-Linux-x86_64-no-installer.tar.xz` (`DOCKER=podman` with podman).

## Downloading the pre-built Natron SDK images

The `natrongithub/natron-sdk` image on [Docker Hub] is the CentOS 7 SDK of Natron 2.5 and older (an alias to `natrongithub/natron-sdk-centos7` and `natrongithub/natron-sdk-centos7-dts8`). Natron 2.6 is built with the Rocky Linux 9 image, `ghcr.io/natrongithub/natron-sdk-rockylinux9:latest` (published by the workflow above, for amd64 and arm64); the examples below use `natrongithub/natron-sdk:latest`: replace it with that image, or with the one you built.

## Launching a build

To launch a build:

- create a directory for the builds (`$(pwd)/builds` in the following), and launch a build that binds this directory to `/home/builds_archive`:
```
mkdir builds
docker run -it --rm --mount src="$(pwd)/builds",target=/home/builds_archive,type=bind natrongithub/natron-sdk:latest
```

By default, this launches a snapshot build, but you may want to customize build variables, which appear when the build starts and are described at the top of `tools/jenkins/launchBuildMain.sh`. This can be done by adding the following options to the `docker run`command: `--env VAR1=value1 --env VAR2=value2 ...`.

For example, to limit the number of parallel build jobs to 2:
```
docker run -it --rm --mount src="$(pwd)/builds",target=/home/builds_archive,type=bind --env MKJOBS=2 natrongithub/natron-sdk:latest
```


### Launching a build from a specific fork, branch or commit

`GIT_URL`, `GIT_BRANCH`, and `GIT_COMMIT` can be set to launch a build from a specific git repository, branch, and/or commit.

For example, to launch a snapshot build on branch `RB-2.4`:
```
docker run -it --rm --mount src="$(pwd)/builds",target=/home/builds_archive,type=bind --env GIT_BRANCH=RB-2.4 natrongithub/natron-sdk:latest
```

If `GIT_URL` is not an official Natron repository (as listed in [gitRepositories.sh](https://github.com/NatronGitHub/Natron/blob/master/tools/jenkins/gitRepositories.sh)), `GIT_URL_IS_NATRON=1` can be used to force a Natron build, as in the following example, which launches a build from branch `my-feature-branch` of repository `https://github.com/mygithubaccount/Natron.git`.

```
docker run -it --rm --mount src="$(pwd)/builds",target=/home/builds_archive,type=bind --env GIT_URL_IS_NATRON=1 --env GIT_URL=https://github.com/mygithubaccount/Natron.git --env GIT_BRANCH=my-feature-branch natrongithub/natron-sdk:latest
```


### Launching a release build

To launch a release, the Natron and plugins repositories must have the appropriate tags. for example, for release 2.4.1, the Natron repository must have tag `v2.4.1`, and the plugin repositories (openfx-misc, openfx-io, openfx-arena, openfx-gmic) must all have the tag `Natron-2.4.1`.

When launching the build, `RELEASE_TAG` must be set to the version number (eg "2.4.1") and `NATRON_BUILD_NUMBER` must be set to an integer value (typically 1 for the first build, and increment for each new build after fixing issues). See [launchBuildMain.sh](https://github.com/NatronGitHub/Natron/blob/master/tools/jenkins/launchBuildMain.sh#L340) for more details.

```
docker run -it --rm --env RELEASE_TAG=2.4.4 --env NATRON_BUILD_NUMBER=1 --env NATRON_DEV_STATUS=STABLE --mount src="$(pwd)/builds",target=/home/builds_archive,type=bind natrongithub/natron-sdk:latest
```


### More build options

Other build options are documented at the beginning of the [launchBuildMain.sh](https://github.com/NatronGitHub/Natron/blob/master/tools/jenkins/launchBuildMain.sh) script.


## Debugging a build

If something goes bad during a build, you may want to launch it from an interactive shell instead.
```
docker run -it --rm --mount src="$(pwd)/artifacts",target=/home/jenkins_artifacts,type=bind --mount src="$(pwd)/builds",target=/home/builds_archive,type=bind  natrongithub/natron-sdk:latest bash
```
Then you can execute manually `launchBuildMain.sh` from the shell.


## Building the SDK images

If you change anything in the Natron SDK, including package versions (as found in `tools/jenkins/include/scripts/pkg`) or the build script itself (`tools/jenkins/include/scripts/build-Linux-sdk.sh`), you may want to re-build the SDK image with `tools/docker/natron-sdk-rockylinux/build.sh` (see "Building the Rocky Linux SDK image" above).



[Docker]: https://docs.docker.com/
[Docker Hub]: https://hub.docker.com
