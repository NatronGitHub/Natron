#!/bin/bash

# Install Python3
# see http://www.linuxfromscratch.org/blfs/view/svn/general/python3.html
PY3_VERSION=3.9.4
PY3_VERSION_SHORT=${PY3_VERSION%.*}
PY3_TAR="Python-${PY3_VERSION}.tar.xz"
PY3_SITE="https://www.python.org/ftp/python/${PY3_VERSION}"
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
