#!/bin/bash

# Install libxslt
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/libxslt.html
LIBXSLT_VERSION=1.1.34
LIBXSLT_TAR="libxslt-${LIBXSLT_VERSION}.tar.gz"
LIBXSLT_SITE="ftp://xmlsoft.org/libxslt"
if download_step; then
    download "$LIBXSLT_SITE" "$LIBXSLT_TAR"
fi
if build_step && { force_build || { [ "${REBUILD_XSLT:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/include/libxslt "$SDK_HOME"/lib/libxsl* "$SDK_HOME"/lib/pkgconfig/libxslt* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libxslt.pc" ] || [ "$(pkg-config --modversion libxslt)" != "$LIBXSLT_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBXSLT_TAR"
    pushd "libxslt-${LIBXSLT_VERSION}"
    # this increases the recursion limit in libxslt. This is needed by some packages for their documentation
    sed -i s/3000/5000/ libxslt/transform.c doc/xsltproc.{1,xml}
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --without-python
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libxslt-${LIBXSLT_VERSION}"
    end_build
fi
