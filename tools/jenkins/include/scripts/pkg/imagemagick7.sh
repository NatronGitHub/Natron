#!/bin/bash

# install ImageMagick7
# see http://www.linuxfromscratch.org/blfs/view/svn/general/imagemagick.html
MAGICK7_VERSION=7.1.1-6
if [ "${CENTOS:-0}" = 6 ] && [ -z "${DTS+x}" ]; then
    MAGICK7_VERSION=7.0.9-8 # 7.0.9-9 (probably) and later fail to compile on CentOS6 with "undefined reference to `aligned_alloc'"
fi
MAGICK7_VERSION_SHORT=${MAGICK7_VERSION%-*}
MAGICK7_TAR="ImageMagick-${MAGICK7_VERSION}.tar.xz"
#MAGICK7_SITE="https://gitlab.com/ImageMagick/ImageMagick/-/archive/${MAGICK7_VERSION}"
MAGICK7_SITE="https://imagemagick.org/archive/releases"
if download_step; then
    download "$MAGICK7_SITE" "$MAGICK7_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/magick7/lib/pkgconfig/Magick++.pc" ] || [ "$(env PKG_CONFIG_PATH=$SDK_HOME/magick7/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion Magick++)" != "$MAGICK7_VERSION_SHORT" ]; }; }; then
    start_build
    untar "$SRC_PATH/$MAGICK7_TAR"
    pushd "ImageMagick-${MAGICK7_VERSION}"
    #if [ "${MAGICK_CL:-}" = "1" ]; then
    #  MAGICK_CL_OPT="--with-x --enable-opencl"
    #else
    MAGICK_CL_OPT="--without-x"
    #fi
    MAGICK_CFLAGS="-DMAGICKCORE_EXCLUDE_DEPRECATED=1"
    if [ -f /etc/centos-release ] && grep -q "release 6"  /etc/centos-release; then
        # 6.9.10-80 and later fail on CentOS 6 with "undefined reference to `aligned_alloc'"
        MAGICK_CFLAGS="$MAGICK_CFLAGS -UMAGICKCORE_HAVE_STDC_ALIGNED_ALLOC"
    fi
    env CFLAGS="$BF $MAGICK_CFLAGS" CXXFLAGS="$BF $MAGICK_CFLAGS" ./configure --prefix=$SDK_HOME/magick7 --with-magick-plus-plus=yes --with-quantum-depth=32 --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --with-lcms --without-openjp2 --without-lqr --without-lzma --without-openexr --without-pango --with-png --without-rsvg --without-tiff --without-webp --without-xml --with-zlib --without-bzlib --disable-static --enable-shared --enable-hdri --with-freetype --with-fontconfig --without-modules --with-libheif --enable-zero-configuration ${MAGICK_CL_OPT:-}
    make -j${MKJOBS}
    make install
    popd
    rm -rf "ImageMagick-${MAGICK7_VERSION}"
    end_build
fi
