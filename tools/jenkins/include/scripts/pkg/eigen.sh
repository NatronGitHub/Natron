#!/bin/bash

exit 1

# Install eigen
EIGEN_TAR=eigen-eigen-bdd17ee3b1b3.tar.gz
if build_step && { force_build || { [ ! -s $SDK_HOME/lib/pkgconfig/eigen3.pc ]; }; }; then
    start_build
    download "$EIGEN_SITE" "$EIGEN_TAR"
    untar "$SRC_PATH/$EIGEN_TAR"
    pushd eigen-*
    rm -rf build
    mkdir build
    cd build
    env CFLAGS="$BF" CXXFLAGS="$BF" cmake .. -DCMAKE_INSTALL_PREFIX=$SDK_HOME -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
    make -j${MKJOBS}
    make install
    mv $SDK_HOME/share/pkgconfig/* $SDK_HOME/lib/pkgconfig
    popd
    rm -rf eigen-*
    end_build
fi
