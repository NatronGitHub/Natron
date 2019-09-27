#!/bin/bash

# Install shared-mime-info (required by gdk-pixbuf)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/shared-mime-info.html
SHAREDMIMEINFO_VERSION=1.14
SHAREDMIMEINFO_TAR="shared-mime-info-${SHAREDMIMEINFO_VERSION}.tar.xz"
#SHAREDMIMEINFO_SITE="http://freedesktop.org/~hadess"
SHAREDMIMEINFO_SITE="https://gitlab.freedesktop.org/xdg/shared-mime-info/uploads/80c7f1afbcad2769f38aeb9ba6317a51"
if build_step && { force_build || { [ ! -s "$SDK_HOME/share/pkgconfig/shared-mime-info.pc" ] || [ "$(pkg-config --modversion shared-mime-info)" != "$SHAREDMIMEINFO_VERSION" ]; }; }; then
    start_build
    download "$SHAREDMIMEINFO_SITE" "$SHAREDMIMEINFO_TAR"
    untar "$SRC_PATH/$SHAREDMIMEINFO_TAR"
    pushd "shared-mime-info-${SHAREDMIMEINFO_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --without-libtiff
    #make -j${MKJOBS}
    make
    make install
    popd
    rm -rf "shared-mime-info-${SHAREDMIMEINFO_VERSION}"
    end_build
fi
