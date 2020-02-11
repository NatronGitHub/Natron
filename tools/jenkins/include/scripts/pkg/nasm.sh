#!/bin/bash

# Install nasm (for x264, lame, and others)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/nasm.html
NASM_VERSION=2.14.02
NASM_TAR="nasm-${NASM_VERSION}.tar.gz"
NASM_SITE="http://www.nasm.us/pub/nasm/releasebuilds/${NASM_VERSION}"
if download_step; then
    download "$NASM_SITE" "$NASM_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/nasm" ]; }; }; then
    start_build
    untar "$SRC_PATH/$NASM_TAR"
    pushd "nasm-${NASM_VERSION}"
    if [ "${GCC_VERSION:0:2}" = 8. ]; then
        patch -Np1 -i "$INC_PATH"/patches/nasm/0001-Remove-invalid-pure_func-qualifiers.patch
    fi
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "nasm-${NASM_VERSION}"
    end_build
fi
