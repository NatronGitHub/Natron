#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -e
# Print commands and their arguments as they are executed.
set -x

# enable testing locally or on forks without multi-os enabled
if [[ "${TRAVIS_OS_NAME:-false}" == false ]]; then
    if [[ $(uname -s) == "Darwin" ]]; then
        TRAVIS_OS_NAME="osx"
        VERBOSE=0
    elif [[ $(uname -s) == "Linux" ]]; then
        TRAVIS_OS_NAME="linux"
        VERBOSE=1
    fi
fi

if [[ ${COVERITY_BUILD_DISABLED} == 1 ]];
then
    echo "Coverity is not executed on this build variant."
    exit 0;
fi

# Ask cmake to search in all homebrew packages
CMAKE_PREFIX_PATH=$(echo /usr/local/Cellar/*/* | sed 's/ /;/g')

git submodule update --init --recursive

if [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
    if [ "$CC" = "gcc" ]; then qmake -r CONFIG+="coverage debug"; else qmake -r -spec unsupported/linux-clang CONFIG+="debug"; fi
    # don't build parallel on the coverity_scan branch, because we reach the 3GB memory limit
    if [[ ${COVERITY_SCAN_BRANCH} == 1 ]]; then
	# compiling Natron overrides the 3GB limit on travis if building parallel
        make -k; # -k: continue building even if compiler is killed because of the 3GB limit
    else
        export MAKEFLAGS="$J" # qmake doesn't seem to pass MAKEFLAGS for recursive builds
	make $J;
    fi
    if [ "$CC" = "gcc" ]; then cd Tests; env OFX_PLUGIN_PATH=Plugins ./Tests; cd ..; fi
    
elif [[ ${TRAVIS_OS_NAME} == "osx" ]]; then
    # on OSX, the tests are done on the clang configuration
    # cairo requires xcb-shm, which has its pkg-config file in /opt/X11
    export PKG_CONFIG_PATH=/opt/X11/lib/pkgconfig
    if [ "$CC" = "gcc" ]; then qmake -r -spec unsupported/macx-clang-libc++ QMAKE_CC=gcc QMAKE_CXX=g++ CONFIG+="debug"; else qmake -spec unsupported/macx-clang-libc++ CONFIG+="debug"; fi
    export MAKEFLAGS="$J" # qmake doesn't seem to pass MAKEFLAGS for recursive builds
    make $J
    if [ "$CC" = "clang" ]; then cd Tests; env OFX_PLUGIN_PATH=Plugins ./Tests; cd ..; fi
fi
