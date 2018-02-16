#!/usr/bin/env bash
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2016 INRIA and Alexandre Gauthier
#
# Natron is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Natron is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****

# Exit immediately if a command exits with a non-zero status
set -e
# Print commands and their arguments as they are executed.
set -x

BREAKPAD=disable-breakpad
#BREAKPAD= # doesn't compile on linux yet
SILENT=silent

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

# get a minimal OCIO config (--trust-server-cert is equivalent to --trust-server-cert-failures=unknown-ca)
#(cd Tests; svn export --non-interactive --trust-server-cert https://github.com/imageworks/OpenColorIO-Configs/trunk/nuke-default)
ln -s $HOME/OpenColorIO-Configs .
export OCIO=$HOME/OpenColorIO-Configs/blender/config.ocio

if [[ ${TRAVIS_OS_NAME} == "linux" ]]; then
    export PKG_CONFIG_PATH=$HOME/ocio/lib/pkgconfig:$HOME/oiio/lib/pkgconfig:$HOME/openexr/lib/pkgconfig:$HOME/seexpr/lib/pkgconfig
    export LD_LIBRARY_PATH=$HOME/ocio/lib:$HOME/oiio/lib:$HOME/openexr/lib:$HOME/seexpr/lib
    if [ "${COVERITY_SCAN_BRANCH}" == 1 ]; then
        qmake -r CONFIG+="$BREAKPAD $SILENT precompile_header";
    elif [ "$CC" = "gcc" ]; then
        qmake -r CONFIG+="nopch coverage c++11 debug $BREAKPAD $SILENT"; # pch config disables precompiled headers
    else
        qmake -r -spec unsupported/linux-clang CONFIG+="nopch c++11 debug $BREAKPAD $SILENT";
    fi
    export MAKEFLAGS="$J" # qmake doesn't seem to pass MAKEFLAGS for recursive builds
    make $J -C libs/gflags
    make $J -C libs/glog
    make $J -C libs/ceres
    make $J -C libs/libmv
    make $J -C libs/openMVG
    make $J -C libs/qhttpserver
    make $J -C libs/hoedown
    make $J -C libs/libtess
    make $J -C HostSupport;
    # don't build parallel on the coverity_scan branch, because we reach the 3GB memory limit
    if [[ ${COVERITY_SCAN_BRANCH} == 1 ]]; then
        # compiling Natron overrides the 3GB limit on travis if building parallel
        J='-j2'
    fi
    export MAKEFLAGS="$J" # qmake doesn't seem to pass MAKEFLAGS for recursive builds
    make $J -C Engine;
    make $J -C Renderer;
    make $J -C Gui;
    make $J -C App; # linking Natron may break the 3Gb limit
    if [ "${COVERITY_SCAN_BRANCH}" != 1 ]; then
        # don't build the tests on coverity, so that we do not exceed the time limit
        make $J -C Tests;
        make $J
        rm -rf "$HOME/.cache/INRIA/Natron"* &> /dev/null || true
        if [ "$CC" = "gcc" ]; then cd Tests; env OFX_PLUGIN_PATH=Plugins OCIO=./nuke-default/config.ocio ./Tests; cd ..; fi
    fi
    
elif [[ ${TRAVIS_OS_NAME} == "osx" ]]; then
    # on OSX, the tests are done on the clang configuration
    # cairo requires xcb-shm, which has its pkg-config file in /opt/X11
    export PKG_CONFIG_PATH=/opt/X11/lib/pkgconfig
    if [ "$CC" = "gcc" ]; then
        qmake -r -spec unsupported/macx-clang-libc++ QMAKE_CC=gcc QMAKE_CXX=g++ CONFIG+="c++11 debug $BREAKPAD $SILENT";
    else
        qmake -r -spec unsupported/macx-clang-libc++ CONFIG+="c++11 debug $BREAKPAD $SILENT";
    fi
    export MAKEFLAGS="$J" # qmake doesn't seem to pass MAKEFLAGS for recursive builds
    make $J -C libs/gflags
    make $J -C libs/glog
    make $J -C libs/ceres
    make $J -C libs/libmv
    make $J -C libs/openMVG
    make $J -C libs/qhttpserver
    make $J -C libs/hoedown
    make $J -C libs/libtess
    make $J -C HostSupport;
    make $J -C Engine;
    make $J -C Renderer;
    make $J -C Gui;
    make -C App; # linking Natron may break the 3Gb limit
    make $J -C Tests;
    make $J
    rm -rf  "$HOME/Library/Caches/INRIA/Natron"* &> /dev/null || true
    if [ "$CC" = "clang" ]; then cd Tests; env OFX_PLUGIN_PATH=Plugins OCIO=./nuke-default/config.ocio ./Tests; cd ..; fi
fi

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
