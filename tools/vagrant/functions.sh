#!/bin/sh
source `pwd`/common.sh

function setup_ssl {
  if [ ! -f "${INSTALL_PATH}/lib/pkgconfig/openssl.pc" ]; then
    if [ ! -f $SRC_PATH/$SSL_TAR ]; then
      curl $THIRD_PARTY_SRC_URL/$SSL_TAR -o $SRC_PATH/$SSL_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$SSL_TAR -C $TMP_PATH || exit 1
    cd $TMP_PATH/openssl-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./config --prefix="$INSTALL_PATH" || exit 1
    make || exit 1
    make install || exit 1
    make clean
    env CFLAGS="$BF" CXXFLAGS="$BF" ./config --prefix="$INSTALL_PATH" -shared || exit 1
    make || exit 1
    make install || exit 1
  fi 
}

function setup_qt {
  if [ "$1" = "installer" ]; then
    QT_CONF="-no-multimedia -no-gif -qt-libpng -no-opengl -no-libmng -no-libtiff -no-libjpeg -static -openssl-linked -confirm-license -release -opensource -nomake demos -nomake docs -nomake examples -no-gtkstyle -no-webkit -no-avx -I${INSTALL_PATH}/include -L${INSTALL_PATH}/lib -prefix ${INSTALL_PATH}/installer"
    QT_MAKE="${INSTALL_PATH}/installer/bin/qmake"
  else
    QT_CONF="-shared -system-zlib -system-libtiff -system-libpng -no-libmng -system-libjpeg -no-gtkstyle -glib -xrender -xrandr -xcursor -xfixes -xinerama -fontconfig -xinput -sm -no-multimedia -openssl-linked -confirm-license -release -opensource -opengl desktop -nomake demos -nomake docs -nomake examples -I${INSTALL_PATH}/include -L${INSTALL_PATH}/lib -prefix ${INSTALL_PATH}"
    QT_MAKE="${INSTALL_PATH}/bin/qmake"
  fi
  if [ ! -f "$QT_MAKE" ]; then
    if [ "$1" = "installer" ]; then
      setup_ssl static || exit 1
    else
      setup_ssl || exit 1
      setup_zlib || exit 1
      setup_bzip || exit 1
      setup_icu || exit 1
      setup_expat || exit 1
      setup_png || exit 1
      setup_freetype || exit 1
      setup_fontconfig || exit 1
      setup_ffi || exit 1
      setup_glib || exit 1
      setup_jpeg || exit 1
      setup_tiff || exit 1
    fi
    if [ ! -f $SRC_PATH/$QT_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$QT_TAR -o $SRC_PATH/$QT_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$QT_TAR -C $TMP_PATH || exit 1
    cd $TMP_PATH/qt-everywhere-opensource-src-4.8* || exit 1
    QT_SRC=`pwd`/src
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure $QT_CONF || exit 1
    # https://bugreports.qt-project.org/browse/QTBUG-5385
    LD_LIBRARY_PATH="$LD_LIBRARY_PATH":`pwd`/lib make -j${MKJOBS} || exit  1
    make install || exit 1
    cd .. || exit 1
    rm -rf qt-everywhere-opensource-src-4.8*
  fi
}

function setup_installer {
  if [ ! -f "$INSTALL_PATH/installer/bin/binarycreator" ]; then
    setup_qt installer || exit 1
    cd $TMP_PATH || exit 1
    if [ -d qtifw ]; then
      rm -rf qtifw
    fi
    git clone $GIT_INSTALLER || exit 1
    cd qtifw || exit 1
    git checkout natron || exit 1
    $INSTALL_PATH/installer/bin/qmake || exit 1
    make -j${MKJOBS} || exit 1
    strip -s bin/*
    cp bin/* $INSTALL_PATH/installer/bin/ || exit 1
  fi
}

function setup_patchelf {
  if [ ! -f "$INSTALL_PATH/bin/patchelf" ] && [ "$PKGOS" = "Linux" ]; then
    if [ ! -f "$SRC_PATH/$ELF_TAR" ]; then
      curl "$THIRD_PARTY_SRC_URL/$ELF_TAR" -o "$SRC_PATH/$ELF_TAR" || exit 1
    fi
    tar xvf $SRC_PATH/$ELF_TAR -C $TMP_PATH || exit 1
    cd $TMP_PATH/patchelf* || exit 1
    ./configure || exit 1
    make || exit 1
    cp src/patchelf "$INSTALL_PATH/bin/" || exit 1
  fi
}

function setup_gcc {
  if [ ! -f "$INSTALL_PATH/gcc/bin/gcc" ] && [ "$PKGOS" = "Linux" ]; then
    cd "$TMP_PATH" || exit 1
    if [ ! -f "$SRC_PATH/$GCC_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$GCC_TAR" -o "$SRC_PATH/$GCC_TAR" || exit 1
    fi
    if [ ! -f "$SRC_PATH/$MPC_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$MPC_TAR" -o "$SRC_PATH/$MPC_TAR" || exit 1
    fi
    if [ ! -f "$SRC_PATH/$MPFR_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$MPFR_TAR" -o "$SRC_PATH/$MPFR_TAR" || exit 1
    fi
    if [ ! -f "$SRC_PATH/$GMP_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$GMP_TAR" -o "$SRC_PATH/$GMP_TAR" || exit 1
    fi
    if [ ! -f "$SRC_PATH/$ISL_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$ISL_TAR" -o "$SRC_PATH/$ISL_TAR" || exit 1
    fi
    if [ ! -f $SRC_PATH/$CLOOG_TAR ]; then
        curl "$THIRD_PARTY_SRC_URL/$CLOOG_TAR" -o "$SRC_PATH/$CLOOG_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$GCC_TAR" || exit 1
    cd "gcc-$TC_GCC" || exit 1
    tar xvf "$SRC_PATH/$MPC_TAR" || exit 1
    mv "mpc-$TC_MPC" mpc || exit 1
    tar xvf "$SRC_PATH/$MPFR_TAR" || exit 1
    mv "mpfr-$TC_MPFR" mpfr || exit 1
    tar xvf "$SRC_PATH/$GMP_TAR" || exit 1
    mv "gmp-$TC_GMP" gmp || exit 1
    tar xvf "$SRC_PATH/$ISL_TAR" || exit 1
    mv "isl-$TC_ISL" isl || exit 1
    tar xvf "$SRC_PATH/$CLOOG_TAR" || exit 1
    mv "cloog-$TC_CLOOG" cloog || exit 1
    ./configure --prefix="$INSTALL_PATH/gcc" --enable-languages=c,c++ || exit 1
    make -j$MKJOBS || exit 1
    #make -k check || exit 1
    make install || exit 1
  fi
}

function setup_zlib {
  if [ ! -f "$INSTALL_PATH/lib/libz.so.1" ]; then
    if [ ! -f "$SRC_PATH/$ZLIB_TAR" ]; then
      curl "$THIRD_PARTY_SRC_URL/$ZLIB_TAR" -o "$SRC_PATH/$ZLIB_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$ZLIB_TAR" -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/zlib-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$1" = "static" ]; then
      rm -f $INSTALL_PATH/lib/libz.so*
    fi
  fi
}

function setup_bzip {
  if [ ! -f "$INSTALL_PATH/lib/libbz2.so.1" ]; then
    if [ ! -f "$SRC_PATH/$BZIP_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$BZIP_TAR" -o "$SRC_PATH/$BZIP_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$BZIP_TAR" -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/bzip* || exit 1
    sed -e 's/^CFLAGS=\(.*\)$/CFLAGS=\1 \$(BIGFILES)/' -i ./Makefile-libbz2_so || exit 1
    sed -i "s/libbz2.so.1.0 -o libbz2.so.1.0.6/libbz2.so.1 -o libbz2.so.1.0.6/;s/rm -f libbz2.so.1.0/rm -f libbz2.so.1/;s/ln -s libbz2.so.1.0.6 libbz2.so.1.0/ln -s libbz2.so.1.0.6 libbz2.so.1/" Makefile-libbz2_so || exit 1
    make -f Makefile-libbz2_so || exit 1
    install -m755 libbz2.so.1.0.6 $INSTALL_PATH/lib || exit 1
    install -m644 bzlib.h $INSTALL_PATH/include/ || exit 1
    cd $INSTALL_PATH/lib || exit 1
    ln -s libbz2.so.1.0.6 libbz2.so || exit 1
    ln -s libbz2.so.1.0.6 libbz2.so.1 || exit 1
  fi
}

function setup_yasm {
  if [ ! -f "$INSTALL_PATH/bin/yasm" ]; then
    if [ ! -f "$SRC_PATH/$YASM_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$YASM_TAR" -o "$SRC_PATH/$YASM_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$YASM_TAR" -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/yasm* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_cmake {
  if [ ! -f "$INSTALL_PATH/cmake/bin/cmake" ]; then
    if [ ! -f "$SRC_PATH/$CMAKE_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$CMAKE_TAR" -o "$SRC_PATH/$CMAKE_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$CMAKE_TAR" -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/cmake* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH/cmake" || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_icu {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/icu-i18n.pc" ]; then
    if [ ! -f "$SRC_PATH/$ICU_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$ICU_TAR" -o "$SRC_PATH/$ICU_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$ICU_TAR" -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/icu/source || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --enable-shared || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_python {
  if [ "$PYV" = "2" ]; then
    PY_TAR=$PY2_TAR
  else
    PY_TAR=$PY3_TAR
  fi
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/python${PYV}.pc" ]; then
    setup_icu || exit 1
    setup_ssl || exit 1
    setup_zlib || exit 
    if [ ! -f "$SRC_PATH/$PY_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$PY_TAR" -o "$SRC_PATH/$PY_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$PY_TAR" -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/Python-$PYV* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix="$INSTALL_PATH" --enable-shared  || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_expat {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/expat.pc" ]; then
    if [ ! -f "$SRC_PATH/$EXPAT_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$EXPAT_TAR" -o "$SRC_PATH/$EXPAT_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$EXPAT_TAR" -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/expat* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-static --enable-shared || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_png {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/libpng.pc" ]; then
    setup_zlib || exit 1
    if [ ! -f $SRC_PATH/$PNG_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$PNG_TAR -o $SRC_PATH/$PNG_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$PNG_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/libpng* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --disable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_freetype {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/freetype2.pc" ]; then
    setup_expat || exit 1
    if [ ! -f "$SRC_PATH/$FTYPE_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$FTYPE_TAR" -o "$SRC_PATH/$FTYPE_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$FTYPE_TAR" -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/freetype* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-static --enable-shared || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_fontconfig {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/fontconfig.pc" ]; then
    setup_freetype || exit 1
    if [ ! -f "$SRC_PATH/$FCONFIG_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$FCONFIG_TAR" -o "$SRC_PATH/$FCONFIG_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$FCONFIG_TAR" -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/fontconfig* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-docs --disable-static --enable-shared || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_ffi {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/libffi.pc" ]; then
    if [ ! -f "$SRC_PATH/$FFI_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$FFI_TAR" -o "$SRC_PATH/$FFI_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$FFI_TAR" -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/libffi* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-docs --disable-static --enable-shared || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_glib {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/glib-2.0.pc" ]; then
    setup_ffi || exit 1
    if [ ! -f "$SRC_PATH/$GLIB_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$GLIB_TAR" -o "$SRC_PATH/$GLIB_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$GLIB_TAR" -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/glib-2* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-docs --disable-static --enable-shared || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_xml {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/libxml-2.0.pc" ]; then
    if [ ! -f "$SRC_PATH/$LIBXML_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$LIBXML_TAR" -o "$SRC_PATH/$LIBXML_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$LIBXML_TAR" -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/libxml* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-docs --disable-static --enable-shared --without-python || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_xslt {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/libxslt.pc" ]; then
    setup_xml || exit 1
    if [ ! -f "$SRC_PATH/$LIBXSLT_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$LIBXSLT_TAR" -o "$SRC_PATH/$LIBXSLT_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$LIBXSLT_TAR" -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/libxslt* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-docs --disable-static --enable-shared --without-python || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_boost {
  if [ ! -f "$INSTALL_PATH/lib/libboost_atomic.so" ]; then
    setup_zlib || exit 1
    setup_bzip || exit 1
    if [ ! -f "$SRC_PATH/$BOOST_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$BOOST_TAR" -o "$SRC_PATH/$BOOST_TAR" || exit 1
    fi
    rm -f $TMP_PATH/boost_1*
    tar xvf "$SRC_PATH/$BOOST_TAR" -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/boost_* || exit 1
    ./bootstrap.sh || exit 1
    ./b2 --prefix=$INSTALL_PATH cflags="$BF" cxxflags="$BF -I${INSTALL_PATH}/include" linkflags="-L${INSTALL_PATH}/lib" link=shared variant=release threading=multi runtime-link=shared --layout=system --disable-icu -j${MKJOBS} install || exit 1
  fi
}

function setup_jpeg {
  if [ ! -f "$INSTALL_PATH/lib/libjpeg.so" ]; then
    if [ ! -f $SRC_PATH/$JPG_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$JPG_TAR -o $SRC_PATH/$JPG_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$JPG_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/jpeg-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --disable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_tiff {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/libtiff-4.pc" ]; then
    setup_zlib || exit 1
    if [ ! -f $SRC_PATH/$TIF_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$TIF_TAR -o $SRC_PATH/$TIF_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$TIF_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/tiff-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --disable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_jasper {
  if [ ! -f "$INSTALL_PATH/lib/libjasper.so" ]; then
    setup_jpeg || exit 1
    if [ ! -f $SRC_PATH/$JASP_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$JASP_TAR -o $SRC_PATH/$JASP_TAR || exit 1
    fi
    cd "$TMP_PATH" || exit 1 
    unzip $SRC_PATH/$JASP_TAR || exit 1
    cd jasper* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --disable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_lcms {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/lcms2.pc" ]; then
    setup_tiff || exit 1
    if [ ! -f $SRC_PATH/$LCMS_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$LCMS_TAR -o $SRC_PATH/$LCMS_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$LCMS_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/lcms2-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --disable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_openjpeg {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/libopenjpeg.pc" ]; then
    setup_zlib || exit 1
    setup_png || exit 1
    setup_tiff || exit 1
    setup_lcms || exit 1
    if [ ! -f $SRC_PATH/$OJPG_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$OJPG_TAR -o $SRC_PATH/$OJPG_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$OJPG_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/openjpeg-* || exit 1
    ./bootstrap.sh || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --disable-shared --disable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_libraw {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/libraw.pc" ]; then
    setup_jasper || exit 1
    setup_lcms || exit 1
    if [ ! -f $SRC_PATH/$LIBRAW_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$LIBRAW_TAR -o $SRC_PATH/$LIBRAW_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$LIBRAW_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/LibRaw* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --disable-static --enable-shared || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_openexr {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/OpenEXR.pc" ]; then
    setup_zlib || exit 1
    if [ ! -f $SRC_PATH/$ILM_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$ILM_TAR -o $SRC_PATH/$ILM_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$ILM_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/ilmbase-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --disable-shared --enable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ ! -f $SRC_PATH/$EXR_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$EXR_TAR -o $SRC_PATH/$EXR_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$EXR_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/openexr-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --disable-shared --enable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_pixman {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/pixman-1.pc" ]; then
    if [ ! -f $SRC_PATH/$PIX_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$PIX_TAR -o $SRC_PATH/$PIX_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$PIX_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/pixman-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --disable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_cairo {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/cairo.pc" ]; then
    setup_fontconfig || exit 1
    setup_png || exit 1
    setup_pixman || exit 1
    if [ ! -f $SRC_PATH/$CAIRO_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$CAIRO_TAR -o $SRC_PATH/$CAIRO_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$CAIRO_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/cairo-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include -I${INSTALL_PATH}/include/pixman-1" LDFLAGS="-L${INSTALL_PATH}/lib -lpixman-1" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --enable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_harfbuzz {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/harfbuzz.pc" ]; then
    setup_freetype || exit 1
    setup_glib || exit 1
    setup_cairo || exit 1
    if [ ! -f $SRC_PATH/$BUZZ_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$BUZZ_TAR -o $SRC_PATH/$BUZZ_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$BUZZ_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/harfbuzz-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --disable-docs --disable-static --enable-shared --with-freetype --with-cairo --with-gobject --with-glib --without-icu || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_pango {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/pango.pc" ]; then
    setup_cairo || exit 1
    setup_harfbuzz || exit 1
    setup_glib || exit 1
    if [ ! -f "$SRC_PATH/$PANGO_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$PANGO_TAR" -o "$SRC_PATH/$PANGO_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$PANGO_TAR" -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/pango-* || exit 1
    env FONTCONFIG_CFLAGS="-I$INSTALL_PATH/include" FONTCONFIG_LIBS="-L$INSTALL_PATH/lib -lfontconfig" CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-docs --disable-static --enable-shared --with-included-modules=basic-fc || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_croco {
  if [ ! -f "$INSTALL_PATH/lib/libcroco-0.6.so" ]; then
    setup_glib || exit 1
    setup_xml || exit 1
    if [ ! -f "$SRC_PATH/$CROCO_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$CROCO_TAR" -o "$SRC_PATH/$CROCO_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$CROCO_TAR" -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/libcroco-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-docs --disable-static --enable-shared || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_gdk {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/gdk-pixbuf-2.0.pc" ]; then
    setup_glib || exit 1
    setup_jasper || exit 1
    setup_jpeg || exit 1
    setup_png || exit 1
    setup_tiff || exit 1
    if [ ! -f "$SRC_PATH/$GDK_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$GDK_TAR" -o "$SRC_PATH/$GDK_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$GDK_TAR" -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/gdk-pix* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix="$INSTALL_PATH" --disable-docs --disable-static --enable-shared --without-libtiff || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_rsvg {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/librsvg-2.0.pc" ]; then
    setup_gdk || exit 1
    setup_croco || exit 1
    setup_pango || exit 1
    setup_glib || exit 1
    if [ ! -f "$SRC_PATH/$SVG_TAR" ]; then
        curl "$THIRD_PARTY_SRC_URL/$SVG_TAR" -o "$SRC_PATH/$SVG_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$SVG_TAR" -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/librsvg-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-docs --disable-static --enable-shared --disable-introspection --disable-vala --disable-pixbuf-loader || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_magick {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/Magick++.pc" ]; then
    setup_fontconfig || exit 1
    setup_lcms || exit 1
    setup_cairo || exit 1
    setup_pango || exit 1
    setup_png || exit 1
    setup_rsvg || exit 1
    setup_xml || exit 1
    setup_zlib || exit 1
    setup_freetype || exit 1
    if [ ! -f $SRC_PATH/$MAGICK_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$MAGICK_TAR -o $SRC_PATH/$MAGICK_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$MAGICK_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/ImageMagick-* || exit 1
    patch -p0 < $CWD/patches/ImageMagick/pango-align-hack.diff || exit 1
    env CFLAGS="$BF -DMAGICKCORE_EXCLUDE_DEPRECATED=1" CXXFLAGS="$BF -DMAGICKCORE_EXCLUDE_DEPRECATED=1" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --with-magick-plus-plus=yes --with-quantum-depth=32 --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --with-lcms --without-openjp2 --without-lqr --without-lzma --without-openexr --with-pango --with-png --with-rsvg --without-tiff --without-webp --with-xml --without-zlib --without-bzlib --disable-static --enable-shared --enable-hdri --with-freetype --with-fontconfig --without-x --without-modules || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_glew {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/glew.pc" ]; then
    if [ ! -f $SRC_PATH/$GLEW_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$GLEW_TAR -o $SRC_PATH/$GLEW_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$GLEW_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/glew-* || exit 1
    if [ "$ARCH" = "i686" ]; then
        make -j${MKJOBS} 'CFLAGS.EXTRA=-O2 -g -march=i686 -mtune=i686' includedir=/usr/include GLEW_DEST= libdir=/usr/lib bindir=/usr/bin || exit 1
    else
        make -j${MKJOBS} 'CFLAGS.EXTRA=-O2 -g -m64 -fPIC -mtune=generic' includedir=/usr/include GLEW_DEST= libdir=/usr/lib64 bindir=/usr/bin || exit 1
    fi
    make install GLEW_DEST=$INSTALL_PATH libdir=/lib bindir=/bin includedir=/include || exit 1
    if [ "$1" = "static" ]; then
      rm -f $INSTALL_PATH/lib/*GLEW*.so*
    fi
  fi
}

function setup_ocio {
  if [ ! -f "$INSTALL_PATH/lib/libOpenColorIO.so" ]; then
    setup_cmake || exit 1
    if [ ! -f $SRC_PATH/$OCIO_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$OCIO_TAR -o $SRC_PATH/$OCIO_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$OCIO_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/OpenColorIO-* || exit 1
    mkdir build || exit 1
    cd build || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" cmake .. -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH -DCMAKE_BUILD_TYPE=Release -DOCIO_BUILD_SHARED=ON -DOCIO_BUILD_STATIC=OFF -DOCIO_BUILD_APPS=OFF -DOCIO_BUILD_PYGLUE=OFF || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    cp ext/dist/lib/{liby*.a,libt*.a} $INSTALL_PATH/lib/ || exit 1
    sed -i "s/-lOpenColorIO/-lOpenColorIO -lyaml-cpp -ltinyxml -llcms2/" $INSTALL_PATH/lib/pkgconfig/OpenColorIO.pc || exit 1
  fi
}

function setup_oiio {
  if [ ! -f "$INSTALL_PATH/lib/libOpenImageIO.so" ]; then
    setup_boost || exit 1
    setup_glew || exit 1
    setup_jasper || exit 1
    setup_tiff || exit 1
    setup_ocio || exit 1
    setup_openexr || exit 1
    setup_freetype || exit 1
    setup_libraw || exit 1
    setup_png || exit 1
    setup_openjpeg || exit 1
    if [ ! -f $SRC_PATH/$OIIO_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$OIIO_TAR -o $SRC_PATH/$OIIO_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$OIIO_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/oiio-Release-* || exit 1
    patch -p1 -i $CWD/patches/OpenImageIO/oiio-exrthreads.patch || exit 1
    mkdir build || exit 1
    cd build || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" CXXFLAGS="-fPIC" cmake -DUSE_OPENCV:BOOL=FALSE -DUSE_OPENSSL:BOOL=FALSE -DOPENEXR_HOME=$INSTALL_PATH -DILMBASE_HOME=$INSTALL_PATH -DTHIRD_PARTY_TOOLS_HOME=$INSTALL_PATH -DUSE_QT:BOOL=FALSE -DUSE_TBB:BOOL=FALSE -DUSE_PYTHON:BOOL=FALSE -DUSE_FIELD3D:BOOL=FALSE -DUSE_OPENJPEG:BOOL=FALSE  -DOIIO_BUILD_TESTS=0 -DOIIO_BUILD_TOOLS=0 -DUSE_LIB_RAW=1 -DLIBRAW_PATH=$INSTALL_PATH -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH -DBOOST_ROOT=$INSTALL_PATH -DSTOP_ON_WARNING:BOOL=FALSE -DUSE_GIF:BOOL=TRUE -DUSE_FREETYPE:BOOL=TRUE -DFREETYPE_INCLUDE_PATH=$INSTALL_PATH/include -DUSE_FFMPEG:BOOL=FALSE -DLINKSTATIC=0 -DBUILDSTATIC=0 .. || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_seexpr {
  if [ ! -f "$INSTALL_PATH/lib/libSeExpr.so" ]; then
    setup_cmake || exit 1
    if [ ! -f $SRC_PATH/$SEE_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$SEE_TAR -o $SRC_PATH/$SEE_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$SEE_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/SeExpr-* || exit 1
    mkdir build || exit 1
    cd build || exit 1
    CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" cmake .. -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH || exit 1
    make || exit 1
    make install || exit 1
    if [ "$1" = "static" ]; then
      rm -f $INSTALL_PATH/lib/libSeExpr.so
    fi
  fi
}

function setup_lame {
  if [ ! -f "$INSTALL_PATH/lib/libmp3lame.so" ]; then
    if [ ! -f $SRC_PATH/$LAME_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$LAME_TAR -o $SRC_PATH/$LAME_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$LAME_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/lame-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --enable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_ogg {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/ogg.pc" ]; then
    if [ ! -f $SRC_PATH/$OGG_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$OGG_TAR -o $SRC_PATH/$OGG_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$OGG_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/libogg-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --enable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_vorbis {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/vorbis.pc" ]; then
    setup_ogg || exit 1
    if [ ! -f $SRC_PATH/$VORBIS_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$VORBIS_TAR -o $SRC_PATH/$VORBIS_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$VORBIS_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/libvorbis-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --disable-shared --enable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_theora {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/theora.pc" ]; then
    setup_ogg || exit 1
    setup_vorbis || exit 1
    if [ ! -f $SRC_PATH/$THEORA_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$THEORA_TAR -o $SRC_PATH/$THEORA_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$THEORA_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/libtheora-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --disable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_modplug {
  if [ ! -f "$INSTALL_PATH/lib/libmodplug.so" ]; then
    if [ ! -f $SRC_PATH/$MODPLUG_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$MODPLUG_TAR -o $SRC_PATH/$MODPLUG_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$MODPLUG_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/libmodplug-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --disable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_vpx {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/vpx.pc" ]; then
    if [ ! -f $SRC_PATH/$VPX_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$VPX_TAR -o $SRC_PATH/$VPX_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$VPX_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/libvpx-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --enable-static --enable-vp8 --enable-vp9 --enable-runtime-cpu-detect --enable-postproc --enable-pic --disable-avx --disable-avx2 --disable-examples || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_opus {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/opus.pc" ]; then
    if [ ! -f $SRC_PATH/$OPUS_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$OPUS_TAR -o $SRC_PATH/$OPUS_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$OPUS_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/opus-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --enable-static --enable-custom-modes || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_orc {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/orc-0.4.pc" ]; then
    if [ ! -f $SRC_PATH/$ORC_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$ORC_TAR -o $SRC_PATH/$ORC_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$ORC_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/orc-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --enable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_dirac {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/schroedinger-1.0.pc" ]; then
    setup_orc || exit 1
    if [ ! -f $SRC_PATH/$DIRAC_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$DIRAC_TAR -o $SRC_PATH/$DIRAC_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$DIRAC_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/schro* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --enable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    sed -i "s/-lschroedinger-1.0/-lschroedinger-1.0 -lorc-0.4/" $INSTALL_PATH/lib/pkgconfig/schroedinger-1.0.pc || exit 1
  fi
}

function setup_x264 {
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/x264.pc" ]; then
    if [ ! -f $SRC_PATH/$X264_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$X264_TAR -o $SRC_PATH/$X264_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$X264_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/x264* || exit 1
    ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --enable-static --enable-pic --bit-depth=10  CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_xvid {
  if [ ! -f "$INSTALL_PATH/lib/libxvidcore.a" ]; then
    if [ ! -f $SRC_PATH/$XVID_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$XVID_TAR -o $SRC_PATH/$XVID_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$XVID_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/xvidcore/build/generic || exit 1
    ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --enable-static CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_ffmpeg {
  LGPL_SETTINGS="--enable-avresample --enable-libmp3lame --enable-libvorbis --enable-libopus --enable-libtheora --enable-libschroedinger --enable-libopenjpeg --enable-libmodplug --enable-libvpx --disable-libspeex --disable-libxcb --disable-libxcb-shm --disable-libxcb-xfixes --disable-indev=jack --disable-outdev=xv --disable-vda --disable-xlib"
  GPL_SETTINGS="${LGPL_SETTINGS} --enable-gpl --enable-libx264 --enable-libxvid --enable-version3"
  if [ ! -d "$INSTALL_PATH/ffmpeg-gpl" ] || [ ! -d "$INSTALL_PATH/ffmpeg-lgpl" ]; then
    setup_lame || exit 1 
    setup_ogg || exit 1
    setup_vorbis || exit 1
    setup_theora || exit 1
    setup_modplug || exit 1
    setup_vpx || exit 1
    setup_opus || exit 1
    setup_orc || exit 1
    setup_dirac || exit 1
    setup_x264 || exit 1
    setup_xvid || exit 1
    setup_openjpeg || exit 1
    if [ ! -f $SRC_PATH/$FFMPEG_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$FFMPEG_TAR -o $SRC_PATH/$FFMPEG_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$FFMPEG_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/ffmpeg-2* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH/ffmpeg-gpl --libdir=$INSTALL_PATH/ffmpeg-gpl/lib --enable-shared --disable-static $GPL_SETTINGS || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    make distclean
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH/ffmpeg-lgpl --libdir=$INSTALL_PATH/ffmpeg-lgpl/lib --enable-shared --disable-static $LGPL_SETTINGS  || exit 1
    make install || exit 1
  fi
}

function setup_shiboken {
  if [ "$PYV" = "3" ]; then
    export PYTHON_PATH=$INSTALL_PATH/lib/python3.4
    export PYTHON_INCLUDE=$INSTALL_PATH/include/python3.4
    PY_EXE=$INSTALL_PATH/bin/python3
    PY_LIB=$INSTALL_PATH/lib/libpython3.4.so
    PY_INC=$INSTALL_PATH/include/python3.4
    USE_PY3=true
  else
    PY_EXE=$INSTALL_PATH/bin/python2
    PY_LIB=$INSTALL_PATH/lib/libpython2.7.so
    PY_INC=$INSTALL_PATH/include/python2.7
    USE_PY3=false
  fi
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/shiboken.pc" ]; then
    setup_xslt || exit 1
    setup_python || exit 1
    setup_qt || exit 1
    setup_cmake || exit 1
    if [ ! -f $SRC_PATH/$SHIBOK_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$SHIBOK_TAR -o $SRC_PATH/$SHIBOK_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$SHIBOK_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/shiboken-* || exit 1
    mkdir -p build && cd build || exit 1
    cmake ../ -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH  \
          -DCMAKE_BUILD_TYPE=Release   \
          -DBUILD_TESTS=OFF            \
          -DPYTHON_EXECUTABLE=$PY_EXE \
          -DPYTHON_LIBRARY=$PY_LIB \
          -DPYTHON_INCLUDE_DIR=$PY_INC \
          -DUSE_PYTHON3=$USE_PY3 \
          -DQT_QMAKE_EXECUTABLE=$INSTALL_PATH/bin/qmake -DCMAKE_EXE_LINKER_FLAGS="-lz"
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}

function setup_pyside {
  if [ "$PYV" = "3" ]; then
    export PYTHON_PATH=$INSTALL_PATH/lib/python3.4
    export PYTHON_INCLUDE=$INSTALL_PATH/include/python3.4
    PY_EXE=$INSTALL_PATH/bin/python3
    PY_LIB=$INSTALL_PATH/lib/libpython3.4.so
    PY_INC=$INSTALL_PATH/include/python3.4
    USE_PY3=true
  else
    PY_EXE=$INSTALL_PATH/bin/python2
    PY_LIB=$INSTALL_PATH/lib/libpython2.7.so
    PY_INC=$INSTALL_PATH/include/python2.7
    USE_PY3=false
  fi
  if [ ! -f "$INSTALL_PATH/lib/pkgconfig/pyside.pc" ]; then
    setup_python || exit 1
    setup_qt || exit 1
    setup_shiboken || exit 1
    setup_cmake || exit 1
    if [ ! -f $SRC_PATH/$PYSIDE_TAR ]; then
        curl $THIRD_PARTY_SRC_URL/$PYSIDE_TAR -o $SRC_PATH/$PYSIDE_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$PYSIDE_TAR -C "$TMP_PATH" || exit 1
    cd $TMP_PATH/pyside-* || exit 1
    mkdir -p build && cd build || exit 1
    cmake .. -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF \
          -DQT_QMAKE_EXECUTABLE=$INSTALL_PATH/bin/qmake \
          -DPYTHON_EXECUTABLE=$PY_EXE \
          -DPYTHON_LIBRARY=$PY_LIB \
          -DPYTHON_INCLUDE_DIR=$PY_INC
    make -j${MKJOBS} || exit 1
    make install || exit 1
  fi
}
