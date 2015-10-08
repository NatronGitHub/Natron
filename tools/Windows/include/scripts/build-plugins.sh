#!/bin/sh
#
# Build Natron Plugins for Windows
#

source `pwd`/common.sh || exit 1


if [ "$1" = "32" ]; then
  INSTALL_PATH=$INSTALL32_PATH
  PKG_PREFIX=$PKG_PREFIX32
  BIT=32
elif [ "$1" = "64" ]; then
  INSTALL_PATH=$INSTALL64_PATH
  PKG_PREFIX=$PKG_PREFIX64
  BIT=64
else
    echo "BIT=?"
    exit 1
fi



#If "workshop" is passed, use master branch for all plug-ins otherwise use the git tags in common.sh
IO_BRANCH=master
MISC_BRANCH=master
ARENA_BRANCH=master
CV_BRANCH=master

if [ "$2" != "workshop" ]; then
  IO_BRANCH=$IOPLUG_GIT_TAG
  MISC_BRANCH=$MISCPLUG_GIT_TAG
  ARENA_V=$ARENAPLUG_GIT_TAG
  CV_V=$CVPLUG_GIT_TAG
fi

if [ "$NATRON_LICENSE" != "GPL" ] && [ "$NATRON_LICENSE" != "COMMERCIAL" ]; then
    echo "Please select a License with NATRON_LICENSE=(GPL,COMMERCIAL)"
    exit 1
fi


if [ -d $TMP_PATH ]; then
  rm -rf $TMP_PATH || exit 1
fi
mkdir -p $TMP_PATH || exit 1

if [ "$NATRON_LICENSE" = "GPL" ]; then
    FFMPEG_PATH=$INSTALL_PATH/ffmpeg-GPL
elif [ "$NATRON_LICENSE" = "COMMERCIAL" ]; then
    FFMPEG_PATH=$INSTALL_PATH/ffmpeg-LGPL
fi

# Setup env
export PKG_CONFIG_PATH=$FFMPEG_PATH/lib/pkgconfig:$PKG_CONFIG_PATH
export QTDIR=$INSTALL_PATH
export BOOST_ROOT=$INSTALL_PATH
export OPENJPEG_HOME=$INSTALL_PATH
export THIRD_PARTY_TOOLS_HOME=$INSTALL_PATH

export GIT_SSL_NO_VERIFY=true

if [ -d $INSTALL_PATH/Plugins ]; then
  rm -rf $INSTALL_PATH/Plugins || exit 1
fi
mkdir -p $INSTALL_PATH/Plugins || exit 1
rm -rf $INSTALL_PATH/docs/openfx-* || exit 1

if [ -z "$BUILD_IO" ]; then
  BUILD_IO=1
fi
if [ -z "$BUILD_MISC" ]; then
  BUILD_MISC=1
fi
if [ -z "$BUILD_ARENA" ]; then
  BUILD_ARENA=1
fi
if [ -z "$BUILD_CV" ]; then
  BUILD_CV=1
fi

# MISC
if [ "$BUILD_MISC" = "1" ]; then

cd $TMP_PATH || exit 1

git clone $GIT_MISC || exit 1
cd openfx-misc || exit 1
git checkout ${MISC_BRANCH} || exit 1
git submodule update -i --recursive || exit 1
if [ "$MISC_BRANCH" = "master" ]; then
    # the snapshots are always built with the latest version of submodules
    git submodule foreach git pull origin master
fi

MISC_GIT_VERSION=`git log|head -1|awk '{print $2}'`

# mksrc
if [ "$MKSRC" = "1" ]; then
  cd .. || exit 1
  cp -a openfx-misc openfx-misc-$MISC_GIT_VERSION || exit 1
  (cd openfx-misc-$MISC_GIT_VERSION;find . -type d -name .git -exec rm -rf {} \;)
  tar cvvJf $SRC_PATH/openfx-misc-$MISC_GIT_VERSION.tar.xz openfx-misc-$MISC_GIT_VERSION || exit 1
  rm -rf openfx-misc-$MISC_GIT_VERSION || exit 1
  cd openfx-misc || exit 1
fi

#Always bump git commit, it is only used to version-stamp binaries
MISC_V=$MISC_GIT_VERSION
sed -i "s/MISCPLUG_DEVEL_GIT=.*/MISCPLUG_DEVEL_GIT=${MISC_V}/" $CWD/commits-hash.sh || exit 1

make -C CImg CImg.h || exit 1
make -C CImg CXXFLAGS_ADD="-fopenmp" LDFLAGS_ADD="-fopenmp" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" CONFIG=relwithdebinfo BITS=$BIT -j${MKJOBS} || exit 1
make CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" CONFIG=relwithdebinfo BITS=$BIT -j${MKJOBS} || exit 1

