#!/bin/bash

# Install libjpeg-turbo
# see http://www.linuxfromscratch.org/blfs/view/svn/general/libjpeg.html
LIBJPEGTURBO_VERSION=2.1.5.1
LIBJPEGTURBO_VERSION_MAJOR=2 # ${LIBJPEGTURBO_VERSION%.*.*} # works for 2.1.5, not for 2.1.5.1

LIBJPEGTURBO_TAR="libjpeg-turbo-${LIBJPEGTURBO_VERSION}.tar.gz"
LIBJPEGTURBO_SITE="https://downloads.sourceforge.net/libjpeg-turbo"
if download_step; then
    download "$LIBJPEGTURBO_SITE" "$LIBJPEGTURBO_TAR"
fi
#if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libturbojpeg.so.0.${LIBJPEGTURBO_VERSION_MAJOR}.0" ]; }; }; then
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libturbojpeg.pc" ] || [ "$(pkg-config --modversion libturbojpeg)" != "$LIBJPEGTURBO_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBJPEGTURBO_TAR"
    pushd "libjpeg-turbo-${LIBJPEGTURBO_VERSION}"
    if [ "$LIBJPEGTURBO_VERSION_MAJOR" = "1" ]; then
	env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --with-jpeg8 --enable-shared --disable-static
	make -j${MKJOBS}
	make install
    else
	mkdir build
	cd    build

	cmake -DCMAKE_INSTALL_PREFIX="$SDK_HOME" \
	      -DCMAKE_BUILD_TYPE=RELEASE  \
	      -DENABLE_STATIC=FALSE       \
	      -DCMAKE_INSTALL_DOCDIR="$SDK_HOME/share/doc/libjpeg-turbo-${LIBJPEGTURBO_VERSION}" \
	      -DCMAKE_INSTALL_DEFAULT_LIBDIR=lib  \
	      ..
	make install
	cd ..
    fi
    popd
    rm -rf "libjpeg-turbo-${LIBJPEGTURBO_VERSION}"
    end_build
fi
