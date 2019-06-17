#!/bin/bash

# install ImageMagick7
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/imagemagick.html
MAGICK7_VERSION=7.0.8-49
MAGICK7_VERSION_SHORT=${MAGICK7_VERSION%-*}
MAGICK7_TAR="ImageMagick-${MAGICK7_VERSION}.tar.gz"
#MAGICK7_SITE="https://gitlab.com/ImageMagick/ImageMagick/-/archive/${MAGICK7_VERSION}"
MAGICK7_SITE="https://www.imagemagick.org/download/releases"
if build_step && { force_build || { [ ! -s "$SDK_HOME/magick7/lib/pkgconfig/Magick++.pc" ] || [ "$(env PKG_CONFIG_PATH=$SDK_HOME/magick7/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion Magick++)" != "$MAGICK7_VERSION_SHORT" ]; }; }; then
    start_build
    download "$MAGICK7_SITE" "$MAGICK7_TAR"
    untar "$SRC_PATH/$MAGICK7_TAR"
    pushd "ImageMagick-${MAGICK7_VERSION}"
    #if [ "${MAGICK_CL:-}" = "1" ]; then
    #  MAGICK_CL_OPT="--with-x --enable-opencl"
    #else
    MAGICK_CL_OPT="--without-x"
    #fi
    env CFLAGS="$BF -DMAGICKCORE_EXCLUDE_DEPRECATED=1" CXXFLAGS="$BF -DMAGICKCORE_EXCLUDE_DEPRECATED=1" ./configure --prefix=$SDK_HOME/magick7 --with-magick-plus-plus=yes --with-quantum-depth=32 --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --with-lcms --without-openjp2 --without-lqr --without-lzma --without-openexr --without-pango --with-png --without-rsvg --without-tiff --without-webp --without-xml --with-zlib --without-bzlib --disable-static --enable-shared --enable-hdri --with-freetype --with-fontconfig --without-modules ${MAGICK_CL_OPT:-}
    make -j${MKJOBS}
    make install
    popd
    rm -rf "ImageMagick-${MAGICK7_VERSION}"
    end_build
fi
