#!/bin/sh
set -e -x
CWD=`pwd`
PKGS="
mingw-w64-libraw-gpl2
mingw-w64-libraw-lgpl
mingw-w64-openimageio
mingw-w64-seexpr-git
mingw-w64-ffmpeg-gpl2
mingw-w64-imagemagick
mingw-w64-osmesa
"
for pkg in ${PKGS}; do
    cd "${CWD}/$pkg"
    ${CWD}/makepkg.sh
    pacman -U "${MINGW_PACKAGE_PREFIX}"-*-any.pkg.tar.*
done
