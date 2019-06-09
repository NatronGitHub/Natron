#!/bin/bash

# Install libraw
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/libraw.html
# do not install 0.19, which dropped support for the demosaic packs
LIBRAW_VERSION=0.18.13
LIBRAW_PACKS_VERSION="${LIBRAW_VERSION}"
LIBRAW_PACKS_VERSION=0.18.8
LIBRAW_TAR="LibRaw-${LIBRAW_VERSION}.tar.gz"
LIBRAW_DEMOSAIC_PACK_GPL2="LibRaw-demosaic-pack-GPL2-${LIBRAW_PACKS_VERSION}.tar.gz"
LIBRAW_DEMOSAIC_PACK_GPL3="LibRaw-demosaic-pack-GPL3-${LIBRAW_PACKS_VERSION}.tar.gz"
LIBRAW_SITE="https://www.libraw.org/data"
if build_step && { force_build || { [ "${REBUILD_LIBRAW:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/libraw-gpl3 || true
    rm -rf "$SDK_HOME"/libraw-gpl2 || true
    rm -rf "$SDK_HOME"/libraw-lgpl || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/libraw-gpl2/lib/pkgconfig/libraw.pc" ] || [ "$(env PKG_CONFIG_PATH=$SDK_HOME/libraw-gpl2/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion libraw)" != "$LIBRAW_VERSION" ]; }; }; then
    start_build
    download "$LIBRAW_SITE" "$LIBRAW_TAR"
    download "$LIBRAW_SITE" "$LIBRAW_DEMOSAIC_PACK_GPL2"
    download "$LIBRAW_SITE" "$LIBRAW_DEMOSAIC_PACK_GPL3"
    untar "$SRC_PATH/$LIBRAW_TAR"
    pushd "LibRaw-${LIBRAW_VERSION}"
    untar "$SRC_PATH/$LIBRAW_DEMOSAIC_PACK_GPL2"
    untar "$SRC_PATH/$LIBRAW_DEMOSAIC_PACK_GPL3"
    LGPL_CONF_FLAGS=( --disable-static --enable-shared --enable-lcms --enable-jasper --enable-jpeg )
    GPL2_CONF_FLAGS=( "${LGPL_CONF_FLAGS[@]}" --enable-demosaic-pack-gpl2 --disable-demosaic-pack-gpl3 )
    GPL3_CONF_FLAGS=( "${LGPL_CONF_FLAGS[@]}" --enable-demosaic-pack-gpl2 --enable-demosaic-pack-gpl3 )
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS" ./configure --prefix="$SDK_HOME/libraw-gpl3" "${GPL3_CONF_FLAGS[@]}"
    make -j${MKJOBS}
    make install
    make distclean
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS" ./configure --prefix="$SDK_HOME/libraw-gpl2" "${GPL2_CONF_FLAGS[@]}"
    make -j${MKJOBS}
    make install
    make distclean
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS" ./configure --prefix="$SDK_HOME/libraw-lgpl" "${LGPL_CONF_FLAGS[@]}"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "LibRaw-${LIBRAW_VERSION}"
    end_build
fi
