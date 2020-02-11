#!/bin/bash
# install sox (for openfx-arena's AudioCurve plugin)
# see https://sourceforge.net/projects/sox/files/sox/
SOX_VERSION=14.4.2
SOX_TAR="sox-${SOX_VERSION}.tar.gz"
SOX_SITE="https://downloads.sourceforge.net/project/sox/sox/${SOX_VERSION}"
if download_step; then
    download "$SOX_SITE" "$SOX_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/sox.pc" ] || [ "$(pkg-config --modversion sox)" != "$SOX_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$SOX_TAR"
    pushd "sox-${SOX_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "sox-${SOX_VERSION}"
    end_build
fi
