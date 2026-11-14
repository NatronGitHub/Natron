#!/bin/bash

# Install libraw
# see http://www.linuxfromscratch.org/blfs/view/svn/general/libraw.html
# LibRaw 0.19+ dropped the GPL demosaic packs: patch-libraw-0.22.3-gpl-demosaic-packs.diff restores them
# (from https://github.com/devernay/LibRaw/commit/56970694). When bumping LIBRAW_VERSION, check that
# the patch still applies and that "make check" (tests/demosaic_test) reports the packs.
LIBRAW_VERSION=0.22.3
LIBRAW_PACKS_VERSION=0.18.8
LIBRAW_TAR="LibRaw-${LIBRAW_VERSION}.tar.gz"
LIBRAW_DEMOSAIC_PACK_GPL2="LibRaw-demosaic-pack-GPL2-${LIBRAW_PACKS_VERSION}.tar.gz"
LIBRAW_DEMOSAIC_PACK_GPL3="LibRaw-demosaic-pack-GPL3-${LIBRAW_PACKS_VERSION}.tar.gz"
LIBRAW_SITE="https://www.libraw.org/data"
if download_step; then
    download "$LIBRAW_SITE" "$LIBRAW_TAR"
    download "$LIBRAW_SITE" "$LIBRAW_DEMOSAIC_PACK_GPL2"
    download "$LIBRAW_SITE" "$LIBRAW_DEMOSAIC_PACK_GPL3"
fi
if build_step && { force_build || { [ "${REBUILD_LIBRAW:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/libraw-gpl3 || true
    rm -rf "$SDK_HOME"/libraw-gpl2 || true
    rm -rf "$SDK_HOME"/libraw-lgpl || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/libraw-gpl2/lib/pkgconfig/libraw.pc" ] || [ "$(env PKG_CONFIG_PATH=$SDK_HOME/libraw-gpl2/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion libraw)" != "$LIBRAW_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBRAW_TAR"
    pushd "LibRaw-${LIBRAW_VERSION}"
    untar "$SRC_PATH/$LIBRAW_DEMOSAIC_PACK_GPL2"
    untar "$SRC_PATH/$LIBRAW_DEMOSAIC_PACK_GPL3"
    patch -p1 -i "$INC_PATH/patches/LibRaw/patch-libraw-${LIBRAW_VERSION}-gpl-demosaic-packs.diff"
    autoreconf -fi
    LIBRAW_PACK_GPL2_DIR="$PWD/LibRaw-demosaic-pack-GPL2-${LIBRAW_PACKS_VERSION}"
    LIBRAW_PACK_GPL3_DIR="$PWD/LibRaw-demosaic-pack-GPL3-${LIBRAW_PACKS_VERSION}"
    # the demosaic packs are off by default (LGPL build); configure fails if an enabled pack is missing
    LGPL_CONF_FLAGS=( --disable-static --enable-shared --enable-lcms --enable-jasper --enable-jpeg )
    GPL2_CONF_FLAGS=( "${LGPL_CONF_FLAGS[@]}" --enable-demosaic-pack-gpl2="$LIBRAW_PACK_GPL2_DIR" )
    GPL3_CONF_FLAGS=( "${LGPL_CONF_FLAGS[@]}" --enable-demosaic-pack-gpl2="$LIBRAW_PACK_GPL2_DIR" --enable-demosaic-pack-gpl3="$LIBRAW_PACK_GPL3_DIR" )
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS" ./configure --prefix="$SDK_HOME/libraw-gpl3" "${GPL3_CONF_FLAGS[@]}"
    make -j${MKJOBS}
    make check   # tests/demosaic_test: checks which demosaic packs are active
    make install
    make distclean
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS" ./configure --prefix="$SDK_HOME/libraw-gpl2" "${GPL2_CONF_FLAGS[@]}"
    make -j${MKJOBS}
    make check   # tests/demosaic_test: checks which demosaic packs are active
    make install
    make distclean
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS" ./configure --prefix="$SDK_HOME/libraw-lgpl" "${LGPL_CONF_FLAGS[@]}"
    make -j${MKJOBS}
    make check   # tests/demosaic_test: checks which demosaic packs are active
    make install
    popd
    rm -rf "LibRaw-${LIBRAW_VERSION}"
    # OpenImageIO must be rebuilt against this libraw-gpl2 (the plugins and OIIO must use the same LibRaw)
    rm -rf "$SDK_HOME/lib/libOpenImage"* "$SDK_HOME/include/OpenImage"* || true
    end_build
fi
