#!/bin/bash
# install libmad (for sox)
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/libmad.html
LIBMAD_VERSION=0.15.1b
LIBMAD_TAR="libmad-${LIBMAD_VERSION}.tar.gz"
LIBMAD_SITE="https://downloads.sourceforge.net/mad"
if download_step; then
    download "$LIBMAD_SITE" "$LIBMAD_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/mad.pc" ] || [ "$(pkg-config --modversion mad)" != "$LIBMAD_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBMAD_TAR"
    pushd "libmad-${LIBMAD_VERSION}"
    # patch from http://www.linuxfromscratch.org/patches/blfs/svn/libmad-0.15.1b-fixes-1.patch
    patch -Np1 -i "$INC_PATH/patches/libmad/libmad-0.15.1b-fixes-1.patch"
    sed "s@AM_CONFIG_HEADER@AC_CONFIG_HEADERS@g" -i configure.ac
    touch NEWS AUTHORS ChangeLog
    autoreconf -fi
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    cat > "${SDK_HOME}/lib/pkgconfig/mad.pc" << EOF
prefix=${SDK_HOME}
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: mad
Description: MPEG audio decoder
Requires:
Version: ${LIBMAD_VERSION}
Libs: -L\${libdir} -lmad
Cflags: -I\${includedir}
EOF
    popd
    rm -rf "libmad-${LIBMAD_VERSION}"
    end_build
fi
