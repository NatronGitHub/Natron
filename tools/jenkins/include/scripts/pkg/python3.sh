#!/bin/bash

# Install Python3
# see http://www.linuxfromscratch.org/blfs/view/svn/general/python3.html

# Note:
# - Python 3.11.0 breaks the Shiboken 1.2.4 build, so
#   so 3.10 may be the last one to work with Qt4.
# /home/tmp/Shiboken-1.2.4/libshiboken/sbkenum.cpp: In function 'PyTypeObject* Shiboken::Enum::newTypeWithName(const char*, const char*)':
# /opt/Natron-sdk/include/python3.11/object.h:136:30: error: lvalue required as left operand of assignment
#   136 | #  define Py_TYPE(ob) Py_TYPE(_PyObject_CAST(ob))
#       |                       ~~~~~~~^~~~~~~~~~~~~~~~~~~~
PY3_VERSION=3.10.11 # 3.11.0 breaks Shiboken 1.2.4
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
	${SDK_HOME}/bin/pip${PY3_VERSION_SHORT} install --upgrade pip setuptools wheel
    popd
    rm -rf "Python-${PY3_VERSION}"
    end_build
fi
