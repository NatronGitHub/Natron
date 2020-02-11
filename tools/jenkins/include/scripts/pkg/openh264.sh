#!/bin/bash

# Install openh264 (for ffmpeg)
# see https://github.com/cisco/openh264/releases
OPENH264_VERSION=2.0.0
OPENH264_TAR="openh264-${OPENH264_VERSION}.tar.gz"
#OPENH264_SITE="https://github.com/cisco/openh264/archive"
if download_step; then
    download_github cisco openh264 "${OPENH264_VERSION}" v "${OPENH264_TAR}"
    #download "$OPENH264_SITE" "$OPENH264_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/openh264.pc" ] || [ "$(pkg-config --modversion openh264)" != "$OPENH264_VERSION" ]; }; }; then
    REBUILD_FFMPEG=1
    start_build
    untar "$SRC_PATH/$OPENH264_TAR"
    pushd "openh264-${OPENH264_VERSION}"
    # AVX2 is only supported from Kaby Lake on
    # see https://en.wikipedia.org/wiki/Intel_Core
    make HAVE_AVX2=No PREFIX="$SDK_HOME" install
    popd
    rm -rf "openh264-${OPENH264_VERSION}"
    end_build
fi
