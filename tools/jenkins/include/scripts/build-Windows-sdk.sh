#!/bin/bash
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2013-2018 INRIA and Alexandre Gauthier
#
# Natron is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Natron is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

# TODO:
# - On 64+32 systems, the script has to be run twice (first with MSYSTEM=MINGW64, then with MSYSTEM=MINGW32 and PATH=/mingw32/bin:$PATH) to build llvm+osmesa and imagemagick6+7 for both archs. This should be fixed

#options:
#BITS: (32, 64): Select architecture for which to build the SDK
#INSTALL_GCC: If 1, installs gcc for the selected toolchain (with BITS)
#SKIP_DISTRIB_PACKAGES: if 1, all packages that are not custom to Natron will not be checked
#TAR_SDK=1 : Make an archive of the SDK when building is done and store it in $SRC_PATH
#UPLOAD_SDK=1 : Upload the SDK tar archive to $REPO_DEST if TAR_SDK=1
#DOWNLOAD_INSTALLER=1: Force a download of the installer
#DOWNLOAD_FFMPEG_BIN:1: Force a download of the pre-built ffmpeg binaries (built against MXE for static depends)

source common.sh

if [ "$BITS" = "32" ]; then
    export MSYSTEM=MINGW32
    MINGW=mingw32
    PKG_PREFIX="mingw-w64-i686-"
    HAS_MINGW32_TOOLCHAIN=1
    HAS_MINGW64_TOOLCHAIN=0
elif [ "$BITS" = "64" ]; then
    export MSYSTEM=MINGW64
    MINGW=mingw64
    PKG_PREFIX="mingw-w64-x86_64-"
    HAS_MINGW32_TOOLCHAIN=1
    HAS_MINGW64_TOOLCHAIN=1
fi

# makepkg-mingw builds both mingw32 and mingw64 at the same time if the toolchain is installed.
# Take advantage of it and install it if possible, even though the user selected BITS

TMP_BUILD_DIR="$TMP_PATH"


BINARIES_URL="$REPO_DEST/Third_Party_Binaries"
SDK="$PKGOS-$BITS-SDK"

if [ -z "${MKJOBS:-}" ]; then
    #Default to 4 threads
    MKJOBS="$DEFAULT_MKJOBS"
fi



echo
echo "Building $SDK using with $MKJOBS threads ..."
echo
sleep 2


if [ -d "$TMP_BUILD_DIR" ]; then
    rm -rf "$TMP_BUILD_DIR"
fi
mkdir -p "$TMP_BUILD_DIR"
if [ ! -d "$SRC_PATH" ]; then
    mkdir -p "$SRC_PATH"
fi

#Make sure GCC is installed
# not necessary: prebuilt GCC 7.2.0 is installed instead
#if [ ! -f "${SDK_HOME}/bin/gcc" ]; then
#    echo "Make sure to run include/scripts/setup-msys.sh first"
#    exit 1
#fi

# Setup env
export PYTHON_HOME="$SDK_HOME"
export PYTHON_PATH="$SDK_HOME/lib/python${PYVER}"
export PYTHON_INCLUDE="$SDK_HOME/include/python${PYVER}"

rebuildpkgs=${REBUILD_PKGS:-}
for p in $rebuildpkgs; do
    echo "Removing ${PKG_PREFIX}$p"
    fail=0
    if [ "$HAS_MINGW32_TOOLCHAIN" = "1" ]; then
        pacman -R -dd --noconfirm mingw-w64-i686-$p || fail=1
    fi
    if [ "$HAS_MINGW64_TOOLCHAIN" = "1" ]; then
        pacman -R -dd --noconfirm mingw-w64-x86_64-$p || fail=1
    fi
done

download() {
    local master_site=$1
    local package=$2
    if [ ! -s "$SRC_PATH/$package" ]; then
        echo "*** downloading $package"
        wget "$THIRD_PARTY_SRC_URL/$package" -O "$SRC_PATH/$package" || \
        wget --no-check-certificate "$master_site/$package" -O "$SRC_PATH/$package" || \
        rm "$SRC_PATH/$package" || true
    fi
    if [ ! -s "$SRC_PATH/$package" ]; then
        echo "*** Error: unable to download $package"
        exit 1
    fi
}

