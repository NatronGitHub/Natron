#!/bin/bash
#
# Run Natron unit tests on all platforms

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

GIT_UNIT=https://github.com/MrKepzie/Natron-Tests.git


FAIL=0

source common.sh
source manageLog.sh
source manageBuildOptions.sh
source checkout-repository.sh
source gitRepositories.sh

updateBuildOptions

if [ "${TMP_PATH:-}" = "" ]; then
    echo "Failed to source common.sh, something is wrong!"
    exit 1
fi


printStatusMessage "Clone/Fetch unit tests repo ..."


TESTDIR="Natron-Tests${BITS}"
# cleanup tests (TEMPORARY) the following lines should be commented most of the time
if [ -d "$TMP_PATH/$TESTDIR" ]; then
    printStatusMessage "*** CLEAN unit tests - please disable in the script"
    rm -rf "${TMP_PATH:?}/$TESTDIR"
fi

CACHEDIR="$TMP_PATH/NatronTmpCacheDir"

checkoutRepository "$GIT_URL_NATRON_TESTS_GITHUB" "$TESTDIR" "master" "" "" "0" || FAIL=$?

if [ "$FAIL" != "0" ]; then
    printStatusMessage "Failed to clone/update unit tests repository!"
    exit 1
else
    printStatusMessage "Clone/Update unit tests ... OK!"
fi

