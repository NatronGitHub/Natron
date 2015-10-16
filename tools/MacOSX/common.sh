#!/bin/sh

CWD=`pwd`

#THE FOLLOWING CAN BE MODIFIED TO CONFIGURE RELEASE BUILDS
#----------------------------------------------------------
NATRON_GIT_TAG=tags/2.0.0-RC1
IOPLUG_GIT_TAG=tags/Natron-2.0.0-RC1
MISCPLUG_GIT_TAG=tags/Natron-2.0.0-RC1
ARENAPLUG_GIT_TAG=tags/Natron-2.0.0-RC1
CVPLUG_GIT_TAG=tags/2.0.0-RC1
#----------------------------------------------------------

GIT_NATRON=https://github.com/MrKepzie/Natron.git
GIT_IO=https://github.com/MrKepzie/openfx-io.git
GIT_MISC=https://github.com/devernay/openfx-misc.git
GIT_OPENCV=https://github.com/devernay/openfx-opencv.git
GIT_ARENA=https://github.com/olear/openfx-arena.git

GIT_OCIO_CONFIG_TAR=https://github.com/MrKepzie/OpenColorIO-Configs/archive/Natron-v2.0.tar.gz

TMP=$CWD/tmp

PKGOS=OSX
BIT=Universal


if [ ! -f "$CWD/local.sh" ]; then
    echo "Please create local.sh, see README."
    exit 1
fi

source $CWD/local.sh || exit 1


if [ -z "$MKJOBS" ]; then
    MKJOBS=1
fi

# Keep existing tag, else make a new one
if [ -z "$TAG" ]; then
    TAG=`date +%Y%m%d%H%M`
fi

OS=`uname -s`
BITS=64
if [ "$OS" = "Darwin" ]; then
    macosx=`uname -r | sed 's|\([0-9][0-9]*\)\.[0-9][0-9]*\.[0-9][0-9]*|\1|'`
    case "$macosx" in
        [1-8])
            BITS=32
            ;;
        9|10)
            BITS=Universal
            ;;
    esac;
fi

#COMPILER=clang
COMPILER=gcc

if [ "$COMPILER" != "gcc" -a "$COMPILER" != "clang" ]; then
    echo "Error: COMPILER must be gcc or clang"
    exit 1
fi
if [ "$COMPILER" = "clang" ]; then
    CC=clang-mp-3.4
    CXX=clang++-mp-3.4
else
    CC=gcc-mp
    CXX=g++-mp
fi
