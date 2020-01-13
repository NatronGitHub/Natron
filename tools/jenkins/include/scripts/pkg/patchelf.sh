#!/bin/bash

# install patchelf
# see https://nixos.org/releases/patchelf
PATCHELF_VERSION=0.10
PATCHELF_TAR="patchelf-${PATCHELF_VERSION}.tar.bz2"
PATCHELF_SITE="https://nixos.org/releases/patchelf/patchelf-${PATCHELF_VERSION}/"
if download_step; then
    download "$PATCHELF_SITE" "$PATCHELF_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/patchelf" ]; }; }; then
    start_build
    untar "$SRC_PATH/$PATCHELF_TAR"
    pushd "patchelf-${PATCHELF_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME"
    make
    make install
    #cp src/patchelf "$SDK_HOME/bin/"
    popd
    rm -rf "patchelf-${PATCHELF_VERSION}"
    end_build
fi
