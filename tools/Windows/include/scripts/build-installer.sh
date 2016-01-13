#!/bin/sh
#
# Build packages and installer for Windows
#
# Options:
# NATRON_LICENSE=(GPL,COMMERCIAL)
# BUILD_CONFIG=(SNAPSHOT,ALPHA,BETA,RC,STABLE,CUSTOM)
# CUSTOM_BUILD_USER_NAME="Toto" : to be set if BUILD_CONFIG=CUSTOM
# BUILD_NUMBER=X: To be set to indicate the revision number of the build. For example RC1,RC2, RC3 etc...
# NO_ZIP=1: Do not produce a zip self-contained archive with Natron distribution.
# OFFLINE=1 : Make the offline installer too
# NO_INSTALLER: Do not make any installer, only zip if NO_ZIP!=1
# Usage:
# NO_ZIP=1 NATRON_LICENSE=GPL OFFLINE=1 BUILD_CONFIG=BETA BUILD_NUMBER=1 sh build-installer 64 workshop

source `pwd`/common.sh || exit 1
source `pwd`/commits-hash.sh || exit 1


if [ "$1" = "32" ]; then
    BIT=32
    INSTALL_PATH=$INSTALL32_PATH
else
    BIT=64
    INSTALL_PATH=$INSTALL64_PATH
fi

if [ "$NATRON_LICENSE" != "GPL" ] && [ "$NATRON_LICENSE" != "COMMERCIAL" ]; then
    echo "Please select a License with NATRON_LICENSE=(GPL,COMMERCIAL)"
    exit 1
fi
if [ "$NATRON_LICENSE" = "GPL" ]; then
    FFMPEG_BIN_PATH=$INSTALL_PATH/ffmpeg-GPL
elif [ "$NATRON_LICENSE" = "COMMERCIAL" ]; then
    FFMPEG_BIN_PATH=$INSTALL_PATH/ffmpeg-LGPL
fi

if [ -d "$INSTALL_PATH/symbols" ]; then
  rm -rf $INSTALL_PATH/symbols || exit 1
fi
mkdir -p $INSTALL_PATH/symbols || exit 1


if [ "$BUILD_CONFIG" = "ALPHA" ]; then
	if [ -z "$BUILD_NUMBER" ]; then
		echo "You must supply a BUILD_NUMBER when BUILD_CONFIG=ALPHA"
		exit 1
	fi
	NATRON_VERSION=$NATRON_VERSION_NUMBER-alpha-$BUILD_NUMBER
elif [ "$BUILD_CONFIG" = "BETA" ]; then
	if [ -z "$BUILD_NUMBER" ]; then
		echo "You must supply a BUILD_NUMBER when BUILD_CONFIG=BETA"
		exit 1
	fi
	NATRON_VERSION=$NATRON_VERSION_NUMBER-beta-$BUILD_NUMBER
elif [ "$BUILD_CONFIG" = "RC" ]; then
	if [ -z "$BUILD_NUMBER" ]; then
		echo "You must supply a BUILD_NUMBER when BUILD_CONFIG=RC"
		exit 1
	fi
	NATRON_VERSION=$NATRON_VERSION_NUMBER-RC$BUILD_NUMBER
elif [ "$BUILD_CONFIG" = "STABLE" ]; then
	NATRON_VERSION=$NATRON_VERSION_NUMBER-stable
elif [ "$BUILD_CONFIG" = "CUSTOM" ]; then
	if [ -z "$CUSTOM_BUILD_USER_NAME" ]; then
		echo "You must supply a CUSTOM_BUILD_USER_NAME when BUILD_CONFIG=CUSTOM"
		exit 1
	fi
	NATRON_VERSION="$CUSTOM_BUILD_USER_NAME"
fi

if [ "$BUILD_CONFIG" = "SNAPSHOT" ]; then
    NATRON_VERSION=$NATRON_DEVEL_GIT
    REPO_BRANCH=snapshots
    APP_INSTALL_SUFFIX=INRIA/Natron-snapshots
	ONLINE_TAG=snapshot
else
    REPO_BRANCH=releases
    APP_INSTALL_SUFFIX=INRIA/Natron-$NATRON_VERSION
	ONLINE_TAG=release
fi

REPO_DIR=$REPO_DIR_PREFIX$ONLINE_TAG$BIT
ARCHIVE_DIR=$REPO_DIR/archive
ARCHIVE_DATA_DIR=$ARCHIVE_DIR/data
rm -rf $REPO_DIR/packages $REPO_DIR/installers $ARCHIVE_DATA_DIR
mkdir -p $REPO_DIR/packages || exit 1
mkdir -p $REPO_DIR/installers || exit 1
mkdir -p $ARCHIVE_DATA_DIR || exit 1


