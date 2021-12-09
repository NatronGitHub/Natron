#!/bin/bash

# Install SeExpr
# Be careful when upgrading, v3.0 uses llvm and is probably not trivial to build.
SEEXPR_VERSION=3.0.1
SEEXPR_TAR="SeExpr-${SEEXPR_VERSION}.tar.gz"
SEEXPR_VERSION_PC=1.0.0
if download_step; then
    download_github wdas SeExpr "$SEEXPR_VERSION" v "$SEEXPR_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/share/pkgconfig/seexpr2.pc" ]|| [ "$(pkg-config --modversion seexpr2)" != "$SEEXPR_VERSION_PC" ]; }; }; then
    start_build
    untar "$SRC_PATH/$SEEXPR_TAR"
    pushd "SeExpr-${SEEXPR_VERSION}"
    mkdir build
    pushd build
    cmake .. -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DENABLE_LLVM_BACKEND=FALSE -DENABLE_QT5=FALSE -DUSE_PYTHON=FALSE
    make -j${MKJOBS}
    make install
    #cp "$SDK_HOME/share/pkgconfig/seexpr2.pc" "$SDK_HOME/lib/pkgconfig/seexpr2.pc"
    #rm -f $SDK_HOME/lib/libSeExpr.so
    popd
    popd
    rm -rf "SeExpr-${SEEXPR_VERSION}"
    end_build
fi
