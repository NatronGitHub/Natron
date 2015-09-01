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

#
# Build packages and installer for Linux
#

source $(pwd)/common.sh || exit 1
source $(pwd)/commits-hash.sh || exit 1

PID=$$
if [ -f $TMP_DIR/natron-build-installer.pid ]; then
  OLDPID=$(cat $TMP_DIR/natron-build-installer.pid)
  PIDS=$(ps aux|awk '{print $2}')
  for i in $PIDS;do
    if [ "$i" == "$OLDPID" ]; then
      echo "already running ..."
      exit 1
    fi
  done
fi
echo $PID > $TMP_DIR/natron-build-installer.pid || exit 1

if [ "$1" == "workshop" ]; then
  NATRON_VERSION=$NATRON_DEVEL_GIT
  REPO_BRANCH=snapshots
else
  NATRON_VERSION=$NATRON_VERSION_NUMBER
  REPO_BRANCH=releases
fi

DATE=$(date +%Y-%m-%d)
PKGOS=Linux-x86_${BIT}bit
REPO_OS=Linux/$REPO_BRANCH/${BIT}bit/packages

export LD_LIBRARY_PATH=$INSTALL_PATH/lib

if [ -d $TMP_PATH ]; then
  rm -rf $TMP_PATH || exit 1
fi
mkdir -p $TMP_PATH || exit 1

# SETUP
INSTALLER=$TMP_PATH/Natron-installer
XML=$INC_PATH/xml
QS=$INC_PATH/qs