if [ "$FAIL" = "0" ]; then
    if [ "$PKGOS" = "Windows" ] && [ "${BITS}" = "64" ]; then
        # On windows, we clean up dangling NatronRenderer processes first
        # sometimes NatronRenderer just hangs. Try taskkill first, then tskill if it fails because of a message like:
        # $ taskkill -f -im NatronRenderer-bin.exe -t
        # ERROR: The process with PID 3260 (child process of PID 3816) could not be terminated.
        # Reason: There is no running instance of the task.
        # mapfile use: see https://github.com/koalaman/shellcheck/wiki/SC2207
        mapfile -t processes < <(taskkill -f -im NatronRenderer-bin.exe -t 2>&1 |grep "ERROR: The process with PID"| awk '{print $6}' || true)
        for p in "${processes[@]}"; do
            tskill "$p" || true
        done
        mapfile -t processes < <(taskkill -f -im NatronRenderer.exe -t 2>&1 |grep "ERROR: The process with PID"| awk '{print $6}' || true)
        for p in "${processes[@]}"; do
            tskill "$p" || true
        done
    fi
    UNIT_TMP="$TMP_PATH"/unit_tmp_${BITS}
    if [ -d "$UNIT_TMP" ]; then
        rm -rf "$UNIT_TMP"
    fi
    mkdir -p "$UNIT_TMP"

    printStatusMessage "Running unit tests ..."
    pushd "$TMP_PATH/$TESTDIR"

    if [ "$PKGOS" = "Linux" ]; then
        rm -rf ~/.cache/INRIA/Natron || true
        mkdir -p ~/.cache/INRIA/Natron/{ViewerCache,DiskCache} || true
        ocio="$TMP_PORTABLE_DIR/Resources/OpenColorIO-Configs/blender/config.ocio"
        if [ ! -f "$ocio" ]; then
            printStatusMessage "*** Error: OCIO file $ocio is missing"
        fi
        bin="$TMP_PORTABLE_DIR/bin/NatronRenderer-bin"
        if [ ! -f "$bin" ]; then
            bin="$TMP_PORTABLE_DIR/bin/NatronRenderer"
            if [ ! -f "$bin" ]; then
                printStatusMessage "*** Error: NatronRenderer binary $bin is missing"
            fi
        fi
        env SRCDIR="$SRC_PATH" NATRON_CACHE_PATH="$CACHEDIR" OCIO="$ocio" FFMPEG="$TMP_PORTABLE_DIR/bin/ffmpeg" COMPARE="$TMP_PORTABLE_DIR/bin/idiff"  $TIMEOUT -s KILL 7200 bash runTests.sh "$bin" || FAIL=$?
        #FAIL=0
    elif [ "$PKGOS" = "Windows" ] && [ "${BITS}" = "64" ]; then
        cp -a "$TMP_BINARIES_PATH"/Natron-installer/packages/*/data/* "$UNIT_TMP"/
        rm -rf "$LOCALAPPDATA\\INRIA\\Natron" || true
        mkdir -p "$LOCALAPPDATA\\INRIA\\Natron\\cache\\"{ViewerCache,DiskCache} || true
        ocio="$TMP_PORTABLE_DIR/Resources/OpenColorIO-Configs/blender/config.ocio"
        if [ ! -f "$ocio" ]; then
            echo "*** Error: OCIO file $ocio is missing"
        fi
        bin="$TMP_PORTABLE_DIR/bin/NatronRenderer-bin.exe"
        if [ ! -f "$bin" ]; then
            bin="$TMP_PORTABLE_DIR/bin/NatronRenderer.exe"
            if [ ! -f "$bin" ]; then
                echo "*** Error: NatronRenderer binary $bin is missing" >> "$ULOG"
            fi
        fi
        env SRCDIR="$SRC_PATH" NATRON_CACHE_PATH="$CACHEDIR" OCIO="$ocio" FFMPEG="$TMP_PORTABLE_DIR/bin/ffmpeg.exe" COMPARE="$TMP_PORTABLE_DIR/bin/idiff.exe" $TIMEOUT -s KILL 7200 bash runTests.sh "$bin" || FAIL=$?
        # sometimes NatronRenderer just hangs. Try taskkill first, then tskill if it fails because of a message like:
        # $ taskkill -f -im NatronRenderer-bin.exe -t
        # ERROR: The process with PID 3260 (child process of PID 3816) could not be terminated.
        # Reason: There is no running instance of the task.
        # mapfile use: see https://github.com/koalaman/shellcheck/wiki/SC2207
        mapfile -t processes < <(taskkill -f -im NatronRenderer-bin.exe -t 2>&1 |grep "ERROR: The process with PID"| awk '{print $6}' || true)
        for p in "${processes[@]}"; do
            tskill "$p" || true
        done
        mapfile -t processes < <(taskkill -f -im NatronRenderer.exe -t 2>&1 |grep "ERROR: The process with PID"| awk '{print $6}' || true)
        for p in "${processes[@]}"; do
            tskill "$p" || true
        done
        #FAIL=0
    elif [ "$PKGOS" = "OSX" ]; then
        rm -rf "$HOME/Library/Caches/INRIA/Natron" || true
        mkdir -p "$HOME/Library/Caches/INRIA/Natron"/{ViewerCache,DiskCache} || true
        ocio="${TMP_PORTABLE_DIR}.app/Contents/Resources/OpenColorIO-Configs/blender/config.ocio"
        if [ ! -f "$ocio" ]; then
            echo "*** Error: OCIO file $ocio is missing"
        fi
        bin="${TMP_PORTABLE_DIR}.app/Contents/MacOS/NatronRenderer-bin"
        if [ ! -f "$bin" ]; then
            bin="${TMP_PORTABLE_DIR}.app/Contents/MacOS/NatronRenderer"
            if [ ! -f "$bin" ]; then
                echo "*** Error: NatronRenderer binary $bin is missing"
            fi
        fi
        env SRCDIR="$SRC_PATH" NATRON_CACHE_PATH="$CACHEDIR" OCIO="$ocio" FFMPEG="${TMP_PORTABLE_DIR}.app/Contents/MacOS/ffmpeg" COMPARE="${TMP_PORTABLE_DIR}.app/Contents/MacOS/idiff" $TIMEOUT -s KILL 7200 bash runTests.sh "$bin" || FAIL=$?
        #FAIL=0
    fi
    popd

    printStatusMessage "*** END unit tests -> $FAIL"

    # Save unit tests results
    pushd "$TMP_PATH/$TESTDIR"
    if [ -d "$CACHEDIR" ]; then
        rm -rf "$CACHEDIR"
    fi
    if [ -d "$UNIT_TMP" ]; then
        rm -rf "$UNIT_TMP"
    fi
    # collect result
    if [ -f "result.txt" ]; then
        echo "-----------------------------------------------------------------------"
        echo "UNIT TESTS RESULTS"
        echo "-----------------------------------------------------------------------"
        cat result.txt
        echo "-----------------------------------------------------------------------"
        printStatusMessage "Moving test results to $BUILD_ARCHIVE_DIRECTORY/${INSTALLER_BASENAME}-tests.txt"
        mv results.txt "$BUILD_ARCHIVE_DIRECTORY/${INSTALLER_BASENAME}-tests.txt"
    fi

    UNIT_TESTS_FAIL_DIR="$BUILD_ARCHIVE_DIRECTORY/unit_tests_failures"
    if [ -n "${UNIT_TESTS_FAIL_DIR:-}" ] && [ -d "failed" ] && [ "$(ls -A failed)" ]; then
        printStatusMessage "Moving test failures to $UNIT_TESTS_FAIL_DIR"
        mkdir -p "$UNIT_TESTS_FAIL_DIR"
        cd failed && mv ./* "$UNIT_TESTS_FAIL_DIR/"
    fi
    popd
fi
cd "$CWD"
exit $FAIL

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
