#!/bin/bash

# Install libjpeg-turbo
# see http://www.linuxfromscratch.org/blfs/view/svn/general/libjpeg.html
LIBJPEGTURBO_VERSION=1.5.3
LIBJPEGTURBO_TAR="libjpeg-turbo-${LIBJPEGTURBO_VERSION}.tar.gz"
LIBJPEGTURBO_SITE="https://sourceforge.net/projects/libjpeg-turbo/files/${LIBJPEGTURBO_VERSION}"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libturbojpeg.so.0.1.0" ]; }; }; then
    start_build
    download "$LIBJPEGTURBO_SITE" "$LIBJPEGTURBO_TAR"
    untar "$SRC_PATH/$LIBJPEGTURBO_TAR"
    pushd "libjpeg-turbo-${LIBJPEGTURBO_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --with-jpeg8 --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libjpeg-turbo-${LIBJPEGTURBO_VERSION}"
    end_build
fi