cp -a */MINGW${BIT}_NT-6.1-${BIT}-*/*.ofx.bundle $INSTALL_PATH/Plugins/ || exit 1

mkdir -p $INSTALL_PATH/docs/openfx-misc || exit 1
cp LICENSE README* $INSTALL_PATH/docs/openfx-misc/ || exit 1
echo $MISC_GIT_VERSION > $INSTALL_PATH/docs/openfx-misc/VERSION || exit 1

fi

# IO
if [ "$BUILD_IO" = "1" ]; then

cd $TMP_PATH || exit 1

git clone $GIT_IO || exit 1
cd openfx-io || exit 1
git checkout ${IO_BRANCH} || exit 1
git submodule update -i --recursive || exit 1
if [ "$IO_BRANCH" = "master" ]; then
    # the snapshots are always built with the latest version of submodules
    git submodule foreach git pull origin master
fi

IO_GIT_VERSION=`git log|head -1|awk '{print $2}'`

# mksrc
if [ "$MKSRC" = "1" ]; then
  cd .. || exit 1
  cp -a openfx-io openfx-io-$IO_GIT_VERSION || exit 1
  (cd openfx-io-$IO_GIT_VERSION;find . -type d -name .git -exec rm -rf {} \;)
  tar cvvJf $SRC_PATH/openfx-io-$IO_GIT_VERSION.tar.xz openfx-io-$IO_GIT_VERSION || exit 1
  rm -rf openfx-io-$IO_GIT_VERSION || exit 1
  cd openfx-io || exit 1
fi

#Always bump git commit, it is only used to version-stamp binaries
IO_V=$IO_GIT_VERSION
sed -i "s/IOPLUG_DEVEL_GIT=.*/IOPLUG_DEVEL_GIT=${IO_V}/" $CWD/commits-hash.sh || exit 1


env CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" make CONFIG=relwithdebinfo BITS=$BIT -j${MKJOBS} || exit 1

cp -a IO/MINGW${BIT}_NT-6.1-${BIT}-*/IO.ofx.bundle $INSTALL_PATH/Plugins/ || exit 1

mkdir -p $INSTALL_PATH/docs/openfx-io || exit 1
cp LICENSE README* $INSTALL_PATH/docs/openfx-io/ || exit 1
echo $IO_GIT_VERSION > $INSTALL_PATH/docs/openfx-io/VERSION || exit 1

fi

# ARENA
if [ "$BUILD_ARENA" = "1" ]; then

cd $TMP_PATH || exit 1

git clone $GIT_ARENA || exit 1
cd openfx-arena || exit 1
git checkout ${ARENA_BRANCH} || exit 1
git submodule update -i --recursive || exit 1
if [ "$ARENA_BRANCH" = "master" ]; then
    # the snapshots are always built with the latest version of submodules
    git submodule foreach git pull origin master
fi

ARENA_GIT_VERSION=`git log|head -1|awk '{print $2}'`

# mksrc
if [ "$MKSRC" = "1" ]; then
  cd .. || exit 1
  cp -a openfx-arena openfx-arena-$ARENA_GIT_VERSION || exit 1
  (cd openfx-arena-$ARENA_GIT_VERSION;find . -type d -name .git -exec rm -rf {} \;)
  tar cvvJf $SRC_PATH/openfx-arena-$ARENA_GIT_VERSION.tar.xz openfx-arena-$ARENA_GIT_VERSION || exit 1
  rm -rf openfx-arena-$ARENA_GIT_VERSION || exit 1
  cd openfx-arena || exit 1
fi

#Always bump git commit, it is only used to version-stamp binaries
ARENA_V=$ARENA_GIT_VERSION
sed -i "s/ARENAPLUG_DEVEL_GIT=.*/ARENAPLUG_DEVEL_GIT=${ARENA_V}/" $CWD/commits-hash.sh || exit 1


env CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" make MINGW=1 USE_SVG=1 USE_PANGO=1 STATIC=1 CONFIG=relwithdebinfo BITS=$BIT -j${MKJOBS} || exit 1
cp -a Bundle/MINGW${BIT}_NT-6.1-${BIT}-*/Arena.ofx.bundle $INSTALL_PATH/Plugins/ || exit 1


mkdir -p $INSTALL_PATH/docs/openfx-arena || exit 1
cp LICENSE README.md $INSTALL_PATH/docs/openfx-arena/ || exit 1
echo $ARENA_V > $INSTALL_PATH/docs/openfx-arena/VERSION || exit 1

fi

# OPENCV
if [ "$BUILD_CV" = "1" ]; then

cd $TMP_PATH || exit 1

git clone $GIT_OPENCV || exit 1
cd openfx-opencv || exit 1
git checkout ${CV_BRANCH} || exit 1
git submodule update -i --recursive || exit 1
if [ "$CV_BRANCH" = "master" ]; then
    # the snapshots are always built with the latest version of submodules
    git submodule foreach git pull origin master
fi

CV_GIT_VERSION=`git log|head -1|awk '{print $2}'`

# mksrc
if [ "$MKSRC" = "1" ]; then
  cd .. || exit 1
  cp -a openfx-opencv openfx-opencv-$CV_GIT_VERSION || exit 1
  (cd openfx-opencv-$CV_GIT_VERSION;find . -type d -name .git -exec rm -rf {} \;)
  tar cvvJf $SRC_PATH/openfx-opencv-$CV_GIT_VERSION.tar.xz openfx-opencv-$CV_GIT_VERSION || exit 1
  rm -rf openfx-opencv-$CV_GIT_VERSION || exit 1
  cd openfx-opencv || exit 1
fi

#Always bump git commit, it is only used to version-stamp binaries
CV_V=$CV_GIT_VERSION
sed -i "s/CVPLUG_DEVEL_GIT=.*/CVPLUG_DEVEL_GIT=${CV_V}/" $CWD/commits-hash.sh || exit 1


cd opencv2fx || exit 1
env CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" make CONFIG=relwithdebinfo BITS=$BIT || exit 1

cp -a */MINGW${BIT}_NT-6.1-${BIT}-*/*.ofx.bundle $INSTALL_PATH/Plugins/ || exit 1

mkdir -p $INSTALL_PATH/docs/openfx-opencv || exit 1
cp LIC* READ* $INSTALL_PATH/docs/openfx-opencv/ 
echo $CV_V > $INSTALL_PATH/docs/openfx-opencv/VERSION || exit 1

fi

echo "Done!"
