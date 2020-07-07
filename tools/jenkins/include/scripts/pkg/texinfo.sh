#!/bin/bash

# Install Texinfo (for gdb)
# see http://www.linuxfromscratch.org/lfs/view/development/chapter08/texinfo.html
TEXINFO_VERSION=6.7
TEXINFO_TAR="texinfo-${TEXINFO_VERSION}.tar.gz"
TEXINFO_SITE="https://ftp.gnu.org/gnu/texinfo"
if download_step; then
    download "$TEXINFO_SITE" "$TEXINFO_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/makeinfo" ]; }; }; then
    start_build
    untar "$SRC_PATH/$TEXINFO_TAR"
    pushd "texinfo-${TEXINFO_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "texinfo-${TEXINFO_VERSION}"
    end_build
fi
