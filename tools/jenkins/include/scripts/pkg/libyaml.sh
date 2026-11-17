#!/bin/bash

# Install libyaml (necessary for ruby)
# see https://www.linuxfromscratch.org/blfs/view/svn/general/libyaml.html
LIBYAML_VERSION=0.2.5
LIBYAML_VERSION_SHORT=${LIBYAML_VERSION%.*}

LIBYAML_TAR="yaml-${LIBYAML_VERSION}.tar.gz"
LIBYAML_SITE=" https://github.com/yaml/libyaml/releases/download/${LIBYAML_VERSION}"
if download_step; then
    download "$LIBYAML_SITE" "$LIBYAML_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/yaml-0.1.pc" ] || [ "$(pkg-config --modversion yaml-0.1)" != "$LIBYAML_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBYAML_TAR"
    pushd "yaml-${LIBYAML_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "yaml-${LIBYAML_VERSION}"
    end_build
fi
