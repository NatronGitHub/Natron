#!/bin/bash

# Install FreeType2
# see http://www.linuxfromscratch.org/blfs/view/svn/general/freetype2.html
FREETYPE_VERSION=2.15.0
FREETYPE_TAR="freetype-${FREETYPE_VERSION}.tar.xz"
FREETYPE_SHA256=93cd47eb719c325107f2d3cf087d06299e944c3b48a40b55537302304dea1dd5
FREETYPE_SITE="https://downloads.sourceforge.net/freetype"
# 2.15.0 + the upstream ULABS/UABS fix 05f343a (rotated text near 45 degrees, openfx-arena #86 #88),
# as on macOS (tools/MacPorts/print/freetype) and Windows (tools/MINGW-packages/mingw-w64-freetype).
# Drop the patch, and the "+05f343a" suffix, with the first release that contains it (2.15.1).
FREETYPE_SDK_VERSION="${FREETYPE_VERSION}+05f343a"
if download_step; then
    download "$FREETYPE_SITE" "$FREETYPE_TAR" "$FREETYPE_SHA256"
fi
# The installed version: freetype2.pc only has the libtool version, and the patch level is
# recorded by this script, so that existing SDKs pick up the patched build.
freetype_installed_version() {
    cat "$SDK_HOME/share/natron-sdk/freetype.version" 2>/dev/null || true
}
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/freetype2.pc" ] || [ "$(freetype_installed_version)" != "$FREETYPE_SDK_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$FREETYPE_TAR"
    pushd "freetype-${FREETYPE_VERSION}"
    patch -p1 -i "$INC_PATH/patches/freetype/0001-freetype-2.15.0-fix-ULABS-UABS-macros-05f343a.patch"
    # First command enables GX/AAT and OpenType table validation
    sed -ri "s:.*(AUX_MODULES.*valid):\1:" modules.cfg
    # second command enables Subpixel Rendering
    # Note that Subpixel Rendering may have patent issues. Be sure to read the 'Other patent issues' part of http://www.freetype.org/patents.html before enabling this option.
    sed -r "s:.*(#.*SUBPIXEL_RENDERING) .*:\1:" -i include/freetype/config/ftoption.h
env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    mkdir -p "$SDK_HOME/share/natron-sdk"
    echo "$FREETYPE_SDK_VERSION" > "$SDK_HOME/share/natron-sdk/freetype.version"
    popd
    rm -rf "freetype-${FREETYPE_VERSION}"
    end_build
fi
