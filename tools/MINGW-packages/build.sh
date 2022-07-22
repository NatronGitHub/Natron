#!/bin/sh
set -e -x
CWD=`pwd`
PKGS="
mingw-w64-qt4
mingw-w64-shiboken
mingw-w64-pyside
mingw-w64-openexr25
mingw-w64-libraw-gpl2
mingw-w64-opencolorio2-git
mingw-w64-openimageio
mingw-w64-opencolorio2-git
mingw-w64-seexpr-git
mingw-w64-x264-git
mingw-w64-ffmpeg-gpl2
mingw-w64-sox
mingw-w64-poppler
mingw-w64-imagemagick
"
echo "mingw-w64-osmesa is currently BROKEN https://github.com/SCons/scons/wiki/LongCmdLinesOnWin32"
sleep 10
for pkg in ${PKGS}; do
    cd "${CWD}/$pkg"
    ${CWD}/makepkg.sh
    pacman -U mingw-w64-x86_64-*-any.pkg.tar.*
done
