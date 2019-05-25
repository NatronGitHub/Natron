#!/bin/bash

# Install giflib
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/giflib.html
GIFLIB_VERSION=5.1.9
GIFLIB_TAR="giflib-${GIFLIB_VERSION}.tar.bz2"
GIFLIB_SITE="https://sourceforge.net/projects/giflib/files"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libgif.so" ]; }; }; then
    start_build
    download "$GIFLIB_SITE" "$GIFLIB_TAR"
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
