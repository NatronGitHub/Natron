#!/bin/bash

# Install libpng
# see http://www.linuxfromscratch.org/blfs/view/svn/general/libpng.html
LIBPNG_VERSION=1.6.59
LIBPNG_TAR="libpng-${LIBPNG_VERSION}.tar.gz"
# the GitHub source archive of the tag, which is also Debian's orig tarball. It has the same files
# as the SourceForge release tarball (including configure), but is compressed differently.
LIBPNG_SHA256=2540302a1844ad2b2b501977abecfa850f265f97b78f065a712ab4074a89f5b5
if download_step; then
    download_urls "$LIBPNG_TAR" "$LIBPNG_SHA256" \
        "https://github.com/pnggroup/libpng/archive/refs/tags/v${LIBPNG_VERSION}.tar.gz" \
        "https://deb.debian.org/debian/pool/main/libp/libpng1.6/libpng1.6_${LIBPNG_VERSION}.orig.tar.gz"
fi
if build_step && { force_build || { [ "${REBUILD_PNG:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/{include,lib}/*png* || true
    rm -f "$SDK_HOME"/lib/pkgconfig/*png* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libpng.pc" ] || [ "$(pkg-config --modversion libpng)" != "$LIBPNG_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBPNG_TAR"
    pushd "libpng-${LIBPNG_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" LIBS=-lpthread ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libpng-${LIBPNG_VERSION}"
    end_build
fi
