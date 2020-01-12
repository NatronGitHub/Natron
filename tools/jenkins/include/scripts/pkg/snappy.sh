#!/bin/bash

# Install snappy (for ffmpeg)
# see https://github.com/google/snappy/releases
SNAPPY_VERSION=1.1.7
SNAPPY_TAR="snappy-${SNAPPY_VERSION}.tar.gz"
SNAPPY_SITE="https://github.com/google/snappy/releases/download/${SNAPPY_VERSION}"
if download_step; then
    #download "$SNAPPY_SITE" "$SNAPPY_TAR"
    download_github google snappy "${SNAPPY_VERSION}" "" "${SNAPPY_TAR}"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libsnappy.so" ]; }; }; then
    REBUILD_FFMPEG=1
    start_build
    untar "$SRC_PATH/$SNAPPY_TAR"
    pushd "snappy-${SNAPPY_VERSION}"
    mkdir tmp_build
    pushd tmp_build
    cmake .. -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF -DNDEBUG"  -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DBUILD_SHARED_LIBS=ON -DSNAPPY_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
    make -j${MKJOBS}
    make install
    popd
    popd
    rm -rf "snappy-${SNAPPY_VERSION}"
    end_build
fi
