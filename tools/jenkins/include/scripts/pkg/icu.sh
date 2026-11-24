#!/bin/bash

# Install icu
# see http://www.linuxfromscratch.org/blfs/view/svn/general/icu.html
ICU_VERSION=78.3
ICU_TAR="icu4c-${ICU_VERSION}-sources.tgz" # before 78: icu4c-${ICU_VERSION//./_}-src.tgz
ICU_SHA256=3a2e7a47604ba702f345878308e6fefeca612ee895cf4a5f222e7955fabfe0c0
#ICU_SITE="http://download.icu-project.org/files/icu4c/${ICU_VERSION}"
ICU_SITE="https://github.com/unicode-org/icu/releases/download/release-${ICU_VERSION}" # before 78: release-${ICU_VERSION//./-}
if download_step; then
    download "$ICU_SITE" "$ICU_TAR" "$ICU_SHA256"
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
