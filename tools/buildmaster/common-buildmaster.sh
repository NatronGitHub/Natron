#!/bin/bash
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2016 INRIA and Alexandre Gauthier
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

#THE FOLLOWING CAN BE MODIFIED TO CONFIGURE RELEASE BUILDS
#----------------------------------------------------------
NATRON_RELEASE_BRANCH=RB-2.3
NATRON_GIT_TAG=tags/2.3.13
IOPLUG_GIT_TAG=tags/Natron-2.3.13
MISCPLUG_GIT_TAG=tags/Natron-2.3.13
ARENAPLUG_GIT_TAG=tags/Natron-2.3.13
GMICPLUG_GIT_TAG=tags/Natron-2.3.13
CVPLUG_GIT_TAG=tags/Natron-2.3.13
#----------------------------------------------------------

# Name of the packages in the installer
# If you change this, don't forget to change the xml file associated in include/xml
NATRON_PKG=fr.inria.natron
IOPLUG_PKG=fr.inria.openfx.io
MISCPLUG_PKG=fr.inria.openfx.misc
ARENAPLUG_PKG=fr.inria.openfx.extra
GMICPLUG_PKG=fr.inria.openfx.gmic
CVPLUG_PKG=fr.inria.openfx.opencv
CORELIBS_PKG=fr.inria.natron.libs
OCIO_PKG=fr.inria.natron.color

PACKAGES="$NATRON_PKG,$CORELIBS_PKG,$PROFILES_PKG,$IOPLUG_PKG,$MISCPLUG_PKG,$ARENAPLUG_PKG,$GMICPLUG_PKG" #,$CVPLUG_PKG

# OCIO package version (linux/windows)
# bump number when OpenColorIO-Configs changes
OCIO_PROFILES_VERSION="20180327000000"

# Libs package version (linux/windows)
# bump timestamp on SDK changes, important!
LINUX_CORELIBS_VERSION="20180627000000"
WINDOWS_CORELIBS_VERSION="20180627000000"
CORELIBS_VERSION=""

if [ "$PKGOS" = "Linux" ]; then
    CORELIBS_VERSION=$LINUX_CORELIBS_VERSION
elif [ "$PKGOS" = "Windows" ]; then
    CORELIBS_VERSION=$WINDOWS_CORELIBS_VERSION
fi

# SDK
#




MASTER_BRANCH=master
CMAKE_PLUGINS=0

if [ "$PKGOS" = "Linux" ]; then
    true
elif [ "$PKGOS" = "Windows" ]; then
    SDK_VERSION=2.0
    MINGW_PREFIX=mingw-w64-
    PKG_PREFIX32=${MINGW_PREFIX}i686-
    PKG_PREFIX64=${MINGW_PREFIX}x86_64-
elif [ "$PKGOS" = "OSX" ]; then
    true
fi


# Keep existing tag, else make a new one
if [ -z "${TAG:-}" ]; then
    TAG=$(date "+%Y%m%d%H%M")
fi

REPO_DIR_PREFIX="$CWD/build_"


#Dist repo is expected to be layout as such:
#downloads.xxx.yyy:
#   Windows/
#   Linux/
#       releases/
#       snapshots/BRANCH
#                 32bit/
#                 64bit/
#                       files/ (where installers should be)
#                       packages/ (where the updates for the maintenance tool should be)

# NOTE! new buildmaster.sh script has different layout, see buildmaster.sh.

# Third-party sources
#

WGET="wget --referer https://natron.fr/"

GIT_OPENCV=https://github.com/NatronGitHub/openfx-opencv.git
GIT_ARENA=https://github.com/NatronGitHub/openfx-arena.git
GIT_GMIC=https://github.com/NatronGitHub/openfx-gmic.git
GIT_INSTALLER=https://github.com/NatronGitHub/qtifw.git
GIT_BREAKPAD=https://github.com/NatronGitHub/breakpad.git # breakpad for server-side or linux dump_syms, not for Natron!
GIT_BREAKPAD_COMMIT=48a13da168f6a82d9edf63a22769cb42a660996c
GIT_NATRON=https://github.com/NatronGitHub/Natron.git
GIT_NATRON_GFORGE=https://gforge.inria.fr/anonscm/git/natron/natron.git
GIT_IO=https://github.com/NatronGitHub/openfx-io.git
GIT_MISC=https://github.com/NatronGitHub/openfx-misc.git
GIT_UNIT=https://github.com/NatronGitHub/Natron-Tests.git
GIT_OSMESA=https://github.com/devernay/osmesa-install.git # integration TODO (this has been done in sdk v2)

