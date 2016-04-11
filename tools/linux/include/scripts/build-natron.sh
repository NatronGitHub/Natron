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

# PYV=3 Use Python 3 otherwise use Python 2
# MKJOBS=X: Nb threads, 4 is default as defined in common
# BUILD_CONFIG=(SNAPSHOT,ALPHA,BETA,RC,STABLE,CUSTOM)
# CUSTOM_BUILD_USER_NAME="Toto" : to be set if BUILD_CONFIG=CUSTOM
# BUILD_NUMBER=X: To be set to indicate the revision number of the build. For example RC1,RC2, RC3 etc...
# DISABLE_BREAKPAD=1: Disable automatic crash report
# GIT_BRANCH=xxxx if not defined git tags from common will be used
# GIT_COMMIT=xxxx if not defined latest commit from GIT_BRANCH will be built

source `pwd`/common.sh || exit 1
PID=$$

# make kill bot
KILLSCRIPT="/tmp/killbot$$.sh"
cat << 'EOF' > "$KILLSCRIPT"
#!/bin/sh
PARENT=$1
sleep 30m
if [ "$PARENT" = "" ]; then
  exit 1
fi
PIDS=`ps aux|awk '{print $2}'|grep $PARENT`
if [ "$PIDS" = "$PARENT" ]; then
  kill -15 $PARENT
fi
EOF
chmod +x $KILLSCRIPT

# we need $BUILD_CONFIG
if [ -z "$BUILD_CONFIG" ]; then
  echo "Please define BUILD_CONFIG"
  exit 1
fi

# Assume that $GIT_BRANCH is the branch to build, otherwise if empty use the NATRON_GIT_TAG in common.sh, but if BUILD_CONFIG=SNAPSHOT use MASTER_BRANCH
NATRON_BRANCH=$GIT_BRANCH
if [ -z "$NATRON_BRANCH" ]; then
  if [ "$BUILD_CONFIG" = "SNAPSHOT" ]; then
    NATRON_BRANCH=$MASTER_BRANCH
    COMMITS_HASH=$CWD/commits-hash-$NATRON_BRANCH.sh
  else
    NATRON_BRANCH=$NATRON_GIT_TAG
    COMMITS_HASH=$CWD/commits-hash.sh
  fi
else
  COMMITS_HASH=$CWD/commits-hash-$NATRON_BRANCH.sh
fi

echo "===> Using branch $NATRON_BRANCH"

# Check for SDK, extract or build if not present
if [ ! -d $INSTALL_PATH ]; then
  if [ -f $SRC_PATH/Natron-$SDK_VERSION-Linux-$ARCH-SDK.tar.xz ]; then
    echo "Found binary SDK, extracting ..."
    tar xvJf $SRC_PATH/Natron-$SDK_VERSION-Linux-$ARCH-SDK.tar.xz -C $SDK_PATH/ || exit 1
  else
    echo "Need to build SDK ..."
    sh $INC_PATH/scripts/build-sdk.sh || exit 1
  fi
fi

# Setup tmp
if [ -d $TMP_PATH ]; then
  rm -rf $TMP_PATH || exit 1
fi
mkdir -p $TMP_PATH || exit 1

# Setup env
export PKG_CONFIG_PATH=$INSTALL_PATH/lib/pkgconfig:$INSTALL_PATH/libdata/pkgconfig
export QTDIR=$INSTALL_PATH
export BOOST_ROOT=$INSTALL_PATH
export PYTHON_HOME=$INSTALL_PATH
export LD_LIBRARY_PATH=$INSTALL_PATH/lib
export PATH=$INSTALL_PATH/gcc/bin:$INSTALL_PATH/bin:$PATH

# gcc libs
if [ "$ARCH" = "x86_64" ]; then
  export LD_LIBRARY_PATH=$INSTALL_PATH/gcc/lib64:$LD_LIBRARY_PATH
else
  export LD_LIBRARY_PATH=$INSTALL_PATH/gcc/lib:$LD_LIBRARY_PATH
fi

