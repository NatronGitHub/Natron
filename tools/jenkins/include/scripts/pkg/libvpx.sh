#!/bin/bash

# Install libvpx
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/libvpx.html
LIBVPX_VERSION=1.7.0
LIBVPX_TAR="libvpx-${LIBVPX_VERSION}.tar.gz"
#LIBVPX_SITE=http://storage.googleapis.com/downloads.webmproject.org/releases/webm
if build_step && { force_build || { [ "${REBUILD_LIBVPX:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/lib/libvpx* || true
    rm -rf "$SDK_HOME"/lib/pkgconfig/vpx.pc || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/vpx.pc" ] || [ "$(pkg-config --modversion vpx)" != "$LIBVPX_VERSION" ]; }; }; then
    start_build
    #download "$LIBVPX_SITE" "$LIBVPX_TAR"
    download_github webmproject libvpx "${LIBVPX_VERSION}" v "${LIBVPX_TAR}"
    untar "$SRC_PATH/$LIBVPX_TAR"
    pushd "libvpx-$LIBVPX_VERSION"
    # This command corrects ownership and permissions of installed files. 
    sed -i 's/cp -p/cp/' build/make/Makefile
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static --enable-vp8 --enable-vp9 --enable-vp9-highbitdepth --enable-multithread --enable-runtime-cpu-detect --enable-postproc --enable-pic --disable-avx --disable-avx2 --disable-examples
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libvpx-$LIBVPX_VERSION"
    end_build
fi
