#!/bin/bash

# Configurable script indicating DLL versions installed on the system that
# should ship with Natron
# Since msys2 is a moving target, each msys2 build machine almost has different  DLLs versions

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

if [ -z "${PYVER:-}" ]; then
	echo "PYVER must be set to Python version"
	exit 1
fi

OUTPUT_FILE_NAME="dllVersions.sh"

echo "#!/bin/bash" > $OUTPUT_FILE_NAME

SDK_HOME=""
if [ -z "${BITS:-}" ]; then
    echo "BITS must be set to 32 or 64"
    exit 1
fi

if [ "$BITS" = "32" ]; then
    SDK_HOME="/mingw32"
else
    SDK_HOME="/mingw64"
fi

SDK_HOME_BIN="$SDK_HOME/bin"

if [ "${NATRON_LICENSE:-}" != "GPL" ] && [ "${NATRON_LICENSE:-}" != "COMMERCIAL" ]; then
    echo "NATRON_LICENSE must be set to GPL or COMMERCIAL"
    exit 1
fi

if [ -z "${LIBRAW_PATH:-}" ]; then
    if [ "$NATRON_LICENSE" = "GPL" ]; then
        LIBRAW_PATH="$SDK_HOME/libraw-gpl2/bin"
    else
        LIBRAW_PATH="$SDK_HOME/libraw-lgpl/bin"
    fi
    echo "LIBRAW_PATH not set, assuming this is $LIBRAW_PATH"
fi

if [ -z "${FFMPEG_PATH:-}" ]; then
    if [ "$NATRON_LICENSE" = "GPL" ]; then
        FFMPEG_PATH="$SDK_HOME/ffmpeg-gpl2/bin"
    else
        FFMPEG_PATH="$SDK_HOME/ffmpeg-lgpl/bin"
    fi
    echo "FFMPEG_PATH not set, assuming this is $FFMPEG_PATH"
fi

function findDll() {
    #Try with given name first, otherwise try with * suffix
    DLL_NAME=$(cd "$BIN_PATH"; ls "$1".dll 2>/dev/null || true)
    if [ -f "$BIN_PATH/$1.dll" ]; then
	DLL_NAME="$1.dll"
    else
	DLL_NAME=$(cd "$BIN_PATH"; ls "$1"*.dll 2>/dev/null || true)
    fi
}

function catDll() {
    DLL_NAME=""
    findDll $1
    DLL_LEN=$(echo "$DLL_NAME" | wc -w)
    if [ "$DLL_LEN" = "0" ]; then
        echo "WARNING, dll $1 was not found!"
   #     sleep 1
	return 1
    elif [ "$DLL_LEN" != "1" ]; then
        echo "WARNING, More than 1 DLL were found for $1: $DLL_NAME"
  #      sleep 1
	return 1
    fi
#    echo "Found DLL: $DLL_NAME"
    if [ "$INIT_VAR" = "1" ]; then
    	echo "${DLL_VAR_PREFIX}_DLL="'('"${BIN_PATH}/${DLL_NAME}"')' >> "$OUTPUT_FILE_NAME"
	INIT_VAR=0
    else
        echo "${DLL_VAR_PREFIX}_DLL="'('"\"\${${DLL_VAR_PREFIX}_DLL[@]}\" ${BIN_PATH}/${DLL_NAME}"')' >> "$OUTPUT_FILE_NAME"
    fi
}

INIT_VAR=1
DLL_VAR_PREFIX="NATRON"
BIN_PATH="$SDK_HOME_BIN"
if [ "$QT_VERSION_MAJOR" = "4" ]; then
    # all Qt libraries are necessary for PySide
    catDll Qt3Support4
    catDll QtCLucene4
    catDll QtCore4
    catDll QtDBus4
    catDll QtDeclarative4
    catDll QtDesigner4
    catDll QtDesignerComponents4
    catDll QtGui4
    catDll QtHelp4
    catDll QtMultimedia4
    catDll QtNetwork4
    catDll QtOpenGL4
    catDll QtScript4
    catDll QtScriptTools4
    catDll QtSql4
    catDll QtSvg4
    catDll QtTest4
    catDll QtXml4
    catDll QtXmlPatterns4

    catDll phonon4
    catDll libmng-
    catDll libpcre-
elif [ "$QT_VERSION_MAJOR" = "5" ]; then
    catDll Qt5Concurrent
    catDll Qt5Core
    catDll Qt5DBus
    catDll Qt5Gui
    catDll Qt5Network
    catDll Qt5OpenGL
    catDll Qt5PrintSupport
    catDll Qt5Qml
    catDll Qt5QmlModels
    catDll Qt5QmlWorkerScript
    catDll Qt5Quick
    catDll Qt5QuickParticles
    catDll Qt5QuickShapes
    catDll Qt5QuickTest
    catDll Qt5QuickWidgets
    catDll Qt5Sql
    catDll Qt5Test
    catDll Qt5Widgets
    catDll Qt5Xml
    catDll Qt5XmlPatterns

    catDll libdouble-conversion
    catDll libicuin
    catDll libpcre2-16-
    catDll libmd4c