# python
if [ "$PYV" = "3" ]; then
  export PYTHON_PATH=$INSTALL_PATH/lib/python3.4
  export PYTHON_INCLUDE=$INSTALL_PATH/include/python3.4
else
  export PYTHON_PATH=$INSTALL_PATH/lib/python2.7
  export PYTHON_INCLUDE=$INSTALL_PATH/include/python2.7
fi

cd $TMP_PATH || exit 1

# kill git if idle for too long
$KILLSCRIPT $PID &
KILLBOT=$! 

# clone natron
git clone $GIT_NATRON || exit 1
cd Natron || exit 1

# checkout branch
git checkout $NATRON_BRANCH || exit 1

# if we have a predefined commit use that, else use latest commit from branch
if [ -z "$GIT_COMMIT" ]; then
  git pull origin $NATRON_BRANCH
else
  git checkout $GIT_COMMIT
fi

# Update submodule
git submodule update -i --recursive || exit 1

# the snapshot are always built with latest version
if [ "$NATRON_BRANCH" = "$MASTER_BRANCH" ]; then
  git submodule foreach git pull origin master
fi

# kill idle checker
kill -9 $KILLBOT

# Get commit
REL_GIT_VERSION=`git log|head -1|awk '{print $2}'`

#Always bump NATRON_DEVEL_GIT, it is only used to version-stamp binaries
NATRON_REL_V=$REL_GIT_VERSION

NATRON_MAJOR=`grep "define NATRON_VERSION_MAJOR" $TMP_PATH/Natron/Global/Macros.h | awk '{print $3}'`
NATRON_MINOR=`grep "define NATRON_VERSION_MINOR" $TMP_PATH/Natron/Global/Macros.h | awk '{print $3}'`
NATRON_REVISION=`grep "define NATRON_VERSION_REVISION" $TMP_PATH/Natron/Global/Macros.h | awk '{print $3}'`

if [ ! -f "$COMMITS_HASH" ]; then
cat <<EOF > "$COMMITS_HASH"
#!/bin/sh
NATRON_DEVEL_GIT=#
IOPLUG_DEVEL_GIT=#
MISCPLUG_DEVEL_GIT=#
ARENAPLUG_DEVEL_GIT=#
CVPLUG_DEVEL_GIT=#
NATRON_VERSION_NUMBER=#
EOF
fi

sed -i "s/NATRON_DEVEL_GIT=.*/NATRON_DEVEL_GIT=${NATRON_REL_V}/" $COMMITS_HASH || exit 1
sed -i "s/NATRON_VERSION_NUMBER=.*/NATRON_VERSION_NUMBER=${NATRON_MAJOR}.${NATRON_MINOR}.${NATRON_REVISION}/" $COMMITS_HASH || exit 1

echo "===> Building Natron $NATRON_REL_V from $NATRON_BRANCH against SDK $SDK_VERSION on $ARCH using $MKJOBS threads."

# Plugins git hash
IO_VERSION_FILE=$INSTALL_PATH/Plugins/IO.ofx.bundle-version.txt
MISC_VERSION_FILE=$INSTALL_PATH/Plugins/Misc.ofx.bundle-version.txt
ARENA_VERSION_FILE=$INSTALL_PATH/Plugins/Arena.ofx.bundle-version.txt

if [ -f "$IO_VERSION_FILE" ]; then
  IO_GIT_HASH=`cat ${IO_VERSION_FILE}`
fi
if [ -f "$MISC_VERSION_FILE" ]; then
  MISC_GIT_HASH=`cat ${MISC_VERSION_FILE}`
fi
if [ -f "$ARENA_VERSION_FILE" ]; then
  ARENA_GIT_HASH=`cat ${ARENA_VERSION_FILE}`
fi

#Update GitVersion to have the correct hash
cat $INC_PATH/natron/GitVersion.h | sed "s#__BRANCH__#${NATRON_BRANCH}#;s#__COMMIT__#${REL_GIT_VERSION}#;s#__IO_COMMIT__#${IO_GIT_HASH}#;s#__MISC_COMMIT__#${MISC_GIT_HASH}#;s#__ARENA_COMMIT__#${ARENA_GIT_HASH}#" > Global/GitVersion.h || exit 1

