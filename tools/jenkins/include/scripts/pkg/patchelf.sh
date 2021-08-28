#!/bin/bash

# install patchelf
# see https://github.com/NixOS/patchelf/releases
PATCHELF_VERSION=0.13
PATCHELF_TAR="patchelf-${PATCHELF_VERSION}.tar.bz2"
PATCHELF_DIR="patchelf-${PATCHELF_VERSION}.20200827.8d3a16e"
#PATCHELF_VERSION=0.11
#PATCHELF_TAR="patchelf-${PATCHELF_VERSION}.tar.gz"
#PATCHELF_DIR="patchelf-${PATCHELF_VERSION}.20200609.d6b2a72"
#PATCHELF_SITE="https://nixos.org/releases/patchelf/patchelf-${PATCHELF_VERSION}/"
#PATCHELF_SITE="https://releases.nixos.org/releases.nixos.org/patchelf/patchelf-${PATCHELF_VERSION}/"
PATCHELF_SITE="http://github.com/NixOS/patchelf/releases/download/${PATCHELF_VERSION}/"
if download_step; then
    download "$PATCHELF_SITE" "$PATCHELF_TAR"
    #download_github NixOS patchelf "${PATCHELF_VERSION}" v "${PATCHELF_TAR}" # doesn't work with bz2
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/patchelf" ]; }; }; then
    start_build
    untar "$SRC_PATH/$PATCHELF_TAR"
    pushd "${PATCHELF_DIR}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME"
    make
    make install
    #cp src/patchelf "$SDK_HOME/bin/"
    popd
    rm -rf "${PATCHELF_DIR}"
    end_build
fi
