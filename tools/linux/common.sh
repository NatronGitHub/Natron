#!/bin/sh
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2015 INRIA and Alexandre Gauthier
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

# Natron Common Build Options

# Versions
#

#THE FOLLOWING CAN BE MODIFIED TO CONFIGURE RELEASE BUILDS
#----------------------------------------------------------
NATRON_GIT_TAG=tags/2.0.0
IOPLUG_GIT_TAG=tags/2.0.0
MISCPLUG_GIT_TAG=tags/2.0.0
ARENAPLUG_GIT_TAG=tags/2.0.0
CVPLUG_GIT_TAG=tags/2.0.0
#----------------------------------------------------------


#Name of the packages in the installer
#If you change this, don't forget to change the xml file associated in include/xml
NATRON_PKG=fr.inria.natron
IOPLUG_PKG=fr.inria.openfx.io
MISCPLUG_PKG=fr.inria.openfx.misc
ARENAPLUG_PKG=fr.inria.openfx.extra
CVPLUG_PKG=fr.inria.openfx.opencv
CORELIBS_PKG=fr.inria.natron.libs
PROFILES_PKG=fr.inria.natron.color

PACKAGES=$NATRON_PKG,$CORELIBS_PKG,$PROFILES_PKG,$IOPLUG_PKG,$MISCPLUG_PKG,$ARENAPLUG_PKG,$CVPLUG_PKG


# bump number when OpenColorIO-Configs changes
COLOR_PROFILES_VERSION=2.0.0

# bump number on bugfixes
CORELIBS_VERSION=2.0.0

# SDK
#

SDK_LIC=GPL #enable gpl features (ffmpeg)
SDK_VERSION=CY2015
SDK_PATH=/opt
PYV=2 # Python 2 or 3

# Common values
#

CWD=$(pwd)
TMP_DIR=/tmp
TMP_PATH=$CWD/tmp
SRC_PATH=$CWD/src
INSTALL_PATH=$SDK_PATH/Natron-$SDK_VERSION
INC_PATH=$CWD/include

# Keep existing tag, else make a new one
if [ -z "$TAG" ]; then
  TAG=$(date +%Y%m%d%H%M)
fi

OS=$(uname -o)
REPO_DIR_PREFIX=$CWD/build_


# Repo settings
#
if [ -f $CWD/repo.sh ]; then
  source $CWD/repo.sh
else
  REPO_DEST=localhost
  REPO_URL=http://localhost
fi

#Dist repo is expected to be layout as such:
#downloads.xxx.yyy:
#   Windows/
#   Linux/
#       releases/
#       snapshots/
#           32bit/
#           64bit/
#               files/ (where installers should be
#               packages/ (where the updates for the maintenance tool should be)


# Third-party sources
#

THIRD_PARTY_SRC_URL=http://downloads.natron.fr/Third_Party_Sources

GIT_OPENCV=https://github.com/devernay/openfx-opencv.git
GIT_ARENA=https://github.com/olear/openfx-arena.git

#Installer is a fork of qtifw to fix a few bugs (TODO update/move to v2 upstream when we move to CY2016+ (qt5))
GIT_INSTALLER=https://github.com/olear/qtifw.git

GIT_NATRON=https://github.com/MrKepzie/Natron.git
GIT_IO=https://github.com/MrKepzie/openfx-io.git
GIT_MISC=https://github.com/devernay/openfx-misc.git

