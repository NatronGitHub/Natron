#!/bin/bash

# Install icu
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/icu.html
ICU_VERSION=68.1
ICU_TAR="icu4c-${ICU_VERSION//./_}-src.tgz"
#ICU_SITE="http://download.icu-project.org/files/icu4c/${ICU_VERSION}"
ICU_SITE="http://github.com/unicode-org/icu/releases/download/release-${ICU_VERSION//./-}"
if download_step; then
    download "$ICU_SITE" "$ICU_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/icu-i18n.pc" ] || [ "$(pkg-config --modversion icu-i18n)" != "$ICU_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$ICU_TAR"
    pushd icu/source
    # Note: removed
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf icu
    end_build
fi
