#!/bin/bash

# Install SeExpr
# Be careful when upgrading, v3.0 uses llvm and is probably not trivial to build.
SEEXPR211_VERSION=2.11
SEEXPR211_TAR="SeExpr-${SEEXPR211_VERSION}.tar.gz"
if download_step; then
    download_github wdas SeExpr "$SEEXPR211_VERSION" v "$SEEXPR211_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libSeExpr.so" ]; }; }; then
    start_build
    untar "$SRC_PATH/$SEEXPR211_TAR"
    pushd "SeExpr-${SEEXPR211_VERSION}"
    patch -Np1 -i "$INC_PATH/patches/SeExpr/0001-seexpr-2.11-fix-cxxflags.patch"
    patch -Np1 -i "$INC_PATH/patches/SeExpr/0002-seexpr-2.11-fix-inline-error.patch"
    patch -Np1 -i "$INC_PATH/patches/SeExpr/0003-seexpr-2.11-fix-dll-installation.patch"
    patch -Np1 -i "$INC_PATH/patches/SeExpr/0004-seexpr-2.11-c++98.patch"
    patch -Np1 -i "$INC_PATH/patches/SeExpr/0005-seexpr-2.11-noeditordemos.patch"
    patch -Np1 -i "$INC_PATH/patches/SeExpr/0006-seexpr-2.11-cstylecast.patch"
    #patch -p0 -i "$INC_PATH/patches/SeExpr/seexpr2-cmake.diff"
    mkdir build
    pushd build
    cmake .. -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
    make -j${MKJOBS}
    make install
    popd
    popd
    rm -rf "SeExpr-${SEEXPR211_VERSION}"
    end_build
fi
