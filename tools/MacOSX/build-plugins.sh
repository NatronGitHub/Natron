#!/bin/sh
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2015 INRIA and Alexandre Gauthier
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

#Usage PLUGINDIR="..." MKJOBS=4 CONFIG=relwithdebinfo BRANCH=workshop ./build-plugins.sh

source `pwd`/common.sh || exit 1

cd $CWD/build || exit 1

#If "workshop" is passed, use master branch for all plug-ins otherwise use the git tags in common.sh
if [ "$BRANCH" = "workshop" ]; then
    IO_BRANCH=master
    MISC_BRANCH=master
    ARENA_BRANCH=master
    CV_BRANCH=master
else
    IO_BRANCH=$IOPLUG_GIT_TAG
    MISC_BRANCH=$MISCPLUG_GIT_TAG
    ARENA_BRANCH=$ARENAPLUG_GIT_TAG
    CV_BRANCH=$CVPLUG_GIT_TAG
fi

if [ ! -d "$PLUGINDIR" ]; then
    echo "Error: plugin directory '$PLUGINDIR' does not exist"
    exit 1
fi

#Build openfx-io
git clone $GIT_IO
cd openfx-io || exit 1
git checkout "$IO_BRANCH" || exit 1
git submodule update -i --recursive || exit 1
if [ "$IO_BRANCH" = "master" ]; then
    # the snapshots are always built with the latest version of submodules
    git submodule foreach git pull origin master
fi

#Always bump git commit, it is only used to version-stamp binaries
IO_GIT_VERSION=`git log|head -1|awk '{print $2}'`
sed -i "" -e "s/IOPLUG_DEVEL_GIT=.*/IOPLUG_DEVEL_GIT=${IO_GIT_VERSION}/" $CWD/commits-hash.sh || exit 1

make CXX="$CXX" BITS=$BITS CONFIG=$CONFIG OCIO_HOME=/opt/local OIIO_HOME=/opt/local SEEXPR_HOME=/opt/local -j${MKJOBS} || exit 1
cp -r IO/$OS-$BITS-$CONFIG/IO.ofx.bundle "$PLUGINDIR/" || exit 1
cd ..

#Build openfx-misc
git clone $GIT_MISC
cd openfx-misc || exit 1
git checkout "$MISC_BRANCH" || exit 1
git submodule update -i --recursive || exit 1
if [ "$MISC_BRANCH" = "master" ]; then
    # the snapshots are always built with the latest version of submodules
    git submodule foreach git pull origin master
fi

#Always bump git commit, it is only used to version-stamp binaries
MISC_GIT_VERSION=`git log|head -1|awk '{print $2}'`
sed -i "" -e "s/MISCPLUG_DEVEL_GIT=.*/MISCPLUG_DEVEL_GIT=${MISC_GIT_VERSION}/" $CWD/commits-hash.sh || exit 1

make -C CImg CImg.h || exit 1
if [ "$COMPILER" = "gcc" ]; then
    # build CImg with OpenMP support
    make -C CImg CXX="$CXX" BITS=$BITS CONFIG=$CONFIG -j${MKJOBS} CXXFLAGS_ADD=-fopenmp LDFLAGS_ADD=-fopenmp
fi
make CXX="$CXX" BITS=$BITS CONFIG=$CONFIG -j${MKJOBS} || exit 1
cp -r Misc/$OS-$BITS-$CONFIG/Misc.ofx.bundle "$PLUGINDIR/" || exit 1
cp -r CImg/$OS-$BITS-$CONFIG/CImg.ofx.bundle "$PLUGINDIR/" || exit 1
cd ..

#Build openfx-arena
git clone $GIT_ARENA
cd openfx-arena || exit 1
git checkout "$ARENA_BRANCH" || exit 1
git submodule update -i --recursive || exit 1
if [ "$ARENA_BRANCH" = "master" ]; then
    # the snapshots are always built with the latest version of submodules
    git submodule foreach git pull origin master
fi

#Always bump git commit, it is only used to version-stamp binaries
ARENA_GIT_VERSION=`git log|head -1|awk '{print $2}'`
sed -i "" -e "s/ARENAPLUG_DEVEL_GIT=.*/ARENAPLUG_DEVEL_GIT=${ARENA_GIT_VERSION}/" $CWD/commits-hash.sh || exit 1

make CXX="$CXX" USE_PANGO=1 USE_SVG=1 BITS=$BITS CONFIG=$CONFIG -j${MKJOBS} || exit 1
cp -r Bundle/$OS-$BITS-$CONFIG/Arena.ofx.bundle "$PLUGINDIR/" || exit 1
cd ..

# move all libraries to the same place, put symbolic links instead
for plugin in "$PLUGINDIR"/*.ofx.bundle; do
    cd "$plugin/Contents/Libraries"
    for lib in lib*.dylib; do
	if [ -f "../../../../Frameworks/$lib" ]; then
	    rm "$lib"
	else
	    mv "$lib" "../../../../Frameworks/$lib"
	fi
	ln -sf "../../../../Frameworks/$lib" "$lib"
    done
    if [ "$COMPILER" = "gcc" ]; then # use gcc's libraries everywhere
	for l in gcc_s.1 gomp.1 stdc++.6; do
	    lib=lib${l}.dylib
	    for deplib in "$plugin"/Contents/MacOS/*.ofx "$plugin"/Contents/Libraries/lib*dylib ; do
		install_name_tool -change /usr/lib/$lib @executable_path/../Frameworks/$lib $deplib
	    done
	done
    fi
done

#Build openfx-opencv
#git clone $GIT_OPENCV
#cd openfx-opencv || exit 1
#git checkout "$CV_BRANCH" || exit 1
#git submodule update -i --recursive || exit 1
#if [ "$CV_BRANCH" = "master" ]; then
#    # the snapshots are always built with the latest version of submodules
#    git submodule foreach git pull origin master
#fi

#Always bump git commit, it is only used to version-stamp binaries
#CV_GIT_VERSION=`git log|head -1|awk '{print $2}'`
#sed -i -e "s/CVPLUG_DEVEL_GIT=.*/CVPLUG_DEVEL_GIT=${CV_GIT_VERSION}/" $CWD/commits-hash.sh || exit 1

#cd opencv2fx || exit 1
#make CXX="$CXX" BITS=$BITS CONFIG=$CONFIG -j${MKJOBS} || exit 1
#cp -r */$OS-$BITS-*/*.ofx.bundle "$PLUGINDIR" || exit 1
#cd ..


