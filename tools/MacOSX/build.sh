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

# Options:
# CONFIG=(debug,release,relwithdebinfo): the build type
# WORKSHOP=(workhop,tag): If workshop, builds dev branch, otherwise uses tags from common.sh
# OFXBRANCH=(master,git tag e.g: tags/Natron-1.0.0-RC1): the branch or tag to build OpenFX plug-ins from
# MKJOBS=N: number of threads
# NO_CLEAN=1: Do not remove build dir before building, useful for debugging and avoiding the cloning etc...

#Usage: CONFIG=release BRANCH=workshop MKJOBS=4 UPLOAD=1 ./build.sh

source `pwd`/common.sh || exit 1
(cd ports; portindex)
# required macports ports (first ones are for Natron, then for OFX plugins)
PORTS="boost qt4-mac glew cairo expat jpeg openexr ffmpeg openjpeg15 freetype lcms ImageMagick lcms2 libraw opencolorio openimageio flex bison openexr seexpr fontconfig py27-shiboken py27-pyside wget"
if [ "$COMPILER" = "gcc" ]; then
    PORTS="$PORTS gcc48"
else
    PORTS="$PORTS clang-3.4"
fi

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

if [ "$COMPILER" = "gcc" ]; then
    if [ ! -x /opt/local/bin/gcc-mp -o ! -x /opt/local/bin/g++-mp ]; then
	echo "The gcc-mp and g++-mp drivers are missing"
	echo "Please execute the following:"
	echo "git clone https://github.com/devernay/macportsGCCfixup.git"
	echo "cd macportsGCCfixup"
	echo "./configure"
	echo "make"
	echo "sudo make install"
	exit 1
    fi
fi

#if port installed ffmpeg |fgrep '(active)' |fgrep '+gpl' > /dev/null; then
#  echo "ffmpeg port should not be installed with the +gpl2 or +gpl3 variants, please reinstall it using 'sudo port install ffmpeg -gpl2 -gpl3'"
#  exit 1
#fi
GLEW_CFLAGS=`pkg-config --cflags glew`
if [ ! -z "$GLEW_CFLAGS" ]; then
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

if [ ! -f "$CWD/commits-hash.sh" ]; then
    cat <<EOF > "$CWD/commits-hash.sh"
#!/bin/sh
NATRON_DEVEL_GIT=#
IOPLUG_DEVEL_GIT=#
MISCPLUG_DEVEL_GIT=#
ARENAPLUG_DEVEL_GIT=#
CVPLUG_DEVEL_GIT=#
NATRON_VERSION_NUMBER=#
EOF
fi

source "$CWD/commits-hash.sh" || exit 1

if [ "$NO_CLEAN" != "1" ]; then
    rm -rf "$CWD/build"
fi
mkdir -p "$CWD/build"

LOGDIR="$CWD/logs"
rm -rf $LOGDIR
mkdir -p $LOGDIR || exit 1

#If OpenColorIO-Configs do not exist, download them
if [ ! -d "$TMP/OpenColorIO-Configs" ]; then
    wget $GIT_OCIO_CONFIG_TAR -O "$TMP/OpenColorIO-Configs.tar.gz" || exit 1
    (cd "$TMP"; tar xf OpenColorIO-Configs.tar.gz) || exit 1
    rm "$TMP/OpenColorIO-Configs.tar.gz" || exit 1
    mv "$TMP/OpenColorIO-Configs"* "$TMP/OpenColorIO-Configs" || exit 1
fi


PLUGINDIR="$CWD/build/Natron/App/Natron.app/Contents/Plugins"
NATRONLOG="$LOGDIR/Natron-$TAG.log"

echo "Building Natron..."
echo
env MKJOBS="$MKJOBS" CONFIG="$CONFIG" BRANCH="$BRANCH" PLUGINDIR="$PLUGINDIR" ./build-natron.sh >& "$NATRONLOG" || FAIL=1

if [ "$FAIL" != "1" ]; then
    echo OK
else
    echo ERROR
    sleep 2
    cat "$NATRONLOG"
fi

PLUGINSLOG="$LOGDIR/Natron-$TAG-plugins.log"
if [ "$FAIL" != "1" ]; then
    echo "Building plug-ins..."
    echo
    PLUGINDIR=$PLUGINDIR MKJOBS=$MKJOBS CONFIG=$CONFIG BRANCH=$BRANCH ./build-plugins.sh >& "$PLUGINSLOG" || FAIL=1
    if [ "$FAIL" != "1" ]; then
        echo OK
    else
        echo ERROR
        sleep 2
        cat "$PLUGINSLOG"
    fi
fi

if [ "$BRANCH" = "workshop" ]; then
    NATRON_V=$NATRON_DEVEL_GIT
    UPLOAD_BRANCH=snapshots
else
    NATRON_V=$NATRON_VERSION_NUMBER
    UPLOAD_BRANCH=releases
fi

INSTALLERLOG="$LOGDIR/Natron-$TAG-${NATRON_V}-installer.log"
NATRONDMG="$CWD/build/Natron-$TAG-${NATRON_V}.dmg"
if [ "$FAIL" != "1" ]; then
    echo "Building installer..."
    echo
    ./build-installer.sh >& "$INSTALLERLOG" || FAIL=1
    mv "$CWD/build/Natron.dmg" "$NATRONDMG" || FAIL=1
    if [ "$FAIL" != "1" ]; then
        echo OK
    else
        echo ERROR
        sleep 2
        cat "$INSTALLERLOG"
    fi
fi

NATRONLOGNEW="$LOGDIR/Natron-$TAG-${NATRON_V}.log"
PLUGINSLOGNEW="$LOGDIR/Natron-$TAG-${NATRON_V}-plugins.log"
mv "$NATRONLOG" "$NATRONLOGNEW"
mv "$PLUGINSLOG" "$PLUGINSLOGNEW"

if [ "$UPLOAD" = "1" ]; then
    if [ "$FAIL" != "1" ]; then
        echo "Uploading $NATRONDMG ..."
        echo
        rsync -avz --progress -e ssh "$NATRONDMG" "$REPO_DEST/Mac/$UPLOAD_BRANCH/" || exit 1
    fi
    echo "Uploading logs..."
    echo
    rsync -avz --progress -e ssh "$LOGDIR/" "$REPO_DEST/Mac/$UPLOAD_BRANCH/logs" || exit 1
fi


echo "Done."