DATE=`date +%Y-%m-%d`
PKGOS="Windows-x86_${BIT}bit"
REPO_OS="Windows/$REPO_BRANCH/${BIT}bit/packages"

TMP_BUILD_DIR=$TMP_PATH$BIT

if [ -d $TMP_BUILD_DIR ]; then
    rm -rf $TMP_BUILD_DIR || exit 1
fi
mkdir -p $TMP_BUILD_DIR || exit 1



# SETUP
INSTALLER="$TMP_BUILD_DIR/Natron-installer"
XML="$INC_PATH/xml"
QS="$INC_PATH/qs"

mkdir -p "$INSTALLER/config" "$INSTALLER/packages" || exit 1
cat "$INC_PATH/config/config.xml" | sed -e "s/_VERSION_/${NATRON_VERSION_NUMBER}/;s#_OS_BRANCH_BIT_#${REPO_OS}#g;s#_URL_#${REPO_URL}#g;s#_APP_INSTALL_SUFFIX_#${APP_INSTALL_SUFFIX}#g" > "$INSTALLER/config/config.xml" || exit 1
cp "$INC_PATH/config/"*.png "$INSTALLER/config/" || exit 1

# OFX IO
if [ "$BUNDLE_IO" = "1" ]; then         
    IO_DLL="LIBICUDT55.DLL LIBICUUC55.DLL LIBLCMS2-2.DLL LIBJASPER-1.DLL LIBLZMA-5.DLL LIBOPENJPEG-5.DLL LIBHALF-2_2.DLL LIBILMIMF-2_2.DLL LIBIEX-2_2.DLL LIBILMTHREAD-2_2.DLL LIBIMATH-2_2.DLL LIBOPENIMAGEIO.DLL LIBRAW_R-10.DLL LIBWEBP-5.DLL LIBBOOST_THREAD-MT.DLL LIBBOOST_SYSTEM-MT.DLL LIBBOOST_REGEX-MT.DLL LIBBOOST_FILESYSTEM-MT.DLL"
    OFX_IO_VERSION="$TAG"
    OFX_IO_PATH="$INSTALLER/packages/$IOPLUG_PKG"
    mkdir -p "$OFX_IO_PATH/data" "$OFX_IO_PATH/meta" "$OFX_IO_PATH/data/Plugins/OFX/Natron" || exit 1
    cat "$XML/openfx-io.xml" | sed -e "s/_VERSION_/${OFX_IO_VERSION}/;s/_DATE_/${DATE}/" > "$OFX_IO_PATH/meta/package.xml" || exit 1
    cat "$QS/openfx-io.qs" > "$OFX_IO_PATH/meta/installscript.qs" || exit 1
    cat "$INSTALL_PATH/docs/openfx-io/VERSION" > "$OFX_IO_PATH/meta/ofx-io-license.txt" || exit 1
    echo "" >> "$OFX_IO_PATH/meta/ofx-io-license.txt" || exit 1
    cat "$INSTALL_PATH/docs/openfx-io/LICENSE" >> "$OFX_IO_PATH/meta/ofx-io-license.txt" || exit 1
    cp -a "$INSTALL_PATH/Plugins/IO.ofx.bundle" "$OFX_IO_PATH/data/Plugins/OFX/Natron/" || exit 1
    for depend in $IO_DLL; do
        cp "$INSTALL_PATH/bin/$depend" "$OFX_IO_PATH/data/Plugins/OFX/Natron/IO.ofx.bundle/Contents/Win$BIT/" || exit 1
    done
    cp $INSTALL_PATH/lib/{LIBOPENCOLORIO.DLL,LIBSEEXPR.DLL} "$OFX_IO_PATH/data/Plugins/OFX/Natron/IO.ofx.bundle/Contents/Win$BIT/" || exit 1
    if [ "${BREAKPAD}" != "0" ]; then
      $INSTALL_PATH/bin/dump_syms $OFX_IO_PATH/data/Plugins/OFX/Natron/*/*/*/IO.ofx > $INSTALL_PATH/symbols/IO.ofx-${TAG}-${PKGOS}.sym || exit 1
    fi
    strip -s $OFX_IO_PATH/data/Plugins/OFX/Natron/*/*/*/*
	
fi

# OFX MISC
if [ "$BUNDLE_MISC" = "1" ]; then 
    CIMG_DLL="LIBGOMP-1.DLL"
    OFX_MISC_VERSION=$TAG
    OFX_MISC_PATH=$INSTALLER/packages/$MISCPLUG_PKG
    mkdir -p $OFX_MISC_PATH/data $OFX_MISC_PATH/meta $OFX_MISC_PATH/data/Plugins/OFX/Natron || exit 1
    cat $XML/openfx-misc.xml | sed "s/_VERSION_/${OFX_MISC_VERSION}/;s/_DATE_/${DATE}/" > $OFX_MISC_PATH/meta/package.xml || exit 1
    cat $QS/openfx-misc.qs > $OFX_MISC_PATH/meta/installscript.qs || exit 1
    cat $INSTALL_PATH/docs/openfx-misc/VERSION > $OFX_MISC_PATH/meta/ofx-misc-license.txt || exit 1
    echo "" >> $OFX_MISC_PATH/meta/ofx-misc-license.txt || exit 1
    cat $INSTALL_PATH/docs/openfx-misc/LICENSE >> $OFX_MISC_PATH/meta/ofx-misc-license.txt || exit 1
    cp -a $INSTALL_PATH/Plugins/{CImg,Misc}.ofx.bundle $OFX_MISC_PATH/data/Plugins/OFX/Natron/ || exit 1
    
    for depend in $CIMG_DLL; do
        cp $INSTALL_PATH/bin/$depend  $OFX_MISC_PATH/data/Plugins/OFX/Natron/CImg.ofx.bundle/Contents/Win$BIT/ || exit 1
    done
    if [ "${BREAKPAD}" != "0" ]; then
      $INSTALL_PATH/bin/dump_syms $OFX_MISC_PATH/data/Plugins/OFX/Natron/*/*/*/CImg.ofx > $INSTALL_PATH/symbols/CImg.ofx-${TAG}-${PKGOS}.sym || exit 1
      $INSTALL_PATH/bin/dump_syms $OFX_MISC_PATH/data/Plugins/OFX/Natron/*/*/*/Misc.ofx > $INSTALL_PATH/symbols/Misc.ofx-${TAG}-${PKGOS}.sym || exit 1
    fi
    strip -s $OFX_MISC_PATH/data/Plugins/OFX/Natron/*/*/*/*
	
fi

# NATRON
NATRON_PATH=$INSTALLER/packages/$NATRON_PKG
mkdir -p $NATRON_PATH/meta $NATRON_PATH/data/docs $NATRON_PATH/data/bin $NATRON_PATH/data/share $NATRON_PATH/data/Plugins/PyPlugs || exit 1
cat $XML/natron.xml | sed "s/_VERSION_/${TAG}/;s/_DATE_/${DATE}/" > $NATRON_PATH/meta/package.xml || exit 1
cat $QS/natron.qs > $NATRON_PATH/meta/installscript.qs || exit 1
cp -a $INSTALL_PATH/docs/natron/* $NATRON_PATH/data/docs/ || exit 1
cat $INSTALL_PATH/docs/natron/LICENSE.txt > $NATRON_PATH/meta/natron-license.txt || exit 1
cp $INSTALL_PATH/bin/Natron.exe $NATRON_PATH/data/bin/ || exit 1
cp $INSTALL_PATH/bin/NatronRenderer.exe $NATRON_PATH/data/bin/ || exit 1
if [ -f "$INSTALL_PATH/bin/NatronCrashReporter.exe" ]; then
    cp $INSTALL_PATH/bin/NatronCrashReporter.exe $NATRON_PATH/data/bin/
    cp $INSTALL_PATH/bin/NatronRendererCrashReporter.exe $NATRON_PATH/data/bin/
fi
strip -s $NATRON_PATH/data/bin/Natron*
wget --no-check-certificate $NATRON_API_DOC || exit 1
mv natron.pdf $NATRON_PATH/data/docs/Natron_Python_API_Reference.pdf || exit 1
rm $NATRON_PATH/data/docs/TuttleOFX-README.txt || exit 1
mkdir -p $NATRON_PATH/data/share/pixmaps || exit 1
cp $CWD/include/config/natronProjectIcon_windows.ico $NATRON_PATH/data/share/pixmaps/ || exit 1
cp $INSTALL_PATH/share/stylesheets/mainstyle.qss $NATRON_PATH/data/share/ || exit 1
if [ "${BREAKPAD}" != "0" ]; then
  $INSTALL_PATH/bin/dump_syms $NATRON_PATH/data/bin/Natron.exe > $INSTALL_PATH/symbols/Natron-${TAG}-${PKGOS}.sym || exit 1
  $INSTALL_PATH/bin/dump_syms $NATRON_PATH/data/bin/NatronRenderer.exe > $INSTALL_PATH/symbols/NatronRenderer-${TAG}-${PKGOS}.sym || exit 1
fi
strip -s $NATRON_PATH/data/bin/*

if [ "$NO_ZIP" != "1" ]; then
	mkdir -p $ARCHIVE_DATA_DIR/bin
	cp -r $NATRON_PATH/data/bin/* $ARCHIVE_DATA_DIR/bin || exit 1
fi

# OCIO
OCIO_VERSION=$COLOR_PROFILES_VERSION
OCIO_PATH=$INSTALLER/packages/$PROFILES_PKG
mkdir -p $OCIO_PATH/meta $OCIO_PATH/data/Resources || exit 1
cat $XML/ocio.xml | sed "s/_VERSION_/${OCIO_VERSION}/;s/_DATE_/${DATE}/" > $OCIO_PATH/meta/package.xml || exit 1
cat $QS/ocio.qs > $OCIO_PATH/meta/installscript.qs || exit 1
cp -a $INSTALL_PATH/share/OpenColorIO-Configs $OCIO_PATH/data/Resources/ || exit 1

if [ "$NO_ZIP" != "1" ]; then
	mkdir -p $ARCHIVE_DATA_DIR/Resources
	cp -r $OCIO_PATH/data/Resources/* $ARCHIVE_DATA_DIR/Resources || exit 1
fi

# CORE LIBS
CLIBS_VERSION=$NATRON_VERSION_NUMBER
CLIBS_PATH=$INSTALLER/packages/$CORELIBS_PKG
mkdir -p $CLIBS_PATH/meta $CLIBS_PATH/data/bin $CLIBS_PATH/data/lib $CLIBS_PATH/data/share/pixmaps $CLIBS_PATH/data/Resources/etc/fonts || exit 1
cat $XML/corelibs.xml | sed "s/_VERSION_/${CLIBS_VERSION}/;s/_DATE_/${DATE}/" > $CLIBS_PATH/meta/package.xml || exit 1
cat $QS/corelibs.qs > $CLIBS_PATH/meta/installscript.qs || exit 1

cp -a $INSTALL_PATH/etc/fonts/* $CLIBS_PATH/data/Resources/etc/fonts || exit 1
cp -a $INSTALL_PATH/share/qt4/plugins/* $CLIBS_PATH/data/bin/ || exit 1
rm -f $CLIBS_PATH/data/bin/*/*d4.dll
NATRON_DLL="LIBEAY32.DLL SSLEAY32.DLL LIBGIF-7.DLL LIBSQLITE3-0.DLL LIBJPEG-8.DLL LIBMNG-2.DLL LIBTIFF-5.DLL LIBFFI-6.DLL LIBICONV-2.DLL LIBINTL-8.DLL GLEW32.DLL LIBGLIB-2.0-0.DLL LIBWINPTHREAD-1.DLL LIBSTDC++-6.DLL LIBBOOST_SERIALIZATION-MT.DLL LIBCAIRO-2.DLL LIBFREETYPE-6.DLL LIBBZ2-1.DLL LIBHARFBUZZ-0.DLL LIBPIXMAN-1-0.DLL LIBPNG16-16.DLL ZLIB1.DLL LIBEXPAT-1.DLL LIBFONTCONFIG-1.DLL LIBPYSIDE-PYTHON2.7.DLL LIBPYTHON2.7.DLL QTCORE4.DLL QTGUI4.DLL QTNETWORK4.DLL QTOPENGL4.DLL LIBSHIBOKEN-PYTHON2.7.DLL"
if [ "$BIT" = "32" ]; then
    GCC_DLL="LIBGCC_S_DW2-1.DLL"
