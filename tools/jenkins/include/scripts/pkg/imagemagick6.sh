#!/bin/bash

# Install ImageMagick6
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/imagemagick6.html
MAGICK_VERSION=6.9.10-62
MAGICK_VERSION_SHORT=${MAGICK_VERSION%-*}
#MAGICK_TAR="ImageMagick6-${MAGICK_VERSION}.tar.gz"
#MAGICK_SITE="https://gitlab.com/ImageMagick/ImageMagick6/-/archive/${MAGICK_VERSION}"
MAGICK_TAR="ImageMagick-${MAGICK_VERSION}.tar.gz"
MAGICK_SITE="https://imagemagick.org/download"
if build_step && { force_build || { [ "${REBUILD_MAGICK:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/include/ImageMagick-6/ "$SDK_HOME"/lib/libMagick* "$SDK_HOME"/share/ImageMagick-6/ "$SDK_HOME"/lib/pkgconfig/{Image,Magick}* "$SDK_HOME"/magick7 || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/Magick++.pc" ] || [ "$(pkg-config --modversion Magick++)" != "$MAGICK_VERSION_SHORT" ]; }; }; then
    start_build
    download "$MAGICK_SITE" "$MAGICK_TAR"
    untar "$SRC_PATH/$MAGICK_TAR"
    #pushd "ImageMagick6-${MAGICK_VERSION}"
    pushd "ImageMagick-${MAGICK_VERSION}"
    #if [ "${MAGICK_CL:-}" = "1" ]; then
    #  MAGICK_CL_OPT="--with-x --enable-opencl"
    #else
    MAGICK_CL_OPT="--without-x"
    #fi
    # the following patch was integrated, see https://github.com/ImageMagick/ImageMagick/issues/1488
    #patch -p0 -i "$INC_PATH/patches/ImageMagick/pango-align-hack.diff"
    env CFLAGS="$BF -DMAGICKCORE_EXCLUDE_DEPRECATED=1" CXXFLAGS="$BF -DMAGICKCORE_EXCLUDE_DEPRECATED=1" ./configure --prefix="$SDK_HOME" --with-magick-plus-plus=yes --with-quantum-depth=32 --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --with-lcms --without-openjp2 --without-lqr --without-lzma --without-openexr --with-pango --with-png --with-rsvg --without-tiff --without-webp --with-xml --with-zlib --without-bzlib --disable-static --enable-shared --enable-hdri --with-freetype --with-fontconfig --without-modules ${MAGICK_CL_OPT:-}
    make -j${MKJOBS}
    make install
    popd
    rm -rf "ImageMagick6-${MAGICK_VERSION}"
    end_build
fi
