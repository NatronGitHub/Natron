#!/bin/bash

# Install Imath
# see https://github.com/AcademySoftwareFoundation/Imath/releases/
IMATH_VERSION=3.2.1
IMATH_TAR="Imath-${IMATH_VERSION}.tar.gz"

if download_step; then
    download_github AcademySoftwareFoundation Imath "${IMATH_VERSION}" v "${IMATH_TAR}"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/Imath.pc" ] || [ "$(pkg-config --modversion Imath)" != "$IMATH_VERSION" ]; }; }; then    
    start_build
    untar "$SRC_PATH/$IMATH_TAR"
    pushd "Imath-${IMATH_VERSION}"
    mkdir build
    pushd build
    cmake .. -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DCMAKE_INSTALL_PREFIX="$SDK_HOME"
    make -j${MKJOBS}
    make install
    popd  # build
    popd  # Imath-${IMATH_VERSION}
    rm -rf "Imath-${IMATH_VERSION}"
    end_build
fi
