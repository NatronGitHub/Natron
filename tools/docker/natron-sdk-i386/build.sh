#!/bin/sh
cd `dirname "$0"`
LABEL="natrongithub/natron-sdk-i386"
env GEN_DOCKERFILE32=1 ../../jenkins/include/scripts/build-Linux-sdk.sh> Dockerfile
cp  ../../jenkins/*.sh .
(cd ../../jenkins/; tar cf - include) | tar xf -
docker build -t "${LABEL}:latest" .
#docker build --no-cache -t "${LABEL}:latest" .
echo "please execute:"
#echo "docker-squash ${LABEL}:latest"
echo "docker login"
echo "docker tag ${LABEL}:latest ${LABEL}:$(date -u +%Y%m%d)"
echo "docker push ${LABEL}:latest"
echo "docker push ${LABEL}:$(date -u +%Y%m%d)"
