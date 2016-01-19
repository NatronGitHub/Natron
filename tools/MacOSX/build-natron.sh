#!/bin/sh
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

# BUILD_CONFIG=(SNAPSHOT,ALPHA,BETA,RC,STABLE,CUSTOM)
# CUSTOM_BUILD_USER_NAME="Toto" : to be set if BUILD_CONFIG=CUSTOM
# BUILD_NUMBER=X: To be set to indicate the revision number of the build. For example RC1,RC2, RC3 etc...
# MKJOBS: Number of threads
# CONFIG=(debug,release,relwithdebinfo): the build type
# DISABLE_BREAKPAD=1: When set, automatic crash reporting (google-breakpad support) will be disabled
#Usage MKJOBS=4 BUILD_CONFIG=SNAPSHOT CONFIG=relwithdebinfo BRANCH=workshop PLUGINDIR="..."  ./build-natron.sh

source `pwd`/common.sh || exit 1
MACPORTS="/opt/local"
QTDIR="${MACPORTS}/libexec/qt4"

cd "$CWD/build" || exit 1

if [ "$PLUGINDIR" = "$CWD/build/Natron/App/Natron.app/Contents/Plugins" ]; then
    echo "Warning: PLUGINDIR is $PLUGINDIR (should *really* be .../OFX/Natron)"
fi

if [ "$BRANCH" = "workshop" ]; then
    NATRON_BRANCH=$BRANCH
else
    NATRON_BRANCH=$NATRON_GIT_TAG
fi

git clone $GIT_NATRON
cd Natron || exit 1
git checkout $NATRON_BRANCH || exit 1
git submodule update -i --recursive || exit 1
if [ "$BRANCH" = "workshop" ]; then
    # the snapshots are always built with the latest version of submodules
    git submodule foreach git pull origin master
fi

#Always bump NATRON_DEVEL_GIT, it is only used to version-stamp binaries
NATRON_REL_V=`git log|head -1|awk '{print $2}'`
sed -i "" -e "s/NATRON_DEVEL_GIT=.*/NATRON_DEVEL_GIT=${NATRON_REL_V}/" $CWD/commits-hash.sh || exit 1
NATRON_MAJOR=`grep "define NATRON_VERSION_MAJOR" $TMP/Natron/Global/Macros.h | awk '{print $3}'`
NATRON_MINOR=`grep "define NATRON_VERSION_MINOR" $TMP/Natron/Global/Macros.h | awk '{print $3}'`
NATRON_REVISION=`grep "define NATRON_VERSION_REVISION" $TMP/Natron/Global/Macros.h | awk '{print $3}'`
sed -i "" -e "s/NATRON_VERSION_NUMBER=.*/NATRON_VERSION_NUMBER=${NATRON_MAJOR}.${NATRON_MINOR}.${NATRON_REVISION}/" "$CWD/commits-hash.sh" || exit 1

echo
echo "Building Natron $NATRON_REL_V from $NATRON_BRANCH on $OS using $MKJOBS threads."
echo
sleep 2

#Update GitVersion to have the correct hash
cp $CWD/GitVersion.h Global/GitVersion.h || exit 1
sed -i "" -e "s#__BRANCH__#${NATRON_BRANCH}#;s#__COMMIT__#${REL_GIT_VERSION}#"  Global/GitVersion.h || exit 1

#Generate config.pri
cat > config.pri <<EOF
boost {
    INCLUDEPATH += $MACPORTS/include
    LIBS += -L$MACPORTS/lib -lboost_serialization-mt
}
shiboken {
    PKGCONFIG -= shiboken
    INCLUDEPATH += $MACPORTS/include/shiboken-2.7
    LIBS += -L$MACPORTS/lib -lshiboken-python2.7.1.2
}
EOF

#Copy OpenColorIO-Config in Natron root as the .pro expects them there to copy it to the application bundle
cp -r $TMP/OpenColorIO-Configs . || exit 1

if [ "$BUILD_CONFIG" = "SNAPSHOT" ]; then
    EXTRA_QMAKE_FLAG="CONFIG+=snapshot"
