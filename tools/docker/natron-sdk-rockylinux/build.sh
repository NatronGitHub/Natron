#!/bin/sh
# Build the Rocky Linux Natron SDK image for one architecture.
#
# Usage: [ROCKY=9|8] ./build.sh [amd64|arm64]
#
# ROCKY is the Rocky Linux major version (default: 9, the Natron 2.6 baseline).
# The architecture defaults to the one of the docker host. Building for another
# architecture works through emulation (qemu/binfmt), but is very slow.
# DOCKER can be set to another docker-compatible command, e.g. DOCKER=podman.
# Extra "docker build" arguments can be given in DOCKER_BUILD_ARGS, e.g.
# DOCKER_BUILD_ARGS=--no-cache.
# Nothing is pushed: the commands to publish the image are printed at the end.
set -e
cd "$(dirname "$0")"
DOCKER="${DOCKER:-docker}"

PLATFORM_ARCH="${1:-}"
if [ -z "$PLATFORM_ARCH" ]; then
    # docker, then podman, then the local machine
    PLATFORM_ARCH="$($DOCKER info --format '{{.Architecture}}' 2>/dev/null || true)"
    case "$PLATFORM_ARCH" in
        ''|'<no value>') PLATFORM_ARCH="$($DOCKER info --format '{{.Host.Arch}}' 2>/dev/null || true)" ;;
    esac
    case "$PLATFORM_ARCH" in
        ''|'<no value>') PLATFORM_ARCH="$(uname -m)" ;;
    esac
fi
case "$PLATFORM_ARCH" in
    amd64|x86_64) PLATFORM_ARCH=amd64 ;;
    arm64|aarch64) PLATFORM_ARCH=arm64 ;;
    *) echo "Error: unsupported architecture: $PLATFORM_ARCH (use amd64 or arm64)" >&2; exit 1 ;;
esac

cp  ../../jenkins/*.sh .
(cd ../../jenkins/; tar cf - include) | tar xf -
export ROCKY="${ROCKY:-9}"
LABEL="natrongithub/natron-sdk${UBUNTU+-ubuntu}${UBUNTU:-}${CENTOS+-centos}${CENTOS:-}${ROCKY+-rockylinux}${ROCKY:-}${DTS+-dts}${DTS:-}"
# The Dockerfile is the same for all architectures: the base image is multi-arch,
# and the build scripts take the architecture from uname -m.
env GEN_DOCKERFILE=1 ../../jenkins/include/scripts/build-Linux-sdk.sh > Dockerfile
# shellcheck disable=SC2086
$DOCKER build --platform "linux/${PLATFORM_ARCH}" ${DOCKER_BUILD_ARGS:-} -t "${LABEL}:latest-${PLATFORM_ARCH}" .
DATE="$(date -u +%Y%m%d)"
echo "Built ${LABEL}:latest-${PLATFORM_ARCH}"
echo
echo "To publish it, execute:"
echo "$DOCKER login docker.io"
echo "$DOCKER tag ${LABEL}:latest-${PLATFORM_ARCH} ${LABEL}:${DATE}-${PLATFORM_ARCH}"
echo "$DOCKER push ${LABEL}:latest-${PLATFORM_ARCH}"
echo "$DOCKER push ${LABEL}:${DATE}-${PLATFORM_ARCH}"
echo
echo "Once both the amd64 and the arm64 images are pushed, create and push the multi-arch tags:"
case "$DOCKER" in
    *podman*)
        for tag in latest "$DATE"; do
            echo "podman manifest create ${LABEL}:${tag} docker://docker.io/${LABEL}:${tag}-amd64 docker://docker.io/${LABEL}:${tag}-arm64"
            echo "podman manifest push --all ${LABEL}:${tag} docker://docker.io/${LABEL}:${tag}"
        done
        ;;
    *)
        for tag in latest "$DATE"; do
            echo "docker manifest create ${LABEL}:${tag} ${LABEL}:${tag}-amd64 ${LABEL}:${tag}-arm64"
            echo "docker manifest push ${LABEL}:${tag}"
        done
        ;;
esac
