#!/bin/bash

# Install Eigen (headers and CMake config, for Ceres Solver)
EIGEN_VERSION=3.4.1
EIGEN_TAR="eigen-${EIGEN_VERSION}.tar.gz"
EIGEN_SITE="https://gitlab.com/libeigen/eigen/-/archive/${EIGEN_VERSION}"
EIGEN_SHA256=b93c667d1b69265cdb4d9f30ec21f8facbbe8b307cf34c0b9942834c6d4fdbe2
if download_step; then
    download "$EIGEN_SITE" "$EIGEN_TAR" "$EIGEN_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/share/eigen3/cmake/Eigen3Config.cmake" ]; }; }; then
    start_build
    untar "$SRC_PATH/$EIGEN_TAR"
    pushd "eigen-${EIGEN_VERSION}"
    mkdir build
    pushd build
    cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DBUILD_TESTING=OFF -DEIGEN_BUILD_DOC=OFF
    make install
    popd
    popd
    rm -rf "eigen-${EIGEN_VERSION}"
    end_build
fi