else
    GCC_DLL="LIBGCC_S_SEH-1.DLL"
fi
NATRON_DLL="$NATRON_DLL $GCC_DLL"

for depend in $NATRON_DLL; do
    cp $INSTALL_PATH/bin/$depend $CLIBS_PATH/data/bin/ || exit 1
done

#Copy Qt dlls (required for all PySide modules to work correctly)
cp $INSTALL_PATH/bin/Qt*4.dll $CLIBS_PATH/data/bin/ || exit 1

#Ignore debug dlls of Qt
rm $CLIBS_PATH/data/bin/*d4.dll
rm $CLIBS_PATH/data/bin/sqldrivers/{*mysql*,*psql*}

#Copy ffmpeg binaries
FFMPEG_DLLS="SWSCALE-3.DLL  AVCODEC-56.DLL SWRESAMPLE-1.DLL AVFORMAT-56.DLL AVUTIL-54.DLL AVDEVICE-56.DLL AVFILTER-5.DLL AVRESAMPLE-2.DLL POSTPROC-53.DLL"
for depend in $FFMPEG_DLLS; do 
    cp $FFMPEG_BIN_PATH/bin/$depend $CLIBS_PATH/data/bin || exit 1
done
#Also embbed ffmpeg.exe and ffprobe.exe
#cp $FFMPEG_BIN_PATH/bin/ffmpeg.exe $CLIBS_PATH/data/bin || exit 1
#cp $FFMPEG_BIN_PATH/bin/ffprobe.exe $CLIBS_PATH/data/bin || exit 1

strip -s $CLIBS_PATH/data/bin/*
strip -s $CLIBS_PATH/data/bin/*/*

