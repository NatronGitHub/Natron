#!/bin/bash

# Install poppler-data
# see https://poppler.freedesktop.org/
POPPLERDATA_VERSION=0.4.12
POPPLERDATA_TAR="poppler-data-${POPPLERDATA_VERSION}.tar.gz"
POPPLERDATA_SHA256=c835b640a40ce357e1b83666aabd95edffa24ddddd49b8daff63adb851cdab74
if download_step; then
    download "$POPPLER_SITE" "$POPPLERDATA_TAR" "$POPPLERDATA_SHA256"
fi
if build_step && { force_build || { [ ! -d "$SDK_HOME/share/poppler" ]; }; }; then
    start_build
    untar "$SRC_PATH/$POPPLERDATA_TAR"
    pushd "poppler-data-${POPPLERDATA_VERSION}"
    make install datadir="${SDK_HOME}/share"
    #cp "$SDK_HOME/share/pkgconfig/poppler-data.pc" "$SDK_HOME/lib/pkgconfig/poppler-data.pc"
    popd
    rm -rf "poppler-data-${POPPLERDATA_VERSION}"
    end_build
fi
