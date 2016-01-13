#!/bin/sh

#options:
#TAR_SDK=1 : Make an archive of the SDK when building is done and store it in $SRC_PATH
#UPLOAD_SDK=1 : Upload the SDK tar archive to $REPO_DEST if TAR_SDK=1
#DOWNLOAD_INSTALLER=1: Force a download of the installer
#DOWNLOAD_FFMPEG_BIN:1: Force a download of the pre-built ffmpeg binaries (built against MXE for static depends)

source $(pwd)/common.sh || exit 1


if [ "$1" = "32" ]; then
    BIT=32
    INSTALL_PATH=$INSTALL32_PATH
    PKG_PREFIX=$PKG_PREFIX32
    FFMPEG_MXE_BIN_GPL=$FFMPEG_MXE_BIN_32_GPL_TAR
    FFMPEG_MXE_BIN_LGPL=$FFMPEG_MXE_BIN_32_LGPL_TAR
    INSTALLER_BIN_TAR=$INSTALLER32_BIN_TAR
elif [ "$1" = "64" ]; then
    BIT=64
    INSTALL_PATH=$INSTALL64_PATH
    PKG_PREFIX=$PKG_PREFIX64
    FFMPEG_MXE_BIN_GPL=$FFMPEG_MXE_BIN_64_GPL_TAR
    FFMPEG_MXE_BIN_LGPL=$FFMPEG_MXE_BIN_64_LGPL_TAR
    INSTALLER_BIN_TAR=$INSTALLER64_BIN_TAR
else
    echo "Usage build-sdk.sh <BIT>"
    exit 1
fi


TMP_BUILD_DIR=$TMP_PATH$BIT


BINARIES_URL=$REPO_DEST/Third_Party_Binaries
SDK=Windows-$OS-$BIT-SDK

if [ -z "$MKJOBS" ]; then
    #Default to 4 threads
    MKJOBS=$DEFAULT_MKJOBS
fi



echo
echo "Building $SDK using with $MKJOBS threads using gcc ${GCC_MAJOR}.${GCC_MINOR} ..."
echo
sleep 2

if [ ! -z "$REBUILD" ]; then
    if [ -d $INSTALL_PATH ]; then
        rm -rf $INSTALL_PATH || exit 1
    fi
fi
if [ -d $$TMP_BUILD_DIR ]; then
    rm -rf $$TMP_BUILD_DIR || exit 1
fi
mkdir -p $TMP_BUILD_DIR || exit 1
if [ ! -d $SRC_PATH ]; then
    mkdir -p $SRC_PATH || exit 1
fi

#Make sure GCC is installed
if [ ! -f ${INSTALL_PATH}/bin/gcc ]; then
    echo "Make sure to run include/scripts/setup-msys.sh first"
    exit 1
fi

# Setup env
export QTDIR=$INSTALL_PATH
export BOOST_ROOT=$INSTALL_PATH
export OPENJPEG_HOME=$INSTALL_PATH
export THIRD_PARTY_TOOLS_HOME=$INSTALL_PATH
export PYTHON_HOME=$INSTALL_PATH
export PYTHON_PATH=$INSTALL_PATH/lib/python2.7
export PYTHON_INCLUDE=$INSTALL_PATH/include/python2.7


# Install magick
if [ "$REBUILD_MAGICK" = "1" ]; then
    rm -rf $INSTALL_PATH/include/ImageMagick-6/ $INSTALL_PATH/lib/libMagick* $INSTALL_PATH/share/ImageMagick-6/ $INSTALL_PATH/lib/pkgconfig/{Image,Magick}*
