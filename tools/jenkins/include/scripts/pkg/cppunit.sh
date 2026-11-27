#!/bin/bash

# install cppunit
CPPUNIT_VERSION=1.15.1
CPPUNIT_SITE="https://dev-www.libreoffice.org/src"
CPPUNIT_TAR="cppunit-${CPPUNIT_VERSION}.tar.gz"
CPPUNIT_SHA256=89c5c6665337f56fd2db36bc3805a5619709d51fb136e51937072f63fcc717a7
if download_step; then
    download "$CPPUNIT_SITE" "$CPPUNIT_TAR" "$CPPUNIT_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/cppunit.pc" ] || [ "$(pkg-config --modversion cppunit)" != "$CPPUNIT_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$CPPUNIT_TAR"
    pushd "cppunit-${CPPUNIT_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "cppunit-${CPPUNIT_VERSION}"
    end_build
fi
