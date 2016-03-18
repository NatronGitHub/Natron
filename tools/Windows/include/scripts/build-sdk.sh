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
elif [ "$1" = "64" ]; then
    BIT=64
    INSTALL_PATH=$INSTALL64_PATH
    PKG_PREFIX=$PKG_PREFIX64
    FFMPEG_MXE_BIN_GPL=$FFMPEG_MXE_BIN_64_GPL_TAR
    FFMPEG_MXE_BIN_LGPL=$FFMPEG_MXE_BIN_64_LGPL_TAR
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
if [ -d $TMP_BUILD_DIR ]; then
    rm -rf $TMP_BUILD_DIR || exit 1
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

# Install llvm
if [ ! -f "$INSTALL_PATH/llvm/bin/llvm-config.exe" ]; then
    cd $TMP_BUILD_DIR || exit 1
    if [ ! -f $SRC_PATH/$LLVM_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$LLVM_TAR -O $SRC_PATH/$LLVM_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$LLVM_TAR || exit 1
    cd llvm-* || exit 1
    patch -p0 < $INC_PATH/patches/llvm/add_pi.diff || exit 1
    mkdir build
    cd build || exit 1
    cmake -G "MSYS Makefiles" -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH/llvm -DLLVM_INCLUDE_TESTS=OFF -DLLVM_ENABLE_CXX1Y=ON -DBUILD_SHARED_LIBS=OFF -DLLVM_ENABLE_RTTI=1 -DCMAKE_BUILD_TYPE=Release .. || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
fi

# Install osmesa
if [ ! -f "$INSTALLPATH/osmesa/lib/osmesa.dll" ]; then
    cd $TMP_BUILD_DIR || exit 1
    if [ ! -f $SRC_PATH/$MESA_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$MESA_TAR -O $SRC_PATH/$MESA_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$MESA_TAR || exit 1
    cd mesa-* || exit 1
    patch -p0 < $INC_PATH/patches/mesa/add_pi.diff
    patch -p0 < $INC_PATH/patches/mesa/mgl_export.diff
    patch -p0 < $INC_PATH/patches/mesa/scons_fix.diff
    # optional, adds failsafe version #patch -p1 < $INC_PATH/patches/mesa/swrast_failsafe.diff
    mkdir -p $INSTALL_PATH/osmesa/include $INSTALL_PATH/osmesa/lib/pkgconfig
    cat $INC_PATH/patches/mesa/osmesa.pc | sed "s/__REPLACE__/${INSTALL_PATH}/" > $INSTALL_PATH/osmesa/lib/pkgconfig/osmesa.pc || exit 1
    (cd include/GL; sed -e 's@gl.h glext.h@gl.h glext.h ../GLES/gl.h@' -e 's@\^GLAPI@^GL_\\?API@' -i.orig gl_mangle.h) 
    (cd include/GL; sh ./gl_mangle.h > gl_mangle.h.new && mv gl_mangle.h.new gl_mangle.h)
    if [ "$BIT" = "64" ]; then
      MESAARCH="x86_64"
    else
      MESAARCH="x86"
    fi
    LLVM_CONFIG=$INSTALL_PATH/llvm/bin/llvm-config.exe LLVM=$INSTALL_PATH/llvm CFLAGS="-DUSE_MGL_NAMESPACE" CXXFLAGS="-std=c++11" LDFLAGS="-static -s" scons build=release platform=windows toolchain=mingw machine=$MESAARCH texture_float=yes llvm=yes verbose=yes osmesa || exit 1
    cp build/windows-$MESAARCH/gallium/targets/osmesa/osmesa.dll $INSTALL_PATH/osmesa/lib/ || exit 1
    cp -a include/GL $INSTALL_PATH/osmesa/include/ || exit 1
fi

# Install glu
if [ ! -f "$INSTALL_PATH/osmesa/lib/pkgconfig/glu.pc" ]; then
    cd $TMP_BUILD_DIR || exit 1
    if [ ! -f $SRC_PATH/$GLU_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$GLU_TAR -O $SRC_PATH/$GLU_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$GLU_TAR || exit 1
    cd glu-* || exit 1
    env PKG_CONFIG_PATH=$INSTALL_PATH/osmesa/lib/pkgconfig LD_LIBRARY_PATH=$INSTALL_PATH/osmesa/lib ./configure --prefix=$INSTALL_PATH/osmesa --disable-shared --enable-static CPPFLAGS=-DUSE_MGL_NAMESPACE || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
fi

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
    patch -p0 < $INC_PATH/patches/ImageMagick/mingw-utf8.diff || exit 1
    env CFLAGS="-DMAGICKCORE_EXCLUDE_DEPRECATED=1" CXXFLAGS="-I${INSTALL_PATH}/include -DMAGICKCORE_EXCLUDE_DEPRECATED=1" LDFLAGS="-lz -lws2_32" ./configure --prefix=$INSTALL_PATH --with-magick-plus-plus=yes --with-quantum-depth=32 --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --with-lcms --without-openjp2 --without-lqr --without-lzma --without-openexr --with-pango --with-png --with-rsvg --without-tiff --without-webp --with-xml --without-zlib --without-bzlib --enable-static --disable-shared --enable-hdri --with-freetype --with-fontconfig --without-x --without-modules || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
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


# Install openexr
EXR_THREAD="pthread"

if [ "$REBUILD_EXR" = "1" ]; then
  rm -rf $INSTALL_PATH/lib/pkgconfig/{IlmBase.pc,OpenEXR.pc}
fi
if [ ! -f $INSTALL_PATH/lib/pkgconfig/OpenEXR.pc ]; then
    cd $TMP_BUILD_DIR || exit 1
    if [ ! -f $SRC_PATH/$EXR_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$EXR_TAR -O $SRC_PATH/$EXR_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$EXR_TAR || exit 1
    cd openexr-* || exit 1

    OPENEXR_BASE_PATCHES=$(find $INC_PATH/patches/OpenEXR -type f)
    for p in $OPENEXR_BASE_PATCHES; do
      if [[ "$p" = *-mingw-use_pthreads* ]] && [ "$EXR_THREAD" != "pthread" ]; then
        continue
      fi
      echo "Patch: $p"
      patch -p1 -i $p || exit 1
    done
    ILM_BASE_PATCHES=$(find $INC_PATH/patches/IlmBase -type f)
    for p in $ILM_BASE_PATCHES; do
      if [[ "$p" = *-mingw-use_pthreads* ]] && [ "$EXR_THREAD" != "pthread" ]; then
        continue
      fi
      if [[ "$p" = *-mingw-use_windows_threads* ]] && [ "$EXR_THREAD" != "win32" ]; then
        continue
      fi
      echo "Patch: $p"
      patch -p1 -i $p || exit 1
    done


    mkdir build_ilmbase
    cd build_ilmbase || exit 1
    cmake -G"MSYS Makefiles" -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH -DBUILD_SHARED_LIBS=ON -DNAMESPACE_VERSIONING=ON  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_CONFIG_NAME=Release ../IlmBase || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    
    cd .. || exit 1


    mkdir build_openexr
    cd build_openexr || exit 1

    cmake -DCMAKE_CXX_FLAGS="-I${INSTALL_PATH}/include/OpenEXR" -DCMAKE_EXE_LINKER_FLAGS="-L${INSTALL_PATH}/bin" -G"MSYS Makefiles" -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH -DBUILD_SHARED_LIBS=ON -DNAMESPACE_VERSIONING=ON -DUSE_ZLIB_WINAPI=OFF ../OpenEXR || exit 1

    make -j${MKJOBS} || exit 1
    make install || exit 1
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
    if [[ "$OIIO_TAR" = *-1.5.* ]]; then
        patches=$(find "${OIIO_PATCHES}/1.5" -type f)
    elif [[ "$OIIO_TAR" = *-1.6.* ]]; then
        patches=$(find "${OIIO_PATCHES}/1.6" -type f)
    fi
    for p in $patches; do
		echo "Applying $p"
        patch -p1 -i $p || exit 1
    done
    rm -rf build
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




# Install qt
if [ ! -f "$INSTALL_PATH/bin/qmake.exe" ]; then
  cd $MINGW_PACKAGES_PATH/${MINGW_PREFIX}qt4 || exit 1
  makepkg-mingw --skipchecksums -sLfC
  pacman --force --noconfirm -U ${PKG_PREFIX}qt4-4.8.7-3-any.pkg.tar.xz || exit 1
fi

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

if [ ! -f "${INSTALL_PATH}/bin/repogen.exe" ] || [ "$DOWNLOAD_INSTALLER" = "1" ] || [ ! -f "/mingw32/bin/dump_syms.exe" ] || [ ! -f "/mingw64/bin/dump_syms.exe" ] || [ ! -f "/mingw32/bin/mt.exe" ] || [ ! -f "/mingw64/bin/mt.exe" ]; then
    cd $SRC_PATH
    wget $THIRD_PARTY_BIN_URL/$INSTALLER_BIN_TAR -O $SRC_PATH/$INSTALLER_BIN_TAR || exit 1
    unzip $CWD/src/$INSTALLER_BIN_TAR || exit 1
    cp -a natron-windows-installer/mingw32/bin/* /mingw32/bin/  || exit 1
    cp -a natron-windonws-installer/mingw64/bin/* /mingw64/bin/ || exit 1
fi

echo
echo "Natron build-sdk done."
echo
exit 0