download_github() {
    # e.g.:
    #download_github uclouvain openjpeg 2.3.0 v openjpeg-2.3.0.tar.gz
    #download_github OpenImageIO oiio 1.6.18 tags/Release- oiio-1.6.18.tar.gz
    local user=$1
    local repo=$2
    local version=$3
    local tagprefix=$4
    local package=$5
    if [ ! -s "$SRC_PATH/$package" ]; then
        echo "*** downloading $package"
        wget "$THIRD_PARTY_SRC_URL/$package" -O "$SRC_PATH/$package" || \
        wget --no-check-certificate --content-disposition "https://github.com/${user}/${repo}/archive/${tagprefix}${version}.tar.gz" -O "$SRC_PATH/$package" || \
        rm "$SRC_PATH/$package" || true
    fi
    if [ ! -s "$SRC_PATH/$package" ]; then
        echo "*** Error: unable to download $package"
        exit 1
    fi
}

untar() {
    echo "*** extracting $1"
    tar xf "$1"
}

# Function build package
# build_package package_name real_db_name packages_to_install
# package_name: the name of the folder, without the mingw-w64-x86_64 prefix
#
# real_db_name: optional, this is the real name of the package in the database. For most
# package it doesn't need to be specified. But some packages (e.g: pyside-qt4), 2 packages
# are produced (pyside-qt4-common and python2-pyside-qt4). If specified, you also
# need to specify packages_to_install
#
# packages_to_install: optional, must be set if real_db_name is set: it is the list of
# packages to install (e.g: in the case of pyside-qt4, that would be "pyside-qt4-common python2-pyside-qt4"
build_package()
{
    package_name="$1"
    real_db_name="${2:-}"
    if [ -z "$real_db_name" ]; then
        real_db_name="$package_name"
    fi
    packages_to_install="${3:-}"
    # Check if the package is installed and extract its version
    pushd $MINGW_PACKAGES_PATH/mingw-w64-${package_name} > /dev/null
    fail=0
    existingpkgversion=`pacman -Qi ${PKG_PREFIX}$real_db_name | grep "Version"` || fail=1
    # Remove prefix
    toremoveprefix="Version*: "
    existingpkgversion=${existingpkgversion#$toremoveprefix}

    # Get the version of our pkg in PKGBUILD
    pkgver=`grep "^pkgver=" PKGBUILD`
    toremoveprefix="pkgver="
    pkgver=${pkgver#$toremoveprefix}

    pkgrel=`grep "^pkgrel=" PKGBUILD`
    toremoveprefix="pkgrel="
    pkgrel=${pkgrel#$toremoveprefix}
    popd > /dev/null
    echo "${PKG_PREFIX}$package_name source is at version $pkgver-$pkgrel"

    if [ -z "$pkgver" ]; then
        echo "Could not determine version of the package in PKGBUILD"
        return 1
    fi

    if [ "$existingpkgversion" = "$pkgver-$pkgrel" ]; then
        echo "${PKG_PREFIX}$package_name is already installed with version $existingpkgversion"
        return 0
    fi

    if [ -z "$packages_to_install" ]; then
        packages_to_install=${real_db_name}-*-any.pkg.tar.xz
    fi

    # -dLfC = no deps check + log + force + clean build, do not add "s" otherwise it will install deps from pacman which may conflict with our own packages
    pushd $MINGW_PACKAGES_PATH/mingw-w64-${package_name} > /dev/null
    rm -rf *.log *.log.[123456789] pkg src *.pkg.tar.xz || true
    makepkg-mingw --skippgpcheck --noconfirm --skipchecksums -dLfC
    if [ "$HAS_MINGW32_TOOLCHAIN" = "1" ]; then
        for p in $packages_to_install; do
            pacman --force --noconfirm -U mingw-w64-i686-$p
        done
    fi
    if [ "$HAS_MINGW64_TOOLCHAIN" = "1" ]; then
        for p in $packages_to_install; do
            pacman --force --noconfirm -U mingw-w64-x86_64-$p
        done
    fi
    popd > /dev/null
}

install_prebuilt_package_for_toolchain()
{
    if [ "$1" = "32" ]; then
        if [ "$HAS_MINGW32_TOOLCHAIN" != "1" ]; then
            return 0
        fi
        PACKAGE=mingw-w64-i686-$2
    elif [ "$1" = "64" ]; then
        if [ "$HAS_MINGW64_TOOLCHAIN" != "1" ]; then
            return 0
        fi
        PACKAGE=mingw-w64-x86_64-$2
    elif [ "$1" = "msys" ]; then
        PACKAGE=$2
    fi

    fail=0
    # Check if the package exists
    existingpkgversion=`pacman -Qi $PACKAGE | grep "Version"` || fail=1
    hasupdate="yes"
    if [ "$fail" = "1" ]; then
        echo "Installing package $PACKAGE"
    fi
    if [ "$fail" = "0" ] && [ ! -z "$existingpkgversion" ]; then
        # If the package exists, check if we have updated available
        hasupdate=`pacman -Qu $PACKAGE` || fail=1
        if [ ! -z "$hasupdate" ]; then
            echo "Updating package $PACKAGE: $hasupdate"
        else
            echo "Package $PACKAGE is up to date"
        fi
    fi
    if [ ! -z "$hasupdate" ]; then
        pacman --noconfirm -S $PACKAGE
    fi
}

install_msys_package()
{
    # This is a system package
    install_prebuilt_package_for_toolchain msys $1
}

install_prebuilt_package()
{
    # This is a mingw32/mingw64 package
    install_prebuilt_package_for_toolchain 32 $1
    install_prebuilt_package_for_toolchain 64 $1
}

# Update remote pre-built packages database, without upgrading anything
pacman --noconfirm  -Sy

# System tools
if [ "${SKIP_DISTRIB_PACKAGES:-}" != "1" ]; then

## install_prebuilt_package toolchain:
install_prebuilt_package binutils
install_prebuilt_package crt-git
install_prebuilt_package gcc
install_prebuilt_package gcc-ada
install_prebuilt_package gcc-fortran
install_prebuilt_package gcc-libgfortran
install_prebuilt_package gcc-libs
install_prebuilt_package gcc-objc
install_prebuilt_package gdb
install_prebuilt_package headers-git
install_prebuilt_package libmangle-git
install_prebuilt_package libwinpthread-git
install_prebuilt_package make
install_prebuilt_package pkg-config
install_prebuilt_package tools-git
install_prebuilt_package winpthreads-git
install_prebuilt_package winstorecompat-git

install_msys_package automake-wrapper
install_msys_package wget
install_msys_package tar
install_msys_package dos2unix
install_msys_package diffutils
install_msys_package file
install_msys_package gawk
install_msys_package libreadline
install_msys_package gettext
install_msys_package grep
install_msys_package make
install_msys_package patch
install_msys_package patchutils
install_msys_package pkg-config
install_msys_package sed
install_msys_package unzip
install_msys_package git
install_msys_package bison
install_msys_package flex
install_msys_package gperf
install_msys_package lndir
install_msys_package m4
install_msys_package perl
install_msys_package rsync
install_msys_package zip
install_msys_package autoconf
install_msys_package m4
install_msys_package libtool
install_msys_package scons
install_msys_package yasm
install_msys_package nasm
install_msys_package python2-pip

# Building osmesa requires package Mako from pip. Since osmesa uses Scons and
# scons is using Msys Python (and not the mingw32/mingw64 one) we must install pip on msys
/usr/bin/pip2 install Mako

# Libs
install_prebuilt_package pkg-config
install_prebuilt_package python2-sphinx
install_prebuilt_package python2-sphinx-alabaster-theme
install_prebuilt_package python2-sphinx_rtd_theme
install_prebuilt_package python2-packaging
install_prebuilt_package python2-pyparsing
install_prebuilt_package gsm
install_prebuilt_package cmake
install_prebuilt_package curl
install_prebuilt_package expat
install_prebuilt_package jsoncpp
install_prebuilt_package libarchive
install_prebuilt_package libbluray
install_prebuilt_package libuv
install_prebuilt_package libiconv
install_prebuilt_package libmng
install_prebuilt_package libxml2
install_prebuilt_package libxslt
install_prebuilt_package zlib
install_prebuilt_package pcre
install_prebuilt_package qtbinpatcher
install_prebuilt_package sqlite3
install_prebuilt_package pkg-config
install_prebuilt_package gdbm
install_prebuilt_package db
install_prebuilt_package python2
install_prebuilt_package python2-mako
install_prebuilt_package boost
install_prebuilt_package libjpeg-turbo
install_prebuilt_package libpng
install_prebuilt_package libtiff
install_prebuilt_package giflib
install_prebuilt_package lcms2
install_prebuilt_package glew
install_prebuilt_package pixman
install_prebuilt_package cairo
install_prebuilt_package openssl
install_prebuilt_package freetype
install_prebuilt_package fontconfig
install_prebuilt_package eigen3
install_prebuilt_package pango
install_prebuilt_package librsvg
install_prebuilt_package libzip
install_prebuilt_package libcdr
#install_prebuilt_package poppler
install_prebuilt_package lame
install_prebuilt_package speex
install_prebuilt_package libtheora
install_prebuilt_package wavpack
install_prebuilt_package xvidcore
install_prebuilt_package tinyxml
install_prebuilt_package yaml-cpp
install_prebuilt_package firebird2-git
install_prebuilt_package libmariadbclient
install_prebuilt_package postgresql
install_prebuilt_package ruby
install_prebuilt_package dbus
install_prebuilt_package libass
install_prebuilt_package libcaca
install_prebuilt_package libmodplug
install_prebuilt_package opencore-amr
install_prebuilt_package opus
install_prebuilt_package rtmpdump-git
install_prebuilt_package openal
install_prebuilt_package opencore-amr
install_prebuilt_package celt
install_prebuilt_package gnutls
install_prebuilt_package fftw

#more dependencies
install_prebuilt_package c-ares
install_prebuilt_package gdk-pixbuf2
install_prebuilt_package gettext
install_prebuilt_package glib2
install_prebuilt_package harfbuzz
install_prebuilt_package icu
install_prebuilt_package libvorbis
install_prebuilt_package ncurses
install_prebuilt_package nghttp2
install_prebuilt_package nspr
install_prebuilt_package nss
install_prebuilt_package python2-setuptools
install_prebuilt_package python3
install_prebuilt_package rhash



fi

# needed by oiio
for prefix in /mingw32 /mingw64; do
    if [ -d "$prefix" ] && [ ! -f "${prefix}/include/byteswap.h" ]; then
        cp $CWD/include/patches/byteswap.h "${prefix}/include/byteswap.h"
    fi
done

# Install llvm+osmesa
# TODO: compile for both toolchains if necessary
OSMESA_VERSION=17.1.10
OSMESA_TAR="mesa-${OSMESA_VERSION}.tar.gz"
OSMESA_SITE="ftp://ftp.freedesktop.org/pub/mesa"
OSMESA_GLU_VERSION=9.0.0
OSMESA_GLU_TAR="glu-${OSMESA_GLU_VERSION}.tar.gz"
OSMESA_GLU_SITE="ftp://ftp.freedesktop.org/pub/mesa/glu"
OSMESA_DEMOS_VERSION=8.3.0
OSMESA_DEMOS_TAR="mesa-demos-${OSMESA_DEMOS_VERSION}.tar.bz2"
OSMESA_DEMOS_SITE="ftp://ftp.freedesktop.org/pub/mesa/demos/${OSMESA_DEMOS_VERSION}"
# LLVM 4.0.0 build crashes on windows 32 bit, see
# http://lists.llvm.org/pipermail/cfe-dev/2016-December/052017.html 
OSMESA_LLVM_VERSION=3.9.1 #4.0.1
OSMESA_LLVM_TAR="llvm-${OSMESA_LLVM_VERSION}.src.tar.xz"
OSMESA_LLVM_SITE="http://releases.llvm.org/${OSMESA_LLVM_VERSION}"
#for prefix in /mingw32 /mingw64; do
#    if [ -d "$prefix" ]; then
prefix="$SDK_HOME"
if [ "${REBUILD_OSMESA:-}" = "1" ]; then
    rm -rf "$prefix/osmesa" || true
    rm -rf "$prefix/llvm" || true
fi
if [ ! -s "$prefix/osmesa/lib/pkgconfig/gl.pc" ] || [ "$(env PKG_CONFIG_PATH=$prefix/osmesa/lib/pkgconfig:${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH} pkg-config --modversion gl)" != "$OSMESA_VERSION" ]; then
    pushd "$TMP_BUILD_DIR"
    download "$OSMESA_SITE" "$OSMESA_TAR"
    download "$OSMESA_GLU_SITE" "$OSMESA_GLU_TAR"
    download "$OSMESA_DEMOS_SITE" "$OSMESA_DEMOS_TAR"
    mkdir -p "${prefix}/osmesa" || true
    LLVM_BUILD=0
    if [ ! -s "$prefix/llvm/bin/llvm-config" ] || [ "$($prefix/llvm/bin/llvm-config --version)" != "$OSMESA_LLVM_VERSION" ]; then
        LLVM_BUILD=1
        download "$OSMESA_LLVM_SITE" "$OSMESA_LLVM_TAR"
        mkdir -p "${prefix}/llvm" || true
    fi
    git clone https://github.com/devernay/osmesa-install
    pushd osmesa-install
    mkdir build
    pushd build
    cp "$SRC_PATH/$OSMESA_TAR" .
    cp "$SRC_PATH/$OSMESA_GLU_TAR" .
    cp "$SRC_PATH/$OSMESA_DEMOS_TAR" .
    if [ "$LLVM_BUILD" = 1 ]; then
       cp "$SRC_PATH/$OSMESA_LLVM_TAR" .
    fi
    env OSMESA_DRIVER=3 OSMESA_PREFIX="${prefix}/osmesa" OSMESA_VERSION="$OSMESA_VERSION" LLVM_PREFIX="${prefix}/llvm" LLVM_VERSION="$OSMESA_LLVM_VERSION" MKJOBS="${MKJOBS}" LLVM_BUILD="$LLVM_BUILD" ../osmesa-install.sh
    popd
    popd
    rm -rf osmesa-install
    popd
fi
#       fi ## if [ -d "$prefix" ]
#fi # for prefix in /mingw32 /mingw64

# Install openh264
install_prebuilt_package openh264

# Install openjpeg
install_prebuilt_package openjpeg2

# Install libwebp
install_prebuilt_package libwebp

# Install libvpx (adds --enable-vp9-highbitdepth)
build_package libvpx

# Install x264 (use stable version)
build_package x264

# Install x265
install_prebuilt_package x265
# Install x265 (add HDR10+ and 12bit support)
#build_package x265

# Install snappy (for ffmpeg)
install_prebuilt_package snappy

# Install ffmpeg
build_package ffmpeg-gpl2
build_package ffmpeg-gpl3
build_package ffmpeg-lgpl

# Install magick
MAGICK_VERSION=6.9.9-21
MAGICK_VERSION_SHORT=${MAGICK_VERSION%-*}
MAGICK_TAR="ImageMagick-${MAGICK_VERSION}.tar.xz"
MAGICK_SITE="https://www.imagemagick.org/download/releases"

#build_package imagemagick-6
# see https://github.com/rwinlib/imagemagick6/tree/master/mingw-w64-imagemagick
rebuildmagick="${REBUILD_MAGICK:-}"
if [ "$rebuildmagick" = "1" ]; then
   rm -rf $SDK_HOME/include/ImageMagick-6/ $SDK_HOME/lib/libMagick* $SDK_HOME/share/ImageMagick-6/ $SDK_HOME/lib/pkgconfig/{Image,Magick}*
fi
if [ ! -f "$SDK_HOME/lib/pkgconfig/Magick++.pc" ] || [ "$(pkg-config --modversion Magick++)" != "$MAGICK_VERSION_SHORT" ]; then
    pushd "$TMP_BUILD_DIR"
    download "$MAGICK_SITE" "$MAGICK_TAR"
    untar "$SRC_PATH/$MAGICK_TAR"
    pushd "ImageMagick-${MAGICK_VERSION}"
    patch -p1 -i $INC_PATH/patches/ImageMagick/ImageMagick-6.8.8.1-mingw.patch
    # 002-build-fixes.patch has the magick/distribute-cache.c patch removed because of:
    # https://github.com/ImageMagick/ImageMagick/commit/ba09c8617914ce50e45950d978dc9f8900e87918
    patch -p1 -i $INC_PATH/patches/ImageMagick/002-build-fixes.patch
    patch -p0 -i $INC_PATH/patches/ImageMagick/pango-align-hack.diff
    #patch -p0 -i $INC_PATH/patches/ImageMagick/mingw-utf8.diff
    env CFLAGS="-DMAGICKCORE_EXCLUDE_DEPRECATED=1" CXXFLAGS="-I${SDK_HOME}/include -DMAGICKCORE_EXCLUDE_DEPRECATED=1" LDFLAGS="-lz -lws2_32" ./configure --prefix=$SDK_HOME --with-magick-plus-plus=yes --with-quantum-depth=32 --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --with-lcms --without-openjp2 --without-lqr --without-lzma --without-openexr --with-pango --with-png --with-rsvg --without-tiff --without-webp --with-xml --with-zlib --without-bzlib --enable-static --disable-shared --enable-hdri --with-freetype --with-fontconfig --without-x --without-modules
    make -j${MKJOBS}
    make install
    popd
    popd
fi

# Disable imagemagick 7 (no time to maintain is, and IM6 works well for us)
# patches should be grabvbed from https://github.com/Alexpux/MINGW-packages/tree/master/mingw-w64-imagemagick
if false; then
MAGICK7_VERSION=7.0.7-9
MAGICK7_VERSION_SHORT=${MAGICK7_VERSION%-*}
MAGICK7_TAR="ImageMagick-${MAGICK7_VERSION}.tar.xz"
rebuildmagick7="${REBUILD_MAGICK7:-}"
if [ "$rebuildmagick7" = "1" ]; then
   rm -rf $SDK_HOME/magick7
fi
if [ ! -d $SDK_HOME/magick7 ]; then
    pushd "$TMP_BUILD_DIR"
    download "$MAGICK_SITE" "$MAGICK7_TAR"
    untar "$SRC_PATH/$MAGICK7_TAR"
    pushd "ImageMagick-${MAGICK7_VERSION}"
    patch -p1 -i $INC_PATH/patches/ImageMagick/im7-mingw.patch
    patch -p0 -i $INC_PATH/patches/ImageMagick/im7-mingw-utf8.diff
    autoreconf -fi
    env LDFLAGS="-lws2_32 -loleaut32" ./configure --disable-deprecated --prefix=$SDK_HOME/magick7 --with-magick-plus-plus=yes --with-quantum-depth=32 --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --with-lcms --without-openjp2 --without-lqr --without-lzma --without-openexr --without-pango --with-png --without-rsvg --without-tiff --without-webp --without-xml --with-zlib --without-bzlib --enable-static --disable-shared --enable-hdri --with-freetype --without-fontconfig --without-x --without-modules
    make -j${MKJOBS}
    make install
    popd
    popd
fi
fi # if false

# Install ocio
# TODO: modify to checkout the right commit, not HEAD
build_package opencolorio-git

# Install libraw
build_package libraw-gpl2
build_package libraw-gpl3
build_package libraw-lgpl

# Install ilmbase (adds ilmbase-2.2.0-threadpool_release_lock_single_thread.patch)
build_package ilmbase
# Install openexr
install_prebuilt_package openexr

# Install oiio
# openimageio 1.8.6 hangs when quitting (even on Test.exe): https://github.com/OpenImageIO/oiio/issues/1795
build_package openimageio17

# Install qt
# TODO: add c++11 support patches from linux sdk
build_package qt4

# Install shiboken
#build_package shiboken-qt4 shiboken-qt4 "python2-shiboken-qt4 shiboken-qt4"
install_prebuilt_package shiboken-qt4
install_prebuilt_package python2-shiboken-qt4
for prefix in /mingw32 /mingw64; do
    if [ -s "${prefix}/lib/pkgconfig/shiboken-py2.pc" ]; then
        sed -i -e "s@^Libs: C:/building/msys[36][24]${prefix}/lib/libpython2.7.dll.a@Libs: -L${prefix}/lib -lpython2.7.dll@" -e "s@^Cflags: -IC:/building/msys[36][24]${prefix}/include/python2.7@Cflags: -I${prefix}/include/python2.7@" -e "s@^python_interpreter=C:/building/msys[36][24]${prefix}/bin/python2.exe@python_interpreter=${prefix}/bin/python2@" -e "s@^python_include_dir=C:/building/msys[36][24]${prefix}/include/python2.7@python_include_dir=${prefix}/include/python2.7@" "${prefix}/lib/pkgconfig/shiboken-py2.pc"
    fi
done

# Install pyside
#build_package pyside-qt4 pyside-qt4-common "python2-pyside-qt4 pyside-qt4-common"
install_prebuilt_package pyside-common-qt4
install_prebuilt_package python2-pyside-qt4
# in /mingw*/lib/pkgconfig/pyside-py2.pc replace shiboken with shiboken-py2
for prefix in /mingw32 /mingw64; do
    if [ -s "${prefix}/lib/pkgconfig/pyside-py2.pc" ]; then
        sed -i 's/^Requires: shiboken$/Requires: shiboken-py2/' "${prefix}/lib/pkgconfig/pyside-py2.pc"
    fi
done

# Install poppler (for openfx-arena, without nss libcurl, qt4 or qt5 support)
build_package poppler


# Install SeExpr
if true; then
    install_prebuilt_package seexpr
else
SEE_VERSION=1.0.1
SEE_TAR="SeExpr-rel-${SEE_VERSION}.tar.gz"
rebuildseexpr="${REBUILD_SEEXPR:-}"
if [ "$rebuildseexpr" = "1" ]; then
    rm -rf $SDK_HOME/lib/libSeExpr* $SDK_HOME/include/SeExpr*
fi
if [ ! -f $SDK_HOME/lib/libSeExpr.a ]; then
    pushd "$TMP_BUILD_DIR"
    download_github wdas SeExpr "$SEE_VERSION" rel- "$SEE_TAR"
    untar "$SRC_PATH/$SEE_TAR"
    pushd "SeExpr-rel-${SEE_VERSION}"
    pushd src/SeExpr
    patch -u -i $CWD/include/patches/SeExpr/mingw-compat.patch 
    popd
    mkdir build
    pushd build
    env CPPFLAGS="-I${SDK_HOME}/include" LDFLAGS="-L${SDK_HOME}/lib" cmake .. -G"MSYS Makefiles" -DCMAKE_INSTALL_PREFIX=$SDK_HOME
    make -j${MKJOBS}
    make install
    mkdir -p $SDK_HOME/docs/seexpr
    cp ../README ../src/doc/license.txt $SDK_HOME/docs/seexpr/
    popd # build
    popd
    popd
fi
fi

if [ ! -z "${TAR_SDK:-}" ]; then
    # Done, make a tarball
    pushd "$SDK_HOME/.."
    tar cvvJf "$SRC_PATH/Natron-$SDK.tar.xz" "$SDK_HOME"
    echo "SDK tar available: $SRC_PATH/Natron-$SDK.tar.xz"
    if [ ! -z "${UPLOAD_SDK:-}" ]; then
        rsync -avz --progress --verbose -e 'ssh -oBatchMode=yes' "$SRC_PATH/Natron-$SDK.tar.xz" "$BINARIES_URL"
    fi
    popd

fi


#Make sure we have mt.exe for embedding manifests

if [ ! -f "${SDK_HOME}/bin/repogen.exe" ] || [ "${DOWNLOAD_INSTALLER:-}" = "1" ] || [ ! -f "SDK_HOME/bin/dump_syms.exe" ] || [ ! -f "$SDK_HOME/bin/mt.exe" ]; then
    INSTALLER_BIN_TAR=natron-windows-installer.zip
    pushd "$SRC_PATH"
    $WGET "$THIRD_PARTY_BIN_URL/$INSTALLER_BIN_TAR" -O "$SRC_PATH/$INSTALLER_BIN_TAR"
    unzip -o "$CWD/src/$INSTALLER_BIN_TAR"
    cp -a natron-windows-installer/mingw32/bin/* /mingw32/bin/
    if [ ! -d /mingw32/dump_syms_x64 ]; then
        mkdir /mingw32/dump_syms_x64
    fi
    cp natron-windows-installer/mingw64/bin/dump_syms.exe /mingw32/dump_syms_x64/
    if [ "$BITS" = 64 ] && [ -d /mingw64 ]; then
        cp -a natron-windows-installer/mingw64/bin/* /mingw64/bin/
        cp /mingw64/bin/libgcc_s_seh-1.dll /mingw32/dump_syms_x64/
        cp /mingw64/bin/libstdc++-6.dll /mingw32/dump_syms_x64/
        cp /mingw64/bin/libwinpthread-1.dll /mingw32/dump_syms_x64/
    fi
    popd
fi

echo
echo "Natron build-sdk done."
echo
exit 0


# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
