#!/bin/sh
cd `dirname "$0"`
env GEN_DOCKERFILE=1 ../../jenkins/include/scripts/build-Linux-sdk.sh > Dockerfile
cp  ../../jenkins/include/scripts/build-Linux-sdk.sh ../../jenkins/common.sh ../../jenkins/compiler-common.sh .
(cd ../../jenkins; tar cf - include/patches) | tar xf -
docker build -t natronsdk:latest .
