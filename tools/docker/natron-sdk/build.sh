#!/bin/sh
cd `dirname "$0"`
env GEN_DOCKERFILE=1 ../../jenkins/include/scripts/build-Linux-sdk.sh > Dockerfile
cp  ../../jenkins/common.sh ../../jenkins/compiler-common.sh .
(cd ../../jenkins/include/scripts; tar cf - build-Linux-sdk.sh pkg) | tar xf -
(cd ../../jenkins; tar cf - include/patches) | tar xf -
docker build -t natrongithub/natronsdk:latest .
#docker build --no-cache -t natronsdk:latest .
