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
catDll libcelt0-
catDll libcroco-
catDll LIBEAY32
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
catDll libgnutls-
catDll libgobject-
catDll libgomp-
catDll libgraphite
catDll libgsm
catDll libHalf-
catDll libharfbuzz-0
catDll libhogweed-
catDll libiconv-
catDll libicudt
catDll libicuuc
catDll libidn2-
catDll libIex-
catDll libIlmImf-
catDll libIlmThread-
catDll libImath-
catDll libintl-
catDll libjasper-
catDll libjpeg-
catDll liblcms2-
catDll liblzma-
catDll libmfx
catDll libmng-
catDll libmodplug-
catDll libmp3lame-
catDll libnettle-
catDll libnghttp2-
catDll libogg-
catDll libopenal-
catDll libOpenColorIO
catDll libOpenImageIO
catDll libopenjp2-
catDll libopus-
catDll libp11-kit-
catDll libpango-
catDll libpangocairo-
catDll libpangoft2-
catDll libpangowin32-
catDll libpcre-
catDll libpixman-
catDll libpng16-
catDll libPtex
catDll libpyside-python${PYVER}
catDll libpython${PYVER}
catDll librsvg-
catDll librtmp-
catDll libshiboken-python${PYVER}
catDll libspeex-
catDll libsqlite3-
catDll libsrt
catDll libssh2-
catDll libstdc++-
catDll libtasn1-
catDll libtheoradec-
catDll libtheoraenc-
catDll libtiff-
catDll libtinyxml-
catDll libunistring-
catDll libvorbis-
catDll libvorbisenc-
catDll libvpx-
catDll libwavpack-
catDll libwebp-
catDll libwebpmux-
catDll libwinpthread-
catDll libxml2-
catDll phonon4
#catDll SDL2
catDll SSLEAY32
catDll zlib1

catDll libSeExpr
catDll yaml-cpp
catDll libopenh264
catDll libthai
catDll libdatrie-
catDll libsnappy
catDll libpugixml
catDll libheif-
catDll libde265-
catDll libx265

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
catDll libcroco-
catDll libicuin
catDll libpoppler-9
catDll libpoppler-glib-
catDll librevenge-0
catDll librevenge-stream-
catDll libzip

INIT_VAR=1
DLL_VAR_PREFIX="GMIC"
BIN_PATH="$SDK_HOME_BIN"
catDll libfftw3-
catDll libcurl-
catDll libnghttp2-
catDll libbrotlidec
catDll libbrotlicommon

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
