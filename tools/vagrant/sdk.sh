#!/bin/sh
#
# Natron SDK
#
set -x
source `pwd`/functions.sh

# setup
#if [ "$PKGOS" = "Linux" ] && [ -f "/etc/redhat-release" ]; then
#  setup_centos
#fi

# basic tools/libs
setup_ssl
setup_qt installer
setup_installer 
setup_patchelf
setup_yasm
setup_cmake

# gcc
if [ "$PKGOS" = "Linux" ]; then
  setup_gcc || exit 1
  export LD_LIBRARY_PATH="$INSTALL_PATH/lib"
  export PATH="$INSTALL_PATH/gcc/bin:$INSTALL_PATH/bin:$INSTALL_PATH/cmake/bin:$PATH"
  if [ "$ARCH" = "x86_64" ]; then
      LD_LIBRARY_PATH="$INSTALL_PATH/gcc/lib64:$LD_LIBRARY_PATH"
  else
      LD_LIBRARY_PATH="$INSTALL_PATH/gcc/lib:$LD_LIBRARY_PATH"
  fi
  export LD_LIBRARY_PATH
  export CC="${INSTALL_PATH}/gcc/bin/gcc"
  export CXX="${INSTALL_PATH}/gcc/bin/g++"
fi

# core depends
setup_zlib 
setup_bzip 
setup_icu 
setup_python 

# common env
PKG_CONFIG_PATH="$INSTALL_PATH/lib/pkgconfig"
QTDIR="$INSTALL_PATH"
BOOST_ROOT="$INSTALL_PATH"
OPENJPEG_HOME="$INSTALL_PATH"
THIRD_PARTY_TOOLS_HOME="$INSTALL_PATH"
PYTHON_HOME="$INSTALL_PATH"
PYTHON_PATH="$INSTALL_PATH/lib/python2.7"
PYTHON_INCLUDE="$INSTALL_PATH/include/python2.7"
export PKG_CONFIG_PATH LD_LIBRARY_PATH PATH QTDIR BOOST_ROOT OPENJPEG_HOME THIRD_PARTY_TOOLS_HOME PYTHON_HOME PYTHON_PATH PYTHON_INCLUDE

# depends
setup_expat 
setup_png 
setup_freetype 
setup_fontconfig 
setup_ffi 
setup_glib 
setup_xml 
setup_xslt 
setup_boost 
setup_jpeg 
setup_tiff 
setup_jasper 
setup_lcms 
setup_openjpeg 
setup_libraw 
setup_openexr 
setup_pixman 
setup_cairo 
setup_harfbuzz 
setup_pango 
setup_croco 
setup_gdk 
setup_rsvg 
setup_glew 
setup_lame
setup_ogg
setup_vorbis
setup_theora
setup_modplug
setup_vpx
setup_opus
setup_orc
setup_dirac
setup_x264
setup_xvid

# OCIO/OIIO/SeExpr/Magick/ffmpeg
setup_magick
setup_ocio 
setup_oiio 
setup_seexpr 
setup_ffmpeg

# Qt/PySide
setup_qt
setup_shiboken
setup_pyside


