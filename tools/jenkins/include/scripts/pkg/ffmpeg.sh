#!/bin/bash

# Install FFmpeg
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/ffmpeg.html
FFMPEG_VERSION=6.0
# see https://ffmpeg.org/download.html
FFMPEG_VERSION_LIBAVCODEC=60.3.100
FFMPEG_TAR="ffmpeg-${FFMPEG_VERSION}.tar.xz"
FFMPEG_SITE="http://www.ffmpeg.org/releases"
if download_step; then
    download "$FFMPEG_SITE" "$FFMPEG_TAR"
fi
if build_step && { force_build || { [ "${REBUILD_FFMPEG:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/ffmpeg-* || true
fi
if build_step && { force_build || { [ ! -d "$SDK_HOME/ffmpeg-gpl2" ] || [ ! -d "$SDK_HOME/ffmpeg-lgpl" ] || [ "$(env PKG_CONFIG_PATH=$SDK_HOME/ffmpeg-gpl2/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion libavcodec)" != "$FFMPEG_VERSION_LIBAVCODEC" ]; }; }; then
    start_build
    untar "$SRC_PATH/$FFMPEG_TAR"
    pushd "ffmpeg-${FFMPEG_VERSION}"
    LGPL_SETTINGS=(
        "--enable-x86asm" "--enable-swscale" "--enable-avfilter"
        "--enable-libmp3lame" "--enable-libvorbis" "--enable-libopus" "--enable-librsvg"
        "--enable-libtheora" "--enable-libopenh264" "--enable-libopenjpeg"
        "--enable-libsnappy" "--enable-libmodplug" "--enable-libvpx" "--enable-libsoxr"
        "--enable-libspeex" "--enable-libass" "--enable-libbluray" "--enable-lzma"
        "--enable-libaom" "--enable-libdav1d"
        "--enable-gnutls" "--enable-fontconfig" "--enable-libfreetype" "--enable-libfribidi"
        "--disable-libxcb" "--disable-libxcb-shm" "--disable-libxcb-xfixes"
        "--disable-indev=jack" "--disable-outdev=xv" "--disable-xlib" "--disable-debug" )
    GPL_SETTINGS=( "${LGPL_SETTINGS[@]}" "--enable-gpl" "--enable-libx264" "--enable-libx265" "--enable-libxvid" "--enable-version3" )

    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME/ffmpeg-gpl2" --libdir="$SDK_HOME/ffmpeg-gpl2/lib" --enable-shared --disable-static "${GPL_SETTINGS[@]}"
    make -j${MKJOBS}
    make install
    make distclean
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME/ffmpeg-lgpl" --libdir="$SDK_HOME/ffmpeg-lgpl/lib" --enable-shared --disable-static "${LGPL_SETTINGS[@]}"
    make install
    popd
    rm -rf "ffmpeg-${FFMPEG_VERSION}"
    end_build
fi