# add config based on python version
if [ "$PYV" = "3" ]; then
  cat $INC_PATH/natron/config_py3.pri > config.pri || exit 1
else
  cat $INC_PATH/natron/config.pri > config.pri || exit 1
fi

# setup build dir
rm -rf build
mkdir build || exit 1
cd build || exit 1

# setup version
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

if [ "$PYV" = "3" ]; then
  PYO="PYTHON_CONFIG=python3.4m-config"
fi

# use breakpad?
if [ "$DISABLE_BREAKPAD" = "1" ]; then
  CONFIG_BREAKPAD_FLAG="CONFIG+=disable-breakpad"
fi

# build
env CFLAGS="$BF" CXXFLAGS="$BF" $INSTALL_PATH/bin/qmake -r CONFIG+=relwithdebinfo ${CONFIG_BREAKPAD_FLAG} CONFIG+=silent ${EXTRA_QMAKE_FLAG} ${PYO} DEFINES+=QT_NO_DEBUG_OUTPUT ../Project.pro || exit 1
make -j${MKJOBS} || exit 1

# install app(s)
rm -f $INSTALL_PATH/Natron*

cp App/Natron $INSTALL_PATH/bin/ || exit 1
cp Renderer/NatronRenderer $INSTALL_PATH/bin/ || exit 1

# install crashapp(s)
if [ "$DISABLE_BREAKPAD" != "1" ]; then
  cp CrashReporter/NatronCrashReporter $INSTALL_PATH/bin/ || exit 1
  cp CrashReporterCLI/NatronRendererCrashReporter $INSTALL_PATH/bin/ || exit 1
fi

# If OpenColorIO-Configs do not exist, download them
if [ ! -d "$SRC_PATH/OpenColorIO-Configs" ]; then
  mkdir -p "$SRC_PATH"
  wget $GIT_OCIO_CONFIG_TAR -O "$SRC_PATH/OpenColorIO-Configs.tar.gz" || exit 1
  (cd "$SRC_PATH"; tar xf OpenColorIO-Configs.tar.gz) || exit 1
  rm "$SRC_PATH/OpenColorIO-Configs.tar.gz" || exit 1
  mv "$SRC_PATH/OpenColorIO-Configs"* "$SRC_PATH/OpenColorIO-Configs" || exit 1
  rm -rf "$SRC_PATH/OpenColorIO-Configs/aces_1.0.1/baked"
  rm -rf "$SRC_PATH/OpenColorIO-Configs/aces_1.0.1/python"
fi

# copy everything else we need
cp -a $SRC_PATH/OpenColorIO-Configs $INSTALL_PATH/share/ || exit 1
mkdir -p $INSTALL_PATH/docs/natron || exit 1
cp ../LICENSE.txt ../README* ../BUGS* ../CONTRI* ../Documentation/* $INSTALL_PATH/docs/natron/
mkdir -p $INSTALL_PATH/share/stylesheets || exit 1
cp ../Gui/Resources/Stylesheets/mainstyle.qss $INSTALL_PATH/share/stylesheets/ || exit 1
mkdir -p $INSTALL_PATH/share/pixmaps || exit 1
cp ../Gui/Resources/Images/natronIcon256_linux.png $INSTALL_PATH/share/pixmaps/ || exit 1
cp ../Gui/Resources/Images/natronProjectIcon_linux.png $INSTALL_PATH/share/pixmaps/ || exit 1
echo $NATRON_REL_V > $INSTALL_PATH/docs/natron/VERSION || exit 1

rm -rf $INSTALL_PATH/PyPlugs
mkdir -p $INSTALL_PATH/PyPlugs || exit 1
cp ../Gui/Resources/PyPlugs/* $INSTALL_PATH/PyPlugs/ || exit 1

rm -f $KILLSCRIPT

echo "Done!"

