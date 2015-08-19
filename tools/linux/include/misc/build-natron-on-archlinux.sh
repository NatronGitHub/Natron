#!/bin/sh
# Build Natron on ArchLinux

echo "Remember to install system depends:"
echo "pacman -S qt4 cairo glew python expat boost pixman ffmpeg opencolorio openimageio wget git cmake gcc make libxslt pkg-config"
echo
sleep 3

CWD=$(pwd)
MKJOBS=2

INSTALL_PATH=/opt/Natron-2.0
export PKG_CONFIG_PATH=$INSTALL_PATH/lib/pkgconfig
export LD_LIBRARY_PATH=$INSTALL_PATH/lib:$LD_LIBRARY_PATH
export PATH=$INSTALL_PATH/bin:$PATH

SRC_URL=http://repo.natronvfx.com/source
SHIBOK_TAR=shiboken-1.2.2.tar.bz2
PYSIDE_TAR=pyside-qt4.8+1.2.2.tar.bz2

if [ ! -f $INSTALL_PATH/lib/pkgconfig/shiboken.pc ]; then
  cd $CWD || exit 1
  if [ ! -f $SHIBOK_TAR ]; then
    wget $SRC_URL/$SHIBOK_TAR -O $SHIBOK_TAR || exit 1
  fi
  tar xvf $SHIBOK_TAR || exit 1
  cd shiboken-* || exit 1
  mkdir -p build && cd build || exit 1
  cmake ../ -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF -DUSE_PYTHON3=yes
  make -j$MKJOBS install || exit 1
fi

if [ ! -f $INSTALL_PATH/lib/pkgconfig/pyside.pc ]; then
  cd $CWD || exit 1
  if [ ! -f $PYSIDE_TAR ]; then
    wget $SRC_URL/$PYSIDE_TAR -O $PYSIDE_TAR || exit 1
  fi
  tar xvf $PYSIDE_TAR || exit 1
  cd pyside-qt4* || exit 1
  mkdir -p build && cd build || exit 1
  cmake .. -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
  make -j$MKJOBS install || exit 1
fi

if [ "$UID" == "0" ]; then
  echo "Please rerun this script as an user."
  exit 1
fi

if [ ! -d $CWD/Natron ]; then
  cd $CWD || exit 1
  git clone https://github.com/MrKepzie/Natron.git || exit 1
  cd Natron || exit 1
  git checkout workshop || exit 1
  git submodule update -i --recursive || exit 1
  wget https://raw.githubusercontent.com/olear/natron-linux/master/include/natron/config_py3.pri -O config.pri || exit 1
fi

cd $CWD/Natron || exit 1
rm -rf build
mkdir -p build && cd build || exit 1
qmake-qt4 ../Project.pro || exit 1
make -j$MKJOBS || exit 1

echo
echo "DONE!!!"
exit 0
