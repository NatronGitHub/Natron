#!/bin/bash

# Install pyside2
# see https://pypi.org/project/PySide2/
#
# PySide2 is built from source (shiboken2 module and generator, PySide2 modules,
# headers, typesystems and CMake config files, as needed to build Natron) on
# Rocky Linux and on non-x86_64 architectures: PyPI only has x86_64 wheels, and
# they don't include the shiboken2 generator. Elsewhere, the PyPI wheel is
# installed, as before.
# The source build needs libclang, installed by the Dockerfile: on Rocky Linux 9, EPEL's
# clang19-devel/llvm19-devel (in /usr/lib64/llvm19), because AppStream's clang 22 is too
# recent for shiboken2 5.15.19; on Rocky Linux 8, AppStream's clang-devel 18 (see
# SHIBOKEN2_CLANG_VERSION in build-Linux-sdk.sh).
if [ -n "${ROCKY:-}" ] || [ "${ARCH:-x86_64}" != "x86_64" ]; then
    PYSIDE2_FROM_SOURCE=1
    # the latest open-source 5.15 release
    PYSIDE2_VERSION="5.15.19"
else
    PYSIDE2_FROM_SOURCE=0
    PYSIDE2_VERSION="5.15.2.1"
    # The PyPI wheels of PySide2 5.15.2.1 require Python < 3.11, and the SDK has Python 3.11
    # (pkg/python3.sh): this path (CentOS 7 and Ubuntu SDKs on x86_64) can't build Natron 2.6,
    # whose Linux SDK is the Rocky Linux one.
    if [ "${PY3_VERSION_SHORT:-3.11}" != "3.10" ] && [ "${PY3_VERSION_SHORT:-3.11}" != "3.9" ] && [ "${PY3_VERSION_SHORT:-3.11}" != "3.8" ] && build_step; then
        (>&2 echo "Error: PyPI has no PySide2 ${PYSIDE2_VERSION} for Python ${PY3_VERSION_SHORT:-3.11}: build the SDK on Rocky Linux (ROCKY=9), which builds PySide2 from source.")
        exit 1
    fi
fi
PYSIDE2_VERSION_SHORT="5.15" # ${PYSIDE2_VERSION%.*}
PYSIDE2_TAR="pyside-setup-opensource-src-${PYSIDE2_VERSION}.tar.xz"
PYSIDE2_SITE="https://download.qt.io/official_releases/QtForPython/pyside2/PySide2-${PYSIDE2_VERSION}-src"

if [ "$PYSIDE2_FROM_SOURCE" = 1 ] && download_step; then
    download "$PYSIDE2_SITE" "$PYSIDE2_TAR"
fi
if [ "$PYSIDE2_FROM_SOURCE" = 1 ]; then
    PYSIDE2_SHIBOKEN2="$SDK_HOME/bin/shiboken2"
else
    PYSIDE2_SHIBOKEN2="$SDK_HOME/lib/python${PY3_VERSION_SHORT:-}/site-packages/PySide2/shiboken2"
fi
if build_step && { force_build || [ ! -x "$PYSIDE2_SHIBOKEN2" ] || [ "$("$PYSIDE2_SHIBOKEN2" --version | head -1 | sed -e 's/^.* v\([^ ]*\)/\1/')" != "$PYSIDE2_VERSION" ]; }; then
    start_build
    if [ "$PYSIDE2_FROM_SOURCE" = 1 ]; then
        # shiboken2 must find clang's builtin headers (stddef.h...), also when building
        # Natron: the Dockerfile sets LLVM_INSTALL_DIR in the SDK image too.
        export LLVM_INSTALL_DIR="${LLVM_INSTALL_DIR:-/usr}"
        untar "$SRC_PATH/$PYSIDE2_TAR"
        pushd "pyside-setup-opensource-src-${PYSIDE2_VERSION}"
        # libclang >= 16 support (PYSIDE-2288), backported from shiboken6 to 5.15 by
        # openSUSE, from nixpkgs c1834898eb (pkgs/development/python-modules/pyside2).
        # Without them, the generated QtGui wrappers don't compile, e.g.
        # "'DragMove' is not a member of 'QOpenGLShader'". The order matters.
        for p in "$INC_PATH"/patches/pyside2/shiboken2-clang-*.patch; do
            patch -Np1 -i "$p"
        done
        mkdir _build
        pushd _build
        # It's OK to install in $SDK_HOME and not $QT5PREFIX, even if it's qt5-specific,
        # because PySide2 only works with Qt5 anyway.
        # Modules that are not in the SDK's Qt5 are skipped.
        cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF" \
            -DCMAKE_PREFIX_PATH="$QT5PREFIX;$SDK_HOME" \
            -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DPYTHON_EXECUTABLE="$PY3_EXE" -DBUILD_TESTS=OFF \
            -DMODULES="Core;Gui;Widgets;PrintSupport;Sql;Network;Test;Concurrent;X11Extras;Xml;XmlPatterns;Help;Multimedia;MultimediaWidgets;OpenGL;OpenGLFunctions;Svg;UiTools;Qml;Quick;QuickWidgets" \
            -GNinja
        ninja -j"${MKJOBS}"
        ninja install
        popd
        popd
        rm -rf "pyside-setup-opensource-src-${PYSIDE2_VERSION}"
        # Also install QtPy for better forward-compatibility https://github.com/NatronGitHub/Natron/pull/687
        # (it was installed with PySide 1.2.4 when that was also built for Python 3)
        "$PY3_EXE" -m pip install qtpy==1.11.2
    else
        #${SDK_HOME}/bin/pip${PY3_VERSION_SHORT} install --index-url=http://download.qt.io/snapshots/ci/pyside/${PYSIDE2_VERSION}/latest/ pyside2 --trusted-host download.qt.io
        ${SDK_HOME}/bin/pip${PY3_VERSION_SHORT} install pyside2=="${PYSIDE2_VERSION}"
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
