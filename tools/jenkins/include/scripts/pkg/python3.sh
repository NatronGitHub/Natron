#!/bin/bash

# Install Python3
# see http://www.linuxfromscratch.org/blfs/view/svn/general/python3.html
PY3_VERSION=3.9.7
PY3_VERSION_SHORT=${PY3_VERSION%.*}
PY3_TAR="Python-${PY3_VERSION}.tar.xz"
PY3_SITE="https://www.python.org/ftp/python/${PY3_VERSION}"

PY3VER="${PY3_VERSION_SHORT}"
PY3VERNODOT=$(echo ${PY3VER:-}| sed 's/\.//')
PY3_EXE=$SDK_HOME/bin/python3
PY3_LIB=$SDK_HOME/lib/libpython${PY3VER}.so
PY3_INC=$SDK_HOME/include/python${PY3VER}
if [ -z "${PYV:-}" ] || [ "$PYV" = "3" ]; then
    PYV=3
    PYVER="${PY3VER}"
    PYVERNODOT="${PY3VERNODOT}"
    PY_EXE="$SDK_HOME/bin/python${PYV}"
    PY_LIB="$SDK_HOME/lib/libpython${PYVER}.so"
    PY_INC="$SDK_HOME/include/python${PYVER}"
    PYTHON_HOME="$SDK_HOME"
    PYTHON_PATH="$SDK_HOME/lib/python${PYVER}"
    PYTHON_INCLUDE="$SDK_HOME/include/python${PYVER}"
    export PYTHON_PATH PYTHON_INCLUDE
fi

if download_step; then
    download "$PY3_SITE" "$PY3_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/python3.pc" ] || [ "$(pkg-config --modversion python3)" != "$PY3_VERSION_SHORT" ]; }; }; then
    start_build
    untar "$SRC_PATH/$PY3_TAR"
    pushd "Python-${PY3_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-shared --with-system-expat --with-system-ffi --with-ensurepip=yes --with-ensurepip=yes
    make -j${MKJOBS}
    make install
    if version_gt 3.8 "$PY3_VERSION"; then
        # python 3.7.x
        chmod -v 755 "$SDK_HOME/lib/libpython${PY3_VERSION_SHORT}m.so"
        chmod -v 755 "$SDK_HOME/lib/libpython3.so"
    fi
	${SDK_HOME}/bin/pip${PY3_VERSION_SHORT} install --upgrade pip
    popd
    rm -rf "Python-${PY3_VERSION}"
    end_build
fi
