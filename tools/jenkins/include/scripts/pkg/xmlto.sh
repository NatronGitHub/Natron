#!/bin/bash

# Install xmlto
# see hhttp://www.linuxfromscratch.org/blfs/view/svn/pst/xmlto.html
XMLTO_VERSION=0.0.28
XMLTO_TAR="xmlto-${XMLTO_VERSION}.tar.bz2"
XMLTO_SITE="https://releases.pagure.org/xmlto"
if download_step; then
    download "$XMLTO_SITE" "$XMLTO_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/xmlto" ] || [ "$("${SDK_HOME}/bin/xmlto" --version)" != "xmlto version $XMLTO_VERSION" ] ; }; }; then
    start_build
    untar "$SRC_PATH/$XMLTO_TAR"
    pushd "xmlto-${XMLTO_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "xmlto-${XMLTO_VERSION}"
    end_build
fi
