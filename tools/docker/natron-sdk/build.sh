#!/bin/sh
cd `dirname "$0"`
cp  ../../jenkins/*.sh .
(cd ../../jenkins/; tar cf - include) | tar xf -
export CENTOS=7
export DTS=10
LABEL="natrongithub/natron-sdk${UBUNTU+-ubuntu}${UBUNTU:-}${CENTOS+-centos}${CENTOS:-}${DTS+-dts}${DTS:-}"
env GEN_DOCKERFILE=1 ../../jenkins/include/scripts/build-Linux-sdk.sh > Dockerfile
docker build -t "${LABEL}:latest" . --progress=plain
#docker build --no-cache -t "${LABEL}:latest" .
echo "please execute:"
#echo "docker-squash ${LABEL}:latest"
echo "docker login"
echo "docker tag ${LABEL}:latest ${LABEL}:$(date -u +%Y%m%d)"
echo "docker push ${LABEL}:latest"
echo "docker push ${LABEL}:$(date -u +%Y%m%d)"
