#!/bin/bash

# Install lcms2
# see www.linuxfromscratch.org/blfs/view/cvs/general/lcms2.html
LCMS_VERSION=2.9
LCMS_TAR="lcms2-${LCMS_VERSION}.tar.gz"
LCMS_SITE="https://sourceforge.net/projects/lcms/files/lcms/${LCMS_VERSION}"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/lcms2.pc" ] || [ "$(pkg-config --modversion lcms2)" != "$LCMS_VERSION" ]; }; }; then
    start_build
    download "$LCMS_SITE" "$LCMS_TAR"
    untar "$SRC_PATH/$LCMS_TAR"
    pushd "lcms2-${LCMS_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "lcms2-${LCMS_VERSION}"
    end_build
fi