fi
if [ ! -f $INSTALL_PATH/lib/pkgconfig/Magick++.pc ]; then
    cd $TMP_BUILD_DIR || exit 1
    if [ ! -f $SRC_PATH/$MAGICK_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$MAGICK_TAR -O $SRC_PATH/$MAGICK_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$MAGICK_TAR || exit 1
    cd ImageMagick-* || exit 1
    patch -p1 < $INC_PATH/patches/ImageMagick/mingw.patch || exit 1
    patch -p0 < $INC_PATH/patches/ImageMagick/pango-align-hack.diff || exit 1
    env CFLAGS="-DMAGICKCORE_EXCLUDE_DEPRECATED=1" CXXFLAGS="-I${INSTALL_PATH}/include -DMAGICKCORE_EXCLUDE_DEPRECATED=1" LDFLAGS="-lws2_32" ./configure --prefix=$INSTALL_PATH --with-magick-plus-plus=yes --with-quantum-depth=32 --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --with-lcms --without-openjp2 --without-lqr --without-lzma --without-openexr --with-pango --with-png --with-rsvg --without-tiff --without-webp --with-xml --without-zlib --without-bzlib --enable-static --disable-shared --enable-hdri --with-freetype --with-fontconfig --without-x --without-modules || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    mkdir -p $INSTALL_PATH/docs/imagemagick || exit 1
    cp LIC* COP* Copy* Lic* README AUTH* CONT* $INSTALL_PATH/docs/imagemagick/
fi


# Install ocio
if [ "$REBUILD_OCIO" = "1" ]; then
    rm -rf $INSTALL_PATH/lib/libOpenColorIO* rm -rf $INSTALL_PATH/share/ocio* $INSTALL_PATH/include/OpenColorIO*
fi
if [ ! -f $INSTALL_PATH/lib/libOpenColorIO.a ]; then
    cd $TMP_BUILD_DIR || exit 1
    if [ ! -f $SRC_PATH/$OCIO_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$OCIO_TAR -O $SRC_PATH/$OCIO_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$OCIO_TAR || exit 1
    cd OpenColorIO-* || exit 1
    OCIO_PATCHES=$CWD/include/patches/OpenColorIO
    patch -p1 -i ${OCIO_PATCHES}/mingw-w64.patch || exit 1
    patch -p1 -i ${OCIO_PATCHES}/fix-redefinitions.patch || exit 1
    patch -p1 -i ${OCIO_PATCHES}/detect-mingw-python.patch || exit 1

    mkdir build || exit 1
    cd build || exit 1
    cmake -G"MSYS Makefiles" -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH -DOCIO_BUILD_SHARED=ON -DOCIO_BUILD_STATIC=ON  -DOCIO_BUILD_PYGLUE=OFF -DOCIO_USE_BOOST_PTR=ON -DOCIO_BUILD_APPS=OFF .. || exit 1
    make || exit 1
    make install || exit 1
    mkdir -p $INSTALL_PATH/docs/ocio || exit 1
    cp ../LICENSE ../README $INSTALL_PATH/docs/ocio/ || exit 1
fi

# Install oiio
if [ "$REBUILD_OIIO" = "1" ]; then
    rm -rf $INSTALL_PATH/lib/libOpenImage* $INSTALL_PATH/include/OpenImage* $INSTALL_PATH/bin/libOpenImage*
fi
if [ ! -f $INSTALL_PATH/bin/libOpenImageIO.dll ]; then
    cd $TMP_BUILD_DIR || exit 1
    if [ ! -f $SRC_PATH/$OIIO_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$OIIO_TAR -O $SRC_PATH/$OIIO_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$OIIO_TAR || exit 1
    cd oiio-Release-* || exit 1
    OIIO_PATCHES=$CWD/include/patches/OpenImageIO
    patch -p1 -i ${OIIO_PATCHES}/fix-mingw-w64.patch  || exit 1
    patch -p0 -i ${OIIO_PATCHES}/fix-mingw-w64.diff || exit 1
    patch -p1 -i ${OIIO_PATCHES}/workaround-ansidecl-h-PTR-define-conflict.patch || exit 1
    patch -p1 -i ${OIIO_PATCHES}/0001-MinGW-w64-include-winbase-h-early-for-TCHAR-types.patch  || exit 1
    #patch -p1 -i ${OIIO_PATCHES}/0002-Also-link-to-opencv_videoio-library.patch  || exit 1
    mkdir build || exit 1
    cd build || exit 1
    cmake -G"MSYS Makefiles" -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH -DCMAKE_SHARED_LINKER_FLAGS=" -Wl,--export-all-symbols -Wl,--enable-auto-import " -DUSE_OPENSSL:BOOL=FALSE -DOPENEXR_HOME=$INSTALL_PATH -DILMBASE_HOME=$INSTALL_PATH -DTHIRD_PARTY_TOOLS_HOME=$INSTALL_PATH -DUSE_QT:BOOL=FALSE -DUSE_TBB:BOOL=FALSE -DUSE_PYTHON:BOOL=FALSE -DUSE_FIELD3D:BOOL=FALSE -DUSE_OPENJPEG:BOOL=TRUE  -DOIIO_BUILD_TESTS=0 -DOIIO_BUILD_TOOLS=0 -DLIBRAW_PATH=$INSTALL_PATH -DBOOST_ROOT=$INSTALL_PATH -DSTOP_ON_WARNING:BOOL=FALSE -DUSE_GIF:BOOL=TRUE -DUSE_FREETYPE:BOOL=TRUE -DFREETYPE_INCLUDE_PATH=$INSTALL_PATH/include/freetype2 -DOPENJPEG_INCLUDE_DIR=${INSTALL_PATH}/include/openjpeg-1.5  -DOPENJPEG_OPENJPEG_LIBRARIES=${INSTALL_PATH}/lib/libopenjpeg.dll.a -DUSE_FFMPEG:BOOL=FALSE -DUSE_OPENCV:BOOL=FALSE .. || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    mkdir -p $INSTALL_PATH/docs/oiio || exit 1
    cp ../LICENSE ../README* ../CREDITS $INSTALL_PATH/docs/oiio || exit 1
fi

# broken, ignore since we dont use it
# Install opencv
#if [ ! -f $INSTALL_PATH/lib/pkgconfig/opencv.pc ]; then
#    cd $TMP_BUILD_DIR || exit 1
#    if [ ! -f $CWD/src/$CV_TAR ]; then
#        wget $THIRD_PARTY_SRC_URL/$CV_TAR -O $CWD/src/$CV_TAR || exit 1
#    fi
#    unzip $CWD/src/$CV_TAR || exit 1
#    CV_PATCHES=$CWD/include/patches/OpenCV
#    cd opencv* || exit 1
#    patch -p0 -i "${CV_PATCHES}/mingw-w64-cmake.patch" || exit 1
#    patch -Np1 -i "${CV_PATCHES}/free-tls-keys-on-dll-unload.patch" || exit 1
#    patch -Np1 -i "${CV_PATCHES}/solve_deg3-underflow.patch" || exit 1
#    mkdir build || exit 1
#    cd build || exit 1
#    env CMAKE_LIBRARY_PATH=$INSTALL_PATH/lib CXXFLAGS="-I$INSTALL_PATH/include/eigen3" CFLAGS="-I$INSTALL_PATH/include/eigen3" CPPFLAGS="-I$INSTALL_PATH/include/eigen3 -I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" cmake -G"MSYS Makefiles" -DCMAKE_INCLUDE_PATH="$INSTALL_PATH/include $INSTALL_PATH/include/eigen3 $(pwd)"  -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH -DWITH_GTK=OFF -DWITH_GSTREAMER=OFF -DWITH_FFMPEG=OFF -DWITH_OPENEXR=OFF -DWITH_OPENCL=OFF -DWITH_OPENGL=ON -DBUILD_WITH_DEBUG_INFO=OFF -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=Release -DENABLE_SSE3=OFF ..  || exit 1
#    make -j${MKJOBS} || exit 1
#    make install || exit 1
#    mkdir -p $INSTALL_PATH/docs/opencv || exit 1
#    cp ../LIC* ../COP* ../README ../AUTH* ../CONT* $INSTALL_PATH/docs/opencv/
#fi

# Install ffmpeg: No longer needed since we use the one built with MXE
#if [ ! -f $INSTALL_PATH/lib/pkgconfig/libavcodec.pc ]; then
#  cd $MINGW_PACKAGES_PATH/${MINGW_PREFIX}ffmpeg || exit 1
#  makepkg-mingw -sLfC
#  pacman --force -U ${PKG_PREFIX}ffmpeg-*-any.pkg.tar.xz
#fi

# Install shiboken
if [ ! -f $INSTALL_PATH/lib/pkgconfig/shiboken-py2.pc ]; then
    cd $MINGW_PACKAGES_PATH/${MINGW_PREFIX}shiboken-qt4 || exit 1
    makepkg-mingw -sLfC
    pacman --force --noconfirm -U ${PKG_PREFIX}shiboken-1.2.2-1-any.pkg.tar.xz
    pacman --force --noconfirm -U ${PKG_PREFIX}python2-shiboken-1.2.2-1-any.pkg.tar.xz
fi

# Install pyside
if [ ! -f $INSTALL_PATH/lib/pkgconfig/pyside-py2.pc ]; then
    cd $MINGW_PACKAGES_PATH/${MINGW_PREFIX}pyside-qt4 || exit 1
    makepkg-mingw -sLfC
    pacman --force --noconfirm -U ${PKG_PREFIX}pyside-common-1.2.2-1-any.pkg.tar.xz
    pacman --force --noconfirm -U ${PKG_PREFIX}python2-pyside-1.2.2-1-any.pkg.tar.xz
fi

# Install SeExpr
if [ "$REBUILD_SEEXPR" = "1" ]; then
    rm -rf $INSTALL_PATH/lib/libSeExpr* $INSTALL_PATH/include/SeExpr*
fi
if [ ! -f $INSTALL_PATH/lib/libSeExpr.a ]; then
    cd $TMP_BUILD_DIR || exit 1
    if [ ! -f $SRC_PATH/$SEE_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$SEE_TAR -O $SRC_PATH/$SEE_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$SEE_TAR || exit 1
    cd SeExpr-* || exit 1
	cd src/SeExpr || exit 1
	patch -u -i $CWD/include/patches/SeExpr/mingw-compat.patch  || exit 1
	cd ../..
    mkdir build || exit 1
    cd build || exit 1
    env CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" cmake .. -G"MSYS Makefiles" -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    mkdir -p $INSTALL_PATH/docs/seexpr || exit 1
    cp ../README ../src/doc/license.txt $INSTALL_PATH/docs/seexpr/ || exit 1
fi

if [ ! -z "$TAR_SDK" ]; then
    # Done, make a tarball
    cd $INSTALL_PATH/.. || exit 1
    tar cvvJf $SRC_PATH/Natron-$SDK_VERSION-$SDK.tar.xz Natron-$SDK_VERSION || exit 1
    echo "SDK tar available: $SRC_PATH/Natron-$SDK_VERSION-$SDK.tar.xz"
    if [ ! -z "$UPLOAD_SDK" ]; then
        rsync -avz --progress --verbose -e ssh $SRC_PATH/Natron-$SDK_VERSION-$SDK.tar.xz $BINARIES_URL || exit 1
    fi

fi

FFMPEG_VERSIONS="GPL LGPL"
for V in $FFMPEG_VERSIONS; do
    if [ ! -f "${INSTALL_PATH}/ffmpeg-${V}/bin/ffmpeg.exe" ] || [ "$DOWNLOAD_FFMPEG_BIN" = "1" ]; then
        cd $SRC_PATH
        
        if [ "$V" = "GPL" ]; then
            FFMPEG_TAR=$FFMPEG_MXE_BIN_GPL
        else 
            FFMPEG_TAR=$FFMPEG_MXE_BIN_LGPL
        fi
        
        wget $THIRD_PARTY_BIN_URL/$FFMPEG_TAR -O $SRC_PATH/$FFMPEG_TAR || exit 1
        tar xvf $CWD/src/$FFMPEG_TAR || exit 1
        rm -rf $INSTALL_PATH/ffmpeg-${V}
        mkdir -p $INSTALL_PATH/ffmpeg-${V} || exit 1
        cd ffmpeg-*-${V}*
        cp -r * $INSTALL_PATH/ffmpeg-${V} || exit 1
    fi
done



#Make sure we have mt.exe for embedding manifests

if [ ! -f "${INSTALL_PATH}/bin/repogen.exe" ] || [ "$DOWNLOAD_INSTALLER" = "1" ]; then
    cd $SRC_PATH
    wget $THIRD_PARTY_BIN_URL/$INSTALLER_BIN_TAR -O $SRC_PATH/$INSTALLER_BIN_TAR || exit 1
    unzip $CWD/src/$INSTALLER_BIN_TAR || exit 1
    cd natron-win$BIT-installer* || exit 1
    if [ -d "bin" ]; then
        cd bin
    fi
    cp mt.exe /usr/bin || exit 1
    cp archivegen.exe $INSTALL_PATH/bin || exit 1
    cp binarycreator.exe $INSTALL_PATH/bin || exit 1
    cp installerbase.exe $INSTALL_PATH/bin || exit 1
    cp repogen.exe $INSTALL_PATH/bin || exit 1
fi

echo
echo "Natron build-sdk done."
echo
exit 0

