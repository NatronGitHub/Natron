#!/bin/bash
# install libsndfile (for sox)
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/libsndfile.html
LIBSNDFILE_VERSION=1.0.28
LIBSNDFILE_TAR="libsndfile-${LIBSNDFILE_VERSION}.tar.gz"
LIBSNDFILE_SITE="http://www.mega-nerd.com/libsndfile/files"
if download_step; then
    download "$LIBSNDFILE_SITE" "$LIBSNDFILE_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/sndfile.pc" ] || [ "$(pkg-config --modversion sndfile)" != "$LIBSNDFILE_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBSNDFILE_TAR"
    pushd "libsndfile-${LIBSNDFILE_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libsndfile-${LIBSNDFILE_VERSION}"
    end_build
fi
