#!/bin/bash

# Install fribidi (for libass and ffmpeg)
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/fribidi.html
FRIBIDI_VERSION=1.0.4
FRIBIDI_TAR="fribidi-${FRIBIDI_VERSION}.tar.bz2"
FRIBIDI_SITE="https://github.com/fribidi/fribidi/releases/download/v${FRIBIDI_VERSION}"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/fribidi.pc" ] || [ "$(pkg-config --modversion fribidi)" != "$FRIBIDI_VERSION" ]; }; }; then
    REBUILD_FFMPEG=1
    start_build
    download "$FRIBIDI_SITE" "$FRIBIDI_TAR"
    untar "$SRC_PATH/$FRIBIDI_TAR"
    pushd "fribidi-${FRIBIDI_VERSION}"
    # git.mk seems to trigger a ./config.status --recheck, which is unnecessary
    # and additionally fails due to quoting
    # (git.mk was removed after 0.19.7)
    if [ -f git.mk ]; then
        rm git.mk
    fi
    if [ ! -f configure ]; then
        autoreconf -i -f
    fi
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared --disable-docs
    make #  -j${MKJOBS}
    make install
    popd
    rm -rf "fribidi-${FRIBIDI_VERSION}"
    end_build
fi
