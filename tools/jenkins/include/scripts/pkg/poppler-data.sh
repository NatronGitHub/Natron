#!/bin/bash

# Install poppler-data
POPPLERDATA_VERSION=0.4.9
POPPLERDATA_TAR="poppler-data-${POPPLERDATA_VERSION}.tar.gz"
if build_step && { force_build || { [ ! -d "$SDK_HOME/share/poppler" ]; }; }; then
    start_build
    download "$POPPLER_SITE" "$POPPLERDATA_TAR"
    untar "$SRC_PATH/$POPPLERDATA_TAR"
    pushd "poppler-data-${POPPLERDATA_VERSION}"
    make install datadir="${SDK_HOME}/share"
    popd
    rm -rf "poppler-data-${POPPLERDATA_VERSION}"
    end_build
fi
