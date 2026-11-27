#!/bin/bash

# install librevenge
# see https://sourceforge.net/p/libwpd/wiki/librevenge/ (LibreOffice hosts the release tarballs
# of the libraries it uses, librevenge included: the same file as on SourceForge)
REVENGE_VERSION=0.0.5
REVENGE_TAR="librevenge-${REVENGE_VERSION}.tar.bz2"
REVENGE_SHA256=5892ca6796f7a2a93d580832e907e849b19d980b40d326a283b18877ab6de0c5
REVENGE_SITE="https://dev-www.libreoffice.org/src"
if download_step; then
    download "$REVENGE_SITE" "$REVENGE_TAR" "$REVENGE_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/librevenge-0.0.pc" ] || [ "$(pkg-config --modversion librevenge-0.0)" != "$REVENGE_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$REVENGE_TAR"
    pushd "librevenge-${REVENGE_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --disable-werror --prefix="$SDK_HOME" --disable-docs --disable-tests --enable-shared # --disable-tests: no cppunit
    make -j${MKJOBS}
    make install
    popd
    rm -rf "librevenge-${REVENGE_VERSION}"
    end_build
fi
