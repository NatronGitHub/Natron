#!/bin/bash

# Install openexr
# see https://github.com/openexr/openexr/releases/
# Note: upgrading to 2.4.1-2.5.1 requires rewriting the download and build scripts
# (openexr is now a single package).
#EXR_VERSION=2.5.7
EXR_VERSION=3.5.4
EXR_CXX11_CONFIGURE_FLAGS=
if version_gt "2.4.0" "${EXR_VERSION}"; then
    EXR_CXX11_CONFIGURE_FLAGS=--enable-cxxstd=11
else
    EXR_CXX11_CONFIGURE_FLAGS=-DCMAKE_CXX_STANDARD=17 # OpenEXR >= 3.3 requires C++17
fi

if version_gt "2.4.0" "${EXR_VERSION}"; then # ilmbase is not a separate build, starting from 2.4.0

EXR_ILM_TAR="ilmbase-${EXR_VERSION}.tar.gz"
EXR_SITE="https://github.com/openexr/openexr/releases/download/v${EXR_VERSION}"

if download_step; then
    download "$EXR_SITE" "$EXR_ILM_TAR"
fi
if build_step && { force_build || { [ "${REBUILD_EXR:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/lib/libI* "$SDK_HOME"/lib/libHalf* || true
    rm -f "$SDK_HOME"/lib/pkgconfig/{OpenEXR,IlmBase}.pc || true
    rm -f "$SDK_HOME"/bin/exr* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/IlmBase.pc" ] || [ "$(pkg-config --modversion IlmBase)" != "$EXR_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$EXR_ILM_TAR"
    pushd "ilmbase-${EXR_VERSION}"
    if version_gt "2.3.0" "${EXR_VERSION}"; then
	patch -p2 -i "$INC_PATH/patches/IlmBase/ilmbase-2.2.0-threadpool_release_lock_single_thread.patch"
	# https://github.com/lgritz/openexr/tree/lg-register
	patch -p2 -i "$INC_PATH/patches/IlmBase/ilmbase-2.2.0-lg-register.patch"
	# https://github.com/lgritz/openexr/tree/lg-cpp11
	patch -p2 -i "$INC_PATH/patches/IlmBase/ilmbase-2.2.0-lg-c++11.patch"
    fi
    #  mkdir build; cd build
    #cmake .. -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF -I${SDK_HOME}/include/OpenEXR" -DCMAKE_SHARED_LINKER_FLAGS="-L${SDK_HOME}/lib" -DCMAKE_C_COMPILER="$SDK_HOME/gcc/bin/gcc" -DCMAKE_CXX_COMPILER="${SDK_HOME}/gcc/bin/g++" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DBUILD_SHARED_LIBS=ON -DNAMESPACE_VERSIONING=ON -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static ${EXR_CXX11_CONFIGURE_FLAGS}
    make -j${MKJOBS}
    make install
    cp IlmBase.pc "$SDK_HOME/lib/pkgconfig/"
    popd
    rm -rf "ilmbase-${EXR_VERSION}"
    end_build
fi
else
# OpenEXR >= 3 uses Imath, the successor of IlmBase. Build it here, or OpenEXR fetches and builds its own.
# see https://github.com/AcademySoftwareFoundation/Imath/releases
IMATH_VERSION=3.2.3
IMATH_TAR="Imath-${IMATH_VERSION}.tar.gz"
IMATH_SHA256=e10c12b3f21f45bf08e09d4215d9c7691368d747beebd840de0b6fefed2df9f8
if download_step; then
    download_github AcademySoftwareFoundation Imath "${IMATH_VERSION}" v "${IMATH_TAR}" "$IMATH_SHA256"
fi
if build_step && { force_build || { [ "${REBUILD_EXR:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/lib/libImath* "$SDK_HOME"/include/Imath "$SDK_HOME"/lib/cmake/Imath || true
    rm -f "$SDK_HOME"/lib/pkgconfig/Imath.pc || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/Imath.pc" ] || [ "$(pkg-config --modversion Imath)" != "$IMATH_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$IMATH_TAR"
    pushd "Imath-${IMATH_VERSION}"
    mkdir build
    pushd build
    cmake .. -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_INSTALL_LIBDIR=lib \
          -DCMAKE_CXX_STANDARD=17 -DBUILD_TESTING=OFF -DPYTHON=OFF -DIMATH_INSTALL_PKG_CONFIG=ON
    make -j${MKJOBS}
    make install
    popd
    popd
    rm -rf "Imath-${IMATH_VERSION}"
    end_build
fi
fi
