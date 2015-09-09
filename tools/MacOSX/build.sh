#!/bin/bash
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

# Options:
# CONFIG=(debug,release,relwithdebinfo): the build type
# WORKSHOP=(workhop,tag): If workshop, builds dev branch, otherwise uses tags from common.sh
# OFXBRANCH=(master,git tag e.g: tags/Natron-1.0.0-RC1): the branch or tag to build OpenFX plug-ins from
# MKJOBS=N: number of threads
# NO_CLEAN=1: Do not remove build dir before building, useful for debugging and avoiding the cloning etc...

#Usage: CONFIG=release BRANCH=workshop MKJOBS=4 UPLOAD=1 ./build.sh

source $(pwd)/common.sh || exit 1

# required macports ports (first ones are for Natron, then for OFX plugins)
PORTS="boost qt4-mac glew cairo expat jpeg openexr ffmpeg openjpeg15 freetype lcms ImageMagick lcms2 libraw opencolorio openimageio flex bison openexr seexpr fontconfig py27-shiboken py27-pyside"

PORTSOK=yes
for p in $PORTS; do
 if port installed $p | fgrep "  $p @" > /dev/null; then
   #echo "port $p OK"
   true
 else
   echo "Error: port $p is missing"
   PORTSOK=no
 fi
done

if [ "$PORTSOK" = "no" ]; then
  echo "At least one port from macports is missing. Please install them."
  exit 1
fi

#if port installed ffmpeg |fgrep '(active)' |fgrep '+gpl' > /dev/null; then
#  echo "ffmpeg port should not be installed with the +gpl2 or +gpl3 variants, please reinstall it using 'sudo port install ffmpeg -gpl2 -gpl3'"
#  exit 1
#fi

if pkg-config --cflags glew; then
  true
else
  echo "Error: missing pkg-config file for package glew"
  cat <<EOFF
To fix this, execute the following:
sudo cat > /opt/local/lib/pkgconfig/glew.pc << EOF
prefix=/opt/local
exec_prefix=/opt/local/bin
libdir=/opt/local/lib
includedir=/opt/local/include/GL

Name: glew
Description: The OpenGL Extension Wrangler library
Version: 1.10.0
Cflags: -I\${includedir}
Libs: -L\${libdir} -lGLEW -framework OpenGL
Requires:
EOF
EOFF
  exit 1
fi

if [ ! -f $CWD/commits-hash.sh ]; then
    touch $CWD/commits-hash.sh
    echo "#!/bin/sh" >> $CWD/commits-hash.sh
    echo "NATRON_DEVEL_GIT=#" >> $CWD/commits-hash.sh
    echo "IOPLUG_DEVEL_GIT=#" >> $CWD/commits-hash.sh
    echo "MISCPLUG_DEVEL_GIT=#" >> $CWD/commits-hash.sh
    echo "ARENAPLUG_DEVEL_GIT=#" >> $CWD/commits-hash.sh
    echo "CVPLUG_DEVEL_GIT=#" >> $CWD/commits-hash.sh
    echo "NATRON_VERSION_NUMBER=#" >> $CWD/commits-hash.sh
fi

source $CWD/commits-hash.sh || exit 1

if [ "$NO_CLEAN" != "1" ]; then
    rm -rf $CWD/build
fi
mkdir -p $CWD/build
cd build || exit 1

LOGS=$CWD/logs
rm -rf $LOGS
mkdir -p $LOGS || exit 1


PLUGINDIR=$CWD/build/Natron/App/Natron.app/Contents/Plugins

    MKJOBS=$MKJOBS CONFIG=$CONFIG BRANCH=$BRANCH PLUGINDIR=$PLUGINDIR ./build-natron.sh >& $LOGS/natron.MacOSX-Universal.$TAG.log || FAIL=1

if [ "$FAIL" != "1" ]; then
    echo OK
else
    echo ERROR
    sleep 2
    cat $LOGS/natron.MacOSX-Universal.$TAG.log
fi

if [ "$FAIL" != "1" ]; then
    PLUGINDIR=$PLUGINDIR MKJOBS=$MKJOBS CONFIG=$CONFIG BRANCH=$BRANCH ./build-plugins.sh >& $LOGS/plugins.MacOSX-Universal.$TAG.log || FAIL=1
    if [ "$FAIL" != "1" ]; then
        echo OK
    else
        echo ERROR
        sleep 2
        cat $LOGS/plugins.MacOSX-Universal.$TAG.log
    fi
fi

if [ "$BRANCH" == "workshop" ]; then
    NATRON_V=$NATRON_DEVEL_GIT
else
    NATRON_V=$NATRON_VERSION_NUMBER
fi

if [ "$FAIL" != "1" ]; then
    ./build-installer.sh >& $LOGS/installer.MacOSX-Universal.$TAG.log || FAIL=1
    mv $CWD/build/Natron.dmg $CWD/build/Natron-${NATRON_V}.dmg || FAIL=1
    if [ "$FAIL" != "1" ]; then
        echo OK
    else
        echo ERROR
        sleep 2
        cat $LOGS/installer.MacOSX-Universal.$TAG.log
    fi
fi

if [ "$UPLOAD" != "1" ]; then
    if [ "$FAIL" != "1" ]; then
        rsync -avz --progress -e ssh $CWD/build/Natron-${NATRON_V}.dmg $REPO_DEST/Mac/snapshots/ || exit 1
    fi
    rsync -avz --progress -e ssh $LOGS $REPO_DEST/Mac/snapshots/ || exit 1
fi


echo "Done."
