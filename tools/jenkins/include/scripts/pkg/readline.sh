#!/bin/bash

# Install readline
# see http://www.linuxfromscratch.org/lfs/view/development/chapter08/readline.html
READLINE_VERSION=8.1
READLINE_VERSION_MAJOR=${READLINE_VERSION%.*}
READLINE_TAR="readline-${READLINE_VERSION}.tar.gz"
READLINE_SITE="https://ftp.gnu.org/gnu/readline"
if download_step; then
    download "$READLINE_SITE" "$READLINE_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libreadline.so.${READLINE_VERSION_MAJOR}" ]; }; }; then
    start_build
    untar "$SRC_PATH/$READLINE_TAR"
    pushd "readline-${READLINE_VERSION}"
    sed -i '/MV.*old/d' Makefile.in
    sed -i '/{OLDSUFF}/c:' support/shlib-install
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME"
    make -j${MKJOBS} SHLIB_LIBS="-L${SDK_HOME}/lib -lncursesw"
    make install
    popd
    rm -rf "readline-${READLINE_VERSION}"
    end_build
fi
