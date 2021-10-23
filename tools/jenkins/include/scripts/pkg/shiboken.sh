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
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/shiboken-py$PYV.pc" ] || [ "$(env PKG_CONFIG_PATH=$SDK_HOME/lib/pkgconfig:$QT4PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion shiboken-py$PYV)" != "$SHIBOKEN_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$SHIBOKEN_TAR"
    pushd "Shiboken-$SHIBOKEN_VERSION"

    # Patches from https://github.com/macports/macports-ports/tree/master/python/py-shiboken/
    patch -Np0 -i "$INC_PATH/patches/shiboken/default_visibility.patch"
    patch -Np0 -i "$INC_PATH/patches/shiboken/patch-cmakepkgconfig.diff"
    patch -Np0 -i "$INC_PATH/patches/shiboken/patch-tpprint-py3.patch"

    mkdir -p build-py2
    pushd build-py2
    # note: PYTHONBRANCH and PYTHONPREFIX are variables added by patch-cmakepkgconfig.diff
    env  cmake ../ -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DCMAKE_INSTALL_PREFIX="$SDK_HOME"  \
        -DCMAKE_PREFIX_PATH="$SDK_HOME;$QT4PREFIX" \
        -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF -I${SDK_HOME}/include/libxml2" \
        -DCMAKE_SHARED_LINKER_FLAGS="-L$QT4PREFIX/lib" \
        -DCMAKE_EXE_LINKER_FLAGS="-L$QT4PREFIX/lib" \
        -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
        -DBUILD_TESTS=OFF            \
        -DPYTHONBRANCH="${PY2VER}" \
        -DPYTHONPREFIX="${SDK_HOME}" \
        -DQT_QMAKE_EXECUTABLE="$QT4PREFIX/bin/qmake"
    make -j${MKJOBS}
    make install
    cd data
    mkdir "$SDK_HOME/lib/cmake/Shiboken-$SHIBOKEN_VERSION/python${PY2VER}" || true
    mv "$SDK_HOME/lib/cmake/Shiboken-$SHIBOKEN_VERSION/ShibokenConfig.cmake" "$SDK_HOME/lib/cmake/Shiboken-$SHIBOKEN_VERSION/python${PY2VER}/"
    mv "$SDK_HOME/lib/cmake/Shiboken-$SHIBOKEN_VERSION/ShibokenConfigVersion.cmake" "$SDK_HOME/lib/cmake/Shiboken-$SHIBOKEN_VERSION/python${PY2VER}/"
    mv "$SDK_HOME/lib/pkgconfig/shiboken.pc" "$SDK_HOME/lib/pkgconfig/shiboken-py2.pc"
    mv "$SDK_HOME/include/shiboken" "$SDK_HOME/include/shiboken-${PY2VER}"
    mv "$SDK_HOME/bin/shiboken" "$SDK_HOME/bin/shiboken-${PY2VER}"
    mv "$SDK_HOME/share/man/man1/shiboken.1" $"$SDK_HOME/share/man/man1/shiboken-${PY2VER}.1"
    popd

    mkdir -p build-py3
    pushd build-py3
    # note: PYTHONBRANCH and PYTHONPREFIX are variables added by patch-cmakepkgconfig.diff
    env  cmake ../ -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DCMAKE_INSTALL_PREFIX="$SDK_HOME"  \
        -DCMAKE_PREFIX_PATH="$SDK_HOME;$QT4PREFIX" \
        -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF -I${SDK_HOME}/include/libxml2" \
        -DCMAKE_SHARED_LINKER_FLAGS="-L$QT4PREFIX/lib" \
        -DCMAKE_EXE_LINKER_FLAGS="-L$QT4PREFIX/lib" \
        -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
        -DBUILD_TESTS=OFF            \
        -DPYTHONBRANCH="${PY3VER}" \
        -DPYTHONPREFIX="${SDK_HOME}" \
        -DUSE_PYTHON3=yes \
        -DPYTHON3_EXECUTABLE="$PY3_EXE" \
        -DPYTHON3_LIBRARY="$PY3_LIB" \
        -DPYTHON3_INCLUDE_DIR="$PY3_INC" \
        -DQT_QMAKE_EXECUTABLE="$QT4PREFIX/bin/qmake"
    make -j${MKJOBS}
    make install
    mkdir "$SDK_HOME/lib/cmake/Shiboken-$SHIBOKEN_VERSION/python${PY3VER}" || true
    mv "$SDK_HOME/lib/cmake/Shiboken-$SHIBOKEN_VERSION/ShibokenConfig.cmake" "$SDK_HOME/lib/cmake/Shiboken-$SHIBOKEN_VERSION/python${PY3VER}/"
    mv "$SDK_HOME/lib/cmake/Shiboken-$SHIBOKEN_VERSION/ShibokenConfigVersion.cmake" "$SDK_HOME/lib/cmake/Shiboken-$SHIBOKEN_VERSION/python${PY3VER}/"
    mv "$SDK_HOME/lib/pkgconfig/shiboken.pc" "$SDK_HOME/lib/pkgconfig/shiboken-py3.pc"
    mv "$SDK_HOME/include/shiboken" "$SDK_HOME/include/shiboken-${PY3VER}"
    mv "$SDK_HOME/bin/shiboken" "$SDK_HOME/bin/shiboken-${PY3VER}"
    mv "$SDK_HOME/share/man/man1/shiboken.1" $"$SDK_HOME/share/man/man1/shiboken-${PY3VER}.1"
    popd

    # Copy back the version corresponding to the default Python version
    #cp "$SDK_HOME/lib/pkgconfig/shiboken-py${PYV}.pc" "$SDK_HOME/lib/pkgconfig/shiboken.pc"

    popd
    rm -rf "Shiboken-$SHIBOKEN_VERSION"
    end_build
fi
