#!/bin/bash

# Install harbuzz
# see http://www.linuxfromscratch.org/blfs/view/svn/general/harfbuzz.html
HARFBUZZ_VERSION=15.1.0
HARFBUZZ_TAR="harfbuzz-${HARFBUZZ_VERSION}.tar.xz"
HARFBUZZ_SHA256=0fc7c565443a5f4be7da65ac5ee5bd737d9b158dc908708b7689a0a5796b0c8d
#HARFBUZZ_SITE="https://www.freedesktop.org/software/harfbuzz/release"
HARFBUZZ_SITE="https://github.com/harfbuzz/harfbuzz/releases/download/${HARFBUZZ_VERSION}"
if download_step; then
    download "$HARFBUZZ_SITE" "$HARFBUZZ_TAR" "$HARFBUZZ_SHA256"
fi
if build_step && { force_build || { [ "${REBUILD_HARFBUZZ:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME/lib/libharfbuzz*" "$SDK_HOME/include/harfbuzz" "$SDK_HOME/lib/pkgconfig/"*harfbuzz* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/harfbuzz.pc" ] || [ "$(pkg-config --modversion harfbuzz)" != "$HARFBUZZ_VERSION" ]; }; }; then
    REBUILD_PANGO=1
    start_build
    untar "$SRC_PATH/$HARFBUZZ_TAR"
    pushd "harfbuzz-${HARFBUZZ_VERSION}"
    if version_gt "$HARFBUZZ_VERSION" 2.8.99; then
		mkdir build
		pushd build
		env CFLAGS="$BF" CXXFLAGS="$BF" meson --prefix="$SDK_HOME" --libdir="lib" \
            --buildtype=release \
			-Dtests=disabled -Ddocs=disabled \
			-Dbenchmark=disabled -Dintrospection=disabled -Dchafa=disabled -Dgpu_demo=disabled \
			..
		env CFLAGS="$BF" CXXFLAGS="$BF" ninja
        env CFLAGS="$BF" CXXFLAGS="$BF" ninja install
		popd
    else
        env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --disable-docs --disable-static --enable-shared --with-freetype --with-cairo --with-gobject --with-glib --with-icu
        make -j${MKJOBS}
        make install
    fi
    popd
    rm -rf "harfbuzz-${HARFBUZZ_VERSION}"
    end_build
fi
