#!/bin/bash

# Install dirac (obsolete since ffmpeg-3.4)
DIRAC_VERSION=1.0.11
DIRAC_TAR=schroedinger-${DIRAC_VERSION}.tar.gz
DIRAC_SITE="http://diracvideo.org/download/schroedinger"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/schroedinger-1.0.pc" ] || [ "$(pkg-config --modversion schroedinger-1.0)" != "$DIRAC_VERSION" ]; }; }; then
    start_build
    download "$DIRAC_SITE" "$DIRAC_TAR"
    untar "$SRC_PATH/$DIRAC_TAR"
    pushd "schroedinger-${DIRAC_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    #rm -f $SDK_HOME/lib/libschro*.so*
    $GSED -i "s/-lschroedinger-1.0/-lschroedinger-1.0 -lorc-0.4/" $SDK_HOME/lib/pkgconfig/schroedinger-1.0.pc
    popd
    rm -rf "schroedinger-${DIRAC_VERSION}"
    end_build
fi
