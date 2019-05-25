#!/bin/bash

# install bison (for SeExpr)
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/bison.html
BISON_VERSION=3.3.2
BISON_TAR="bison-${BISON_VERSION}.tar.gz"
BISON_SITE="http://ftp.gnu.org/pub/gnu/bison"
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/bison" ]; }; }; then
    start_build
    download "$BISON_SITE" "$BISON_TAR"
    untar "$SRC_PATH/$BISON_TAR"
    pushd "bison-${BISON_VERSION}"
    # 2 patches for use with glibc 2.28 and up. (for bison 3.0.4)
    if version_gt 3.1.0 "$BISON_VERSION"; then
	patch -Np1 -i "$INC_PATH"/patches/bison/0001-fflush-adjust-to-glibc-2.28-libio.h-removal.patch
	patch -Np1 -i "$INC_PATH"/patches/bison/0002-fflush-be-more-paranoid-about-libio.h-change.patch
    fi
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "bison-${BISON_VERSION}"
    end_build
fi