# Used by the $REPO variable in buildmaster.sh to determine the label of the repository to use
GIT_NATRON_REPO_LABEL="Natron_github"
GIT_NATRON_GFORGE_REPO_LABEL="Natron_gforge"

QT4_TAR=qt-everywhere-opensource-src-4.8.7.tar.gz
CV_TAR=opencv-2.4.11.zip
EIGEN_TAR=eigen-eigen-bdd17ee3b1b3.tar.gz
YASM_TAR=yasm-1.3.0.tar.gz
CMAKE_TAR=cmake-3.7.2.tar.gz
PY2_TAR=Python-2.7.12.tar.xz
PY3_TAR=Python-3.5.1.tar.xz
TJPG_TAR=libjpeg-turbo-1.5.0.tar.gz
OJPG_TAR=openjpeg-2.1.2.tar.gz # can be bumped to v2?
PNG_TAR=libpng-1.6.25.tar.xz
TIF_TAR=tiff-4.0.7.tar.gz
ILM_TAR=ilmbase-2.2.0.tar.gz
EXR_TAR=openexr-2.2.0.tar.gz.orig
GLEW_TAR=glew-1.12.0.tgz
BOOST_TAR=boost_1_55_0.tar.bz2
if [ "${SDK_VERSION:-}" = "CY2017" ]; then
    BOOST_TAR=boost_1_61_0.tar.bz2
fi
CAIRO_TAR=cairo-1.14.6.tar.xz
FFMPEG_TAR=ffmpeg-3.2.4.tar.xz
OCIO_TAR=OpenColorIO-1.0.9.tar.gz
OIIO_TAR=oiio-Release-1.7.12.tar.gz
PYSIDE_TAR=pyside-qt4.8+1.2.2.tar.bz2
SHIBOK_TAR=shiboken-1.2.2.tar.bz2
LIBXML_TAR=libxml2-2.9.4.tar.gz
LIBXSLT_TAR=libxslt-1.1.29.tar.gz
SEE_TAR=SeExpr-rel-1.0.1.tar.gz
SEE2_TAR=SeExpr-2.11.tar.gz
LIBRAW_TAR=LibRaw-0.18.1.tar.gz # 0.18-201604 gives better blackmagic support
LIBRAW_DEMOSAIC_PACK_GPL2=LibRaw-demosaic-pack-GPL2-0.18.1.tar.gz
PIX_TAR=pixman-0.34.0.tar.bz2
LCMS_TAR=lcms2-2.8.tar.gz
MAGICK_TAR=ImageMagick-6.9.7-10.tar.xz
MAGICK7_TAR=ImageMagick-7.0.3-4.tar.xz
SSL_TAR=openssl-1.0.2j.tar.gz # always a new version around the corner
JASP_TAR=jasper-1.900.1.zip # add CVE's
LAME_TAR=lame-3.99.5.tar.gz
OGG_TAR=libogg-1.3.2.tar.gz
VORBIS_TAR=libvorbis-1.3.5.tar.gz
THEORA_TAR=libtheora-1.1.1.tar.bz2
MODPLUG_TAR=libmodplug-0.8.8.5.tar.gz
VPX_TAR=libvpx-1.6.1.tar.gz
SPEEX_TAR=speex-1.2rc1.tar.gz
OPUS_TAR=opus-1.1.tar.gz
DIRAC_TAR=schroedinger-1.0.11.tar.gz
ORC_TAR=orc-0.4.23.tar.xz
X264_TAR=x264-snapshot-20170313-2245-stable.tar.bz2 
X265_TAR=x265_2.3.tar.gz
XVID_TAR=xvidcore-1.3.4.tar.gz
ICU_TAR=icu4c-55_1-src.tgz
ZLIB_TAR=zlib-1.2.11.tar.gz
EXPAT_TAR=expat-2.2.0.tar.bz2
FCONFIG_TAR=fontconfig-2.10.2.tar.gz
FCONFIG12_TAR=fontconfig-2.12.1.tar.bz2
FTYPE_TAR=freetype-2.4.11.tar.gz
FFI_TAR=libffi-3.2.1.tar.gz
GLIB_TAR=glib-2.42.2.tar.xz
PKGCONF_TAR=pkg-config-0.29.1.tar.gz
GETTEXT_TAR=gettext-0.19.8.1.tar.gz

# Linux
if [ "${SDK_VERSION:-}" = "CY2016" ] || [ "${SDK_VERSION:-}" = "CY2017" ]; then
    BUZZ_TAR=harfbuzz-1.2.3.tar.bz2
