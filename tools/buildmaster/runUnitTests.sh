#!/bin/bash
#
# Run Natron unit tests on all platforms
# Options:
# QUEUE_ID: Must be set to identify the build number
# ENABLE_DB_LOG: If set to 1, logging will be performed in the database on the server, otherwise it's just echoed
# UDIR: If set this is the path where to store failed results of the unit tests

#set -e -u -x
FAIL=0

source `pwd`/common.sh
if [ "${TMP_PATH:-}" = "" ]; then
    echo "Failed to source common.sh, something is wrong!"
    exit 1
fi

if [ -z "$QUEUE_ID" ]; then
    echo "runUnitTests: $QUEUE_ID must be set"
    exit 1
fi

if [ "$ENABLE_DB_LOG=-}" = "1" ]; then
    updateStatusLog "$QUEUE_ID" "Clone/Fetch unit tests repo ..."
else
    echo "$QUEUE_ID" "Clone/Fetch unit tests repo ..."
fi

UNIT_TMP="$CWD"/unit_tmp_${BITS}
if [ -d "$UNIT_TMP" ]; then
    rm -rf "$UNIT_TMP"
fi
mkdir -p "$UNIT_TMP"

TESTDIR="Natron-Tests${BITS}"
# cleanup tests (TEMPORARY) the following lines should be commented most of the time
if [ -d "$CWD/$TESTDIR" ]; then
    echo "$(date '+%Y-%m-%d %H:%M:%S') *** CLEAN unit tests - please disable in the script" >> "$ULOG"
    rm -rf "${CWD:?}/$TESTDIR"
fi

if [ ! -d "$CWD/$TESTDIR" ]; then
    cd "$CWD" && $TIMEOUT 1800 git clone "$GIT_UNIT" "$TESTDIR" && cd "$TESTDIR" || FAIL=$?
else
    cd "$CWD/$TESTDIR" && $TIMEOUT 1800 git pull || FAIL=$?
fi
if [ "$FAIL" != "0" ]; then
    if [ "$ENABLE_DB_LOG=-}" = "1" ]; then
        updateStatus "$QUEUE_ID" 3
        updateStatusLog "$QUEUE_ID" "Failed to clone/update unit tests repository!"
    else
        echo "$QUEUE_ID" "Failed to clone/fetch unit tests repository!"
    fi
else
    if [ "$ENABLE_DB_LOG=-}" = "1" ]; then
        updateStatusLog "$QUEUE_ID" "Clone/Update unit tests ... OK!"
    else
        echo "$QUEUE_ID" "Clone/fetch unit tests ... OK!"
    fi
