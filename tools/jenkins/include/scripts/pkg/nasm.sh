#!/bin/bash

# Install nasm (for x264, lame, and others)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/nasm.html
NASM_VERSION=3.02
NASM_TAR="nasm-${NASM_VERSION}.tar.gz"
NASM_SHA256=f504227b2f529e658d41629075f0503b38d67d790af345f34eba4af60c6a5998
NASM_SITE="https://www.nasm.us/pub/nasm/releasebuilds/${NASM_VERSION}"
if download_step; then
    download "$NASM_SITE" "$NASM_TAR" "$NASM_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/nasm" ]; }; }; then
    start_build
    untar "$SRC_PATH/$NASM_TAR"
    pushd "nasm-${NASM_VERSION}"
    if [ "${NASM_VERSION}" = 2.13.03 ]; then
        # patch for GCC 8
        patch -Np1 -i "$INC_PATH"/patches/nasm/0001-Remove-invalid-pure_func-qualifiers.patch
    fi
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "nasm-${NASM_VERSION}"
    end_build
fi
