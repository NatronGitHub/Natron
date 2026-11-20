#!/bin/bash

# Install zstd
# see https://www.linuxfromscratch.org/lfs/view/development/chapter08/zstd.html
# libtiff, curl, libarchive, boost_iostreams, Qt and others use libzstd if they find it.
# On Rocky Linux 9, libzstd-devel comes with the toolchain packages, so without this
# package they would link the system libzstd.so.1, which is not bundled, and the SDK's
# pkg-config could not resolve their "Requires.private: libzstd".
ZSTD_VERSION=1.5.7
ZSTD_TAR="zstd-${ZSTD_VERSION}.tar.gz"
ZSTD_SITE="https://github.com/facebook/zstd/releases/download/v${ZSTD_VERSION}"
if download_step; then
    download "$ZSTD_SITE" "$ZSTD_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libzstd.pc" ] || [ "$(pkg-config --modversion libzstd)" != "$ZSTD_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$ZSTD_TAR"
    pushd "zstd-${ZSTD_VERSION}" || exit 1
    # The zstd program is built without zlib, lzma and lz4 support, so that it doesn't link
    # the system libraries. CFLAGS is passed in the environment: the Makefiles append to it.
    env CFLAGS="$BF" CXXFLAGS="$BF" make -j"${MKJOBS}" PREFIX="$SDK_HOME" LIBDIR="$SDK_HOME/lib" HAVE_ZLIB=0 HAVE_LZMA=0 HAVE_LZ4=0
    env CFLAGS="$BF" CXXFLAGS="$BF" make PREFIX="$SDK_HOME" LIBDIR="$SDK_HOME/lib" HAVE_ZLIB=0 HAVE_LZMA=0 HAVE_LZ4=0 install
    rm -f "$SDK_HOME/lib/libzstd.a"
    popd || exit 1
    rm -rf "zstd-${ZSTD_VERSION}"
    end_build
fi
