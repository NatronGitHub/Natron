#!/bin/bash
# install sox (for openfx-arena's AudioCurve plugin)
# see https://sourceforge.net/projects/sox/files/sox/
SOX_VERSION=14.4.2
# the .tar.bz2 (same contents as the .tar.gz), because MacPorts' distfiles mirror has it
SOX_TAR="sox-${SOX_VERSION}.tar.bz2"
SOX_SHA256=81a6956d4330e75b5827316e44ae381e6f1e8928003c6aa45896da9041ea149c
SOX_SITE="https://downloads.sourceforge.net/project/sox/sox/${SOX_VERSION}"
if download_step; then
    # sox is only released on SourceForge: MacPorts' distfiles mirror (the same file) first
    download_urls "$SOX_TAR" "$SOX_SHA256" \
        "https://distfiles.macports.org/sox/$SOX_TAR" \
        "$SOX_SITE/$SOX_TAR"
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
