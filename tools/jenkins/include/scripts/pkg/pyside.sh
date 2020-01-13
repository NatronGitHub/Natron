#!/bin/bash

# Install pyside
PYSIDE_VERSION=1.2.4
#PYSIDE_TAR="pyside-qt4.8+${PYSIDE_VERSION}.tar.bz2"
#PYSIDE_SITE="https://download.qt.io/official_releases/pyside"
PYSIDE_TAR="PySide-${PYSIDE_VERSION}.tar.gz"
if download_step; then
    #download "$PYSIDE_SITE" "$PYSIDE_TAR"
    download_github pyside PySide "${PYSIDE_VERSION}" "" "${PYSIDE_TAR}"
fi
if dobuild; then
    PYSIDE_PREFIX="$QT4PREFIX"
fi
if build_step && { force_build || { [ ! -s "$PYSIDE_PREFIX/lib/pkgconfig/pyside${PYSIDE_V:-}.pc" ] || [ "$(env PKG_CONFIG_PATH=$PYSIDE_PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion pyside)" != "$PYSIDE_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$PYSIDE_TAR"
    #pushd "pyside-qt4.8+${PYSIDE_VERSION}"
    pushd "PySide-${PYSIDE_VERSION}"

    mkdir -p build
    pushd build
    cmake .. -DCMAKE_INSTALL_PREFIX="$PYSIDE_PREFIX" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DBUILD_TESTS=OFF \
          -DCMAKE_PREFIX_PATH="$SDK_HOME;$PYSIDE_PREFIX;$QT4PREFIX" \
          -DQT_QMAKE_EXECUTABLE="$QT4PREFIX/bin/qmake" \
          -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF" \
          -DCMAKE_SHARED_LINKER_FLAGS="-L$QT4PREFIX/lib" \
          -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
          -DPYTHON_EXECUTABLE="$PY_EXE" \
          -DPYTHON_LIBRARY="$PY_LIB" \
          -DPYTHON_INCLUDE_DIR="$PY_INC"
    make -j${MKJOBS}
    make install
    popd
    popd
    #rm -rf "pyside-qt4.8+${PYSIDE_VERSION}"
    rm -rf "PySide-${PYSIDE_VERSION}"
    end_build
fi
