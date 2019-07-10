#!/bin/sh
cd `dirname "$0"`
env GEN_DOCKERFILE=1 ../../jenkins/include/scripts/build-Linux-sdk.sh > Dockerfile
cp  ../../jenkins/common.sh ../../jenkins/compiler-common.sh .
(cd ../../jenkins/include/scripts; tar cf - build-Linux-sdk.sh pkg) | tar xf -
(cd ../../jenkins; tar cf - include/patches) | tar xf -
docker build -t natrongithub/natron-sdk:latest .
#docker build --no-cache -t natron-sdk:latest .
echo "please execute:"
echo "docker login"
echo "docker tag natrongithub/natron-sdk:latest natrongithub/natron-sdk:$(date +%Y%m%d)"
echo "docker push natrongithub/natron-sdk:latest"
echo "docker push natrongithub/natron-sdk:$(date +%Y%m%d)"
