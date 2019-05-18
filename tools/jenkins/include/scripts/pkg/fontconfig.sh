#!/bin/bash

# Install fontconfig
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/fontconfig.html
FONTCONFIG_VERSION=2.13.0
FONTCONFIG_TAR="fontconfig-${FONTCONFIG_VERSION}.tar.gz"
FONTCONFIG_SITE="https://www.freedesktop.org/software/fontconfig/release"
if build_step && { force_build || { [ "${REBUILD_FONTCONFIG:-}" = "1" ]; }; }; then
    rm -f "$SDK_HOME"/lib/libfontconfig* || true
    rm -rf "$SDK_HOME"/include/fontconfig || true
    rm -f "$SDK_HOME"/lib/pkgconfig/fontconfig.pc || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/fontconfig.pc" ] || [ "$(pkg-config --modversion fontconfig)" != "$FONTCONFIG_VERSION" ]; }; }; then
    start_build
    download "$FONTCONFIG_SITE" "$FONTCONFIG_TAR"
    untar "$SRC_PATH/$FONTCONFIG_TAR"
    pushd "fontconfig-${FONTCONFIG_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "fontconfig-${FONTCONFIG_VERSION}"
    end_build
fi

