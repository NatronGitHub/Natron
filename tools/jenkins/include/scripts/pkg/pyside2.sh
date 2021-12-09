#!/bin/bash

# Install pyside2
# see https://pypi.org/project/PySide2/
PYSIDE2_VERSION="5.15.2"
PYSIDE2_VERSION_SHORT=${PYSIDE2_VERSION%.*}
#PYSIDE2_TAR="pyside-setup-opensource-src-${PYSIDE2_VERSION}.tar.xz"
#PYSIDE2_SITE="https://download.qt.io/official_releases/QtForPython/pyside2/PySide2-${PYSIDE2_VERSION}-src"

#if download_step; then
#    download "$PYSIDE2_SITE" "$PYSIDE2_TAR"
#fi
if build_step && { force_build || { [ ! -x "$SDK_HOME/lib/python{PY3_VERSION_SHORT}$/site-packages/PySide2/shiboken2" ] || [ "$($SDK_HOME/lib/python${PY3_VERSION_SHORT}/site-packages/PySide2/shiboken2 --version|head -1 | sed -e 's/^.* v\([^ ]*\)/\1/')" != "$PYSIDE2_VERSION" ]; }; }; then
    start_build
    if false; then
        # Building from source requires LLVM
        untar "$SRC_PATH/$PYSIDE2_TAR"
        pushd "pyside-setup-opensource-src-${PYSIDE2_VERSION}"
        # Don't copy Qt5 tools: 
        # https://src.fedoraproject.org/rpms/python-pyside2/raw/009100c67a63972e4c5252576af1894fec2e8855/f/pyside2-tools-obsolete.patch
        #patch -Np1 -i "$INC_PATH"/patches/pyside2/pyside2-tools-obsolete.patch

        #$SDK_HOME/bin/python${PY3_VERSION_SHORT} setup.py install --no-user-cfg build -j8 --verbose-build --qmake=$QT5PREFIX/bin/qmake --cmake=$SDK_HOME/bin/cmake --parallel=${MKJOBS}
        mkdir _build
        pushd _build
        # It's OK to install in $SDK_HOME and not $QT5PREFIX, even if it's qt5-specific,
        # because PySide2 only works with Qt5 anyway.
        cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  \
        -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DPYTHON_EXECUTABLE="$PY3_EXE" -GNinja
        ninja install
        popd
        popd
        rm -rf "pyside-setup-opensource-src-${PYSIDE2_VERSION}"
    else
        #${SDK_HOME}/bin/pip${PY3_VERSION_SHORT} install --index-url=http://download.qt.io/snapshots/ci/pyside/${PYSIDE2_VERSION}/latest/ pyside2 --trusted-host download.qt.io
        ${SDK_HOME}/bin/pip${PY3_VERSION_SHORT} install pyside2
        pushd ${SDK_HOME}/lib/python${PY3_VERSION_SHORT}/site-packages/PySide2
        ln -s ../shiboken2/libshiboken2.abi3.so.${PYSIDE2_VERSION_SHORT} .
        cd Qt/lib
        for f in *.so.5; do
            rm $f
            ln -s ${QT5PREFIX}/lib/$f .
        done
        popd
    fi
    end_build
fi
