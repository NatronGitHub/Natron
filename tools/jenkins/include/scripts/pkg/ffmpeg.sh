#!/bin/bash

# Install FFmpeg
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/ffmpeg.html
FFMPEG_VERSION=9.0.2
# see https://ffmpeg.org/download.html
FFMPEG_VERSION_LIBAVCODEC=63.1.102
FFMPEG_TAR="ffmpeg-${FFMPEG_VERSION}.tar.xz"
FFMPEG_SHA256=8c3850283eb25fa026482078a04051e0be17347b09ef81a0849bec15a96e002e
FFMPEG_SITE="https://ffmpeg.org/releases"
# 9.0.2 + the 10-bit simple IDCT saturation patch (openfx-io#266, "+idct10sat") and the dnxhd
# encoder mb_qscale patch (openfx-io#267, "+dnxqscale"), as on macOS (tools/MacPorts/multimedia/ffmpeg).
# Drop each patch, and its suffix, with the first release that contains the fix.
FFMPEG_SDK_VERSION="${FFMPEG_VERSION}+idct10sat+dnxqscale"
if download_step; then
    download "$FFMPEG_SITE" "$FFMPEG_TAR" "$FFMPEG_SHA256"
fi
if build_step && { force_build || { [ "${REBUILD_FFMPEG:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/ffmpeg-* || true
fi
if build_step && { force_build || { [ ! -d "$SDK_HOME/ffmpeg-gpl2" ] || [ ! -d "$SDK_HOME/ffmpeg-lgpl" ] || [ "$(env PKG_CONFIG_PATH=$SDK_HOME/ffmpeg-gpl2/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion libavcodec)" != "$FFMPEG_VERSION_LIBAVCODEC" ] || [ "$(cat "$SDK_HOME/share/natron-sdk/ffmpeg.version" 2>/dev/null)" != "$FFMPEG_SDK_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$FFMPEG_TAR"
    pushd "ffmpeg-${FFMPEG_VERSION}"
    # Saturate the C 10-bit int16 simple IDCT row outputs, as the x86 SIMD does: aarch64 decoded
    # 10-bit DNxHD/DNxHR with full white with black 1-pixel columns (openfx-io#266). The same
    # patch as tools/MacPorts/multimedia/ffmpeg. FFMPEG_SDK_VERSION, recorded below, makes existing
    # SDKs pick it up.
    patch -p0 -i "$INC_PATH/patches/FFmpeg/patch-libavcodec-simple_idct10-saturate.diff"
    # Store the dnxhd encoder's per-macroblock qscale in 16 bits: it wrapped above 255, and DNxHR 444
    # of high-entropy images aborted the host (openfx-io#267). The same patch as on macOS.
    patch -p0 -i "$INC_PATH/patches/FFmpeg/patch-libavcodec-dnxhdenc-mb_qscale.diff"
    LGPL_SETTINGS=(
        "--enable-swscale" "--enable-avfilter"
        "--enable-libmp3lame" "--enable-libvorbis" "--enable-libopus" "--enable-librsvg"
        "--enable-libtheora" "--enable-libopenh264" "--enable-libopenjpeg"
        "--enable-libsnappy" "--enable-libmodplug" "--enable-libvpx" "--enable-libsoxr"
        "--enable-libspeex" "--enable-libass" "--enable-libbluray" "--enable-lzma"
        "--enable-libaom" "--enable-libdav1d"
        "--enable-lcms2" "--enable-libharfbuzz" "--enable-libwebp"
        "--enable-gnutls" "--enable-fontconfig" "--enable-libfreetype" "--enable-libfribidi"
        "--disable-libxcb" "--disable-libxcb-shm" "--disable-libxcb-xfixes" "--disable-libxcb-shape"
        "--disable-indev=jack" "--disable-outdev=xv" "--disable-xlib" "--disable-debug" )
    if [ "$ARCH" = "x86_64" ] || [ "$ARCH" = "i686" ]; then
        LGPL_SETTINGS+=( "--enable-x86asm" )
    fi
    # GPL version 2 or later: no --enable-version3, which would make the binaries GPL 3 (or LGPL 3
    # for the LGPL build). None of the enabled libraries requires it (configure fails otherwise):
    # the version3 ones are gmp, libaribb24, liblensfun, libopencore-amr*, libvmaf, libvo-amrwbenc,
    # mbedtls, rkmpp, and libsmbclient (GPL 3).
    GPL_SETTINGS=( "${LGPL_SETTINGS[@]}" "--enable-gpl" "--enable-libx264" "--enable-libx265" "--enable-libxvid" )

    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME/ffmpeg-gpl2" --libdir="$SDK_HOME/ffmpeg-gpl2/lib" --enable-shared --disable-static "${GPL_SETTINGS[@]}"
    make -j${MKJOBS}
    make install
    make distclean
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME/ffmpeg-lgpl" --libdir="$SDK_HOME/ffmpeg-lgpl/lib" --enable-shared --disable-static "${LGPL_SETTINGS[@]}"
    make install
    mkdir -p "$SDK_HOME/share/natron-sdk"
    echo "$FFMPEG_SDK_VERSION" > "$SDK_HOME/share/natron-sdk/ffmpeg.version"
    popd
    rm -rf "ffmpeg-${FFMPEG_VERSION}"
    end_build
fi