elif [ "${SDK_VERSION:-}" = "CY2015" ]; then
    BUZZ_TAR=harfbuzz-0.9.40.tar.bz2
fi

PANGO_TAR=pango-1.40.1.tar.xz
BZIP_TAR=bzip2-1.0.6.tar.gz
CROCO_TAR=libcroco-0.6.8.tar.xz
SVG_TAR=librsvg-2.40.16.tar.xz
GDK_TAR=gdk-pixbuf-2.32.1.tar.xz
ELF_TAR=patchelf-0.8.tar.bz2
ZIP_TAR=libzip-1.0.1.tar.xz
GIF_TAR=giflib-5.1.2.tar.bz2
CPPU_TAR=cppunit-1.13.2.tar.gz
CDR_TAR=libcdr-0.1.2.tar.xz
REVENGE_TAR=librevenge-0.0.4.tar.xz
LLVM_TAR=llvm-3.8.0.src.tar.xz
MESA_TAR=mesa-11.2.1.tar.gz
GLU_TAR=glu-9.0.0.tar.bz2
POPPLER_TAR=poppler-0.43.0.tar.xz
POPPLERD_TAR=poppler-data-0.4.7.tar.gz
GLFW_TAR=glfw-3.1.2.tar.gz
CURL_TAR=curl-7.50.3.tar.bz2

# Qt5 and PySide2
PYSIDE2_GIT=https://codereview.qt-project.org/pyside/pyside
PYSIDE2_COMMIT=cd0edf5ffdb289992eadd6f3acc91d2567006069
SHIBOK2_GIT=https://codereview.qt-project.org/pyside/shiboken
SHIBOK2_COMMIT=de736cf64f34e815d5bbf55751577dc1ee373924
QT5_VERSION=5.6.1
QTBASE_TAR=qtbase-vfxplatform-src-${QT5_VERSION}.tar.xz # we use vfxplatform fork
QTXMLP_TAR=qtxmlpatterns-opensource-src-${QT5_VERSION}.tar.xz
QTDEC_TAR=qtdeclarative-opensource-src-${QT5_VERSION}.tar.xz
QTSVG_TAR=qtsvg-opensource-src-${QT5_VERSION}.tar.xz
QTX11_TAR=qtx11extras-vfxplatform-src-${QT5_VERSION}.tar.xz # we use vfxplatform fork

# Linux toolchain
if [ "$PKGOS" = "Linux" ]; then
    BUILD_GCC5=0
    GDB_TAR=gdb-7.11.1.tar.xz # when using the sdk during debug we get a conflict with native gdb, so bundle our own
    TC5_GCC=5.4.0 # gcc5 just for testing, not used
    TC_GCC=4.8.5
    TC_MPC=1.0.1
    TC_MPFR=3.1.3
    TC_GMP=5.1.3
    TC_ISL=0.11.1
    TC5_ISL=0.16.1
    TC_CLOOG=0.18.0
    GCC_TAR=gcc-${TC_GCC}.tar.bz2
    GCC5_TAR=gcc-${TC5_GCC}.tar.bz2
    MPC_TAR=mpc-${TC_MPC}.tar.gz
    MPFR_TAR=mpfr-${TC_MPFR}.tar.bz2
    GMP_TAR=gmp-${TC_GMP}.tar.bz2
    ISL_TAR=isl-${TC_ISL}.tar.bz2
    ISL5_TAR=isl-${TC5_ISL}.tar.bz2
    CLOOG_TAR=cloog-${TC_CLOOG}.tar.gz
fi

# Windows custom/override
if [ "$PKGOS" = "Windows" ]; then
    INSTALLER_BIN_TAR=natron-windows-installer.zip
    LLVM_TAR=llvm-3.7.1.src.tar.xz
    FFMPEG_MXE_VERSION=3.2.4
    FFMPEG_MXE_BIN_64_GPL_TAR=ffmpeg-$FFMPEG_MXE_VERSION-windows-x86_64-shared-GPLv2.tar.xz
    FFMPEG_MXE_BIN_32_GPL_TAR=ffmpeg-$FFMPEG_MXE_VERSION-windows-i686-shared-GPLv2.tar.xz
    FFMPEG_MXE_BIN_64_LGPL_TAR=ffmpeg-$FFMPEG_MXE_VERSION-windows-x86_64-shared-LGPL.tar.xz
    FFMPEG_MXE_BIN_32_LGPL_TAR=ffmpeg-$FFMPEG_MXE_VERSION-windows-i686-shared-LGPL.tar.xz
