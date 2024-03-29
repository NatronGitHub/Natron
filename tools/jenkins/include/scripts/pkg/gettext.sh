#!/bin/bash

# Install gettext
# see http://www.linuxfromscratch.org/lfs/view/development/chapter08/gettext.html
GETTEXT_VERSION=0.21.1
GETTEXT_TAR="gettext-${GETTEXT_VERSION}.tar.gz"
GETTEXT_SITE="http://ftp.gnu.org/pub/gnu/gettext"
if download_step; then
    download "$GETTEXT_SITE" "$GETTEXT_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/gettext" ]; }; }; then
    start_build
    untar "$SRC_PATH/$GETTEXT_TAR"
    pushd "gettext-${GETTEXT_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "gettext-${GETTEXT_VERSION}"
    end_build
fi
