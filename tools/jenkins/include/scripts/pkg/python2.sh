#!/bin/bash

# Install Python2
# see http://www.linuxfromscratch.org/blfs/view/svn/general/python2.html
PY2_VERSION=2.7.16
PY2_VERSION_SHORT=${PY2_VERSION%.*}
PY2_TAR="Python-${PY2_VERSION}.tar.xz"
PY2_SITE="https://www.python.org/ftp/python/${PY2_VERSION}"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/python2.pc" ] || [ "$(pkg-config --modversion python2)" != "$PY2_VERSION_SHORT" ]; }; }; then
    start_build
    download "$PY2_SITE" "$PY2_TAR"
    untar "$SRC_PATH/$PY2_TAR"
    pushd "Python-${PY2_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-shared --with-system-expat --with-system-ffi --with-ensurepip=yes --enable-unicode=ucs4
    make -j${MKJOBS}
    make install
    chmod -v 755 "$SDK_HOME/lib/libpython2.7.so.1.0"
    ${SDK_HOME}/bin/pip${PY2_VERSION_SHORT} install --upgrade pip
    popd
    rm -rf "Python-${PY2_VERSION}"
    end_build
fi
