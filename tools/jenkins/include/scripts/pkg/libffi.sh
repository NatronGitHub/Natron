#!/bin/bash

# Install libffi
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/libffi.html
FFI_VERSION=3.3
FFI_TAR="libffi-${FFI_VERSION}.tar.gz"
FFI_SITE="ftp://sourceware.org/pub/libffi"
if download_step; then
    download "$FFI_SITE" "$FFI_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libffi.pc" ] || [ "$(pkg-config --modversion libffi)" != "$FFI_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$FFI_TAR"
    pushd "libffi-${FFI_VERSION}"
    # Modify the Makefile to install headers into the standard /usr/include directory instead of /usr/lib/libffi-3.2.1/include.
    sed -e '/^includesdir/ s/$(libdir).*$/$(includedir)/' \
        -i include/Makefile.in
    sed -e '/^includedir/ s/=.*$/=@includedir@/' \
        -e 's/^Cflags: -I${includedir}/Cflags:/' \
        -i libffi.pc.in
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libffi-${FFI_VERSION}"
    end_build
fi
