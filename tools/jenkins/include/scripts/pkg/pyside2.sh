#!/bin/bash

# Install pyside2
# see https://pypi.org/project/PySide2/
PYSIDE2_VERSION="5.15.2"
PYSIDE2_TAR="pyside-setup-opensource-src-${PYSIDE2_VERSION}.tar.xz"
PYSIDE2_SITE="https://download.qt.io/official_releases/QtForPython/pyside2/PySide2-${PYSIDE2_VERSION}-src"

if download_step; then
    download "$PYSIDE2_SITE" "$PYSIDE2_TAR"
fi
if build_step && { force_build || { [ ! -x "$SDK_HOME/lib/python{PY3_VERSION_SHORT}$/site-packages/PySide2/shiboken2" ] || [ "$($SDK_HOME/lib/python${PY3_VERSION_SHORT}/site-packages/PySide2/shiboken2 --version|head -1 | sed -e 's/^.* v\([^ ]*\)/\1/')" != "$PYSIDE2_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$PYSIDE2_TAR"
    pushd "pyside-setup-opensource-src-${PYSIDE2_VERSION}"
    $SDK_HOME/bin/python${PY3_VERSION_SHORT} setup.py install --no-user-cfg build -j8 --verbose-build --qmake=$QT5PREFIX/bin/qmake --cmake=$SDK_HOME/bin/cmake --parallel=${MKJOBS}
    popd
    rm -rf "pyside-setup-opensource-src-${PYSIDE2_VERSION}"
    end_build
fi
