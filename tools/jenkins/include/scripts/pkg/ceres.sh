#!/bin/bash

# Install Ceres Solver (Natron's tracker), configured as Natron's bundled libs/ceres:
# a static library with its miniglog (no glog, no gflags), dense linear algebra with Eigen only.
CERES_VERSION=2.2.0
CERES_TAR="ceres-solver-${CERES_VERSION}.tar.gz"
CERES_SHA256=12efacfadbfdc1bbfa203c236e96f4d3c210bed96994288b3ff0c8e7c6f350d4
if download_step; then
    download_github ceres-solver ceres-solver "$CERES_VERSION" "" "$CERES_TAR" "$CERES_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libceres.a" ]; }; }; then
    start_build
    untar "$SRC_PATH/$CERES_TAR"
    pushd "ceres-solver-${CERES_VERSION}"
    mkdir build
    pushd build
    cmake .. -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF" -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=OFF \
        -DMINIGLOG=ON -DMINIGLOG_MAX_LOG_LEVEL=0 -DGFLAGS=OFF -DSUITESPARSE=OFF -DLAPACK=OFF -DEIGENSPARSE=OFF -DUSE_CUDA=OFF \
        -DSCHUR_SPECIALIZATIONS=OFF -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF -DBUILD_BENCHMARKS=OFF -DBUILD_DOCUMENTATION=OFF \
        -DEigen3_DIR="$SDK_HOME/share/eigen3/cmake"
    make -j${MKJOBS}
    make install
    popd
    popd
    rm -rf "ceres-solver-${CERES_VERSION}"
    end_build
fi
