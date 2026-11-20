#!/bin/bash

# Install libde265
# see https://github.com/strukturag/libde265/releases
LIBDE265_VERSION=1.1.1
LIBDE265_TAR="libde265-${LIBDE265_VERSION}.tar.gz"
LIBDE265_SITE="https://github.com/strukturag/libde265/releases/download/v${LIBDE265_VERSION}"
if download_step; then
    download "$LIBDE265_SITE" "$LIBDE265_TAR"
    #download_github strukturag libde265 "$LIBDE265_VERSION" v "$LIBDE265_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libde265.pc" ] || [ "$(pkg-config --modversion libde265)" != "$LIBDE265_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBDE265_TAR"
    pushd "libde265-${LIBDE265_VERSION}"
    if [ -x configure ]; then
        env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
        make -j${MKJOBS}
        make install
    else
        # libde265 >= 1.0.17 only builds with CMake
        mkdir build
        pushd build
        cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF" \
              -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DBUILD_SHARED_LIBS=ON -DENABLE_DECODER=OFF -DENABLE_ENCODER=OFF -DENABLE_SDL=OFF
        make -j${MKJOBS}
        make install
        popd
    fi
    popd
    rm -rf "libde265-${LIBDE265_VERSION}"
    end_build
fi
