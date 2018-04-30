#!/bin/bash
# Installs the required Msys/Mingw packages to build Natron and plug-ins

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

source common.sh
source common-buildmaster.sh

if [ "${BITS}" = "32" ]; then
    PKG_PREFIX=$PKG_PREFIX32
else
    PKG_PREFIX=$PKG_PREFIX64
fi

# Remove stuff

if [ "${REMOVE_PYTHON:-}" = "1" ]; then
    PY_PKGS=`pacman -Q|grep py | awk '{print $1}'`
    for i in $PY_PKGS; do
        pacman -R $i
    done
fi

#Update database
pacman --noconfirm  -Syu

PKG_INSTALL_OPS="--noconfirm -S"
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}toolchain
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}yasm
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}gsm
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}gdbm
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}db
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}python2
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}python2-mako
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}boost
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}libjpeg-turbo
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}libpng
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}libtiff
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}giflib
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}lcms2
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}glew
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}pixman
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}cairo
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}openssl
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}freetype
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}fontconfig
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}eigen3
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}pango
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}librsvg
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}libzip
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}cmake
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}libcdr
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}glfw
pacman $PKG_INSTALL_OPS ${PKG_PREFIX}poppler
pacman $PKG_INSTALL_OPS wget tar diffutils file gawk gettext grep make patch patchutils pkg-config sed unzip git bison flex rsync zip automake autoconf m4 scons

# needed by oiio
if [ ! -f /mingw${BITS}/include/byteswap.h ]; then
    cp $CWD/include/patches/byteswap.h /mingw${BITS}/include/byteswap.h || exit  1
fi

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
