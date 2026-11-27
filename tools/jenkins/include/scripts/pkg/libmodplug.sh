#!/bin/bash

# Install libmodplug
# see https://sourceforge.net/projects/modplug-xmms/files/libmodplug/
LIBMODPLUG_VERSION=0.8.9.0
LIBMODPLUG_TAR="libmodplug-${LIBMODPLUG_VERSION}.tar.gz"
LIBMODPLUG_SHA256=457ca5a6c179656d66c01505c0d95fafaead4329b9dbaa0f997d00a3508ad9de
LIBMODPLUG_SITE="https://sourceforge.net/projects/modplug-xmms/files/libmodplug/${LIBMODPLUG_VERSION}"
if download_step; then
    # Debian's copy of the release tarball first (the same file), then SourceForge
    download_urls "$LIBMODPLUG_TAR" "$LIBMODPLUG_SHA256" \
        "https://deb.debian.org/debian/pool/main/libm/libmodplug/libmodplug_${LIBMODPLUG_VERSION}.orig.tar.gz" \
        "$LIBMODPLUG_SITE/$LIBMODPLUG_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libmodplug.so" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBMODPLUG_TAR"
    pushd "libmodplug-$LIBMODPLUG_VERSION"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libmodplug-$LIBMODPLUG_VERSION"
    end_build
fi