QT4_TAR=qt-everywhere-opensource-src-4.8.7.tar.gz
#QT5_TAR=qt-everywhere-opensource-src-5.4.1.tar.gz # CY2016+
CV_TAR=opencv-2.4.11.zip
EIGEN_TAR=eigen-eigen-bdd17ee3b1b3.tar.gz
YASM_TAR=yasm-1.3.0.tar.gz
CMAKE_TAR=cmake-3.1.2.tar.gz
PY2_TAR=Python-2.7.10.tar.xz
PY3_TAR=Python-3.4.3.tar.xz # CY2017+?
JPG_TAR=jpegsrc.v9a.tar.gz
OJPG_TAR=openjpeg-1.5.2.tar.gz
PNG_TAR=libpng-1.2.53.tar.gz
TIF_TAR=tiff-4.0.4.tar.gz
ILM_TAR=ilmbase-2.2.0.tar.gz
EXR_TAR=openexr-2.2.0.tar.gz
GLEW_TAR=glew-1.12.0.tgz
BOOST_TAR=boost_1_55_0.tar.bz2
CAIRO_TAR=cairo-1.14.2.tar.xz
FFMPEG_TAR=ffmpeg-2.7.2.tar.bz2
OCIO_TAR=OpenColorIO-1.0.9.tar.gz
OIIO_TAR=oiio-Release-1.5.18.tar.gz
PYSIDE_TAR=pyside-qt4.8+1.2.2.tar.bz2
SHIBOK_TAR=shiboken-1.2.2.tar.bz2
#LIBXML_TAR=libxml2-2.9.2.tar.gz (#25)
#LIBXSL_TAR=libxslt-1.1.28.tar.gz (#25)
SEE_TAR=SeExpr-rel-1.0.1.tar.gz
LIBRAW_TAR=LibRaw-0.16.0.tar.gz
PIX_TAR=pixman-0.32.6.tar.gz
LCMS_TAR=lcms2-2.6.tar.gz
MAGICK_TAR=openfx-ImageMagick-6.8.10-1.tar.gz # (tmp) fork for use with openfx
#SSL_TAR=openssl-1.0.0r.tar.gz #TODO ssl support in installer
JASP_TAR=jasper-1.900.1.zip
NATRON_API_DOC=https://media.readthedocs.org/pdf/natron/workshop/natron.pdf # TODO generate own?
LAME_TAR=lame-3.99.5.tar.gz
OGG_TAR=libogg-1.3.2.tar.gz
VORBIS_TAR=libvorbis-1.3.5.tar.gz
THEORA_TAR=libtheora-1.1.1.tar.bz2
MODPLUG_TAR=libmodplug-0.8.8.5.tar.gz
VPX_TAR=libvpx-1.4.0.tar.bz2
SPEEX_TAR=speex-1.2rc1.tar.gz
OPUS_TAR=opus-1.1.tar.gz
DIRAC_TAR=schroedinger-1.0.11.tar.gz
ORC_TAR=orc-0.4.23.tar.xz
X264_TAR=x264-snapshot-20150725-2245.tar.bz2 #GPL-only
XVID_TAR=xvidcore-1.3.4.tar.gz #GPL-only

# GCC version
#
# Check for minimal required GCC version (4.8)

GCC_V=$(gcc --version | awk '/gcc /{print $0;exit 0;}' | awk '{print $3}' | sed 's#\.# #g' | awk '{print $2}')
if [ "$GCC_V" -lt "8" ]; then
  echo "Wrong GCC version. Run ${INC_PATH}/scripts/setup-gcc.sh"
  exit 1
fi

# Linux version
#
# Check distro and version. CentOS/RHEL 6.4 only!

if [ ! -f /etc/redhat-release ]; then
  echo "Wrong distro, stupid :P"
  exit 1
else
  RHEL_MAJOR=$(cat /etc/redhat-release | cut -d" " -f3 | cut -d "." -f1)
  RHEL_MINOR=$(cat /etc/redhat-release | cut -d" " -f3 | cut -d "." -f2)
  if [ "$RHEL_MAJOR" != "6" ] || [ "$RHEL_MINOR" != "4" ]; then
    echo "Wrong distro version, 6.4 only at the moment!"
    exit 1
  fi
fi

# Arch
#
# Default build flags

if [ -z "$ARCH" ]; then
  case "$( uname -m )" in
    i?86) export ARCH=i686 ;;
       *) export ARCH=$( uname -m ) ;;
  esac
fi
if [ "$ARCH" = "i686" ]; then
  BF="-O2 -march=i686 -mtune=i686"
  BIT=32
elif [ "$ARCH" = "x86_64" ]; then
  BF="-O2 -fPIC"
  BIT=64
else
  BF="-O2"
fi

# Threads
#
# Set build threads to 4 if not exists
DEFAULT_MKJOBS=4
if [ -z "$MKJOBS" ]; then
    MKJOBS=$DEFAULT_MKJOBS
fi

