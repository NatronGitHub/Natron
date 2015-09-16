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

#Usage MKJOBS=4 CONFIG=relwithdebinfo BRANCH=workshop PLUGINDIR="..."  ./build-natron.sh

source `pwd`/common.sh || exit 1

cd "$CWD/build" || exit 1

if [ "$BRANCH" == "workshop" ]; then
    NATRON_BRANCH=$BRANCH
else
    NATRON_BRANCH=$NATRON_GIT_TAG
fi

git clone $GIT_NATRON
cd Natron || exit 1
git checkout $NATRON_BRANCH || exit 1
git submodule update -i --recursive || exit 1

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
    INCLUDEPATH += /opt/local/include
    LIBS += -L/opt/local/lib -lboost_serialization-mt
}
shiboken {
    PKGCONFIG -= shiboken
    INCLUDEPATH += /opt/local/include/shiboken-2.7
    LIBS += -L/opt/local/lib -lshiboken-python2.7.1.2
}
EOF

#Copy OpenColorIO-Config in Natron root as the .pro expects them there to copy it to the application bundle
cp -r $TMP/OpenColorIO-Configs . || exit 1

# Add CONFIG+=snapshot to indicate the build is a snapshot
if [ "$BRANCH" == "workshop" ]; then
    QMAKEEXTRAFLAGS=CONFIG+=snapshot
#else
#    QMAKEEXTRAFLAGS=CONFIG+=gbreakpad Enable when ready
fi


if [ "$COMPILER" = "clang" ]; then
    SPEC=unsupported/macx-clang
else
    SPEC=macx-g++
fi
qmake -r -spec "$SPEC" QMAKE_CC="$CC" QMAKE_CXX="$CXX" QMAKE_LINK="$CXX" CONFIG+="$CONFIG" CONFIG+=`echo $BITS| awk '{print tolower($0)}'` CONFIG+=noassertions $QMAKEEXTRAFLAGS || exit 1
make -j${MKJOBS} || exit 1

${CWD}/build-natron-deploy.sh "App/Natron.app" || exit 1