fi
if [ "$FAIL" = "0" ]; then
    if [ "$ENABLE_DB_LOG=-}" = "1" ]; then
        updateStatusLog "$QUEUE_ID" "Running unit tests ..."
    else
        echo "$QUEUE_ID" "Running unit tests ..."
    fi
    if [ "$PKGOS" = "Linux" ]; then
        if  [ "${BITS}" = "64" ]; then
            l=64
        else
            l=""
        fi
        export FONTCONFIG_PATH="${SDK_HOME}/etc/fonts/fonts.conf"
        export LD_LIBRARY_PATH="${SDK_HOME}/gcc/lib${l}:${SDK_HOME}/lib:${FFMPEG_PATH}/lib:${LIBRAW_PATH}/lib"
        export PATH="${SDK_HOME}/bin:$PATH"
	if [ -d "${HOME}/.cache/INRIA/Natron" ]; then
	    rm -rf "${HOME}/.cache/INRIA/Natron"
	fi
        mkdir -p ~/.cache/INRIA/Natron/{ViewerCache,DiskCache}
        echo "$(date '+%Y-%m-%d %H:%M:%S') *** START unit tests" >> "$ULOG"
        env NATRON_PLUGIN_PATH="$TMP_BINARIES_PATH/PyPlugs" OFX_PLUGIN_PATH="$TMP_BINARIES_PATH/OFX/Plugins" OCIO="$TMP_BINARIES_PATH/Resources/OpenColorIO-Configs/blender/config.ocio" FFMPEG="$FFMPEG_PATH/bin/ffmpeg" COMPARE="$SDK_HOME/bin/idiff"  $TIMEOUT -s KILL 7200 bash runTests.sh "$TMP_BINARIES_PATH/bin/NatronRenderer" >> "$ULOG" 2>&1 || FAIL=$?
        echo "$(date '+%Y-%m-%d %H:%M:%S') *** END unit tests -> $FAIL" >> "$ULOG"
        FAIL=0
    elif [ "$PKGOS" = "Windows" ] && [ "${BITS}" = "64" ]; then
        cp -a "$TMP_PATH"/Natron-installer/packages/*/data/* "$UNIT_TMP"/
        export FONTCONFIG_PATH="$UNIT_TMP/Resources/etc/fonts/fonts.conf"
        echo "$(date '+%Y-%m-%d %H:%M:%S') *** START unit tests" >> "$ULOG"
        # idiff needs libraw dll which is not in the same dir as other dlls
        if [ ! -f "${TMP_BINARIES_PATH}/bin/libraw_r-16.dll" ]; then
            cp "${LIBRAW_PATH}/bin/libraw_r-16.dll" "${TMP_BINARIES_PATH}/bin/"
        fi
	
        env PATH="${TMP_BINARIES_PATH}/bin:$PATH:${FFMPEG_PATH}/bin:${LIBRAW_PATH}/bin" NATRON_PLUGIN_PATH="$TMP_BINARIES_PATH/PyPlugs" OFX_PLUGIN_PATH="$TMP_BINARIES_PATH/OFX/Plugins" OCIO="$TMP_BINARIES_PATH/Resources/OpenColorIO-Configs/blender/config.ocio" FFMPEG="$FFMPEG_PATH/bin/ffmpeg.exe" COMPARE="${SDK_HOME}/bin/idiff.exe" $TIMEOUT -s KILL 7200 bash runTests.sh "$UNIT_TMP/bin/NatronRenderer.exe" >> "$ULOG" 2>&1 || FAIL=$?
        echo "$(date '+%Y-%m-%d %H:%M:%S') *** END unit tests -> $FAIL" >> "$ULOG"
        FAIL=0
    elif [ "$PKGOS" = "OSX" ]; then
        if [ "$MACOSX_DEPLOYMENT_TARGET" = "10.6" ] && [ "$COMPILER" = "gcc" ]; then
            export DYLD_LIBRARY_PATH="$MACPORTS"/lib/libgcc:/System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/ImageIO.framework/Versions/A/Resources:"$MACPORTS"/lib
        fi
        echo "$(date '+%Y-%m-%d %H:%M:%S') *** START unit tests" >> "$ULOG"
        env NATRON_PLUGIN_PATH="$TMP_BINARIES_PATH/Natron.app/Contents/Plugins/PyPlugs" OFX_PLUGIN_PATH="$TMP_BINARIES_PATH/Natron.app/Contents/Plugins/OFX" OCIO="$TMP_BINARIES_PATH/Natron.app/Contents/Resources/OpenColorIO-Configs/blender/config.ocio" FFMPEG="$FFMPEG_PATH/bin/ffmpeg" COMPARE="idiff" $TIMEOUT -s KILL 7200 bash runTests.sh "$TMP_BINARIES_PATH/Natron.app/Contents/MacOS/NatronRenderer-bin" >> "$ULOG" 2>&1 || FAIL=$?
        echo "$(date '+%Y-%m-%d %H:%M:%S') *** END unit tests -> $FAIL" >> "$ULOG"
        FAIL=0
    fi
    rm -rf "$UNIT_TMP"
    # collect result
    if [ -f "result.txt" ]; then
        if [ "$ENABLE_DB_LOG=-}" = "1" ]; then
            mv result.txt "$RULOG"
        else
            echo "-----------------------------------------------------------------------"
            echo "UNIT TESTS RESULTS"
            echo "-----------------------------------------------------------------------"
            cat result.txt
            echo "-----------------------------------------------------------------------"
        fi
    fi
    if [ -d "failed" ]; then
        if [ ! -z "$UDIR:-}" ]; then
            mv failed "$UDIR"
        fi
    fi
fi
cd "$CWD"
exit $FAIL
