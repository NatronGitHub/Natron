#!/bin/bash

# Install openjpeg
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/openjpeg2.html
OPENJPEG2_VERSION=2.3.1
OPENJPEG2_VERSION_SHORT=${OPENJPEG2_VERSION%.*}
OPENJPEG2_TAR="openjpeg-${OPENJPEG2_VERSION}.tar.gz"
if download_step; then
    download_github uclouvain openjpeg "${OPENJPEG2_VERSION}" v "${OPENJPEG2_TAR}"
fi
if build_step && { force_build || { [ "${REBUILD_OPENJPEG:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/lib/libopenjp* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libopenjp2.pc" ] || [ "$(pkg-config --modversion libopenjp2)" != "$OPENJPEG2_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$OPENJPEG2_TAR"
    pushd "openjpeg-${OPENJPEG2_VERSION}"
    mkdir build
    pushd build
    cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
    make -j${MKJOBS}
    make install
    # ffmpeg < 3.4 looks for openjpeg 2.1
    #if [ ! -e "${SDK_HOME}/include/openjpeg-2.1" ]; then
    #    ln -sfv openjpeg-2.3 "${SDK_HOME}/include/openjpeg-2.1"
    #fi
    popd
    popd
    rm -rf "openjpeg-${OPENJPEG2_VERSION}"
    end_build
fi
