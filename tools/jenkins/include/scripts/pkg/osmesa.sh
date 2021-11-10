#!/bin/bash

# Install llvm+osmesa
# be careful if upgrading, the script applies important patches
OSMESA_VERSION=18.3.6
OSMESA_TAR="mesa-${OSMESA_VERSION}.tar.xz"
OSMESA_SITE="ftp://ftp.freedesktop.org/pub/mesa"
OSMESA_GLU_VERSION=9.0.1
OSMESA_GLU_TAR="glu-${OSMESA_GLU_VERSION}.tar.gz"
OSMESA_GLU_SITE="ftp://ftp.freedesktop.org/pub/mesa/glu"
OSMESA_DEMOS_VERSION=8.4.0
OSMESA_DEMOS_TAR="mesa-demos-${OSMESA_DEMOS_VERSION}.tar.bz2"
OSMESA_DEMOS_SITE="ftp://ftp.freedesktop.org/pub/mesa/demos/${OSMESA_DEMOS_VERSION}"
OSMESA_LLVM_VERSION=6.0.1
OSMESA_LLVM_TAR="llvm-${OSMESA_LLVM_VERSION}.src.tar.xz"
OSMESA_LLVM_SITE="http://releases.llvm.org/${OSMESA_LLVM_VERSION}"
if download_step; then
    download "$OSMESA_SITE" "$OSMESA_TAR"
    download "$OSMESA_GLU_SITE" "$OSMESA_GLU_TAR"
    download "$OSMESA_DEMOS_SITE" "$OSMESA_DEMOS_TAR"
    download "$OSMESA_LLVM_SITE" "$OSMESA_LLVM_TAR"
fi
if build_step && { force_build || { [ "${REBUILD_OSMESA:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME/osmesa" || true
    rm -rf "$SDK_HOME/llvm" || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/osmesa/lib/pkgconfig/gl.pc" ] || [ "$(env PKG_CONFIG_PATH=$SDK_HOME/osmesa/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion gl)" != "$OSMESA_VERSION" ]; }; }; then
    start_build
    mkdir -p "${SDK_HOME}/osmesa" || true
    LLVM_BUILD=0
    if [ ! -s "$SDK_HOME/llvm/bin/llvm-config" ] || [ "$($SDK_HOME/llvm/bin/llvm-config --version)" != "$OSMESA_LLVM_VERSION" ]; then
        LLVM_BUILD=1
        mkdir -p "${SDK_HOME}/llvm" || true
    fi
    git clone https://github.com/devernay/osmesa-install
    pushd osmesa-install
    mkdir build
    pushd build
    cp "$SRC_PATH/$OSMESA_TAR" .
    cp "$SRC_PATH/$OSMESA_GLU_TAR" .
    cp "$SRC_PATH/$OSMESA_DEMOS_TAR" .
    if [ "$LLVM_BUILD" = 1 ]; then
       cp "$SRC_PATH/$OSMESA_LLVM_TAR" .
    fi
    env BUILD_OSDEMO=0 CFLAGS="-fPIC" OSMESA_DRIVER=3 OSMESA_PREFIX="${SDK_HOME}/osmesa" OSMESA_VERSION="$OSMESA_VERSION" LLVM_PREFIX="${SDK_HOME}/llvm" LLVM_VERSION="$OSMESA_LLVM_VERSION" MKJOBS="${MKJOBS}" LLVM_BUILD="$LLVM_BUILD" ../osmesa-install.sh
    popd
    popd
    rm -rf osmesa-install
    end_build
fi