fi

# Linux version
#
# Check distro and version. CentOS/RHEL 6.4 only!

if [ "$PKGOS" = "Linux" ]; then
    if [ ! -f /etc/redhat-release ]; then
        echo "Build system has been designed for CentOS/RHEL, use at OWN risk!"
        sleep 5
    else
        RHEL_MAJOR=`cat /etc/redhat-release | cut -d" " -f3 | cut -d "." -f1`
        RHEL_MINOR=`cat /etc/redhat-release | cut -d" " -f3 | cut -d "." -f2`
        if [ "$RHEL_MAJOR" != "6" ] || [ "$RHEL_MINOR" != "4" ]; then
            echo "Wrong version of CentOS/RHEL, 6.4 is the only supported version!"
            sleep 5
        fi
    fi
fi

if [ "$PKGOS" = "Linux" ]; then
    true
elif [ "$PKGOS" = "OSX" ]; then
    true
else
    if [ "${BITS}" = "32" ]; then
        PKG_PREFIX="$PKG_PREFIX32"
    elif [ "$BITS" = "64" ]; then
        PKG_PREFIX="$PKG_PREFIX64"
    else
        echo "Unsupported BITS"
        exit 1
    fi
fi

# License
#
#
if [ "${NATRON_LICENSE:-}" != "GPL" ] && [ "${NATRON_LICENSE:-}" != "COMMERCIAL" ]; then
    echo "Please select a License with NATRON_LICENSE=(GPL,COMMERCIAL)"
    exit 1
fi
#echo "===> NATRON_LICENSE set to $NATRON_LICENSE"




# queue for buildmaster.sh
REMOTE_USER="buildmaster"
REMOTE_URL="downloads.natron.fr"
QUEUE_URL="${REMOTE_USER}@${REMOTE_URL}"

QUEUE_LOG_UPLOAD_PATH="logs"
QUEUE_FILES_UPLOAD_PATH="files"
QUEUE_PACKAGES_UPLOAD_PATH="packages"
QUEUE_SYMBOLS_UPLOAD_PATH="symbols"

QUEUE_LOG_PATH="$CWD/qlogs"
QUEUE_TMP_PATH="$CWD/qbuild_"
MYSQL_BIN="mysql"
if [ ! -d "$QUEUE_LOG_PATH" ]; then
    mkdir -p "$QUEUE_LOG_PATH" || exit 1
fi
if [ "$PKGOS" = "Linux" ]; then
    QOS="linux${BITS}"
elif [ "$PKGOS" = "Windows" ]; then
    QOS="win${BITS}"
    MYSQL_BIN="/opt/mariadb/bin/mysql.exe"
elif [ "$PKGOS" = "OSX" ]; then
    QOS=mac
    MYSQL_BIN="${MACPORTS}/lib/mariadb/bin/mysql"
else
    echo "OS check failed!"
    exit 1
fi

MYSQL="${MYSQL_BIN} -hsupport.natron.fr -ubuildmaster -pNatronBuildMasterWWW2016 buildmaster -e"

function updateStatusLog() {
    $MYSQL "UPDATE queue SET ${QOS}_status='${2}' WHERE id=${1};" || true
}
function updateStatus() {
    $MYSQL "UPDATE queue SET ${QOS}=${2} WHERE id=${1};" || true
}
function updateLog() {
    $MYSQL "INSERT INTO log (message,timestamp) VALUES('${1}',NOW());" || true
}
function buildFailed() {
    $MYSQL "INSERT INTO status (qbuild,qos,qstatus,timestamp) VALUES (${1},'${2}',2,NOW());" || true
}
function buildOK() {
    $MYSQL "INSERT INTO status (qbuild,qos,qstatus,timestamp) VALUES (${1},'${2}',1,NOW());" || true
}
function updateUnitStatus() {
    $MYSQL "UPDATE queue SET ${QOS}_unit='${2}' WHERE id=${1};" || true
}
function pingServer() {
    $MYSQL "UPDATE servers SET ping=NOW(),${QOS}_current=${2} WHERE servername='${1}';" || true
}

TMP_BINARIES_PATH="$TMPDIR/Natron-${PKGOS}-${ARCH}"



# Does install path exists?

if [ ! -d "$TMP_BINARIES_PATH" ]; then
    mkdir -p "$TMP_BINARIES_PATH" || exit 1
fi

if ! type -p keychain > /dev/null; then
    echo "Error: keychain not available, install from https://www.funtoo.org/Keychain"
    exit 1
fi

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
