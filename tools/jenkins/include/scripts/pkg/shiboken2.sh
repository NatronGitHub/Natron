#!/bin/bash


# Install shiboken2. Requires clang. Not required for PySide2 5.6, which bundles shiboken2
SHIBOKEN2_VERSION=2
SHIBOKEN2_GIT="https://code.qt.io/pyside/shiboken"
SHIBOKEN2_COMMIT="5.9"
# SKIP: not required when using pyside-setup.git, see below
if false; then #[[ build_step && ( force_build || ( [ ! -s "$QT5PREFIX/lib/pkgconfig/shiboken2.pc" ] || [ "$(env PKG_CONFIG_PATH=$QT5PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion "shiboken2")" != "$SHIBOKEN2_VERSION" ]; }; }; then
    start_build
    git_clone_commit "$SHIBOKEN2_GIT" "$SHIBOKEN2_COMMIT"
    pushd shiboken

    mkdir -p build
    pushd build
    env PATH="$SDK_HOME/llvm/bin:$PATH" cmake ../ -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DCMAKE_INSTALL_PREFIX="$QT5PREFIX"  \
        -DCMAKE_PREFIX_PATH="$SDK_HOME;$QT5PREFIX" \
        -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF -I${SDK_HOME}/include/libxml2" \
        -DCMAKE_SHARED_LINKER_FLAGS="-L${QT5PREFIX}/lib" \
        -DCMAKE_EXE_LINKER_FLAGS="-L${QT5PREFIX}/lib" \
        -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
        -DBUILD_TESTS=OFF            \
        -DPYTHON_EXECUTABLE="$PY_EXE" \
        -DPYTHON_LIBRARY="$PY_LIB" \
        -DPYTHON_INCLUDE_DIR="$PY_INC" \
        -DUSE_PYTHON3="$USE_PY3" \
        -DQT_QMAKE_EXECUTABLE="$QT5PREFIX/bin/qmake"
    make -j${MKJOBS} 
    make install
    popd
    popd
    rm -rf shiboken
    end_build
fi
