#!/bin/bash

# Install poppler-data
# see https://poppler.freedesktop.org/
POPPLERDATA_VERSION=0.4.12
POPPLERDATA_TAR="poppler-data-${POPPLERDATA_VERSION}.tar.gz"
if download_step; then
    download "$POPPLER_SITE" "$POPPLERDATA_TAR"
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
