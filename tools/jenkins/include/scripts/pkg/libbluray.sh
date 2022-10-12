#!/bin/bash

# Install libbluray (for ffmpeg)
# see http://ftp.videolan.org/pub/videolan/libbluray
# BD-Java support is disabled, because it requires apache-ant and a JDK for building
LIBBLURAY_VERSION=1.3.3
LIBBLURAY_TAR="libbluray-${LIBBLURAY_VERSION}.tar.bz2"
LIBBLURAY_SITE="http://ftp.videolan.org/pub/videolan/libbluray/${LIBBLURAY_VERSION}"
if download_step; then
    download "$LIBBLURAY_SITE" "$LIBBLURAY_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libbluray.pc" ] || [ "$(pkg-config --modversion libbluray)" != "$LIBBLURAY_VERSION" ]; }; }; then
    REBUILD_FFMPEG=1
    start_build
    untar "$SRC_PATH/$LIBBLURAY_TAR"
    pushd "libbluray-${LIBBLURAY_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared --disable-bdjava --disable-bdjava-jar
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libbluray-${LIBBLURAY_VERSION}"
    end_build
fi
