#!/bin/bash

# install pkg-config
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/pkg-config.html
PKGCONFIG_VERSION=0.29.2
PKGCONFIG_TAR="pkg-config-${PKGCONFIG_VERSION}.tar.gz"
PKGCONFIG_SITE="https://pkg-config.freedesktop.org/releases"
if download_step; then
    download "$PKGCONFIG_SITE" "$PKGCONFIG_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/pkg-config" ] || [ "$($SDK_HOME/bin/pkg-config --version)" != "$PKGCONFIG_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$PKGCONFIG_TAR"
    pushd "pkg-config-${PKGCONFIG_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --with-internal-glib --disable-host-tool
    make -j${MKJOBS}
    make install
    popd
    rm -rf "pkg-config-${PKGCONFIG_VERSION}"
    end_build
fi
