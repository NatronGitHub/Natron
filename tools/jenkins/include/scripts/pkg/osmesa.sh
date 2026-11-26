#!/bin/bash

# Install llvm+osmesa
# be careful if upgrading, the script applies important patches
OSMESA_VERSION=18.3.6
OSMESA_TAR="mesa-${OSMESA_VERSION}.tar.xz"
OSMESA_SHA256=aaf17638dcf5a90b93b6389e152fdc9ef147768b09598f24d2c5cf482fcfc705
OSMESA_SITE="https://archive.mesa3d.org/older-versions/${OSMESA_VERSION%%.*}.x"
OSMESA_GLU_VERSION=9.0.1 # 9.0.2 doesn't have glu_mangle.h
OSMESA_GLU_TAR="glu-${OSMESA_GLU_VERSION}.tar.gz"
OSMESA_GLU_SHA256=f6f484cfcd51e489afe88031afdea1e173aa652697e4c19ddbcb8260579a10f7
OSMESA_GLU_SITE="https://archive.mesa3d.org/glu"
OSMESA_DEMOS_VERSION=8.4.0
OSMESA_DEMOS_TAR="mesa-demos-${OSMESA_DEMOS_VERSION}.tar.bz2"
OSMESA_DEMOS_SHA256=01e99c94a0184e63e796728af89bfac559795fb2a0d6f506fa900455ca5fff7d
OSMESA_DEMOS_SITE="https://archive.mesa3d.org/demos/${OSMESA_DEMOS_VERSION}"
OSMESA_LLVM_VERSION=6.0.1
OSMESA_LLVM_TAR="llvm-${OSMESA_LLVM_VERSION}.src.tar.xz"
OSMESA_LLVM_SHA256=b6d6c324f9c71494c0ccaf3dac1f16236d970002b42bb24a6c9e1634f7d0f4e2
OSMESA_LLVM_SITE="https://releases.llvm.org/${OSMESA_LLVM_VERSION}"
if download_step; then
    download "$OSMESA_SITE" "$OSMESA_TAR" "$OSMESA_SHA256"
    download "$OSMESA_GLU_SITE" "$OSMESA_GLU_TAR" "$OSMESA_GLU_SHA256"
    download "$OSMESA_DEMOS_SITE" "$OSMESA_DEMOS_TAR" "$OSMESA_DEMOS_SHA256"
    download "$OSMESA_LLVM_SITE" "$OSMESA_LLVM_TAR" "$OSMESA_LLVM_SHA256"
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
    git clone --depth 1 https://github.com/devernay/osmesa-install
    pushd osmesa-install
    mkdir build
    pushd build
    cp "$SRC_PATH/$OSMESA_TAR" .
    cp "$SRC_PATH/$OSMESA_GLU_TAR" .
    cp "$SRC_PATH/$OSMESA_DEMOS_TAR" .
    if [ "$LLVM_BUILD" = 1 ]; then
       cp "$SRC_PATH/$OSMESA_LLVM_TAR" .
    fi
    # LLVM 6 relies on <cstdint> being included indirectly by libstdc++ headers,
    # which is not the case anymore since GCC 13
    OSMESA_CXXFLAGS="-fPIC"
    if version_gt "$GCC_VERSION" 12.99; then
        OSMESA_CXXFLAGS="$OSMESA_CXXFLAGS -include cstdint"
    fi
    env BUILD_OSDEMO=0 CFLAGS="-fPIC ${GCC14_CFLAGS:-}" CXXFLAGS="$OSMESA_CXXFLAGS" OSMESA_DRIVER=3 OSMESA_PREFIX="${SDK_HOME}/osmesa" OSMESA_VERSION="$OSMESA_VERSION" LLVM_PREFIX="${SDK_HOME}/llvm" LLVM_VERSION="$OSMESA_LLVM_VERSION" MKJOBS="${MKJOBS}" LLVM_BUILD="$LLVM_BUILD" ../osmesa-install.sh
    popd
    popd
    rm -rf osmesa-install
    end_build
fi
