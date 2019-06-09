#!/bin/bash

# Install pyside2
PYSIDE2_VERSION="5.6.0"
PYSIDE2_GIT="https://code.qt.io/pyside/pyside-setup"
PYSIDE2_COMMIT="5.6" # 5.9 is unstable, see https://github.com/fredrikaverpil/pyside2-wheels/issues/55
if build_step && { force_build || { [ ! -x "$SDK_HOME/lib/python${PY2_VERSION_SHORT}/site-packages/PySide2/shiboken2" ] || [ "$($SDK_HOME/lib/python${PY2_VERSION_SHORT}/site-packages/PySide2/shiboken2 --version|head -1 | sed -e 's/^.* v\([^ ]*\)/\1/')" != "$PYSIDE2_VERSION" ]; }; }; then
    start_build
    git_clone_commit "$PYSIDE2_GIT" "$PYSIDE2_COMMIT"
    pushd pyside-setup

    # mkdir -p build
    # pushd build
    # cmake .. -DCMAKE_INSTALL_PREFIX="$QT5PREFIX" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DBUILD_TESTS=OFF \
    #       -DCMAKE_PREFIX_PATH="$SDK_HOME;$QT5PREFIX" \
    #       -DQT_QMAKE_EXECUTABLE="$QT5PREFIX/bin/qmake" \
    #       -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF" \
    #       -DCMAKE_SHARED_LINKER_FLAGS="-L$QT5PREFIX/lib" \
    #       -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
    #       -DPYTHON_EXECUTABLE="$PY_EXE" \
    #       -DPYTHON_LIBRARY="$PY_LIB" \
    #       -DPYTHON_INCLUDE_DIR="$PY_INC"
    # make -j${MKJOBS}
    # make install
    # popd

    # it really installs in SDK_HOME, whatever option is given to --prefix
    env CC="$CC" CXX="$CXX" CMAKE_PREFIX_PATH="$SDK_HOME;$QT5PREFIX" python${PYV} setup.py install --qmake "$QT5PREFIX/bin/qmake" --prefix "$QT5PREFIX" --openssl "$SDK_HOME/bin/openssl" --record files.txt
    cat files.txt
    ln -sf "$SDK_HOME/lib/python${PY2_VERSION_SHORT}/site-packages/PySide2/shiboken2" "$QT5PREFIX/bin/shiboken2"
    popd
    rm -rf pyside-setup
    end_build
fi
