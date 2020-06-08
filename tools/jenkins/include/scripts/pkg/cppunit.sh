#!/bin/bash

# install cppunit
CPPUNIT_VERSION=1.15.1 # 1.14.0 is the first version to require c++11
CPPUNIT_SITE="http://dev-www.libreoffice.org/src"
if version_gt 5.0.0 "$GCC_VERSION"; then
    CPPUNIT_VERSION=1.13.2 # does not require c++11
    CPPUNIT_SITE="http://dev-www.libreoffice.org/src/cppunit"
fi
CPPUNIT_TAR="cppunit-${CPPUNIT_VERSION}.tar.gz"
if download_step; then
    download "$CPPUNIT_SITE" "$CPPUNIT_TAR"
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
