#!/bin/bash

# Install oiio
# see https://github.com/OpenImageIO/oiio/releases
#OIIO_VERSION=1.7.19
OIIO_VERSION=2.0.9
OIIO_VERSION_SHORT=${OIIO_VERSION%.*}
OIIO_TAR="oiio-Release-${OIIO_VERSION}.tar.gz"
if build_step && { force_build || { [ "${REBUILD_OIIO:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME/lib/libOpenImage"* "$SDK_HOME/include/OpenImage"* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libOpenImageIO.so" ]; }; }; then
    start_build
    download_github OpenImageIO oiio "$OIIO_VERSION" Release- "$OIIO_TAR"
    untar "$SRC_PATH/$OIIO_TAR"
    pushd "oiio-Release-${OIIO_VERSION}"

    if [ -d "$INC_PATH/patches/OpenImageIO/$OIIO_VERSION_SHORT" ]; then
	patches=$(find  "$INC_PATH/patches/OpenImageIO/$OIIO_VERSION_SHORT" -type f)
	for p in $patches; do
            if [[ "$p" == *-mingw-* ]] && [ "$OS" != "MINGW64_NT-6.1" ]; then
		continue
            fi
            patch -p1 -i "$p"
	done
    fi
    if [ "${DEBUG:-}" = "1" ]; then
        OIIOMAKE="debug"
    fi
    OIIO_CMAKE_FLAGS=( "-DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE" "-DCMAKE_C_FLAGS=$BF" "-DCMAKE_CXX_FLAGS=$BF" "-DCMAKE_LIBRARY_PATH=${SDK_HOME}/lib" "-DUSE_OPENCV:BOOL=FALSE" "-DUSE_OPENSSL:BOOL=FALSE" "-DOPENEXR_HOME=$SDK_HOME" "-DILMBASE_HOME=$SDK_HOME" "-DTHIRD_PARTY_TOOLS_HOME=$SDK_HOME" "-DUSE_QT:BOOL=FALSE" "-DUSE_PYTHON:BOOL=FALSE" "-DUSE_FIELD3D:BOOL=FALSE" "-DOIIO_BUILD_TESTS=0" "-DOIIO_BUILD_TOOLS=1" "-DBOOST_ROOT=$SDK_HOME" "-DSTOP_ON_WARNING:BOOL=FALSE" "-DUSE_GIF:BOOL=TRUE" "-DUSE_FREETYPE:BOOL=TRUE" "-DFREETYPE_INCLUDE_PATH=$SDK_HOME/include" "-DUSE_FFMPEG:BOOL=FALSE" "-DLINKSTATIC=0" "-DBUILDSTATIC=0" "-DOPENJPEG_HOME=$SDK_HOME" "-DOPENJPEG_INCLUDE_DIR=$SDK_HOME/include/openjpeg-$OPENJPEG2_VERSION_SHORT" "-DUSE_OPENJPEG:BOOL=TRUE" )
    if [[ ! "$GCC_VERSION" =~ ^4\. ]]; then
        OIIO_CMAKE_FLAGS+=( "-DUSE_CPP11:BOOL=FALSE" "-DUSE_CPP14:BOOL=FALSE" "-DUSE_CPP17:BOOL=FALSE" )
    fi
    OIIO_CMAKE_FLAGS+=( "-DUSE_LIBRAW:BOOL=TRUE" "-DCMAKE_INSTALL_PREFIX=$SDK_HOME" )

    mkdir build
    pushd build

    cmake "${OIIO_CMAKE_FLAGS[@]}" ..
    make -j${MKJOBS} ${OIIOMAKE:-}
    make install
    popd
    popd
    rm -rf "oiio-Release-${OIIO_VERSION}"
    end_build
fi
