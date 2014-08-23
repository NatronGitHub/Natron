#!/usr/bin/env bash

set -e

# enable testing locally or on forks without multi-os enabled
if [[ "${TRAVIS_OS_NAME:-false}" == false ]]; then
    if [[ $(uname -s) == "Darwin" ]]; then
        TRAVIS_OS_NAME="osx"
    elif [[ $(uname -s) == "Linux" ]]; then
        TRAVIS_OS_NAME="linux"
    fi
fi

if [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
    if [ "$CC" = "gcc" ]; then qmake -r CONFIG+="coverage debug"; else qmake -spec unsupported/linux-clang; fi
    make $MAKEFLAGSPARALLEL
    if [ "$CC" = "gcc" ]; then cd Tests; env OFX_PLUGIN_PATH=Plugins ./Tests; cd ..; fi
elif [[ ${TRAVIS_OS_NAME} == "osx" ]]; then
    qmake -t CONFIG+="coverage debug"
    make $MAKEFLAGSPARALLEL
    if [ "$CC" = "gcc" ]; then cd Tests; env OFX_PLUGIN_PATH=Plugins ./Tests; cd ..; fi
fi
