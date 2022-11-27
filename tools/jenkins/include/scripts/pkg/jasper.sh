#!/bin/bash

# Install jasper
# see http://www.linuxfromscratch.org/blfs/view/svn/general/jasper.html
JASPER_VERSION=4.0.0
JASPER_TAR="jasper-${JASPER_VERSION}.tar.gz"
JASPER_SITE="https://github.com/jasper-software/jasper/archive/version-${JASPER_VERSION}"
if download_step; then
    download "$JASPER_SITE" "$JASPER_TAR"
fi
if build_step && { force_build || { [ "${REBUILD_JASPER:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/{include,lib}/*jasper* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libjasper.so" ]; }; }; then
    start_build
    untar "$SRC_PATH/$JASPER_TAR"
    pushd "jasper-version-${JASPER_VERSION}"
    #env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    mkdir build-natron
    pushd build-natron
    cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DJAS_ENABLE_DOC=NO
    make -j${MKJOBS}
    make install
    popd
    popd
    rm -rf "jasper-version-${JASPER_VERSION}"
    end_build
fi
