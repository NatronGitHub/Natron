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

#
# Build Natron SDK (CY2015) for Linux
#

#options:
#TAR_SDK=1 : Make an archive of the SDK when building is done and store it in $SRC_PATH
#UPLOAD_SDK=1 : Upload the SDK tar archive to $REPO_DEST if TAR_SDK=1

source `pwd`/common.sh || exit 1

PID=$$
if [ -f "$TMP_DIR/natron-build-sdk.pid" ]; then
    OLDPID=`cat "$TMP_DIR/natron-build-sdk.pid"`
    PIDS=`ps aux|awk '{print $2}'`
    for i in $PIDS;do
        if [ "$i" = "$OLDPID" ]; then
            echo "already running ..."
            exit 1
        fi
    done
fi
echo $PID > $TMP_DIR/natron-build-sdk.pid || exit 1

BINARIES_URL=$REPO_DEST/Third_Party_Binaries
SDK=Linux-$ARCH-SDK

if [ -z "$MKJOBS" ]; then
    #Default to 4 threads
    MKJOBS=$DEFAULT_MKJOBS
fi

echo
echo "Building Natron-$SDK_VERSION-$SDK with $MKJOBS threads ..."
echo
sleep 2

if [ ! -z "$REBUILD" ]; then
    if [ -d "$INSTALL_PATH" ]; then
        rm -rf "$INSTALL_PATH" || exit 1
    fi
else
    echo "Rebuilding ..."
fi
if [ -d "$TMP_PATH" ]; then
    rm -rf "$TMP_PATH" || exit 1
fi
mkdir -p "$TMP_PATH" || exit 1
if [ ! -d "$SRC_PATH" ]; then
    mkdir -p "$SRC_PATH" || exit 1
fi

# check for dirs
if [ ! -d "$INSTALL_PATH" ]; then
    mkdir -p "$INSTALL_PATH" || exit 1
fi
if [ ! -h "$INSTALL_PATH/lib64" ]; then
    if [ ! -d "$INSTALL_PATH/lib" ]; then
        mkdir -p "$INSTALL_PATH/lib" || exit 1
    fi
    cd "$INSTALL_PATH" || exit 1
    ln -sf lib lib64 || exit 1
fi

