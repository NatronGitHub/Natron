#!/bin/bash

# x264
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/x264.html
X264_VERSION=20200218
X264_VERSION_PKG=0.159.x
X264_TAR="x264-${X264_VERSION}.tar.xz"
X264_SITE=" http://anduin.linuxfromscratch.org/BLFS/x264"

#X264_VERSION=20190815-2245-stable
#X264_VERSION_PKG=0.157.x
#X264_TAR="x264-snapshot-${X264_VERSION}.tar.bz2"
#X264_SITE="https://download.videolan.org/x264/snapshots"
if download_step; then
    download "$X264_SITE" "$X264_TAR"
fi
if build_step && { force_build || { [ "${REBUILD_X264:-}" = "1" ]; }; }; then
    REBUILD_FFMPEG=1
    rm -rf "$SDK_HOME"/lib/libx264* || true
    rm -rf "$SDK_HOME"/lib/pkgconfig/x264* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/x264.pc" ] || [ "$(pkg-config --modversion x264)" != "$X264_VERSION_PKG" ]; }; }; then
    start_build
    untar "$SRC_PATH/$X264_TAR"
    pushd "x264-${X264_VERSION}"
    #pushd "x264-snapshot-${X264_VERSION}"
    # add --bit-depth=10 to build a x264 that can only produce 10bit videos
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static --enable-pic --disable-cli
    make -j${MKJOBS}
    make install
    popd
    rm -rf "x264-${X264_VERSION}"
    #rm -rf "x264-snapshot-${X264_VERSION}"
    end_build
fi
