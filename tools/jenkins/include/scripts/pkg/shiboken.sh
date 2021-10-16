#!/bin/bash

# Install shiboken
# See https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=shiboken
# for building both for Python 2 and Python 3.
SHIBOKEN_VERSION=1.2.4
#SHIBOKEN_TAR="shiboken-${SHIBOKEN_VERSION}.tar.bz2"
#SHIBOKEN_SITE="https://download.qt.io/official_releases/pyside"
SHIBOKEN_TAR="Shiboken-${SHIBOKEN_VERSION}.tar.gz"
if download_step; then
    #download "$SHIBOKEN_SITE" "$SHIBOKEN_TAR"
    download_github pyside Shiboken "${SHIBOKEN_VERSION}" "" "${SHIBOKEN_TAR}"
fi
if dobuild; then
    SHIBOKEN_PREFIX="$QT4PREFIX"
fi
if build_step && { force_build || { [ ! -s "$SHIBOKEN_PREFIX/lib/pkgconfig/shiboken.pc" ] || [ "$(env PKG_CONFIG_PATH=$SHIBOKEN_PREFIX/lib/pkgconfig:$QT4PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion shiboken)" != "$SHIBOKEN_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$SHIBOKEN_TAR"
    #pushd "shiboken-$SHIBOKEN_VERSION"
    pushd "Shiboken-$SHIBOKEN_VERSION"

    mkdir -p build
    pushd build
    env  cmake ../ -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DCMAKE_INSTALL_PREFIX="$SHIBOKEN_PREFIX"  \
        -DCMAKE_PREFIX_PATH="$SDK_HOME;$SHIBOKEN_PREFIX;$QT4PREFIX" \
        -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF -I${SDK_HOME}/include/libxml2" \
        -DCMAKE_SHARED_LINKER_FLAGS="-L$QT4PREFIX/lib" \
        -DCMAKE_EXE_LINKER_FLAGS="-L$QT4PREFIX/lib" \
        -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
        -DBUILD_TESTS=OFF            \
        -DPYTHON_EXECUTABLE="$PY_EXE" \
        -DPYTHON_LIBRARY="$PY_LIB" \
        -DPYTHON_INCLUDE_DIR="$PY_INC" \
        -DUSE_PYTHON3="$USE_PY3" \
        -DQT_QMAKE_EXECUTABLE="$QT4PREFIX/bin/qmake"
    make -j${MKJOBS} 
    make install
    popd
    popd
    rm -rf "shiboken-$SHIBOKEN_VERSION"
    end_build
fi
