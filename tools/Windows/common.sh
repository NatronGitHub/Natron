#!/bin/sh
# Natron Common Build Options

# Versions
#


CWD=`pwd`

#The local.sh file must exist, please see the README.
if [ -f $CWD/local.sh ]; then
    source $CWD/local.sh || exit 1
else
    REPO_DEST=localhost
    REPO_URL=http://localhost
fi


#THE FOLLOWING CAN BE MODIFIED TO CONFIGURE RELEASE BUILDS
#----------------------------------------------------------
NATRON_GIT_TAG=tags/2.0.0-RC4
IOPLUG_GIT_TAG=tags/Natron-2.0.0-RC4
MISCPLUG_GIT_TAG=tags/Natron-2.0.0-RC4
ARENAPLUG_GIT_TAG=tags/Natron-2.0.0-RC4
CVPLUG_GIT_TAG=tags/Natron-2.0.0-RC4
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
GIT_OCIO_CONFIG_TAR=https://github.com/MrKepzie/OpenColorIO-Configs/archive/Natron-v2.0.tar.gz
COLOR_PROFILES_VERSION=2.0


# SDK
#

SDK_VERSION=2.0
MINGW_PACKAGES_PATH=$CWD/MINGW-packages
MINGW_PREFIX=mingw-w64-
PKG_PREFIX32=${MINGW_PREFIX}i686-
PKG_PREFIX64=${MINGW_PREFIX}x86_64-
INSTALL32_PATH=/mingw32
INSTALL64_PATH=/mingw64

# Common values
#
TMP_DIR=/tmp
TMP_PATH=$CWD/tmp
SRC_PATH=$CWD/src
INC_PATH=$CWD/include

# Keep existing tag, else make a new one
if [ -z "$TAG" ]; then
    TAG=`date +%Y%m%d%H%M`
fi

OS=`uname -o`
REPO_DIR_PREFIX=$CWD/build_


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

THIRD_PARTY_SRC_URL=$REPO_URL/Third_Party_Sources
THIRD_PARTY_BIN_URL=$REPO_URL/Third_Party_Binaries

GIT_OPENCV=https://github.com/devernay/openfx-opencv.git
GIT_ARENA=https://github.com/olear/openfx-arena.git

#Installer is a fork of qtifw to fix a few bugs
GIT_INSTALLER=https://github.com/olear/qtifw.git

GIT_NATRON=https://github.com/MrKepzie/Natron.git
GIT_IO=https://github.com/MrKepzie/openfx-io.git
GIT_MISC=https://github.com/devernay/openfx-misc.git

QT4_TAR=qt-everywhere-opensource-src-4.8.7.tar.gz
#QT5_TAR=qt-everywhere-opensource-src-5.4.1.tar.gz
CV_TAR=opencv-2.4.11.zip
EIGEN_TAR=eigen-eigen-bdd17ee3b1b3.tar.gz
YASM_TAR=yasm-1.3.0.tar.gz
CMAKE_TAR=cmake-3.1.2.tar.gz
PY3_TAR=Python-3.4.3.tar.xz
PY2_TAR=Python-2.7.10.tar.xz
JPG_TAR=jpegsrc.v9a.tar.gz
OJPG_TAR=openjpeg-1.5.2.tar.gz
PNG_TAR=libpng-1.2.53.tar.gz
TIF_TAR=tiff-4.0.4.tar.gz
ILM_TAR=ilmbase-2.2.0.tar.gz
EXR_TAR=openexr-2.2.0.tar.gz
GLEW_TAR=glew-1.12.0.tgz
BOOST_TAR=boost_1_55_0.tar.gz
CAIRO_TAR=cairo-1.14.2.tar.xz
FFMPEG_TAR=ffmpeg-2.7.2.tar.bz2
OCIO_TAR=OpenColorIO-1.0.9.tar.gz
OIIO_TAR=oiio-Release-1.5.20.tar.gz
PYSIDE_TAR=pyside-qt4.8+1.2.2.tar.bz2
SHIBOK_TAR=shiboken-1.2.2.tar.bz2
LIBXML_TAR=libxml2-2.9.2.tar.gz
LIBXSL_TAR=libxslt-1.1.28.tar.gz
SEE_TAR=SeExpr-rel-1.0.1.tar.gz
LIBRAW_TAR=LibRaw-0.16.0.tar.gz
PIX_TAR=pixman-0.32.6.tar.gz
LCMS_TAR=lcms2-2.6.tar.gz
MAGICK_TAR=ImageMagick-6.9.1-10.tar.gz
GIF_TAR=giflib-5.1.1.tar.gz
#SSL_TAR=openssl-1.0.0r.tar.gz 
JASP_TAR=jasper-1.900.1.zip
INSTALLER32_BIN_TAR=natron-win32-installer-extra.zip
INSTALLER64_BIN_TAR=natron-win64-installer-extra.zip

FFMPEG_MXE_BIN_64_GPL_TAR=ffmpeg-2.7.2-windows-x86_64-shared-GPLv2.tar.xz
FFMPEG_MXE_BIN_32_GPL_TAR=ffmpeg-2.7.2-windows-i686-shared-GPLv2.tar.xz
FFMPEG_MXE_BIN_64_LGPL_TAR=ffmpeg-2.7.2-windows-x86_64-shared-LGPL.tar.xz
FFMPEG_MXE_BIN_32_LGPL_TAR=ffmpeg-2.7.2-windows-i686-shared-LGPL.tar.xz

NATRON_API_DOC=https://media.readthedocs.org/pdf/natron/workshop/natron.pdf # TODO generate own

GCC_V=`gcc --version | awk '{print $0;exit 0;}' | awk '{print $7}' | sed -e 's#\.# #g'`
GCC_MAJOR=`echo $GCC_V | awk '{print $1}'`
GCC_MINOR=`echo $GCC_V | awk '{print $2}'`
if [ "$GCC_MAJOR" -lt "4" ]; then
    echo "Wrong GCC version. Must be at least 4.8"
    exit 1
elif [ "$GCC_MAJOR" -eq "4" ] && [ "$GCC_MINOR" -lt "8" ]; then
    echo "Wrong GCC version. Must be at least 4.8"
    exit 1
fi

# Threads
#
# Set build threads to 4 if not exists
DEFAULT_MKJOBS=4
if [ -z "$MKJOBS" ]; then
    MKJOBS=$DEFAULT_MKJOBS
fi

