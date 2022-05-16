#!/bin/bash
# install flac (for libsndfile, sox)
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/flac.html
FLAC_VERSION=1.3.4
FLAC_TAR="flac-${FLAC_VERSION}.tar.xz"
FLAC_SITE="https://downloads.xiph.org/releases/flac"
if download_step; then
    download "$FLAC_SITE" "$FLAC_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/flac.pc" ] || [ "$(pkg-config --modversion flac)" != "$FLAC_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$FLAC_TAR"
    pushd "flac-${FLAC_VERSION}"
    EXTRA_CONFFLAGS=
    # disable assembly on CentOS6, because binutils-2.20.51.0.2 is too old
    if [ -f /etc/redhat-release ] && version_gt 7 "$(grep -oE '[0-9]+\.[0-9]+' /etc/redhat-release)"; then
        EXTRA_CONFFLAGS="--disable-sse --disable-asm-optimizations"
    fi
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-shared --disable-static --disable-thorough-tests $EXTRA_CONFFLAGS
    make -j${MKJOBS}
    make install
    popd
    rm -rf "flac-${FLAC_VERSION}"
    end_build
fi
