#!/bin/bash

# Install ImageMagick6
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/imagemagick6.html
MAGICK_VERSION=6.9.10-78 # latest is 6.9.11-0, but 6.9.10-79 and later fail to compile on CentOS6 with "undefined reference to `aligned_alloc'"
#MAGICK_VERSION=6.9.11-0
MAGICK_VERSION_SHORT=${MAGICK_VERSION%-*}
#MAGICK_TAR="ImageMagick6-${MAGICK_VERSION}.tar.gz"
#MAGICK_SITE="https://gitlab.com/ImageMagick/ImageMagick6/-/archive/${MAGICK_VERSION}"
MAGICK_TAR="ImageMagick-${MAGICK_VERSION}.tar.gz"
MAGICK_SITE="https://imagemagick.org/download/releases"
if download_step; then
    download "$MAGICK_SITE" "$MAGICK_TAR"
fi
if build_step && { force_build || { [ "${REBUILD_MAGICK:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/include/ImageMagick-6/ "$SDK_HOME"/lib/libMagick* "$SDK_HOME"/share/ImageMagick-6/ "$SDK_HOME"/lib/pkgconfig/{Image,Magick}* "$SDK_HOME"/magick7 || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/Magick++.pc" ] || [ "$(pkg-config --modversion Magick++)" != "$MAGICK_VERSION_SHORT" ]; }; }; then
    start_build
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

    # backport by @rodlie of xcf (GIMP) format from IM7:
    patch -p0 -i "$INC_PATH/patches/ImageMagick/ImageMagick-6.9.10-gimp210.diff"

    MAGICK_CFLAGS="-DMAGICKCORE_EXCLUDE_DEPRECATED=1"
    if [ -f /etc/centos-release ] && grep -q "release 6"  /etc/centos-release; then
        # 6.9.10-80 and later fail on CentOS 6 with "undefined reference to `aligned_alloc'"
        #MAGICK_CFLAGS="$MAGICK_CFLAGS -UMAGICKCORE_HAVE_STDC_ALIGNED_ALLOC"
        true
    fi
    env CFLAGS="$BF $MAGICK_CFLAGS" CXXFLAGS="$BF $MAGICK_CFLAGS" ./configure --prefix="$SDK_HOME" --with-magick-plus-plus=yes --with-quantum-depth=32 --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --with-lcms --without-openjp2 --without-lqr --without-lzma --without-openexr --with-pango --with-png --with-rsvg --without-tiff --without-webp --with-xml --with-zlib --without-bzlib --disable-static --enable-shared --enable-hdri --with-freetype --with-fontconfig --without-modules --with-libheic ${MAGICK_CL_OPT:-}
    make -j${MKJOBS}
    make install
    popd
    rm -rf "ImageMagick6-${MAGICK_VERSION}"
    end_build
fi