mkdir -p $INSTALLER/config $INSTALLER/packages || exit 1
cat $INC_PATH/config/config.xml | sed "s/_VERSION_/${NATRON_VERSION_NUMBER}/;s#_OS_BRANCH_BIT_#${REPO_OS}#g;s#_URL_#${REPO_URL}#g" > $INSTALLER/config/config.xml || exit 1
cp $INC_PATH/config/*.png $INSTALLER/config/ || exit 1

# OFX IO
OFX_IO_VERSION=$TAG
OFX_IO_PATH=$INSTALLER/packages/$IOPLUG_PKG
mkdir -p $OFX_IO_PATH/data $OFX_IO_PATH/meta $OFX_IO_PATH/data/Plugins || exit 1
cat $XML/openfx-io.xml | sed "s/_VERSION_/${OFX_IO_VERSION}/;s/_DATE_/${DATE}/" > $OFX_IO_PATH/meta/package.xml || exit 1
cat $QS/openfx-io.qs > $OFX_IO_PATH/meta/installscript.qs || exit 1
cat $INSTALL_PATH/docs/openfx-io/VERSION > $OFX_IO_PATH/meta/ofx-io-license.txt || exit 1
echo "" >> $OFX_IO_PATH/meta/ofx-io-license.txt || exit 1
cat $INSTALL_PATH/docs/openfx-io/LICENSE >> $OFX_IO_PATH/meta/ofx-io-license.txt || exit 1
cp -a $INSTALL_PATH/Plugins/IO.ofx.bundle $OFX_IO_PATH/data/Plugins/ || exit 1
strip -s $OFX_IO_PATH/data/Plugins/*/*/*/*
IO_LIBS=$OFX_IO_PATH/data/Plugins/IO.ofx.bundle/Libraries
mkdir -p $IO_LIBS || exit 1

OFX_DEPENDS=$(ldd $INSTALLER/packages/*/data/Plugins/*/*/*/*|grep opt | awk '{print $3}')
for x in $OFX_DEPENDS; do
  cp -v $x $IO_LIBS/ || exit 1
done

if [ "$SDK_LIC" == "GPL" ]; then
  FFLIC=gpl
else
  FFLIC=lgpl
fi
cp -v $INSTALL_PATH/ffmpeg-$FFLIC/lib/{libavformat.so.56,libavcodec.so.56,libswscale.so.3,libavutil.so.54,libswresample.so.1} $IO_LIBS/ || exit 1
OFX_LIB_DEP=$(ldd $IO_LIBS/*|grep opt | awk '{print $3}')
for y in $OFX_LIB_DEP; do
  cp -v $y $IO_LIBS/ || exit 1
done

IO_LIC=$OFX_IO_PATH/meta/ofx-io-license.txt
echo "" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
echo "BOOST:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/boost/LICENSE_1_0.txt >>$IO_LIC || exit 1

echo "" >>$IO_LIC || exit 1
echo "FFMPEG:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
if [ "$SDK_LIC" == "GPL" ]; then
  cat $INSTALL_PATH/docs/ffmpeg/COPYING.GPLv3 >> $IO_LIC || exit 1
else
  cat $INSTALL_PATH/docs/ffmpeg/COPYING.LGPLv2.1 >>$IO_LIC || exit 1
fi

echo "" >>$IO_LIC || exit 1
echo "JPEG:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/jpeg/README  >>$IO_LIC  || exit 1

echo "" >>$IO_LIC || exit 1
echo "OPENCOLORIO:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/ocio/LICENSE >>$IO_LIC || exit 1

echo "" >>$IO_LIC || exit 1
echo "OPENIMAGEIO:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/oiio/LICENSE >>$IO_LIC || exit 1

echo "" >>$IO_LIC || exit 1
echo "OPENEXR:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/openexr/LICENSE >>$IO_LIC || exit 1

echo "" >>$IO_LIC || exit 1
echo "OPENJPEG:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/openjpeg/LICENSE >>$IO_LIC || exit 1

echo "" >>$IO_LIC || exit 1
echo "PNG:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/png/LICENSE >>$IO_LIC || exit 1

echo "" >>$IO_LIC || exit 1
echo "TIFF:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/tiff/COPYRIGHT >>$IO_LIC || exit 1

echo "" >>$IO_LIC || exit 1
echo "SEEXPR:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/seexpr/license.txt >>$IO_LIC || exit 1

echo "" >>$IO_LIC || exit 1
echo "LIBRAW:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/libraw/COPYRIGHT >>$IO_LIC || exit 1

echo "" >>$IO_LIC || exit 1
echo "JASPER:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/jasper/COPYRIGHT >>$IO_LIC || exit 1

echo "" >>$IO_LIC || exit 1
echo "LCMS:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/lcms/COPYING >>$IO_LIC || exit 1

echo "" >>$IO_LIC || exit 1
echo "DIRAC:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/dirac/COPYING.MPL >>$IO_LIC || exit 1

echo "" >>$IO_LIC || exit 1
echo "LAME:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/lame/COPYING >>$IO_LIC || exit 1

echo "" >>$IO_LIC || exit 1
echo "MODPLUG:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/libmodplug/COPYING >>$IO_LIC || exit 1

echo "" >>$IO_LIC || exit 1
echo "OGG:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/libogg/COPYING >>$IO_LIC || exit 1

echo "" >>$IO_LIC || exit 1
echo "THEORA:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/libtheora/COPYING >>$IO_LIC || exit 1

echo "" >>$IO_LIC || exit 1
echo "VORBIS:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/libvorbis/COPYING >>$IO_LIC || exit 1

echo "" >>$IO_LIC || exit 1
echo "VPX:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/libvpx/LICENSE >>$IO_LIC || exit 1

echo "" >>$IO_LIC || exit 1
echo "OPUS:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/opus/COPYING >>$IO_LIC || exit 1

echo "" >>$IO_LIC || exit 1
echo "ORC:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/orc/COPYING >>$IO_LIC || exit 1

echo "" >>$IO_LIC || exit 1
echo "SPEEX:" >>$IO_LIC || exit 1
echo "" >>$IO_LIC || exit 1
cat $INSTALL_PATH/docs/speex/COPYING >>$IO_LIC || exit 1

if [ "$SDK_LIC" == "GPL" ]; then
  echo "" >>$IO_LIC || exit 1
  echo "X264:" >>$IO_LIC || exit 1
  echo "" >>$IO_LIC || exit 1
  cat $INSTALL_PATH/docs/x264/COPYING >>$IO_LIC || exit 1

  echo "" >>$IO_LIC || exit 1
  echo "XVID:" >>$IO_LIC || exit 1
  echo "" >>$IO_LIC || exit 1
  cat $INSTALL_PATH/docs/xvidcore/LICENSE >>$IO_LIC || exit 1
fi

# OFX MISC
OFX_MISC_VERSION=$TAG
OFX_MISC_PATH=$INSTALLER/packages/$MISCPLUG_PKG
mkdir -p $OFX_MISC_PATH/data $OFX_MISC_PATH/meta $OFX_MISC_PATH/data/Plugins || exit 1
cat $XML/openfx-misc.xml | sed "s/_VERSION_/${OFX_MISC_VERSION}/;s/_DATE_/${DATE}/" > $OFX_MISC_PATH/meta/package.xml || exit 1
cat $QS/openfx-misc.qs > $OFX_MISC_PATH/meta/installscript.qs || exit 1
cat $INSTALL_PATH/docs/openfx-misc/VERSION > $OFX_MISC_PATH/meta/ofx-misc-license.txt || exit 1
echo "" >> $OFX_MISC_PATH/meta/ofx-misc-license.txt || exit 1
cat $INSTALL_PATH/docs/openfx-misc/LICENSE >> $OFX_MISC_PATH/meta/ofx-misc-license.txt || exit 1
cp -a $INSTALL_PATH/Plugins/{CImg,Misc}.ofx.bundle $OFX_MISC_PATH/data/Plugins/ || exit 1
strip -s $OFX_MISC_PATH/data/Plugins/*/*/*/*

# NATRON
NATRON_PATH=$INSTALLER/packages/$NATRON_PKG
mkdir -p $NATRON_PATH/meta $NATRON_PATH/data/docs $NATRON_PATH/data/bin || exit 1
cat $XML/natron.xml | sed "s/_VERSION_/${TAG}/;s/_DATE_/${DATE}/" > $NATRON_PATH/meta/package.xml || exit 1
cat $QS/natron.qs > $NATRON_PATH/meta/installscript.qs || exit 1
cp -a $INSTALL_PATH/docs/natron/* $NATRON_PATH/data/docs/ || exit 1
cat $INSTALL_PATH/docs/natron/LICENSE.txt > $NATRON_PATH/meta/natron-license.txt || exit 1
cp $INSTALL_PATH/bin/Natron* $NATRON_PATH/data/bin/ || exit 1
strip -s $NATRON_PATH/data/bin/Natron $NATRON_PATH/data/bin/NatronRenderer $NATRON_PATH/data/bin/NatronCrashReporter
wget $NATRON_API_DOC || exit 1
mv natron.pdf $NATRON_PATH/data/docs/Natron_Python_API_Reference.pdf || exit 1
rm $NATRON_PATH/data/docs/TuttleOFX-README.txt || exit 1
cat $INC_PATH/scripts/Natron.sh > $NATRON_PATH/data/Natron || exit 1
cat $INC_PATH/scripts/Natron.sh | sed "s#bin/Natron#bin/NatronRenderer#" > $NATRON_PATH/data/NatronRenderer || exit 1
chmod +x $NATRON_PATH/data/Natron $NATRON_PATH/data/NatronRenderer || exit 1

# OCIO
OCIO_VERSION=$COLOR_PROFILES_VERSION
OCIO_PATH=$INSTALLER/packages/$PROFILES_PKG
mkdir -p $OCIO_PATH/meta $OCIO_PATH/data/share || exit 1
cat $XML/ocio.xml | sed "s/_VERSION_/${OCIO_VERSION}/;s/_DATE_/${DATE}/" > $OCIO_PATH/meta/package.xml || exit 1
cat $QS/ocio.qs > $OCIO_PATH/meta/installscript.qs || exit 1
cp -a $INSTALL_PATH/share/OpenColorIO-Configs $OCIO_PATH/data/share/ || exit 1

# CORE LIBS
CLIBS_VERSION=$CORELIBS_VERSION
CLIBS_PATH=$INSTALLER/packages/$CORELIBS_PKG
mkdir -p $CLIBS_PATH/meta $CLIBS_PATH/data/bin $CLIBS_PATH/data/lib $CLIBS_PATH/data/share/pixmaps || exit 1
cat $XML/corelibs.xml | sed "s/_VERSION_/${CLIBS_VERSION}/;s/_DATE_/${DATE}/" > $CLIBS_PATH/meta/package.xml || exit 1
cat $QS/corelibs.qs > $CLIBS_PATH/meta/installscript.qs || exit 1
cp $INSTALL_PATH/lib/libQtDBus.so.4 $CLIBS_PATH/data/lib/ || exit 1
cp $INSTALL_PATH/share/pixmaps/natronIcon256_linux.png $CLIBS_PATH/data/share/pixmaps/ || exit 1

cp -a $INSTALL_PATH/plugins/* $CLIBS_PATH/data/bin/ || exit 1
CORE_DEPENDS=$(ldd $NATRON_PATH/data/bin/*|grep opt | awk '{print $3}')
for i in $CORE_DEPENDS; do
  cp -v $i $CLIBS_PATH/data/lib/ || exit 1
done
LIB_DEPENDS=$(ldd $CLIBS_PATH/data/lib/*|grep opt | awk '{print $3}')
for y in $LIB_DEPENDS; do
  cp -v $y $CLIBS_PATH/data/lib/ || exit 1
done
PLUG_DEPENDS=$(ldd $CLIBS_PATH/data/bin/*/*|grep opt | awk '{print $3}')
for z in $PLUG_DEPENDS; do
  cp -v $z $CLIBS_PATH/data/lib/ || exit 1
