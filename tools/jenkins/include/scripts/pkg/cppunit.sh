#!/bin/bash

# install cppunit
CPPUNIT_VERSION=1.13.2 # does not require c++11
if [[ ! "$GCC_VERSION" =~ ^4\. ]]; then
    CPPUNIT_VERSION=1.14.0 # 1.14.0 is the first version to require c++11
fi
CPPUNIT_TAR="cppunit-${CPPUNIT_VERSION}.tar.gz"
CPPUNIT_SITE="http://dev-www.libreoffice.org/src/cppunit"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/cppunit.pc" ] || [ "$(pkg-config --modversion cppunit)" != "$CPPUNIT_VERSION" ]; }; }; then
    start_build
    download "$CPPUNIT_SITE" "$CPPUNIT_TAR"
    untar "$SRC_PATH/$CPPUNIT_TAR"
    pushd "cppunit-${CPPUNIT_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "cppunit-${CPPUNIT_VERSION}"
    end_build
fi