CORE_DOC=$CLIBS_PATH
echo "" >> $CORE_DOC/meta/3rdparty-license.txt 
cat $CWD/include/natron/3rdparty.txt >> $CORE_DOC/meta/3rdparty-license.txt || exit 1


#Copy Python distrib
mkdir -p $CLIBS_PATH/data/Plugins || exit 1
if [ "$PYV" = "3" ]; then
    cp -a $INSTALL_PATH/lib/python3.4 $CLIBS_PATH/data/lib/ || exit 1
    mv $CLIBS_PATH/data/lib/python3.4/site-packages/PySide $CLIBS_PATH/data/Plugins/ || exit 1
    rm -rf $CLIBS_PATH/data/lib/python3.4/{test,config-3.4m} || exit 1
else
    cp -a $INSTALL_PATH/lib/python2.7 $CLIBS_PATH/data/lib/ || exit 1
    mv $CLIBS_PATH/data/lib/python2.7/site-packages/PySide $CLIBS_PATH/data/Plugins/ || exit 1
    rm -rf $CLIBS_PATH/data/lib/python2.7/{test,config} || exit 1
fi

(cd $CLIBS_PATH ; find . -type d -name __pycache__ -exec rm -rf {} \;)
strip -s $CLIBS_PATH/data/Plugins/PySide/*
strip -s $CLIBS_PATH/data/lib/python*/* 
strip -s $CLIBS_PATH/data/lib/python*/*/*

if [ "$NO_ZIP" != "1" ]; then
	mkdir -p $ARCHIVE_DATA_DIR/Resources
	mkdir -p $ARCHIVE_DATA_DIR/lib
	mkdir -p $ARCHIVE_DATA_DIR/bin
	mkdir -p $ARCHIVE_DATA_DIR/Plugins
	cp -r $CLIBS_PATH/data/Resources/* $ARCHIVE_DATA_DIR/Resources || exit 1
	cp -r $CLIBS_PATH/data/lib/* $ARCHIVE_DATA_DIR/lib || exit 1
	cp -r $CLIBS_PATH/data/bin/* $ARCHIVE_DATA_DIR/bin || exit 1
	cp -r $CLIBS_PATH/data/Plugins/* $ARCHIVE_DATA_DIR/Plugins || exit 1
fi

# OFX ARENA
if [ "$BUNDLE_ARENA" = "1" ]; then 
    ARENA_DLL="LIBZIP-4.DLL LIBCROCO-0.6-3.DLL LIBGOMP-1.DLL LIBGMODULE-2.0-0.DLL LIBGDK_PIXBUF-2.0-0.DLL LIBGOBJECT-2.0-0.DLL LIBGIO-2.0-0.DLL LIBLCMS2-2.DLL LIBPANGO-1.0-0.DLL LIBPANGOCAIRO-1.0-0.DLL LIBPANGOWIN32-1.0-0.DLL LIBPANGOFT2-1.0-0.DLL LIBRSVG-2-2.DLL LIBXML2-2.DLL"
    OFX_ARENA_VERSION=$TAG
    OFX_ARENA_PATH=$INSTALLER/packages/$ARENAPLUG_PKG
    mkdir -p $OFX_ARENA_PATH/meta $OFX_ARENA_PATH/data/Plugins/OFX/Natron || exit 1
    cat $XML/openfx-arena.xml | sed "s/_VERSION_/${OFX_ARENA_VERSION}/;s/_DATE_/${DATE}/" > $OFX_ARENA_PATH/meta/package.xml || exit 1
    cat $QS/openfx-arena.qs > $OFX_ARENA_PATH/meta/installscript.qs || exit 1
    cat $INSTALL_PATH/docs/openfx-arena/VERSION > $OFX_ARENA_PATH/meta/ofx-extra-license.txt || exit 1
    echo "" >> $OFX_ARENA_PATH/meta/ofx-extra-license.txt || exit 1
    cat $INSTALL_PATH/docs/openfx-arena/LICENSE >> $OFX_ARENA_PATH/meta/ofx-extra-license.txt || exit 1
    cp -av $INSTALL_PATH/Plugins/Arena.ofx.bundle $OFX_ARENA_PATH/data/Plugins/OFX/Natron/ || exit 1
    for depend in $ARENA_DLL; do
        cp $INSTALL_PATH/bin/$depend  $OFX_ARENA_PATH/data/Plugins/OFX/Natron/Arena.ofx.bundle/Contents/Win$BIT/ || exit 1
    done
    cp $INSTALL_PATH/lib/LIBOPENCOLORIO.DLL $OFX_ARENA_PATH/data/Plugins/OFX/Natron/Arena.ofx.bundle/Contents/Win$BIT/ || exit 1
    if [ "${BREAKPAD}" != "0" ]; then
      $INSTALL_PATH/bin/dump_syms $OFX_ARENA_PATH/data/Plugins/OFX/Natron/*/*/*/Arena.ofx > $INSTALL_PATH/symbols/Arena.ofx-${TAG}-${PKGOS}.sym || exit 1
    fi
    strip -s $OFX_ARENA_PATH/data/Plugins/OFX/Natron/*/*/*/*
    echo "ImageMagick License:" >> $OFX_ARENA_PATH/meta/ofx-extra-license.txt || exit 1
    cat $INSTALL_PATH/docs/imagemagick/LICENSE >> $OFX_ARENA_PATH/meta/ofx-extra-license.txt || exit 1
	
	
fi

#echo "LCMS License:" >>$OFX_ARENA_PATH/meta/ofx-extra-license.txt || exit 1
#cat $INSTALL_PATH/docs/lcms/COPYING >>$OFX_ARENA_PATH/meta/ofx-extra-license.txt || exit 1

# OFX CV
if [ "$BUNDLE_CV" = "1" ]; then 
    CV_DLL="LIBOPENCV_CORE2411.DLL LIBOPENCV_IMGPROC2411.DLL LIBOPENCV_PHOTO2411.DLL"
    SEGMENT_DLL="LIBLZMA-5.DLL LIBOPENCV_FLANN2411.DLL LIBJASPER-1.DLL LIBOPENCV_CALIB3D2411.DLL LIBOPENCV_FEATURES2D2411.DLL LIBOPENCV_HIGHGUI2411.DLL LIBOPENCV_ML2411.DLL LIBOPENCV_VIDEO2411.DLL LIBOPENCV_LEGACY2411.DLL"
    OFX_CV_VERSION=$TAG
    OFX_CV_PATH=$INSTALLER/packages/$CVPLUG_PKG
    mkdir -p $OFX_CV_PATH/{data,meta} $OFX_CV_PATH/data/Plugins/OFX/Natron $OFX_CV_PATH/data/docs/openfx-opencv || exit 1
    cat $XML/openfx-opencv.xml | sed "s/_VERSION_/${OFX_CV_VERSION}/;s/_DATE_/${DATE}/" > $OFX_CV_PATH/meta/package.xml || exit 1
    cat $QS/openfx-opencv.qs > $OFX_CV_PATH/meta/installscript.qs || exit 1
    cp -a $INSTALL_PATH/docs/openfx-opencv $OFX_CV_PATH/data/docs/ || exit 1
    cat $OFX_CV_PATH/data/docs/openfx-opencv/README > $OFX_CV_PATH/meta/ofx-cv-license.txt || exit 1
    cp -a $INSTALL_PATH/Plugins/{inpaint,segment}.ofx.bundle $OFX_CV_PATH/data/Plugins/OFX/Natron/ || exit 1
    for depend in $CV_DLL; do
        cp -v $INSTALL_PATH/bin/$depend $OFX_CV_PATH/data/Plugins/OFX/Natron/inpaint.ofx.bundle/Contents/Win$BIT/ || exit 1
        cp -v $INSTALL_PATH/bin/$depend $OFX_CV_PATH/data/Plugins/OFX/Natron/segment.ofx.bundle/Contents/Win$BIT/ || exit 1
    done
    for depend in $SEGMENT_DLL; do
        cp -v $INSTALL_PATH/bin/$depend $OFX_CV_PATH/data/Plugins/OFX/Natron/segment.ofx.bundle/Contents/Win$BIT/ || exit 1
    done
    strip -s $OFX_CV_PATH/data/Plugins/OFX/Natron/*/*/*/*
    
    #$BUNDLE_CV
fi

# manifests

if [ "$BUNDLE_MISC" = "1" ]; then 
    CIMG_MANIFEST=$OFX_MISC_PATH/data/Plugins/OFX/Natron/CImg.ofx.bundle/Contents/Win$BIT/manifest
    echo "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" > $CIMG_MANIFEST
    echo "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">" >> $CIMG_MANIFEST
    echo "<assemblyIdentity name=\"CImg.ofx\" version=\"1.0.0.0\" type=\"win32\" processorArchitecture=\"amd64\"/>" >> $CIMG_MANIFEST

    for depend in $CIMG_DLL; do
        echo "<file name=\"${depend}\"></file>" >> $CIMG_MANIFEST || exit 1
    done
    echo "</assembly>" >> $CIMG_MANIFEST || exit 1
    cd $OFX_MISC_PATH/data/Plugins/OFX/Natron/CImg.ofx.bundle/Contents/Win$BIT || exit 1
    mt -manifest manifest -outputresource:"CImg.ofx;2"
	
	if [ "$NO_ZIP" != "1" ]; then
		mkdir -p $ARCHIVE_DATA_DIR/Plugins
		cp -r $OFX_MISC_PATH/data/Plugins/OFX/Natron/* $ARCHIVE_DATA_DIR/Plugins || exit 1
	fi
fi

if [ "$BUNDLE_IO" = "1" ]; then 
    IO_MANIFEST=$OFX_IO_PATH/data/Plugins/OFX/Natron/IO.ofx.bundle/Contents/Win$BIT/manifest
    echo "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" > $IO_MANIFEST
    echo "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">" >> $IO_MANIFEST
    echo "<assemblyIdentity name=\"IO.ofx\" version=\"1.0.0.0\" type=\"win32\" processorArchitecture=\"amd64\"/>" >> $IO_MANIFEST
    for depend in $IO_DLL; do
        echo "<file name=\"${depend}\"></file>" >> $IO_MANIFEST || exit 1
    done
    echo "<file name=\"LIBOPENCOLORIO.DLL\"></file>" >> $IO_MANIFEST || exit 1
    echo "<file name=\"LIBSEEXPR.DLL\"></file>" >> $IO_MANIFEST || exit 1
    echo "</assembly>" >> $IO_MANIFEST || exit 1
    cd $OFX_IO_PATH/data/Plugins/OFX/Natron/IO.ofx.bundle/Contents/Win$BIT || exit 1
    mt -manifest manifest -outputresource:"IO.ofx;2"
	
	if [ "$NO_ZIP" != "1" ]; then
		mkdir -p $ARCHIVE_DATA_DIR/Plugins
		cp -r $OFX_IO_PATH/data/Plugins/OFX/Natron/* $ARCHIVE_DATA_DIR/Plugins || exit 1
	fi
fi

if [ "$BUNDLE_ARENA" = "1" ]; then 
    ARENA_MANIFEST=$OFX_ARENA_PATH/data/Plugins/OFX/Natron/Arena.ofx.bundle/Contents/Win$BIT/manifest
    
    echo "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" > $ARENA_MANIFEST
    echo "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">" >> $ARENA_MANIFEST
    echo "<assemblyIdentity name=\"Arena.ofx\" version=\"1.0.0.0\" type=\"win32\" processorArchitecture=\"amd64\"/>" >> $ARENA_MANIFEST
    for depend in $ARENA_DLL; do
        echo "<file name=\"${depend}\"></file>" >> $ARENA_MANIFEST || exit 1
    done
    echo "<file name=\"LIBOPENCOLORIO.DLL\"></file>" >> $ARENA_MANIFEST || exit 1
    echo "</assembly>" >> $ARENA_MANIFEST || exit 1
    cd $OFX_ARENA_PATH/data/Plugins/OFX/Natron/Arena.ofx.bundle/Contents/Win$BIT || exit 1
    mt -manifest manifest -outputresource:"Arena.ofx;2"
	
	if [ "$NO_ZIP" != "1" ]; then
		mkdir -p $ARCHIVE_DATA_DIR/Plugins
		cp -r $OFX_ARENA_PATH/data/Plugins/OFX/Natron/* $ARCHIVE_DATA_DIR/Plugins || exit 1
	fi
fi

if [ "$BUNDLE_CV" = "1" ]; then 
    INPAINT_MANIFEST=$OFX_CV_PATH/data/Plugins/OFX/Natron/inpaint.ofx.bundle/Contents/Win$BIT/manifest
    echo "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" > $INPAINT_MANIFEST
    echo "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">" >> $INPAINT_MANIFEST
    echo "<assemblyIdentity name=\"inpaint.ofx\" version=\"1.0.0.0\" type=\"win32\" processorArchitecture=\"amd64\"/>" >> $INPAINT_MANIFEST
    
    for depend in $CV_DLL; do
        echo "<file name=\"${depend}\"></file>" >> $INPAINT_MANIFEST || exit 1
    done
    echo "</assembly>" >> $INPAINT_MANIFEST || exit 1
    cd $OFX_CV_PATH/data/Plugins/OFX/Natron/inpaint.ofx.bundle/Contents/Win$BIT || exit 1
    mt -manifest manifest -outputresource:"inpaint.ofx;2"

    SEGMENT_MANIFEST=$OFX_CV_PATH/data/Plugins/OFX/Natron/segment.ofx.bundle/Contents/Win$BIT/manifest
    echo "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" > $SEGMENT_MANIFEST
    echo "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">" >> $SEGMENT_MANIFEST
    echo "<assemblyIdentity name=\"segment.ofx\" version=\"1.0.0.0\" type=\"win32\" processorArchitecture=\"amd64\"/>" >> $SEGMENT_MANIFEST
    
    for depend in $CV_DLL; do
        echo "<file name=\"${depend}\"></file>" >> $SEGMENT_MANIFEST || exit 1
    done
    for depend in $SEGMENT_DLL; do
        echo "<file name=\"${depend}\"></file>" >> $SEGMENT_MANIFEST || exit 1
    done
    echo "</assembly>" >> $SEGMENT_MANIFEST || exit 1
    cd $OFX_CV_PATH/data/Plugins/OFX/Natron/segment.ofx.bundle/Contents/Win$BIT || exit 1
    mt -manifest manifest -outputresource:"segment.ofx;2"
	
	if [ "$NO_ZIP" != "1" ]; then
		mkdir -p $ARCHIVE_DATA_DIR/Plugins
		cp -r $OFX_CV_PATH/data/Plugins/OFX/Natron/* $ARCHIVE_DATA_DIR/Plugins || exit 1
	fi
fi

# Clean and perms
(cd $INSTALLER; find . -type d -name .git -exec rm -rf {} \;)

# Build repo and package
if [ "$NO_INSTALLER" != "1" ]; then
    

    ONLINE_INSTALL=Natron-${PKGOS}-online-$ONLINE_TAG-setup.exe
    BUNDLED_INSTALL=Natron-$NATRON_VERSION-${PKGOS}-setup.exe
	ZIP_ARCHIVE_BASE=Natron-$NATRON_VERSION-${PKGOS}-no-installer

    $INSTALL_PATH/bin/repogen -v --update-new-components -p $INSTALLER/packages -c $INSTALLER/config/config.xml $REPO_DIR/packages || exit 1

    if [ "$OFFLINE" == "1" ]; then
        $INSTALL_PATH/bin/binarycreator -v -f -p $INSTALLER/packages -c $INSTALLER/config/config.xml -i $PACKAGES $REPO_DIR/installers/$BUNDLED_INSTALL || exit 1 
    fi
    cd $REPO_DIR/installers || exit 1
    $INSTALL_PATH/bin/binarycreator -v -n -p $INSTALLER/packages -c $INSTALLER/config/config.xml $ONLINE_INSTALL || exit 1
fi

if [ "$NO_ZIP" != "1" ]; then
	mkdir -p $REPO_DIR/
    mv $ARCHIVE_DATA_DIR $ARCHIVE_DIR/${ZIP_ARCHIVE_BASE} || exit 1
    (cd $ARCHIVE_DIR; zip -r ${ZIP_ARCHIVE_BASE}.zip ${ZIP_ARCHIVE_BASE} || exit 1; rm -rf ${ZIP_ARCHIVE_BASE};)
fi

echo "All Done!!!"