else
    echo "Unsupported QT_MAJOR_VERSION" ${QT_VERSION_MAJOR}
    exit 1
fi


catDll fbclient
catDll libass-
catDll libbluray-
catDll libboost_filesystem-mt
catDll libboost_regex-mt
catDll libboost_serialization-mt
catDll libboost_system-mt
catDll libboost_thread-mt
catDll libbz2-
catDll libcaca-
catDll libcairo-2
#catDll libcroco-
#catDll LIBEAY32
catDll libexpat-
catDll libffi-
catDll libfontconfig-
catDll libfreetype-
catDll libfribidi-
catDll libgdk_pixbuf-
catDll libgif-
catDll libgio-
catDll libglib-
catDll libgmodule-
catDll libgmp-
catDll libgnutls-30
catDll libgobject-
catDll libgomp-
catDll libgraphite
catDll libgsm
catDll libharfbuzz-0
catDll libhogweed-
catDll libiconv-
catDll libicudt
catDll libicuuc
catDll libidn2-
catDll libIex-
catDll libIlmThread-
catDll libImath-
catDll libintl-
catDll libjasper
catDll libjpeg-
catDll liblcms2-
catDll liblzma-
catDll libmfx
catDll libmodplug-
catDll libmp3lame-
catDll libnettle-
catDll libnghttp2-
catDll libogg-
catDll libopenal-
catDll libOpenColorIO
catDll libOpenEXR-
catDll libOpenEXRCore
catDll libOpenImageIO
catDll libOpenImageIO_Util
catDll libopenjp2-
catDll libopus-
catDll libp11-kit-
catDll libpango-
catDll libpangocairo-
catDll libpangoft2-
catDll libpangowin32-
catDll libpcre2-8-
catDll libpixman-
catDll libpng16-
#catDll libPtex
catDll librsvg-
catDll librtmp-
catDll libsharpyuv-
catDll libspeex-
catDll libsqlite3-
catDll libsrt
catDll libssh2-
catDll libstdc++-
catDll libSvtAv1Enc
catDll libtasn1-
catDll libtheoradec-
catDll libtheoraenc-
catDll libtiff-
catDll libunistring-
catDll libunibreak-
catDll libvorbis-
catDll libvorbisenc-
catDll libvpx-
#catDll libwavpack-
catDll libwebp-
catDll libwebpdemux-
catDll libwebpmux-
catDll libwinpthread-
catDll libxml2-
#catDll SDL2
#catDll SSLEAY32
catDll zlib1
catDll libbrotlicommon
catDll libbrotlidec
catDll libbrotlienc
catDll libssl-3-
catDll libcrypto-3-
catDll libzstd

catDll libSeExpr
catDll libyaml-cpp
catDll libopenh264
catDll libthai
catDll libdatrie-
catDll libsnappy
#catDll libpugixml
catDll libheif
catDll rav1e
catDll libde265-
catDll libx265
catDll libdav1d
catDll libaom
catDll libdeflate
catDll libjbig-
catDll liblerc
catDll libpsl-5
catDll libcairo-gobject-

# python
catDll libpython${PYVER}
if [ "$PYV" = 2 ]; then
    catDll libpyside-python${PYVER}
    catDll libshiboken-python${PYVER}
else
    catDll libpyside
    catDll libshiboken
fi

# gcc
if [ "$BITS" = "32" ]; then
    catDll libgcc_s_dw2-
else
    catDll libgcc_s_seh-
fi

if [ "$NATRON_LICENSE" = "GPL" ]; then
    catDll libx264-
    #catDll libx265
    #catDll libx265_main
    catDll xvidcore
fi

BIN_PATH="$SDK_HOME/osmesa/lib"
catDll osmesa
BIN_PATH="$LIBRAW_PATH"
catDll libraw-
catDll libraw_r-
BIN_PATH="$FFMPEG_PATH"
catDll avcodec-
catDll avdevice-
catDll avfilter-
catDll avformat-
#catDll avresample- # removed from ffmpeg 4
catDll avutil-
catDll swresample-
catDll swscale-
if [ "$NATRON_LICENSE" = "GPL" ]; then
    catDll postproc-
fi

INIT_VAR=1
DLL_VAR_PREFIX="ARENA"
BIN_PATH="$SDK_HOME_BIN"
catDll libcdr-
#catDll libcroco-
catDll libicuin
catDll libpoppler-117
catDll libpoppler-glib-
catDll librevenge-0
catDll librevenge-stream-
catDll libzip
catDll libmad-0
catDll libsox-

INIT_VAR=1
DLL_VAR_PREFIX="GMIC"
BIN_PATH="$SDK_HOME_BIN"
catDll libfftw3-
catDll libfftw3_threads-
catDll libcurl-
catDll libnghttp2-
#catDll libbrotlidec
#catDll libbrotlicommon

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
