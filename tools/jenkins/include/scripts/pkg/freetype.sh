#!/bin/bash

# Install FreeType2
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/freetype2.html
FREETYPE_VERSION=2.10.1
FREETYPE_TAR="freetype-${FREETYPE_VERSION}.tar.gz"
FREETYPE_SITE="http://download.savannah.gnu.org/releases/freetype"
if download_step; then
    download "$FREETYPE_SITE" "$FREETYPE_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/freetype2.pc" ]; }; }; then # || [ "$(pkg-config --modversion freetype2)" != "$FREETYPE_VERSION" ]; then
    start_build
    untar "$SRC_PATH/$FREETYPE_TAR"
    pushd "freetype-${FREETYPE_VERSION}"
    # First command enables GX/AAT and OpenType table validation
    sed -ri "s:.*(AUX_MODULES.*valid):\1:" modules.cfg
    # second command enables Subpixel Rendering
    # Note that Subpixel Rendering may have patent issues. Be sure to read the 'Other patent issues' part of http://www.freetype.org/patents.html before enabling this option.
    sed -r "s:.*(#.*SUBPIXEL_RENDERING) .*:\1:" -i include/freetype/config/ftoption.h
env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "freetype-${FREETYPE_VERSION}"
    end_build
fi
