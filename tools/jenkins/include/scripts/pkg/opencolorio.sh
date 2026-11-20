#!/bin/bash

# Install OpenColorIO (uses yaml-cpp, tinyxml, lcms)
# see https://github.com/imageworks/OpenColorIO/releases
# We build a version more recent than 1.0.9, because 1.0.9 only supports yaml-cpp 0.3 and we
# installed yaml-cpp 0.5.
OCIO_BUILD_GIT=0 # set to 1 to build the git version instead of the release
# non-GIT version:
OCIO_VERSION=2.6.0
OCIO_TAR="OpenColorIO-${OCIO_VERSION}.tar.gz"
OCIO_SHA256=784ac37bde5b9c6e5dc15f23d1235bd1452d376cdc03627d6d6dd310f375a735
# OCIO 2 dependencies that the SDK doesn't have, built statically (into OCIO) from checked tarballs
# instead of being cloned by OCIO's CMake (OCIO_INSTALL_EXT_PACKAGES=MISSING). The versions are
# OCIO's RECOMMENDED_VERSION (share/cmake/modules/FindExtPackages.cmake) and Installsse2neon.cmake.
OCIO_PYSTRING_VERSION=1.1.4
OCIO_PYSTRING_TAR="pystring-${OCIO_PYSTRING_VERSION}.tar.gz"
OCIO_PYSTRING_SHA256=49da0fe2a049340d3c45cce530df63a2278af936003642330287b68cefd788fb
OCIO_MINIZIP_VERSION=4.2.2
OCIO_MINIZIP_TAR="minizip-ng-${OCIO_MINIZIP_VERSION}.tar.gz"
OCIO_MINIZIP_SHA256=71af7b9799856d8b03619df3949e9c1be9703f8de0795af71399ba283cb27aac
OCIO_SSE2NEON_COMMIT=227cc413fb2d50b2a10073087be96b59d5364aea # header-only, used on aarch64
OCIO_SSE2NEON_TAR="sse2neon-${OCIO_SSE2NEON_COMMIT}.tar.gz"
OCIO_SSE2NEON_SHA256=3427a495743bb6fd1b5f9f806b80f57d67b1ac7ccf39a5f44aedd487fd7e6da1
if download_step; then
    download_github AcademySoftwareFoundation OpenColorIO "$OCIO_VERSION" v "$OCIO_TAR" "$OCIO_SHA256"
    download_github imageworks pystring "$OCIO_PYSTRING_VERSION" v "$OCIO_PYSTRING_TAR" "$OCIO_PYSTRING_SHA256"
    download_github zlib-ng minizip-ng "$OCIO_MINIZIP_VERSION" "" "$OCIO_MINIZIP_TAR" "$OCIO_MINIZIP_SHA256"
    download_github DLTcollab sse2neon "$OCIO_SSE2NEON_COMMIT" "" "$OCIO_SSE2NEON_TAR" "$OCIO_SSE2NEON_SHA256"
fi
# if [ "$OCIO_BUILD_GIT" = 1 ]; then
#     ## GIT version:
#     OCIO_GIT="https://github.com/imageworks/OpenColorIO.git"
#     ## 7bd4b1e556e6c98c0aa353d5ecdf711bb272c4fa is October 25, 2017
#     OCIO_COMMIT="7bd4b1e556e6c98c0aa353d5ecdf711bb272c4fa"
# fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/OpenColorIO.pc" ] || [ "$(pkg-config --modversion OpenColorIO)" != "$OCIO_VERSION" ]; }; }; then
    start_build
    if [ "$OCIO_BUILD_GIT" = 1 ]; then
        git_clone_commit "$OCIO_GIT" "$OCIO_COMMIT"
        pushd OpenColorIO
    else
        untar "$SRC_PATH/$OCIO_TAR"
        pushd "OpenColorIO-${OCIO_VERSION}"
    fi
    # the static dependencies, in a private prefix that is removed with the build tree
    OCIO_EXT="$PWD/ext-dist"
    untar "$SRC_PATH/$OCIO_PYSTRING_TAR"
    pushd "pystring-${OCIO_PYSTRING_VERSION}"
    cp ../share/cmake/projects/Buildpystring.cmake CMakeLists.txt # as OCIO's Installpystring.cmake
    cmake -S . -B build -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DCMAKE_CXX_FLAGS="$BF" -DCMAKE_CXX_STANDARD=17 -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX="$OCIO_EXT" -DCMAKE_INSTALL_LIBDIR=lib
    cmake --build build -j${MKJOBS} --target install
    popd
    untar "$SRC_PATH/$OCIO_MINIZIP_TAR"
    pushd "minizip-ng-${OCIO_MINIZIP_VERSION}"
    # the options of OCIO's Installminizip-ng.cmake
    cmake -S . -B build -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DCMAKE_C_FLAGS="$BF" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX="$OCIO_EXT" -DCMAKE_INSTALL_LIBDIR=lib \
          -DBUILD_SHARED_LIBS=OFF -DMZ_OPENSSL=OFF -DMZ_LIBBSD=OFF -DMZ_BUILD_TESTS=OFF -DMZ_COMPAT=OFF -DMZ_BZIP2=OFF -DMZ_LZMA=OFF -DMZ_LIBCOMP=OFF -DMZ_ZSTD=OFF -DMZ_PPMD=OFF \
          -DMZ_PKCRYPT=OFF -DMZ_WZAES=OFF -DMZ_SIGNING=OFF -DMZ_ZLIB=ON -DMZ_ICONV=OFF -DMZ_FETCH_LIBS=OFF -DMZ_FORCE_FETCH_LIBS=OFF -DZLIB_ROOT="$SDK_HOME"
    cmake --build build -j${MKJOBS} --target install
    popd
    untar "$SRC_PATH/$OCIO_SSE2NEON_TAR"
    mkdir -p "$OCIO_EXT/include/sse2neon"
    cp "sse2neon-${OCIO_SSE2NEON_COMMIT}/sse2neon.h" "$OCIO_EXT/include/sse2neon/"
    mkdir build
    pushd build
    # OCIO_INSTALL_EXT_PACKAGES=NONE: never download anything at configure or build time
    cmake .. -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_INSTALL_LIBDIR=lib \
          -DOCIO_BUILD_SHARED=ON -DOCIO_BUILD_STATIC=OFF -DOCIO_BUILD_APPS=OFF -DOCIO_BUILD_NUKE=OFF \
          -DCMAKE_CXX_STANDARD=17 -DOCIO_BUILD_PYTHON=OFF -DOCIO_BUILD_TESTS=OFF -DOCIO_BUILD_GPU_TESTS=OFF -DOCIO_BUILD_DOCS=OFF -DOCIO_BUILD_OPENFX=OFF \
          -DOCIO_INSTALL_EXT_PACKAGES=NONE -Dpystring_ROOT="$OCIO_EXT" -Dminizip-ng_ROOT="$OCIO_EXT" -Dsse2neon_ROOT="$OCIO_EXT/include" \
          -DCMAKE_PREFIX_PATH="$SDK_HOME" -DFETCHCONTENT_FULLY_DISCONNECTED=ON
    make -j${MKJOBS}
    make install
    popd
    popd
    if [ "$OCIO_BUILD_GIT" = 1 ]; then
        rm -rf OpenColorIO
    else
        rm -rf "OpenColorIO-${OCIO_VERSION}"
    fi
    end_build
fi
