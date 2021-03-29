#!/bin/bash

# Install nettle (for gnutls)
# see http://www.linuxfromscratch.org/blfs/view/svn/postlfs/nettle.html
# disable doc, because install-info in CentOS 6.4 is buggy and always returns exit status 1.
NETTLE_VERSION=3.7.1
NETTLE_VERSION_SHORT=${NETTLE_VERSION%.*}
NETTLE_TAR="nettle-${NETTLE_VERSION}.tar.gz"
NETTLE_SITE="https://ftp.gnu.org/gnu/nettle"
if download_step; then
    download "$NETTLE_SITE" "$NETTLE_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/nettle.pc" ] || [ "$(pkg-config --modversion nettle)" != "$NETTLE_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$NETTLE_TAR"
    pushd "nettle-${NETTLE_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared --disable-documentation
    make -j${MKJOBS}
    make -k install
    make -k install
    popd
    rm -rf "nettle-${NETTLE_VERSION}"
    end_build
fi
