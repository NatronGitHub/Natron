#!/bin/bash

# Install libtheora
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/libtheora.html
LIBTHEORA_VERSION=1.1.1
LIBTHEORA_TAR="libtheora-${LIBTHEORA_VERSION}.tar.bz2"
LIBTHEORA_SITE="http://downloads.xiph.org/releases/theora"
if download_step; then
    download "$LIBTHEORA_SITE" "$LIBTHEORA_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/theora.pc" ] || [ "$(pkg-config --modversion theora)" != "$LIBTHEORA_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBTHEORA_TAR"
    pushd "libtheora-${LIBTHEORA_VERSION}"
    $GSED -i 's/png_\(sizeof\)/\1/g' examples/png2theora.c # fix libpng16
    if [ "$ARCH" != "x86_64" ] && [ "$ARCH" != "i686" ]; then
        # libtheora 1.1.1's config.guess/config.sub (2009) don't know aarch64: use the
        # ones from the SDK's automake.
        for f in config.guess config.sub; do
            src="$(ls "$SDK_HOME"/share/automake-*/"$f" /usr/share/automake-*/"$f" 2>/dev/null | head -1)"
            [ -n "$src" ] && cp -f "$src" "$f"
        done
    fi
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libtheora-${LIBTHEORA_VERSION}"
    end_build
fi
