#!/bin/bash

# Install jasper
# see http://www.linuxfromscratch.org/blfs/view/svn/general/jasper.html
JASPER_VERSION=4.2.9
JASPER_TAR="jasper-${JASPER_VERSION}.tar.gz"
JASPER_SHA256=b0e5af6b54c274b9670c7e32ddbf6c802d88c896062d760267695dd0aa7014ff
JASPER_SITE="https://github.com/jasper-software/jasper/archive/version-${JASPER_VERSION}"
if download_step; then
    download "$JASPER_SITE" "$JASPER_TAR" "$JASPER_SHA256"
fi
if build_step && { force_build || { [ "${REBUILD_JASPER:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/{include,lib}/*jasper* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libjasper.so" ]; }; }; then
    start_build
    untar "$SRC_PATH/$JASPER_TAR"
    # the GitHub archive of tag version-X unpacks to jasper-version-X
    if [ -d "jasper-version-${JASPER_VERSION}" ]; then
        mv "jasper-version-${JASPER_VERSION}" "jasper-${JASPER_VERSION}"
    fi
    pushd "jasper-${JASPER_VERSION}"
    #env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    mkdir tmp_build # jasper >= 4.1 rejects build dirs under the source dir, except tmp*
    pushd tmp_build
    cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DJAS_ENABLE_DOC=NO -DCMAKE_INSTALL_LIBDIR=lib -DJAS_ENABLE_PROGRAMS=OFF
    make -j${MKJOBS}
    make install
    popd
    popd
    rm -rf "jasper-${JASPER_VERSION}"
    end_build
fi
