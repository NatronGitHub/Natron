#!/bin/bash

# Install boost
# see http://www.linuxfromscratch.org/blfs/view/svn/general/boost.html
# TestImagePNG and TestImagePNGOIIO used to fail with boost 1.66.0, and succeed with boost 1.65.1,
# but this should be fixed by https://github.com/NatronGitHub/Natron/commit/e360b4dd31a07147f2ad9073a773ff28714c2a69
BOOST_VERSION=1.82.0
BOOST_LIB_VERSION=$(echo "${BOOST_VERSION//./_}" | sed -e 's/_0$//')
BOOST_TAR="boost_${BOOST_VERSION//./_}.tar.bz2"
BOOST_SITE="https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source"
if download_step; then
    download "$BOOST_SITE" "$BOOST_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libboost_atomic.so" ] || ! fgrep "define BOOST_LIB_VERSION \"${BOOST_LIB_VERSION}\"" "$SDK_HOME/include/boost/version.hpp" &>/dev/null ; }; }; then
    start_build
    untar "$SRC_PATH/$BOOST_TAR"
    pushd "boost_${BOOST_VERSION//./_}"
    # fix a bug with the header files path, when Python3 is used:
    sed -e '/using python/ s@;@: /usr/include/python${PYTHON_VERSION/3*/${PYTHON_VERSION}m} ;@' -i bootstrap.sh
    env CPATH="$SDK_HOME/include:$SDK_HOME/include/python${PY2_VERSION_SHORT}" ./bootstrap.sh --prefix="$SDK_HOME"
    env CPATH="$SDK_HOME/include:$SDK_HOME/include/python${PY2_VERSION_SHORT}" CFLAGS="$BF" CXXFLAGS="$BF" ./b2 -s --prefix="$SDK_HOME" cflags="-fPIC" -j${MKJOBS} install threading=multi link=shared # link=static
    popd
    rm -rf "boost_${BOOST_VERSION//./_}"
    end_build
fi
