#!/bin/bash

# Install giflib
# see http://www.linuxfromscratch.org/blfs/view/svn/general/giflib.html
GIFLIB_VERSION=5.2.1
GIFLIB_TAR="giflib-${GIFLIB_VERSION}.tar.gz"
GIFLIB_SHA256=31da5562f44c5f15d63340a09a4fd62b48c45620cd302f77a6d9acf0077879bd
GIFLIB_SITE="https://sourceforge.net/projects/giflib/files"
if download_step; then
    download "$GIFLIB_SITE" "$GIFLIB_TAR" "$GIFLIB_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libgif.so" ]; }; }; then
    start_build
    untar "$SRC_PATH/$GIFLIB_TAR"
    pushd "giflib-${GIFLIB_VERSION}"
    if version_gt "5.1.6" "${GIFLIB_VERSION}"; then
	env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
	make -j${MKJOBS}
	make install
    else
	env CFLAGS="$BF" CXXFLAGS="$BF" make -j${MKJOBS}
	env CFLAGS="$BF" CXXFLAGS="$BF" make PREFIX="$SDK_HOME" install
	rm -vf "$SDK_HOME/"lib/libgif.a
    fi
    popd
    rm -rf "giflib-${GIFLIB_VERSION}"
    end_build
fi
