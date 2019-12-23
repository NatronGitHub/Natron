#!/bin/bash

# Install p11-kit (for gnutls)
# http://www.linuxfromscratch.org/blfs/view/svn/postlfs/p11-kit.html
P11KIT_VERSION=0.23.18.1
#P11KIT_VERSION_SHORT=${P11KIT_VERSION} # if P11KIT_VERSION has 3 components, eg 0.23.16
P11KIT_VERSION_SHORT=${P11KIT_VERSION%.*} # if P11KIT_VERSION has 4 components, eg 0.23.16.1
P11KIT_TAR="p11-kit-${P11KIT_VERSION}.tar.gz"
P11KIT_SITE="https://github.com/p11-glue/p11-kit/releases/download/${P11KIT_VERSION}"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/p11-kit-1.pc" ] || [ "$(pkg-config --modversion p11-kit-1)" != "$P11KIT_VERSION_SHORT" ]; }; }; then
    start_build
    download "$P11KIT_SITE" "$P11KIT_TAR"
    untar "$SRC_PATH/$P11KIT_TAR"
    pushd "p11-kit-${P11KIT_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "p11-kit-${P11KIT_VERSION}"
    end_build
fi
