#!/bin/bash
# install sox (for openfx-arena's AudioCurve plugin)
# see https://sourceforge.net/projects/sox/files/sox/
SOX_VERSION=14.4.2
SOX_TAR="sox-${SOX_VERSION}.tar.gz"
SOX_SHA256=b45f598643ffbd8e363ff24d61166ccec4836fea6d3888881b8df53e3bb55f6c
SOX_SITE="https://downloads.sourceforge.net/project/sox/sox/${SOX_VERSION}"
if download_step; then
    download "$SOX_SITE" "$SOX_TAR" "$SOX_SHA256"
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
