#!/bin/bash

# Install itstool
# see http://www.linuxfromscratch.org/blfs/view/svn/pst/itstool.html
ITSTOOL_VERSION=2.0.7
ITSTOOL_TAR="itstool-${ITSTOOL_VERSION}.tar.bz2"
ITSTOOL_SITE="http://files.itstool.org/itstool"
if download_step; then
    download "$ITSTOOL_SITE" "$ITSTOOL_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/itstool" ] || [ "$(itstool -v)" != "itstool $ITSTOOL_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$ITSTOOL_TAR"
    pushd "itstool-${ITSTOOL_VERSION}"
    env PYTHON="$SDK_HOME/bin/python3" CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "itstool-${ITSTOOL_VERSION}"
    end_build
fi
