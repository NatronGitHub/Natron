#!/bin/bash

# Install oiio
# see https://github.com/OpenImageIO/oiio/releases
OIIO_VERSION=2.3.10.1 # compiled for C++14 (see -DCMAKE_CXX_STANDARD=14 below, and openexr build)
OIIO_VERSION_SHORT=2.3 # ${OIIO_VERSION%.*}
OIIO_TAR="oiio-${OIIO_VERSION}.tar.gz"
if download_step; then
    download_github OpenImageIO oiio "$OIIO_VERSION" v "$OIIO_TAR"
fi
if build_step && { force_build || { [ "${REBUILD_OIIO:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME/lib/libOpenImage"* "$SDK_HOME/include/OpenImage"* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libOpenImageIO.so" ]; }; }; then
    start_build
    untar "$SRC_PATH/$OIIO_TAR"
    pushd "oiio-${OIIO_VERSION}"

    if [ -d "$INC_PATH/patches/OpenImageIO/$OIIO_VERSION_SHORT" ]; then
        patches=$(find  "$INC_PATH/patches/OpenImageIO/$OIIO_VERSION_SHORT" -type f)
	for p in $patches; do
            if [[ "$p" == *-mingw-* ]] && [ "$OS" != "MINGW64_NT-6.1" ]; then
                continue
            fi
            echo "*** applying $p"
            patch -p1 -i "$p"
	done
    fi
    if [ "${DEBUG:-}" = "1" ]; then
        OIIOMAKE="debug"
    fi
    OIIO_CMAKE_FLAGS=( "-DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE" "-DCMAKE_C_FLAGS=$BF" "-DCMAKE_CXX_FLAGS=$BF" "-DCMAKE_LIBRARY_PATH=${SDK_HOME}/lib" )
    OIIO_CMAKE_FLAGS+=( "-DUSE_OPENCV:BOOL=FALSE" "-DUSE_OPENSSL:BOOL=FALSE" )
    OIIO_CMAKE_FLAGS+=( "-DOpenEXR_ROOT=$SDK_HOME" "-DOPENEXR_HOME=$SDK_HOME" "-DIlmbase_ROOT=$SDK_HOME" "-DILMBASE_HOME=$SDK_HOME" )
    OIIO_CMAKE_FLAGS+=( "-DTHIRD_PARTY_TOOLS_HOME=$SDK_HOME" "-DUSE_QT:BOOL=FALSE" )
    OIIO_CMAKE_FLAGS+=( "-DUSE_PYTHON:BOOL=FALSE" "-DUSE_FIELD3D:BOOL=FALSE" )
    OIIO_CMAKE_FLAGS+=( "-DOIIO_BUILD_TESTS=0" "-DOIIO_BUILD_TOOLS=1" )
    OIIO_CMAKE_FLAGS+=( "-DBOOST_ROOT=$SDK_HOME" "-DSTOP_ON_WARNING:BOOL=FALSE" )
    OIIO_CMAKE_FLAGS+=( "-DUSE_GIF:BOOL=TRUE" )
    OIIO_CMAKE_FLAGS+=( "-DUSE_FREETYPE:BOOL=TRUE" "-DFREETYPE_INCLUDE_PATH=$SDK_HOME/include" )
    OIIO_CMAKE_FLAGS+=( "-DUSE_FFMPEG:BOOL=FALSE" )
    OIIO_CMAKE_FLAGS+=( "-DLINKSTATIC=0" "-DBUILDSTATIC=0" )
    OIIO_CMAKE_FLAGS+=( "-DOpenJPEG_ROOT=$SDK_HOME" "-DOPENJPEG_HOME=$SDK_HOME" "-DOPENJPEG_INCLUDE_DIR=$SDK_HOME/include/openjpeg-$OPENJPEG2_VERSION_SHORT" "-DUSE_OPENJPEG:BOOL=TRUE" )
    OIIO_CMAKE_FLAGS+=( "-DUSE_HEIF:BOOL=TRUE" )
    if version_gt 4.8.1 "$GCC_VERSION"; then
        OIIO_CMAKE_FLAGS+=( "-DUSE_CPP11:BOOL=FALSE" )
    fi
    if version_gt 6.1 "$GCC_VERSION"; then
        OIIO_CMAKE_FLAGS+=( "-DUSE_CPP14:BOOL=FALSE" )
    fi
    if version_gt 7.1 "$GCC_VERSION"; then
        OIIO_CMAKE_FLAGS+=( "-DUSE_CPP17:BOOL=FALSE" )
    fi
    # Several versions of libraw are installed, use the GPL2 version during SDK build
    OIIO_CMAKE_FLAGS+=( "-DUSE_LIBRAW:BOOL=TRUE" "-DLibRaw_ROOT=$SDK_HOME/libraw-gpl2")
    OIIO_CMAKE_FLAGS+=( "-DCMAKE_INSTALL_PREFIX=$SDK_HOME" )
    OIIO_CMAKE_FLAGS+=( "-DBUILD_FMT_FORCE:BOOL=TRUE" )
    OIIO_CMAKE_FLAGS+=( "-DCMAKE_CXX_STANDARD=14" )
    mkdir build
    pushd build

    env PKG_CONFIG_PATH="$SDK_HOME/libraw-gpl2/lib/pkgconfig:$PKG_CONFIG_PATH" cmake "${OIIO_CMAKE_FLAGS[@]}" ..
    make -j${MKJOBS} ${OIIOMAKE:-}
    make install
    popd
    popd
    rm -rf "oiio-${OIIO_VERSION}"
    end_build
fi
