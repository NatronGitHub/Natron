#!/bin/bash

CWD=`pwd`

#THE FOLLOWING CAN BE MODIFIED TO CONFIGURE RELEASE BUILDS
#----------------------------------------------------------
NATRON_GIT_TAG=tags/2.0.0
IOPLUG_GIT_TAG=tags/2.0.0
MISCPLUG_GIT_TAG=tags/2.0.0
ARENAPLUG_GIT_TAG=tags/2.0.0
CVPLUG_GIT_TAG=tags/2.0.0
#----------------------------------------------------------

GIT_NATRON=https://github.com/MrKepzie/Natron.git
GIT_IO=https://github.com/MrKepzie/openfx-io.git
GIT_MISC=https://github.com/devernay/openfx-misc.git
GIT_OPENCV=https://github.com/devernay/openfx-opencv.git
GIT_ARENA=https://github.com/olear/openfx-arena.git

TMP=$CWD/tmp

PKGOS=OSX
BIT=Universal


if [ !-f "$CWD/local.sh" ]; then
    echo "Please create local.sh, see README."
    exit 1
fi

source $CWD/local.sh || exit 1


if [ -z "$MKJOBS" ]; then
    MKJOBS=1
fi

# Keep existing tag, else make a new one
if [ -z "$TAG" ]; then
    TAG=$(date +%Y%m%d%H%M)
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