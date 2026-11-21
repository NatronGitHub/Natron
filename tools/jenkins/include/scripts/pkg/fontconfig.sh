#!/bin/bash

# Install fontconfig
# see http://www.linuxfromscratch.org/blfs/view/svn/general/fontconfig.html
FONTCONFIG_VERSION=2.18.3
FONTCONFIG_TAR="fontconfig-${FONTCONFIG_VERSION}.tar.xz"
FONTCONFIG_SHA256=4f7b554a38cdf78c033f666c8871f3749e14a094f65a07f630c91ed0b43d35e3
FONTCONFIG_SITE="https://gitlab.freedesktop.org/api/v4/projects/890/packages/generic/fontconfig/${FONTCONFIG_VERSION}"
if download_step; then
    download "$FONTCONFIG_SITE" "$FONTCONFIG_TAR" "$FONTCONFIG_SHA256"
fi
if build_step && { force_build || { [ "${REBUILD_FONTCONFIG:-}" = "1" ]; }; }; then
    rm -f "$SDK_HOME"/lib/libfontconfig* || true
    rm -rf "$SDK_HOME"/include/fontconfig || true
    rm -f "$SDK_HOME"/lib/pkgconfig/fontconfig.pc || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/fontconfig.pc" ] || [ "$(pkg-config --modversion fontconfig)" != "$FONTCONFIG_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$FONTCONFIG_TAR"
    pushd "fontconfig-${FONTCONFIG_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "fontconfig-${FONTCONFIG_VERSION}"
    end_build
fi