# Install ssl for installler
if [ ! -f "$INSTALL_PATH/lib/libcrypto.a" ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$SSL_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$SSL_TAR -O $SRC_PATH/$SSL_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$SSL_TAR || exit 1
    cd openssl* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./config --prefix=$INSTALL_PATH || exit 1
    make || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
    cd .. && rm -rf openssl*
fi

# Install static qt4 for installer
if [ ! -f "$INSTALL_PATH/installer/bin/qmake" ]; then
    cd "$TMP_PATH" || exit 1
    QTIFW_CONF="-no-multimedia -no-gif -qt-libpng -no-opengl -no-libmng -no-libtiff -no-libjpeg -static -openssl-linked -confirm-license -release -opensource -nomake demos -nomake docs -nomake examples -no-gtkstyle -no-webkit -no-avx -I${INSTALL_PATH}/include -L${INSTALL_PATH}/lib"

    if [ ! -f "$SRC_PATH/$QT4_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$QT4_TAR" -O "$SRC_PATH/$QT4_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$QT4_TAR" || exit 1
    cd qt*4.8* || exit 1
    ./configure -prefix $INSTALL_PATH/installer $QTIFW_CONF -v || exit 1

    # https://bugreports.qt-project.org/browse/QTBUG-5385
    env LD_LIBRARY_PATH="$LD_LIBRARY_PATH":`pwd`/lib make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make INSTALL_ROOT="${DDIR}" install || exit 1
    fi
    cd .. || exit 1
    rm -rf qt*4.8*
fi

# Install setup tools
if [ ! -f $INSTALL_PATH/installer/bin/binarycreator ]; then
    cd $TMP_PATH || exit 1
    git clone $GIT_INSTALLER || exit 1
    cd qtifw || exit 1
    git checkout natron || exit 1
    $INSTALL_PATH/installer/bin/qmake || exit 1
    make -j${MKJOBS} || exit 1
    strip -s bin/*
    cp bin/* $INSTALL_PATH/installer/bin/ || exit 1
    if [ "$DDIR" != "" ]; then
      cp bin/* "${DDIR}/installer/bin/" || exit 1
    fi
fi

# install patchelf
if [ ! -f "$INSTALL_PATH/bin/patchelf" ]; then
  cd "$TMP_PATH" || exit 1
  if [ ! -f "$SRC_PATH/$ELF_TAR" ]; then
    wget "$THIRD_PARTY_SRC_URL/$ELF_TAR" -O "$SRC_PATH/$ELF_TAR" || exit 1
  fi
  tar xvf $SRC_PATH/$ELF_TAR || exit 1
  cd patchelf* || exit 1
  ./configure || exit 1
  make || exit 1
  cp src/patchelf "$INSTALL_PATH/bin/" || exit 1
  if [ "$DDIR" != "" ]; then
    cp src/patchelf "$DDIR/bin/" || exit 1
  fi
fi

# Install gcc
if [ ! -f "$INSTALL_PATH/gcc/bin/gcc" ]; then
    cd "$TMP_PATH" || exit 1
    if [ ! -f "$SRC_PATH/$GCC_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$GCC_TAR" -O "$SRC_PATH/$GCC_TAR" || exit 1
    fi
    if [ ! -f "$SRC_PATH/$MPC_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$MPC_TAR" -O "$SRC_PATH/$MPC_TAR" || exit 1
    fi
    if [ ! -f "$SRC_PATH/$MPFR_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$MPFR_TAR" -O "$SRC_PATH/$MPFR_TAR" || exit 1
    fi
    if [ ! -f "$SRC_PATH/$GMP_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$GMP_TAR" -O "$SRC_PATH/$GMP_TAR" || exit 1
    fi
    if [ ! -f "$SRC_PATH/$ISL_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$ISL_TAR" -O "$SRC_PATH/$ISL_TAR" || exit 1
    fi
    if [ ! -f $SRC_PATH/$CLOOG_TAR ]; then
        wget "$THIRD_PARTY_SRC_URL/$CLOOG_TAR" -O "$SRC_PATH/$CLOOG_TAR" || exit 1
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
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

export LD_LIBRARY_PATH="$INSTALL_PATH/lib"
export PATH="$INSTALL_PATH/gcc/bin:$INSTALL_PATH/bin:$INSTALL_PATH/cmake/bin:$PATH"

if [ "$ARCH" = "x86_64" ]; then
    LD_LIBRARY_PATH="$INSTALL_PATH/gcc/lib64:$LD_LIBRARY_PATH"
else
    LD_LIBRARY_PATH="$INSTALL_PATH/gcc/lib:$LD_LIBRARY_PATH"
fi
export LD_LIBRARY_PATH
export CC="${INSTALL_PATH}/gcc/bin/gcc"
export CXX="${INSTALL_PATH}/gcc/bin/g++"

# Install zlib
if [ ! -f "$INSTALL_PATH/lib/libz.so.1" ]; then
    cd "$TMP_PATH" || exit 1
    if [ ! -f "$SRC_PATH/$ZLIB_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$ZLIB_TAR" -O "$SRC_PATH/$ZLIB_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$ZLIB_TAR" || exit 1
    cd zlib* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
    #rm -f $INSTALL_PATH/lib/libz.so*
fi

# Install libzip
if [ ! -f "$INSTALL_PATH/lib/pkgconfig/libzip.pc" ]; then
    cd "$TMP_PATH" || exit 1
    if [ ! -f "$SRC_PATH/$ZIP_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$ZIP_TAR" -O "$SRC_PATH/$ZIP_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$ZIP_TAR" || exit 1
    cd libzip* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-static --enable-shared || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install bzip
if [ ! -f "$INSTALL_PATH/lib/libbz2.so.1" ]; then
    cd "$TMP_PATH" || exit 1
    if [ ! -f "$SRC_PATH/$BZIP_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$BZIP_TAR" -O "$SRC_PATH/$BZIP_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$BZIP_TAR" || exit 1
    cd bzip* || exit 1
    sed -e 's/^CFLAGS=\(.*\)$/CFLAGS=\1 \$(BIGFILES)/' -i ./Makefile-libbz2_so || exit 1
    sed -i "s/libbz2.so.1.0 -o libbz2.so.1.0.6/libbz2.so.1 -o libbz2.so.1.0.6/;s/rm -f libbz2.so.1.0/rm -f libbz2.so.1/;s/ln -s libbz2.so.1.0.6 libbz2.so.1.0/ln -s libbz2.so.1.0.6 libbz2.so.1/" Makefile-libbz2_so || exit 1
    make -f Makefile-libbz2_so || exit 1
    install -m755 libbz2.so.1.0.6 $INSTALL_PATH/lib || exit 1
    install -m644 bzlib.h $INSTALL_PATH/include/ || exit 1
    cd $INSTALL_PATH/lib || exit 1
    ln -s libbz2.so.1.0.6 libbz2.so || exit 1
    ln -s libbz2.so.1.0.6 libbz2.so.1 || exit 1
fi

# Install ssl
if [ ! -f "$INSTALL_PATH/lib/libcrypto.so" ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$SSL_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$SSL_TAR -O $SRC_PATH/$SSL_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$SSL_TAR || exit 1
    cd openssl* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./config --prefix=$INSTALL_PATH -shared || exit 1
    make || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install yasm
if [ ! -f "$INSTALL_PATH/bin/yasm" ]; then
    cd "$TMP_PATH" || exit 1
    if [ ! -f "$SRC_PATH/$YASM_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$YASM_TAR" -O "$SRC_PATH/$YASM_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$YASM_TAR" || exit 1
    cd yasm* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install cmake
if [ ! -f "$INSTALL_PATH/cmake/bin/cmake" ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f "$SRC_PATH/$CMAKE_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$CMAKE_TAR" -O "$SRC_PATH/$CMAKE_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$CMAKE_TAR" || exit 1
    cd cmake* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH/cmake" || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install icu
if [ ! -f "$INSTALL_PATH/lib/pkgconfig/icu-i18n.pc" ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f "$SRC_PATH/$ICU_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$ICU_TAR" -O "$SRC_PATH/$ICU_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$ICU_TAR" || exit 1
    cd icu/source || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --enable-shared || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install Python2
if [ ! -f "$INSTALL_PATH/lib/pkgconfig/python2.pc" ]; then
    cd "$TMP_PATH" || exit 1
    if [ ! -f "$SRC_PATH/$PY2_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$PY2_TAR" -O "$SRC_PATH/$PY2_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$PY2_TAR" || exit 1
    cd Python-2* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix="$INSTALL_PATH" --enable-shared  || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install Python3
if [ ! -f "$INSTALL_PATH/lib/pkgconfig/python3.pc" ] && [ "$PYV" = "3" ]; then
    cd "$TMP_PATH" || exit 1
    if [ ! -f "$SRC_PATH/$PY3_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$PY3_TAR" -O "$SRC_PATH/$PY3_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$PY3_TAR" || exit 1
    cd Python-3* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --enable-shared || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Setup env
PKG_CONFIG_PATH="$INSTALL_PATH/lib/pkgconfig"
QTDIR="$INSTALL_PATH"
BOOST_ROOT="$INSTALL_PATH"
OPENJPEG_HOME="$INSTALL_PATH"
THIRD_PARTY_TOOLS_HOME="$INSTALL_PATH"
PYTHON_HOME="$INSTALL_PATH"
PYTHON_PATH="$INSTALL_PATH/lib/python2.7"
PYTHON_INCLUDE="$INSTALL_PATH/include/python2.7"
export PKG_CONFIG_PATH LD_LIBRARY_PATH PATH QTDIR BOOST_ROOT OPENJPEG_HOME THIRD_PARTY_TOOLS_HOME PYTHON_HOME PYTHON_PATH PYTHON_INCLUDE

# Install expat
if [ ! -f "$INSTALL_PATH/lib/pkgconfig/expat.pc" ]; then
    cd "$TMP_PATH" || exit 1
    if [ ! -f "$SRC_PATH/$EXPAT_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$EXPAT_TAR" -O "$SRC_PATH/$EXPAT_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$EXPAT_TAR" || exit 1
    cd expat* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-static --enable-shared || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install png
if [ "$REBUILD_PNG" = "1" ]; then
  rm -rf $INSTALL_PATH/{include,lib}/*png*
  rm -f $INSTALL_PATH/lib/pkgconfig/*png*
fi
if [ ! -f $INSTALL_PATH/lib/pkgconfig/libpng.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$PNG_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$PNG_TAR -O $SRC_PATH/$PNG_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$PNG_TAR || exit 1
    cd libpng* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --disable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install freetype
if [ ! -f "$INSTALL_PATH/lib/pkgconfig/freetype2.pc" ]; then
    cd "$TMP_PATH" || exit 1
    if [ ! -f "$SRC_PATH/$FTYPE_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$FTYPE_TAR" -O "$SRC_PATH/$FTYPE_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$FTYPE_TAR" || exit 1
    cd freetype* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-static --enable-shared || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install fontconfig
if [ ! -f "$INSTALL_PATH/lib/pkgconfig/fontconfig.pc" ]; then
    cd "$TMP_PATH" || exit 1
    if [ ! -f "$SRC_PATH/$FCONFIG_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$FCONFIG_TAR" -O "$SRC_PATH/$FCONFIG_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$FCONFIG_TAR" || exit 1
    cd fontconfig* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-docs --disable-static --enable-shared || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install ffi
if [ ! -f "$INSTALL_PATH/lib/pkgconfig/libffi.pc" ]; then
    cd "$TMP_PATH" || exit 1
    if [ ! -f "$SRC_PATH/$FFI_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$FFI_TAR" -O "$SRC_PATH/$FFI_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$FFI_TAR" || exit 1
    cd libffi* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-docs --disable-static --enable-shared || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install glib
if [ ! -f "$INSTALL_PATH/lib/pkgconfig/glib-2.0.pc" ]; then
    cd "$TMP_PATH" || exit 1
    if [ ! -f "$SRC_PATH/$GLIB_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$GLIB_TAR" -O "$SRC_PATH/$GLIB_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$GLIB_TAR" || exit 1
    cd glib-2* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-docs --disable-static --enable-shared || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install xml
if [ ! -f "$INSTALL_PATH/lib/pkgconfig/libxml-2.0.pc" ]; then
    cd "$TMP_PATH" || exit 1
    if [ ! -f "$SRC_PATH/$LIBXML_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$LIBXML_TAR" -O "$SRC_PATH/$LIBXML_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$LIBXML_TAR" || exit 1
    cd libxml* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-docs --disable-static --enable-shared --without-python --without-lzma || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install xslt
if [ ! -f "$INSTALL_PATH/lib/pkgconfig/libxslt.pc" ]; then
    cd "$TMP_PATH" || exit 1
    if [ ! -f "$SRC_PATH/$LIBXSLT_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$LIBXSLT_TAR" -O "$SRC_PATH/$LIBXSLT_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$LIBXSLT_TAR" || exit 1
    cd libxslt* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-docs --disable-static --enable-shared --without-python || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install boost
if [ ! -f "$INSTALL_PATH/lib/libboost_atomic.so" ]; then
    cd "$TMP_PATH" || exit 1
    if [ ! -f "$SRC_PATH/$BOOST_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$BOOST_TAR" -O "$SRC_PATH/$BOOST_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$BOOST_TAR" || exit 1
    cd boost_* || exit 1
    ./bootstrap.sh || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./b2 -s NO_BZIP2=1 --prefix=$INSTALL_PATH cflags="-fPIC" --disable-icu -j${MKJOBS} install || exit 1 # link=static
fi

# Install jpeg
if [ ! -f $INSTALL_PATH/lib/libjpeg.so ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$JPG_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$JPG_TAR -O $SRC_PATH/$JPG_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$JPG_TAR || exit 1
    cd jpeg-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --disable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install gif
if [ ! -f $INSTALL_PATH/lib/libgif.so ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$GIF_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$GIF_TAR -O $SRC_PATH/$GIF_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$GIF_TAR || exit 1
    cd giflib-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --disable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi


# Install tiff
if [ "$REBUILD_TIFF" = "1" ]; then
  rm -rf $INSTALL_PATH/{lib,include}/*tiff*
  rm -f $INSTALL_PATH/lib/pkgconfig/*tiff*
fi
if [ ! -f $INSTALL_PATH/lib/pkgconfig/libtiff-4.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$TIF_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$TIF_TAR -O $SRC_PATH/$TIF_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$TIF_TAR || exit 1
    cd tiff-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --disable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install jasper
if [ "$REBUILD_JASPER" = "1" ]; then
  rm -rf $INSTALL_PATH/{include,lib}/*jasper*
fi
if [ ! -f $INSTALL_PATH/lib/libjasper.so ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$JASP_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$JASP_TAR -O $SRC_PATH/$JASP_TAR || exit 1
    fi
    unzip $SRC_PATH/$JASP_TAR || exit 1
    cd jasper* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --disable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install lcms
if [ ! -f $INSTALL_PATH/lib/pkgconfig/lcms2.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$LCMS_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$LCMS_TAR -O $SRC_PATH/$LCMS_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$LCMS_TAR || exit 1
    cd lcms2-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --disable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install openjpeg
if [ ! -f $INSTALL_PATH/lib/pkgconfig/libopenjpeg.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$OJPG_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$OJPG_TAR -O $SRC_PATH/$OJPG_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$OJPG_TAR || exit 1
    cd openjpeg-* || exit 1
    ./bootstrap.sh || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --disable-shared --disable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install libraw
if [ ! -f $INSTALL_PATH/lib/pkgconfig/libraw.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$LIBRAW_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$LIBRAW_TAR -O $SRC_PATH/$LIBRAW_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$LIBRAW_TAR || exit 1
    cd LibRaw* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --disable-static --enable-shared || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install openexr
if [ "$REBUILD_EXR" = "1" ]; then
  rm -rf $INSTALL_PATH/lib/libI* $INSTALL_PATH/lib/libHalf*
  rm -f $INSTALL_PATH/lib/pkgconfig/{OpenEXR,IlmBase}.pc
  rmn -f $INSTALL_PATH/bin/exr*
fi
if [ ! -f $INSTALL_PATH/lib/pkgconfig/OpenEXR.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$ILM_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$ILM_TAR -O $SRC_PATH/$ILM_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$ILM_TAR || exit 1
    cd ilmbase-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --disable-shared --enable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi

    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$EXR_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$EXR_TAR -O $SRC_PATH/$EXR_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$EXR_TAR || exit 1
    cd openexr-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --disable-shared --enable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install pixman
if [ "$REBUILD_PIXMAN" = "1" ]; then
  rm -rf $INSTALL_PATH/{lib,include}/*pixman*
  rm -f $INSTALL_PATH/lib/pkgconfig/*pixman*
fi
if [ ! -f $INSTALL_PATH/lib/pkgconfig/pixman-1.pc ]; then
    if [ ! -f $SRC_PATH/$PIX_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$PIX_TAR -O $SRC_PATH/$PIX_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$PIX_TAR || exit 1
    cd pixman-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --disable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install cairo
if [ "$REBUILD_CAIRO" = "1" ]; then
  rm -rf $INSTALL_PATH/include/cairo
  rm -f $INSTALL_PATH/lib/pkgconfig/{cairo-*.pc,cairo.pc}
  rm -f $INSTALL_PATH/lib/libcairo*
fi
if [ ! -f $INSTALL_PATH/lib/pkgconfig/cairo.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$CAIRO_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$CAIRO_TAR -O $SRC_PATH/$CAIRO_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$CAIRO_TAR || exit 1
    cd cairo-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include -I${INSTALL_PATH}/include/pixman-1" LDFLAGS="-L${INSTALL_PATH}/lib -lpixman-1" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --enable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install harbuzz
if [ ! -f $INSTALL_PATH/lib/pkgconfig/harfbuzz.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$BUZZ_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$BUZZ_TAR -O $SRC_PATH/$BUZZ_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$BUZZ_TAR || exit 1
    cd harfbuzz-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --disable-docs --disable-static --enable-shared --with-freetype --with-cairo --with-gobject --with-glib --without-icu || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install pango
if [ ! -f "$INSTALL_PATH/lib/pkgconfig/pango.pc" ]; then
    cd "$TMP_PATH" || exit 1
    if [ ! -f "$SRC_PATH/$PANGO_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$PANGO_TAR" -O "$SRC_PATH/$PANGO_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$PANGO_TAR" || exit 1
    cd pango-* || exit 1
    env FONTCONFIG_CFLAGS="-I$INSTALL_PATH/include" FONTCONFIG_LIBS="-L$INSTALL_PATH/lib -lfontconfig" CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-docs --disable-static --enable-shared --with-included-modules=basic-fc || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install croco
if [ ! -f "$INSTALL_PATH/lib/libcroco-0.6.so" ]; then
    cd "$TMP_PATH" || exit 1
    if [ ! -f "$SRC_PATH/$CROCO_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$CROCO_TAR" -O "$SRC_PATH/$CROCO_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$CROCO_TAR" || exit 1
    cd libcroco-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-docs --disable-static --enable-shared || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install gdk
if [ ! -f "$INSTALL_PATH/lib/pkgconfig/gdk-pixbuf-2.0.pc" ]; then
    cd "$TMP_PATH" || exit 1
    if [ ! -f "$SRC_PATH/$GDK_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$GDK_TAR" -O "$SRC_PATH/$GDK_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$GDK_TAR" || exit 1
    cd gdk-pix* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix="$INSTALL_PATH" --disable-docs --disable-static --enable-shared --without-libtiff || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install svg
if [ ! -f "$INSTALL_PATH/lib/pkgconfig/librsvg-2.0.pc" ]; then
    cd "$TMP_PATH" || exit 1
    if [ ! -f "$SRC_PATH/$SVG_TAR" ]; then
        wget "$THIRD_PARTY_SRC_URL/$SVG_TAR" -O "$SRC_PATH/$SVG_TAR" || exit 1
    fi
    tar xvf "$SRC_PATH/$SVG_TAR" || exit 1
    cd librsvg-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$INSTALL_PATH" --disable-docs --disable-static --enable-shared --disable-introspection --disable-vala --disable-pixbuf-loader || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
fi

# Install magick
if [ "$REBUILD_MAGICK" = "1" ]; then
    rm -rf $INSTALL_PATH/include/ImageMagick-6/ $INSTALL_PATH/lib/libMagick* $INSTALL_PATH/share/ImageMagick-6/ $INSTALL_PATH/lib/pkgconfig/{Image,Magick}*
fi
if [ ! -f $INSTALL_PATH/lib/pkgconfig/Magick++.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$MAGICK_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$MAGICK_TAR -O $SRC_PATH/$MAGICK_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$MAGICK_TAR || exit 1
    cd ImageMagick-* || exit 1
    patch -p0 < $INC_PATH/patches/ImageMagick/pango-align-hack.diff || exit 1
    patch -p0 < $INC_PATH/patches/ImageMagick/xcf-layername.diff || exit 1
    env CFLAGS="$BF -DMAGICKCORE_EXCLUDE_DEPRECATED=1" CXXFLAGS="$BF -DMAGICKCORE_EXCLUDE_DEPRECATED=1" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --with-magick-plus-plus=yes --with-quantum-depth=32 --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --with-lcms --without-openjp2 --without-lqr --without-lzma --without-openexr --with-pango --with-png --with-rsvg --without-tiff --without-webp --with-xml --without-zlib --without-bzlib --disable-static --enable-shared --enable-hdri --with-freetype --with-fontconfig --without-x --without-modules || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi    
fi

# Install glew
if [ ! -f $INSTALL_PATH/lib/pkgconfig/glew.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$GLEW_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$GLEW_TAR -O $SRC_PATH/$GLEW_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$GLEW_TAR || exit 1
    cd glew-* || exit 1
    if [ "$ARCH" = "i686" ]; then
        make -j${MKJOBS} 'CFLAGS.EXTRA=-O2 -g -march=i686 -mtune=i686' includedir=/usr/include GLEW_DEST= libdir=/usr/lib bindir=/usr/bin || exit 1
    else
        make -j${MKJOBS} 'CFLAGS.EXTRA=-O2 -g -m64 -fPIC -mtune=generic' includedir=/usr/include GLEW_DEST= libdir=/usr/lib64 bindir=/usr/bin || exit 1
    fi
    make install GLEW_DEST=$INSTALL_PATH libdir=/lib bindir=/bin includedir=/include || exit 1
    #rm -f $INSTALL_PATH/lib/*GLEW*.so*
fi

# Install ocio
if [ ! -f $INSTALL_PATH/lib/libOpenColorIO.so ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$OCIO_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$OCIO_TAR -O $SRC_PATH/$OCIO_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$OCIO_TAR || exit 1
    cd OpenColorIO-* || exit 1
    mkdir build || exit 1
    cd build || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" cmake .. -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH -DCMAKE_BUILD_TYPE=Release -DOCIO_BUILD_SHARED=ON -DOCIO_BUILD_STATIC=OFF -DOCIO_BUILD_APPS=OFF -DOCIO_BUILD_PYGLUE=OFF || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    cp ext/dist/lib/{liby*.a,libt*.a} $INSTALL_PATH/lib/ || exit 1
    sed -i "s/-lOpenColorIO/-lOpenColorIO -lyaml-cpp -ltinyxml -llcms2/" $INSTALL_PATH/lib/pkgconfig/OpenColorIO.pc || exit 1
fi

# Install oiio
if [ "$REBUILD_OIIO" = "1" ]; then
    rm -rf $INSTALL_PATH/lib/libOpenImage* $INSTALL_PATH/include/OpenImage*
fi
if [ ! -f $INSTALL_PATH/lib/libOpenImageIO.so ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$OIIO_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$OIIO_TAR -O $SRC_PATH/$OIIO_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$OIIO_TAR || exit 1
    cd oiio-Release-* || exit 1
    patch -p1 -i $INC_PATH/patches/OpenImageIO/oiio-exrthreads.patch || exit 1
    mkdir build || exit 1
    cd build || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" CXXFLAGS="-fPIC" cmake -DUSE_OPENCV:BOOL=FALSE -DUSE_OPENSSL:BOOL=FALSE -DOPENEXR_HOME=$INSTALL_PATH -DILMBASE_HOME=$INSTALL_PATH -DTHIRD_PARTY_TOOLS_HOME=$INSTALL_PATH -DUSE_QT:BOOL=FALSE -DUSE_TBB:BOOL=FALSE -DUSE_PYTHON:BOOL=FALSE -DUSE_FIELD3D:BOOL=FALSE -DOIIO_BUILD_TESTS=0 -DOIIO_BUILD_TOOLS=1 -DUSE_LIB_RAW=1 -DLIBRAW_PATH=$INSTALL_PATH -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH -DBOOST_ROOT=$INSTALL_PATH -DSTOP_ON_WARNING:BOOL=FALSE -DUSE_GIF:BOOL=TRUE -DUSE_FREETYPE:BOOL=TRUE -DFREETYPE_INCLUDE_PATH=$INSTALL_PATH/include -DUSE_FFMPEG:BOOL=FALSE -DLINKSTATIC=0 -DBUILDSTATIC=0 -DOPENJPEG_HOME=$INSTALL_PATH -DOPENJPEG_INCLUDE_DIR=$INSTALL_PATH/include/openjpeg-1.5 -DOPENJPEG_LIBRARIES=$INSTALL_PATH/lib -DUSE_OPENJPEG:BOOL=TRUE .. || exit 1 
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install SeExpr
if [ ! -f $INSTALL_PATH/lib/libSeExpr.so ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$SEE_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$SEE_TAR -O $SRC_PATH/$SEE_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$SEE_TAR || exit 1
    cd SeExpr-* || exit 1
    mkdir build || exit 1
    cd build || exit 1
    CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" cmake .. -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH || exit 1
    make || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
    #rm -f $INSTALL_PATH/lib/libSeExpr.so
fi

# Install eigen
#if [ ! -f $INSTALL_PATH/lib/pkgconfig/eigen3.pc ]; then
#    cd $TMP_PATH || exit 1
#    if [ ! -f $CWD/src/$EIGEN_TAR ]; then
#        wget $THIRD_PARTY_SRC_URL/$EIGEN_TAR -O $CWD/src/$EIGEN_TAR || exit 1
#    fi
#    tar xvf $CWD/src/$EIGEN_TAR || exit 1
#    cd eigen-* || exit 1
#    rm -rf build
#    mkdir build || exit 1
#    cd build || exit 1
#    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" cmake .. -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH || exit 1
#    make -j${MKJOBS} || exit 1
#    make install || exit 1
#    mv $INSTALL_PATH/share/pkgconfig/* $INSTALL_PATH/lib/pkgconfig
#fi

# Install opencv
#if [ ! -f $INSTALL_PATH/lib/pkgconfig/opencv.pc ]; then
#    cd $TMP_PATH || exit 1
#    if [ ! -f $CWD/src/$CV_TAR ]; then
#        wget $THIRD_PARTY_SRC_URL/$CV_TAR -O $CWD/src/$CV_TAR || exit 1
#    fi
#    unzip $CWD/src/$CV_TAR || exit 1
#    cd opencv* || exit 1
#    mkdir build || exit 1
#    cd build || exit 1
#    env CFLAGS="$BF" CXXFLAGS="$BF" CMAKE_INCLUDE_PATH=$INSTALL_PATH/include CMAKE_LIBRARY_PATH=$INSTALL_PATH/lib CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" cmake -DWITH_GTK=OFF -DWITH_GSTREAMER=OFF -DWITH_FFMPEG=OFF -DWITH_OPENEXR=OFF -DWITH_OPENCL=OFF -DWITH_OPENGL=ON -DBUILD_WITH_DEBUG_INFO=OFF -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=Release -DENABLE_SSE3=OFF .. -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH || exit 1
#    make -j${MKJOBS} || exit 1
#    make install || exit 1
#    mkdir -p $INSTALL_PATH/docs/opencv || exit 1
#    cp ../LIC* ../COP* ../README ../AUTH* ../CONT* $INSTALL_PATH/docs/opencv/
#fi


# Install lame
if [ ! -f $INSTALL_PATH/lib/libmp3lame.so ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$LAME_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$LAME_TAR -O $SRC_PATH/$LAME_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$LAME_TAR || exit 1
    cd lame-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --enable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install ogg
if [ ! -f $INSTALL_PATH/lib/pkgconfig/ogg.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$OGG_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$OGG_TAR -O $SRC_PATH/$OGG_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$OGG_TAR || exit 1
    cd libogg-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --enable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install vorbis
if [ ! -f $INSTALL_PATH/lib/pkgconfig/vorbis.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$VORBIS_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$VORBIS_TAR -O $SRC_PATH/$VORBIS_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$VORBIS_TAR || exit 1
    cd libvorbis-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --disable-shared --enable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install theora
if [ ! -f $INSTALL_PATH/lib/pkgconfig/theora.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$THEORA_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$THEORA_TAR -O $SRC_PATH/$THEORA_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$THEORA_TAR || exit 1
    cd libtheora-* || exit 1
    sed -i 's/png_\(sizeof\)/\1/g' examples/png2theora.c # fix libpng16
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --disable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install modplug
if [ ! -f $INSTALL_PATH/lib/libmodplug.so ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$MODPLUG_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$MODPLUG_TAR -O $SRC_PATH/$MODPLUG_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$MODPLUG_TAR || exit 1
    cd libmodplug-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --disable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install vpx
if [ ! -f $INSTALL_PATH/lib/pkgconfig/vpx.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$VPX_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$VPX_TAR -O $SRC_PATH/$VPX_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$VPX_TAR || exit 1
    cd libvpx-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --enable-static --enable-vp8 --enable-vp9 --enable-runtime-cpu-detect --enable-postproc --enable-pic --disable-avx --disable-avx2 --disable-examples || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install speex (EOL, use opus)
if [ ! -f $INSTALL_PATH/lib/pkgconfig/speex.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$SPEEX_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$SPEEX_TAR -O $SRC_PATH/$SPEEX_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$SPEEX_TAR || exit 1
    cd speex-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --enable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install opus
if [ ! -f $INSTALL_PATH/lib/pkgconfig/opus.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$OPUS_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$OPUS_TAR -O $SRC_PATH/$OPUS_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$OPUS_TAR || exit 1
    cd opus-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --enable-static --enable-custom-modes || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install orc
if [ ! -f $INSTALL_PATH/lib/pkgconfig/orc-0.4.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$ORC_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$ORC_TAR -O $SRC_PATH/$ORC_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$ORC_TAR || exit 1
    cd orc-* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --enable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install dirac
if [ ! -f $INSTALL_PATH/lib/pkgconfig/schroedinger-1.0.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$DIRAC_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$DIRAC_TAR -O $SRC_PATH/$DIRAC_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$DIRAC_TAR || exit 1
    cd schro* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --enable-static || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    #rm -f $INSTALL_PATH/lib/libschro*.so*
    sed -i "s/-lschroedinger-1.0/-lschroedinger-1.0 -lorc-0.4/" $INSTALL_PATH/lib/pkgconfig/schroedinger-1.0.pc || exit 1
fi

# x264
if [ ! -f $INSTALL_PATH/lib/pkgconfig/x264.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$X264_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$X264_TAR -O $SRC_PATH/$X264_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$X264_TAR || exit 1
    cd x264* || exit 1
    ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --enable-static --enable-pic --bit-depth=10  CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
    #rm -f $INSTALL_PATH/lib/libx264*.so*
fi

# xvid
if [ ! -f $INSTALL_PATH/lib/libxvidcore.a ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$XVID_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$XVID_TAR -O $SRC_PATH/$XVID_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$XVID_TAR || exit 1
    cd xvidcore/build/generic || exit 1
    ./configure --prefix=$INSTALL_PATH --libdir=$INSTALL_PATH/lib --enable-shared --enable-static CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
    #rm -f $INSTALL_PATH/lib/libxvidcore*.so*
fi

# Install ffmpeg
LGPL_SETTINGS="--enable-avresample --enable-libmp3lame --enable-libvorbis --enable-libopus --enable-libtheora --enable-libschroedinger --enable-libopenjpeg --enable-libmodplug --enable-libvpx --disable-libspeex --disable-libxcb --disable-libxcb-shm --disable-libxcb-xfixes --disable-indev=jack --disable-outdev=xv --disable-vda --disable-xlib"
GPL_SETTINGS="${LGPL_SETTINGS} --enable-gpl --enable-libx264 --enable-libxvid --enable-version3"

if [ "$REBUILD_FFMPEG" = "1" ]; then
    rm -rf $INSTALL_PATH/ffmpeg-*
fi
if [ ! -d $INSTALL_PATH/ffmpeg-gpl ] || [ ! -d $INSTALL_PATH/ffmpeg-lgpl ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$FFMPEG_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$FFMPEG_TAR -O $SRC_PATH/$FFMPEG_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$FFMPEG_TAR || exit 1
    cd ffmpeg-2* || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH/ffmpeg-gpl --libdir=$INSTALL_PATH/ffmpeg-gpl/lib --enable-shared --disable-static $GPL_SETTINGS || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
    make distclean
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure --prefix=$INSTALL_PATH/ffmpeg-lgpl --libdir=$INSTALL_PATH/ffmpeg-lgpl/lib --enable-shared --disable-static $LGPL_SETTINGS  || exit 1
    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make DESTDIR="${DDIR}" install || exit 1
    fi
fi

# Install qt
if [ ! -f $INSTALL_PATH/bin/qmake ]; then
    cd $TMP_PATH || exit 1
    if [ "$1" = "qt5" ]; then
        QT_TAR=$QT5_TAR
        QT_CONF="-openssl-linked -opengl desktop -opensource -nomake examples -nomake tests -release -no-gtkstyle -confirm-license -no-c++11 -I${INSTALL_PATH}/include -L${INSTALL_PATH}/lib"
    else
        QT_TAR=$QT4_TAR
        QT_CONF="-system-zlib -system-libtiff -system-libpng -no-libmng -system-libjpeg -no-gtkstyle -glib -xrender -xrandr -xcursor -xfixes -xinerama -fontconfig -xinput -sm -no-multimedia -openssl-linked -confirm-license -release -opensource -opengl desktop -nomake demos -nomake docs -nomake examples -I${INSTALL_PATH}/include -L${INSTALL_PATH}/lib"
    fi

    if [ ! -f $SRC_PATH/$QT_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$QT_TAR -O $SRC_PATH/$QT_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$QT_TAR || exit 1
    cd qt* || exit 1
    QT_SRC=`pwd`/src
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" ./configure -prefix $INSTALL_PATH $QT_CONF -shared || exit 1

    # https://bugreports.qt-project.org/browse/QTBUG-5385
    LD_LIBRARY_PATH="$LD_LIBRARY_PATH":`pwd`/lib make -j${MKJOBS} || exit  1

    make install || exit 1
    if [ "$DDIR" != "" ]; then
      make INSTALL_ROOT="${DDIR}" install || exit 1
    fi
    rm -rf $TMP_PATH/qt*
fi

# pysetup
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

# Install shiboken
if [ ! -f $INSTALL_PATH/lib/pkgconfig/shiboken.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$SHIBOK_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$SHIBOK_TAR -O $SRC_PATH/$SHIBOK_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$SHIBOK_TAR || exit 1
    cd shiboken-* || exit 1
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

# Install pyside
if [ ! -f $INSTALL_PATH/lib/pkgconfig/pyside.pc ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $SRC_PATH/$PYSIDE_TAR ]; then
        wget $THIRD_PARTY_SRC_URL/$PYSIDE_TAR -O $SRC_PATH/$PYSIDE_TAR || exit 1
    fi
    tar xvf $SRC_PATH/$PYSIDE_TAR || exit 1
    cd pyside-* || exit 1
    mkdir -p build && cd build || exit 1
    cmake .. -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF \
          -DQT_QMAKE_EXECUTABLE=$INSTALL_PATH/bin/qmake \
          -DPYTHON_EXECUTABLE=$PY_EXE \
          -DPYTHON_LIBRARY=$PY_LIB \
          -DPYTHON_INCLUDE_DIR=$PY_INC
    make -j${MKJOBS} || exit 1 
    make install || exit 1
fi

# Install breakpad tools
if [ ! -f "${INSTALL_PATH}/bin/dump_syms" ]; then
  cd "$TMP_PATH" || exit 1
  rm -f google-breakpad
  git clone $GIT_BREAKPAD || exit 1
  cd google-breakpad || exit 1 
  ./configure || exit 1
  make || exit 1
  cp src/tools/linux/dump_syms/dump_syms "$INSTALL_PATH/bin/" || exit 1
fi

if [ ! -z "$TAR_SDK" ]; then
    # Done, make a tarball
    cd "$INSTALL_PATH/.." || exit 1
    tar cvvJf "$SRC_PATH/Natron-$SDK_VERSION-$SDK.tar.xz" "Natron-$SDK_VERSION" || exit 1
    echo "SDK available at $SRC_PATH/Natron-$SDK_VERSION-$SDK.tar.xz"

    if [ ! -z "$UPLOAD_SDK" ]; then
        rsync -avz --progress --verbose -e ssh "$SRC_PATH/Natron-$SDK_VERSION-$SDK.tar.xz" "$BINARIES_URL" || exit 1
    fi

fi

echo
echo "Natron SDK Done"
echo
exit 0