elif [ "$BUILD_CONFIG" = "ALPHA" ]; then
	EXTRA_QMAKE_FLAG="CONFIG+=alpha  BUILD_NUMBER=$BUILD_NUMBER"
	if [ -z "$BUILD_NUMBER" ]; then
		echo "You must supply a BUILD_NUMBER when BUILD_CONFIG=ALPHA"
		exit 1
	fi

elif [ "$BUILD_CONFIG" = "BETA" ]; then
	if [ -z "$BUILD_NUMBER" ]; then
		echo "You must supply a BUILD_NUMBER when BUILD_CONFIG=BETA"
		exit 1
	fi
	EXTRA_QMAKE_FLAG="CONFIG+=beta  BUILD_NUMBER=$BUILD_NUMBER"
elif [ "$BUILD_CONFIG" = "RC" ]; then
	if [ -z "$BUILD_NUMBER" ]; then
		echo "You must supply a BUILD_NUMBER when BUILD_CONFIG=RC"
		exit 1
	fi
	EXTRA_QMAKE_FLAG="CONFIG+=RC BUILD_NUMBER=$BUILD_NUMBER"
elif [ "$BUILD_CONFIG" = "STABLE" ]; then
	EXTRA_QMAKE_FLAG="CONFIG+=stable"
elif [ "$BUILD_CONFIG" = "CUSTOM" ]; then
	if [ -z "$CUSTOM_BUILD_USER_NAME" ]; then
		echo "You must supply a CUSTOM_BUILD_USER_NAME when BUILD_CONFIG=CUSTOM"
		exit 1
	fi
	EXTRA_QMAKE_FLAG="CONFIG+=stable BUILD_USER_NAME=$CUSTOM_BUILD_USER_NAME"
fi

if [ "$COMPILER" = "clang" ]; then
    SPEC=unsupported/macx-clang
else
    SPEC=macx-g++
fi

if [ "$DISABLE_BREAKPAD" = "1" ]; then
    QMAKE_BREAKPAD="CONFIG+=disable-breakpad"
fi

$QTDIR/bin/qmake -r -spec "$SPEC" QMAKE_CC="$CC" QMAKE_CXX="$CXX" QMAKE_LINK="$CXX" ${EXTRA_QMAKE_FLAG} CONFIG+=`echo $BITS| awk '{print tolower($0)}'` CONFIG+=noassertions CONFIG+=silent CONFIG+=$CONFIG $QMAKEEXTRAFLAGS $QMAKE_BREAKPAD || exit 1
make -j${MKJOBS} -C Engine mocables || exit 1
make -j${MKJOBS} -C Gui mocables || exit 1
make -j${MKJOBS} -C App mocables || exit 1
make -j${MKJOBS} || exit 1

env TAG=$TAG DISABLE_BREAKPAD="$DISABLE_BREAKPAD" DUMP_SYMS="$DUMP_SYMS" SYMBOLS_PATH="$CWD/build/symbols" ${CWD}/build-natron-deploy.sh "App/Natron.app" || exit 1

package="$CWD/build/Natron/App/Natron.app"
if [ "$PLUGINDIR" = "${package}/Contents/Plugins" ]; then
    mkdir -p "${package}/Contents/Plugins/PyPlugs"
    mkdir -p "${package}/Contents/Plugins/OFX/Natron"
    echo "HACK: moving Qt plugins from ${package}/Contents/PlugIns to ${package}/Contents/Plugins (should not be necessary when the OFX Plugins dir is ${package}/Contents/OFX/Natron)"
    mv "${package}/Contents/PlugIns" "${package}/Contents/Plugins" || exit 1

    ## the original qt.conf points to PlugIns (not Plugins)
    rm "${package}/Contents/Resources/qt.conf" || exit 1

    #Make a qt.conf file in Contents/Resources/
    cat > "${package}/Contents/Resources/qt.conf" <<EOF
[Paths]
Plugins = Plugins
EOF
fi

