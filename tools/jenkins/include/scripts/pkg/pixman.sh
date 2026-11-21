#!/bin/bash

# Install pixman
# see http://www.linuxfromscratch.org/blfs/view/svn/general/pixman.html
PIXMAN_VERSION=0.46.4
PIXMAN_TAR="pixman-${PIXMAN_VERSION}.tar.gz"
PIXMAN_SHA256=d09c44ebc3bd5bee7021c79f922fe8fb2fb57f7320f55e97ff9914d2346a591c
PIXMAN_SITE="https://www.cairographics.org/releases"
if download_step; then
    download "$PIXMAN_SITE" "$PIXMAN_TAR" "$PIXMAN_SHA256"
fi
if build_step && { force_build || { [ "${REBUILD_PIXMAN:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/{lib,include}/*pixman* || true
    rm -f "$SDK_HOME"/lib/pkgconfig/*pixman* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/pixman-1.pc" ] || [ "$(pkg-config --modversion pixman-1)" != "$PIXMAN_VERSION" ]; }; }; then
    REBUILD_CAIRO=1
    start_build
    untar "$SRC_PATH/$PIXMAN_TAR"
    pushd "pixman-${PIXMAN_VERSION}"
    if [ -x configure ]; then
        env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
        make -j${MKJOBS}
        make install
    else
        # pixman >= 0.43 only builds with meson
        env CFLAGS="$BF" CXXFLAGS="$BF" meson setup build --prefix="$SDK_HOME" --libdir=lib --buildtype=release \
            --default-library=both -Dtests=disabled -Ddemos=disabled -Dgtk=disabled -Dlibpng=disabled
        ninja -C build -j${MKJOBS}
        ninja -C build install
    fi
    popd
    rm -rf "pixman-${PIXMAN_VERSION}"
    end_build
fi