done
if [ -f $INC_PATH/misc/compat${BIT}.tgz ]; then
  tar xvf $INC_PATH/misc/compat${BIT}.tgz -C $CLIBS_PATH/data/lib/ || exit 1
fi

# TODO: At this point send unstripped binaries (and debug binaries?) to Socorro server for breakpad

strip -s $CLIBS_PATH/data/lib/*
strip -s $CLIBS_PATH/data/bin/*/*

CORE_DOC=$CLIBS_PATH
cat $INSTALL_PATH/docs/boost/LICENSE_1_0.txt >> $CORE_DOC/meta/3rdparty-license.txt 
cat $INSTALL_PATH/docs/cairo/COPYING-MPL-1.1 >> $CORE_DOC/meta/3rdparty-license.txt
cat $INSTALL_PATH/docs/glew/LICENSE.txt >> $CORE_DOC/meta/3rdparty-license.txt
cat $INSTALL_PATH/docs/jpeg/README >> $CORE_DOC/meta/3rdparty-license.txt
cat $INSTALL_PATH/docs/png/LICENSE >> $CORE_DOC/meta/3rdparty-license.txt
cat $INSTALL_PATH/docs/qt/*LGPL* >> $CORE_DOC/meta/3rdparty-license.txt
cat $INSTALL_PATH/docs/tiff/COPYRIGHT >> $CORE_DOC/meta/3rdparty-license.txt

if [ "$PYV" == "3" ]; then
  cat $INSTALL_PATH/docs/python3/LICENSE >> $CORE_DOC/meta/3rdparty-license.txt || exit 1
else
  cat $INSTALL_PATH/docs/python2/LICENSE >> $CORE_DOC/meta/3rdparty-license.txt || exit 1
fi
cat $INSTALL_PATH/docs/pyside/* >> $CORE_DOC/meta/3rdparty-license.txt
cat $INSTALL_PATH/docs/shibroken/* >> $CORE_DOC/meta/3rdparty-license.txt 

#Copy Python distrib
mkdir -p $CLIBS_PATH/data/Plugins || exit 1
if [ "$PYV" == "3" ]; then
  cp -a $INSTALL_PATH/lib/python3.4 $CLIBS_PATH/data/lib/ || exit 1
  mv $CLIBS_PATH/data/lib/python3.4/site-packages/PySide $CLIBS_PATH/data/Plugins/ || exit 1
  (cd $CLIBS_PATH/data/lib/python3.4/site-packages; ln -sf ../../../Plugins/PySide . )
  rm -rf $CLIBS_PATH/data/lib/python3.4/{test,config-3.4m} || exit 1
else
  cp -a $INSTALL_PATH/lib/python2.7 $CLIBS_PATH/data/lib/ || exit 1
  mv $CLIBS_PATH/data/lib/python2.7/site-packages/PySide $CLIBS_PATH/data/Plugins/ || exit 1
  (cd $CLIBS_PATH/data/lib/python2.7/site-packages; ln -sf ../../../Plugins/PySide . )
  rm -rf $CLIBS_PATH/data/lib/python2.7/{test,config} || exit 1
fi
#rm -f $CLIBS_PATH/data/Plugins/PySide/{QtDeclarative,QtHelp,QtScript,QtScriptTools,QtSql,QtTest,QtUiTools,QtXmlPatterns}.so || exit 1
PY_DEPENDS=$(ldd $CLIBS_PATH/data/Plugins/PySide/*|grep opt | awk '{print $3}')
for y in $PY_DEPENDS; do
  cp -v $y $CLIBS_PATH/data/lib/ || exit 1
done
(cd $CLIBS_PATH ; find . -type d -name __pycache__ -exec rm -rf {} \;)
strip -s $CLIBS_PATH/data/Plugins/PySide/* $CLIBS_PATH/data/lib/python*/* $CLIBS_PATH/data/lib/python*/*/*

# OFX ARENA
OFX_ARENA_VERSION=$TAG
OFX_ARENA_PATH=$INSTALLER/packages/$ARENAPLUG_PKG
mkdir -p $OFX_ARENA_PATH/meta $OFX_ARENA_PATH/data/Plugins || exit 1
cat $XML/openfx-arena.xml | sed "s/_VERSION_/${OFX_ARENA_VERSION}/;s/_DATE_/${DATE}/" > $OFX_ARENA_PATH/meta/package.xml || exit 1
cat $QS/openfx-arena.qs > $OFX_ARENA_PATH/meta/installscript.qs || exit 1
cat $INSTALL_PATH/docs/openfx-arena/VERSION > $OFX_ARENA_PATH/meta/ofx-extra-license.txt || exit 1
echo "" >> $OFX_ARENA_PATH/meta/ofx-extra-license.txt || exit 1
cat $INSTALL_PATH/docs/openfx-arena/LICENSE >> $OFX_ARENA_PATH/meta/ofx-extra-license.txt || exit 1
cp -av $INSTALL_PATH/Plugins/Arena.ofx.bundle $OFX_ARENA_PATH/data/Plugins/ || exit 1
strip -s $OFX_ARENA_PATH/data/Plugins/*/*/*/*
echo "ImageMagick License:" >> $OFX_ARENA_PATH/meta/ofx-extra-license.txt || exit 1
cat $INSTALL_PATH/docs/imagemagick/LICENSE >> $OFX_ARENA_PATH/meta/ofx-extra-license.txt || exit 1
echo "LCMS License:" >>$OFX_ARENA_PATH/meta/ofx-extra-license.txt || exit 1
cat $INSTALL_PATH/docs/lcms/COPYING >>$OFX_ARENA_PATH/meta/ofx-extra-license.txt || exit 1

ARENA_LIBS=$OFX_ARENA_PATH/data/Plugins/Arena.ofx.bundle/Libraries
mkdir -p $ARENA_LIBS || exit 1
OFX_ARENA_DEPENDS=$(ldd $OFX_ARENA_PATH/data/Plugins/*/*/*/*|grep opt | awk '{print $3}')
for x in $OFX_ARENA_DEPENDS; do
  cp -v $x $ARENA_LIBS/ || exit 1
done
strip -s $ARENA_LIBS/*
rm -rf $ARENA_LIBS/libcairo*

# OFX CV
OFX_CV_VERSION=$TAG
OFX_CV_PATH=$INSTALLER/packages/$CVPLUG_PKG
mkdir -p $OFX_CV_PATH/{data,meta} $OFX_CV_PATH/data/Plugins $OFX_CV_PATH/data/docs/openfx-opencv || exit 1
cat $XML/openfx-opencv.xml | sed "s/_VERSION_/${OFX_CV_VERSION}/;s/_DATE_/${DATE}/" > $OFX_CV_PATH/meta/package.xml || exit 1
cat $QS/openfx-opencv.qs > $OFX_CV_PATH/meta/installscript.qs || exit 1
cp -a $INSTALL_PATH/docs/openfx-opencv $OFX_CV_PATH/data/docs/ || exit 1
cat $OFX_CV_PATH/data/docs/openfx-opencv/README > $OFX_CV_PATH/meta/license.txt || exit 1
cp -a $INSTALL_PATH/Plugins/{inpaint,segment}.ofx.bundle $OFX_CV_PATH/data/Plugins/ || exit 1
strip -s $OFX_CV_PATH/data/Plugins/*/*/*/*

mkdir -p $OFX_CV_PATH/data/lib || exit 1
OFX_CV_DEPENDS=$(ldd $OFX_CV_PATH/data/Plugins/*/*/*/*|grep opt | awk '{print $3}')
for x in $OFX_CV_DEPENDS; do
  cp -v $x $OFX_CV_PATH/data/lib/ || exit 1
done
strip -s $OFX_CV_PATH/data/lib/*
cp -a $INSTALL_PATH/docs/opencv $OFX_CV_PATH/data/docs/ || exit 1
cat $INSTALL_PATH/docs/opencv/LICENSE >> $OFX_CV_PATH/meta/ofx-cv-license.txt || exit 1

mkdir -p $OFX_CV_PATH/data/Plugins/inpaint.ofx.bundle/Libraries || exit 1
mv $OFX_CV_PATH/data/lib/* $OFX_CV_PATH/data/Plugins/inpaint.ofx.bundle/Libraries/ || exit 1
(cd $OFX_CV_PATH/data/Plugins/segment.ofx.bundle; ln -sf ../inpaint.ofx.bundle/Libraries .)
rm -rf $OFX_CV_PATH/data/lib || exit 1

# Clean and perms
chown root:root -R $INSTALLER/*
(cd $INSTALLER; find . -type d -name .git -exec rm -rf {} \;)

# Build repo and package
if [ "$NO_INSTALLER" != "1" ]; then
  if [ "$1" == "workshop" ]; then
    ONLINE_TAG=snapshot
  else
    ONLINE_TAG=release
  fi

  ONLINE_INSTALL=Natron-${PKGOS}-online-install-$ONLINE_TAG
  BUNDLED_INSTALL=Natron-$NATRON_VERSION-${PKGOS}

  REPO_DIR=$REPO_DIR_PREFIX$ONLINE_TAG
  rm -rf $REPO_DIR

  mkdir -p $REPO_DIR/packages || exit 1

  $INSTALL_PATH/bin/repogen -v --update-new-components -p $INSTALLER/packages -c $INSTALLER/config/config.xml $REPO_DIR/packages || exit 1

  mkdir -p $REPO_DIR/installers || exit 1

  if [ "$OFFLINE" != "0" ]; then
    $INSTALL_PATH/bin/binarycreator -v -f -p $INSTALLER/packages -c $INSTALLER/config/config.xml -i $PACKAGES $REPO_DIR/installers/$BUNDLED_INSTALL || exit 1 
    cd $REPO_DIR/installers || exit 1
    tar cvvzf $BUNDLED_INSTALL.tgz $BUNDLED_INSTALL || exit 1
    ln -sf $BUNDLED_INSTALL.tgz Natron-latest-$PKGOS-$ONLINE_TAG.tgz || exit 1
  fi

  $INSTALL_PATH/bin/binarycreator -v -n -p $INSTALLER/packages -c $INSTALLER/config/config.xml $ONLINE_INSTALL || exit 1
  tar cvvzf $ONLINE_INSTALL.tgz $ONLINE_INSTALL || exit 1
fi

rm $REPO_DIR/installers/$ONLINE_INSTALL $REPO_DIR/installers/$BUNDLED_INSTALL

echo "All Done!!!"
