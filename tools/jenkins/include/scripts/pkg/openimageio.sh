#!/bin/bash

# Install oiio
# see https://github.com/AcademySoftwareFoundation/OpenImageIO/releases
OIIO_VERSION=3.1.17.0 # OIIO 3 requires C++17 (see -DCMAKE_CXX_STANDARD=17 below)
OIIO_VERSION_SHORT=3.1 # ${OIIO_VERSION%.*}
OIIO_TAR="OpenImageIO-${OIIO_VERSION}.tar.gz"
OIIO_SHA256=92a26c0af4ffc6676d72d9dfe0e991eb45fdf3192abee3d0855a24d6c721b013
# The header-only dependencies that the SDK doesn't have, from checked tarballs, in a private
# prefix (fmt is copied into OIIO's headers, robin-map is only used internally), instead of
# OIIO's OpenImageIO_BUILD_MISSING_DEPS, which clones them at configure time. The versions are
# those of OIIO's src/cmake/build_fmt.cmake and build_Robinmap.cmake.
OIIO_FMT_VERSION=12.1.0
OIIO_FMT_TAR="fmt-${OIIO_FMT_VERSION}.tar.gz"
OIIO_FMT_SHA256=ea7de4299689e12b6dddd392f9896f08fb0777ac7168897a244a6d6085043fea
OIIO_ROBINMAP_VERSION=1.4.0
OIIO_ROBINMAP_TAR="robin-map-${OIIO_ROBINMAP_VERSION}.tar.gz"
OIIO_ROBINMAP_SHA256=7930dbf9634acfc02686d87f615c0f4f33135948130b8922331c16d90a03250c
if download_step; then
    download_github AcademySoftwareFoundation OpenImageIO "$OIIO_VERSION" v "$OIIO_TAR" "$OIIO_SHA256"
    download_github fmtlib fmt "$OIIO_FMT_VERSION" "" "$OIIO_FMT_TAR" "$OIIO_FMT_SHA256"
    download_github Tessil robin-map "$OIIO_ROBINMAP_VERSION" v "$OIIO_ROBINMAP_TAR" "$OIIO_ROBINMAP_SHA256"
fi
if build_step && { force_build || { [ "${REBUILD_OIIO:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME/lib/libOpenImage"* "$SDK_HOME/include/OpenImage"* || true
fi
# (the version check rebuilds OIIO on incremental SDK updates, e.g. against a new OpenEXR or OCIO)
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libOpenImageIO.so" ] || [ "$(pkg-config --modversion OpenImageIO)" != "$OIIO_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$OIIO_TAR"
    pushd "OpenImageIO-${OIIO_VERSION}"
    OIIO_EXT="$PWD/ext-dist"
    untar "$SRC_PATH/$OIIO_FMT_TAR"
    cmake -S "fmt-${OIIO_FMT_VERSION}" -B fmt-build -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DCMAKE_INSTALL_PREFIX="$OIIO_EXT" -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DFMT_DOC=OFF -DFMT_TEST=OFF
    cmake --build fmt-build -j${MKJOBS} --target install
    untar "$SRC_PATH/$OIIO_ROBINMAP_TAR"
    cmake -S "robin-map-${OIIO_ROBINMAP_VERSION}" -B robin-map-build -DCMAKE_INSTALL_PREFIX="$OIIO_EXT"
    cmake --build robin-map-build --target install

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
    # OIIO 3: never download dependencies (fmt and robin-map come from $OIIO_EXT); use the SDK's OCIO 2
    OIIO_CMAKE_FLAGS+=( "-DOpenImageIO_BUILD_MISSING_DEPS=" "-DOpenImageIO_BUILD_LOCAL_DEPS=" "-DOpenColorIO_ROOT=$SDK_HOME" )
    OIIO_CMAKE_FLAGS+=( "-Dfmt_ROOT=$OIIO_EXT" "-DRobinmap_ROOT=$OIIO_EXT" )
    OIIO_CMAKE_FLAGS+=( "-DUSE_PTEX:BOOL=FALSE" "-DUSE_DCMTK:BOOL=FALSE" "-DUSE_OPENVDB:BOOL=FALSE" "-DUSE_JXL:BOOL=FALSE" "-DUSE_LIBUHDR:BOOL=FALSE" "-DUSE_TBB:BOOL=FALSE" "-DUSE_NUKE:BOOL=FALSE" )
    OIIO_CMAKE_FLAGS+=( "-DCMAKE_INSTALL_LIBDIR=lib" "-DCMAKE_CXX_STANDARD=17" )
    mkdir build
    pushd build

    env PKG_CONFIG_PATH="$SDK_HOME/libraw-gpl2/lib/pkgconfig:$PKG_CONFIG_PATH" cmake "${OIIO_CMAKE_FLAGS[@]}" ..
    make -j${MKJOBS} ${OIIOMAKE:-}
    make install
    popd
    popd
    rm -rf "OpenImageIO-${OIIO_VERSION}"
    end_build
fi
