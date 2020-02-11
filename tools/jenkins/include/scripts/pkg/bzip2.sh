#!/bin/bash

# Install bzip2
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/bzip2.html
BZIP2_VERSION=1.0.8
BZIP2_TAR="bzip2-${BZIP2_VERSION}.tar.gz"
BZIP2_SITE="https://sourceware.org/pub/bzip2"
if download_step; then
    download "$BZIP2_SITE" "$BZIP2_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libbz2.so.1" ]; }; }; then
    start_build
    untar "$SRC_PATH/$BZIP2_TAR"
    pushd "bzip2-${BZIP2_VERSION}"
    $GSED -e 's/^CFLAGS=\(.*\)$/CFLAGS=\1 \$\(BIGFILES\)/' -i ./Makefile-libbz2_so
    $GSED -i "s/libbz2.so.1.0 -o libbz2.so.1.0.6/libbz2.so.1 -o libbz2.so.1.0.6/;s/rm -f libbz2.so.1.0/rm -f libbz2.so.1/;s/ln -s libbz2.so.1.0.6 libbz2.so.1.0/ln -s libbz2.so.1.0.6 libbz2.so.1/" Makefile-libbz2_so
    make -f Makefile-libbz2_so
    install -m755 "libbz2.so.${BZIP2_VERSION}" "$SDK_HOME/lib"
    install -m644 bzlib.h "$SDK_HOME/include/"
    cd "$SDK_HOME/lib"
    ln -sfv "libbz2.so.${BZIP2_VERSION}" libbz2.so.1.0
    ln -sfv libbz2.so.1.0 libbz2.so.1
    ln -sfv libbz2.so.1 libbz2.so
    popd
    rm -rf "bzip2-${BZIP2_VERSION}"
    end_build
fi
