#!/bin/bash

# Install pyside
# See https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=python-pyside
# for building both for Python 2 and Python 3.
PYSIDE_VERSION=1.2.4
#PYSIDE_TAR="pyside-qt4.8+${PYSIDE_VERSION}.tar.bz2"
#PYSIDE_SITE="https://download.qt.io/official_releases/pyside"
PYSIDE_TAR="PySide-${PYSIDE_VERSION}.tar.gz"
if download_step; then
    #download "$PYSIDE_SITE" "$PYSIDE_TAR"
    download_github pyside PySide "${PYSIDE_VERSION}" "" "${PYSIDE_TAR}"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/pyside-py$PYV.pc" ] || [ "$(env PKG_CONFIG_PATH=$SDK_HOME/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion pyside-py$PYV)" != "$PYSIDE_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$PYSIDE_TAR"
    #pushd "pyside-qt4.8+${PYSIDE_VERSION}"
    pushd "PySide-${PYSIDE_VERSION}"

    # Patches from https://github.com/macports/macports-ports/tree/master/python/py-pyside/
    patch -Np0 -i "$INC_PATH/patches/pyside/patch-cmakepkgconfig.diff"

    fgrep Shiboken CMakeLists.txt
    mkdir -p build-py2
    pushd build-py2
    # note: PYTHONBRANCH and PYTHONPREFIX are variables added by patch-cmakepkgconfig.diff
    cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DBUILD_TESTS=OFF \
          -DShiboken_DIR="$SDK_HOME/lib/cmake/Shiboken-$SHIBOKEN_VERSION/python${PY2VER}" \
          -DCMAKE_PREFIX_PATH="$SDK_HOME;$QT4PREFIX" \
          -DQT_QMAKE_EXECUTABLE="$QT4PREFIX/bin/qmake" \
          -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF" \
          -DCMAKE_SHARED_LINKER_FLAGS="-L$QT4PREFIX/lib" \
          -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
          -DSHIBOKEN_PYTHON_INTERPRETER="$PY2_EXE" \
          -DPYTHONBRANCH="${PY2VER}" \
          -DPYTHONPREFIX="${SDK_HOME}"
    make -j${MKJOBS}
    make install
    #cp -r "../pyside_package/PySide.egg-info" "$SDK_HOME/lib/python${PY2VER}/site-packages/pyside-${PYSIDE_VERSION}-py${PY2VER}.egg-info"
    ls -l "$SDK_HOME/lib/pkgconfig/pyside"*
    mv "$SDK_HOME/lib/pkgconfig/pyside.pc" "$SDK_HOME/lib/pkgconfig/pyside-py2.pc"
    $GSED -i 's#^Requires: shiboken$#Requires: shiboken-py2#' "$SDK_HOME/lib/pkgconfig/pyside-py2.pc"
    mv "$SDK_HOME/include/PySide" "$SDK_HOME/include/PySide-${PY2VER}"
    mv "$SDK_HOME/share/PySide" "$SDK_HOME/share/PySide-${PY2VER}"
    ls "$SDK_HOME/lib/cmake"
    #mv "$SDK_HOME/lib/cmake" "${destroot}${python.prefix}/lib/cmake"
    popd

    mkdir -p build-py3
    pushd build-py3
    # note: PYTHONBRANCH and PYTHONPREFIX are variables added by patch-cmakepkgconfig.diff
    cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DBUILD_TESTS=OFF \
          -DShiboken_DIR="$SDK_HOME/lib/cmake/Shiboken-$SHIBOKEN_VERSION/python${PY3VER}" \
          -DCMAKE_PREFIX_PATH="$SDK_HOME;$QT4PREFIX" \
          -DQT_QMAKE_EXECUTABLE="$QT4PREFIX/bin/qmake" \
          -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF" \
          -DCMAKE_SHARED_LINKER_FLAGS="-L$QT4PREFIX/lib" \
          -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
          -DSHIBOKEN_PYTHON_INTERPRETER="$PY3_EXE" \
          -DPYTHONBRANCH="${PY3VER}" \
          -DPYTHONPREFIX="${SDK_HOME}"
    make -j${MKJOBS}
    make install
    #cp -r "../pyside_package/PySide.egg-info" "$SDK_HOME/lib/python${PY3VER}/site-packages/pyside-${PYSIDE_VERSION}-py${PY3VER}.egg-info"
    ls -l "$SDK_HOME/lib/pkgconfig/pyside"*
    mv "$SDK_HOME/lib/pkgconfig/pyside.pc" "$SDK_HOME/lib/pkgconfig/pyside-py3.pc"
    $GSED -i 's#^Requires: shiboken$#Requires: shiboken-py3#' "$SDK_HOME/lib/pkgconfig/pyside-py3.pc"
    mv "$SDK_HOME/include/PySide" "$SDK_HOME/include/PySide-${PY3VER}"
    mv "$SDK_HOME/share/PySide" "$SDK_HOME/share/PySide-${PY3VER}"
    ls "$SDK_HOME/lib/cmake"
    popd


    popd
    #rm -rf "pyside-qt4.8+${PYSIDE_VERSION}"
    rm -rf "PySide-${PYSIDE_VERSION}"
    end_build
fi
