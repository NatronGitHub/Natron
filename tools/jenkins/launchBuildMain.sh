#!/bin/bash
#
# Main script to launch a Natron or plug-in repository build.
#
set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
set -x # Print commands and their arguments as they are executed.


# Configurable Options:
# -------------------
# Depending in the options, the script can have different build types: build a Natron release, snapshot,
# just CI or a specific plug-in repository CI.
# A snapshot differs from CI because the resulting build is uploaded as well as its debug symbols.
# A snapshot is stamped with a specific development tag so that it doesn't get mixed up with releases.
#
# To launch a release, you must set RELEASE_TAG and point GIT_URL to the Natron repo to build from.
# To launch a snapshot, you must set SNAPSHOT_BRANCH and SNAPSHOT_COMMIT and point GIT_URL to the Natron repo to build from.
# To launch a CI (Natron or plug-in repo) you must set GIT_URL, GIT_BRANCH, GIT_COMMIT. GIT_URL may point to either a Natron repository or plug-in repository. The GIT_URL must be known by the script. List of repositories is maintained in gitRepositories.sh
#
# RELEASE_TAG: string "x.y.z" indicating the tag name of the repository to use. Do not add the prefix "tags/", it will be added automatically. Only indicate this to trigger a release.
# SNAPSHOT_BRANCH: If set and RELEASE_TAG is not set, this describes a specific branch to build as a snapshot
# SNAPSHOT_COMMIT: The commit to build for a snapshot on the given SNAPSHOT_BRANCH. If not set, the latest commit of the branch will be built
# UNIT_TESTS: "false" or "true" (for compat with jenkins) to run the full unit tests suite after the build
# NATRON_LICENSE: (GPL or COMMERCIAL) The license to build Natron with. If GPL then GPL components will be included
# NATRON_DEV_STATUS: ALPHA, BETA, RC, STABLE, CUSTOM: this is useful when doing a release (i.e: if specifying RELEASE_TAG.)
# NATRON_BUILD_NUMBER: When doing a release this is the number of the release (if doing a rebuild)
# NATRON_CUSTOM_BUILD_USER_NAME: For a release, Tag the build with a specific user/client name
# COMPILE_TYPE: The type of build to do, i.e in terms of compilation. Valid values are (relwithdebinfo, release, debug)
# NATRON_EXTRA_QMAKE_FLAGS: Optional qmake flags to pass when building Natron
# BITS: Windows only: Must indicate if this is a 64 or 32 bits build
# DEBUG_SCRIPTS: If set to 1, binaries from previous build with same options are not cleaned so that the scripts can continue the same compilation
# DISABLE_BREAKPAD: If set to 1, natron will be built without crash reporting capabilities
# DISABLE_RPM_DEB_PKGS: If set to 1, deb and rpm packages will not be built. Otherwise they are only built for a release
# DISABLE_PORTABLE_ARCHIVE: If set to 1, a portable archive will not be built
# BUILD_NAME: Give a name to the build so that it can be found in the archive
# EXTRA_PYTHON_MODULES_SCRIPT: Path to a python script that should install extra modules with pip or easy_install.
# The options above are set by custom parameters in the build configuration
# https://wiki.jenkins-ci.org/display/JENKINS/Git+Plugin#GitPlugin-Jenkins,GITpluginandWindows


# GIT_URL, GIT_BRANCH, GIT_COMMIT are set from the git jenkins plug-in automatically
# BUILD_NUMBER is also given by jenkins, see https://wiki.jenkins-ci.org/display/JENKINS/Building+a+software+project#Buildingasoftwareproject-JenkinsSetEnvironmentVariables
# The above options must be set if calling this script manually. If GIT_COMMIT is omitted, the latest commit of GIT_BRANCH is actually built
echo "-----------------------------------------------------------------------"
echo "NATRON JENKINS BUILDMASTER"
echo "-----------------------------------------------------------------------"
echo "BUILD_NUMBER                      : \"${BUILD_NUMBER:-}\""
echo "BUILD_NAME                        : \"${BUILD_NAME:-}\""
echo "WORKSPACE                         : \"${WORKSPACE:-}\""
echo "GIT_URL                           : \"${GIT_URL:-}\""
echo "GIT_BRANCH                        : \"${GIT_BRANCH:-}\""
echo "GIT_COMMIT                        : \"${GIT_COMMIT:-}\""
echo "RELEASE_TAG                       : \"${RELEASE_TAG:-}\""
echo "SNAPSHOT_COMMIT                   : \"${SNAPSHOT_COMMIT:-}\""
echo "SNAPSHOT_BRANCH                   : \"${SNAPSHOT_BRANCH:-}\""
echo "UNIT_TESTS                        : \"${UNIT_TESTS:-}\""
echo "NATRON_LICENSE                    : \"${NATRON_LICENSE:-}\""
echo "NATRON_DEV_STATUS                 : \"${NATRON_DEV_STATUS:-}\""
echo "NATRON_BUILD_NUMBER               : \"${NATRON_BUILD_NUMBER:-}\""
echo "NATRON_CUSTOM_BUILD_USER_NAME     : \"${NATRON_CUSTOM_BUILD_USER_NAME:-}\""
echo "COMPILE_TYPE                      : \"${COMPILE_TYPE:-}\""
echo "NATRON_EXTRA_QMAKE_FLAGS          : \"${NATRON_EXTRA_QMAKE_FLAGS:-}\""
echo "DISABLE_RPM_DEB_PKGS              : \"${DISABLE_RPM_DEB_PKGS:-}\""
echo "DISABLE_PORTABLE_ARCHIVE          : \"${DISABLE_PORTABLE_ARCHIVE:-}\""
echo "EXTRA_PYTHON_MODULES_SCRIPT       : \"${EXTRA_PYTHON_MODULES_SCRIPT:-}\""
echo "REMOTE_URL                        : \"${REMOTE_URL}\""
echo "REMOTE_USER                       : \"${REMOTE_USER}\""
echo "REMOTE_PREFIX                     : \"${REMOTE_PREFIX}\""
echo "-----------------------------------------------------------------------"

if [ "${NATRON_LICENSE:-}" != "GPL" ] && [ "${NATRON_LICENSE:-}" != "COMMERCIAL" ]; then
    echo "Please select a License with NATRON_LICENSE=(GPL,COMMERCIAL)"
    exit 1
fi

if [ -z "${BUILD_NAME:-}" ] || [ -z "${BUILD_NUMBER:-}" ]; then
    echo "Must set BUILD_NAME and BUILD_NUMBER"
    exit 1
fi

if [ -z "${GIT_COMMIT:-}" ]; then
    GIT_COMMIT=""
fi

source common.sh
source manageBuildOptions.sh
source manageLog.sh

# Source known git repositories
source gitRepositories.sh

# refresh credentials
keychainstatus=0
eval `keychain -q --eval --agents ssh id_rsa_build_master` || keychainstatus=1
if [ $keychainstatus != 0 ]; then
    echo "Restarting ssh-agent"
    keychain -k
    eval `keychain --eval --agents ssh id_rsa_build_master`
fi

GIT_BRANCH=$(echo $GIT_BRANCH | sed 's#origin/##')
BUILD_NATRON=0
DO_UTEST=0

# Defaults to relwithdebinfo if not set
if [ -z "${COMPILE_TYPE:-}" ]; then
    echo "COMPILE_TYPE not specified, default to relwithdebinfo"
    COMPILE_TYPE="relwithdebinfo"
fi


# By default do not debug scripts!
if [ "${DEBUG_SCRIPTS:-}" != "1" ]; then
    DEBUG_SCRIPTS=0
else
    echo "DEBUG_SCRIPTS specified: picking up any existing previous compilation"
fi

# By default breakpad is enabled
if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    DISABLE_BREAKPAD=0
else
    echo "DISABLE_BREAKPAD specified: crash reporting support disabled"
fi

# Remove any previous jenkins artifacts from previous builds
if [ ! -z "${WORKSPACE:-}" ]; then
    if [ -d "${WORKSPACE}/jenkins_artifacts" ]; then
        rm -rf "${WORKSPACE}/jenkins_artifacts" || true
    fi
fi

# Do we need to build Natron ? No if the script was not invoked from a Natron project
IS_GIT_URL_NATRON_REPO=0
isNatronRepo "$GIT_URL" || IS_GIT_URL_NATRON_REPO=1


if [ "$IS_GIT_URL_NATRON_REPO" = "1" ]; then
    # Set the git urls from the Natron url in gitRepositories.sh
    setGitUrlsForNatronUrl "$GIT_URL"
else
    # This is a plug-in CI, set the other repositories url by default
    setGitUrlsForPluginCI
fi

# Ensure the git urls are set
if [ -z "$GIT_URL_NATRON" ] || [ -z "$GIT_URL_OPENFX_IO" ] || [ -z "$GIT_URL_OPENFX_MISC" ] || [ -z "$GIT_URL_OPENFX_ARENA" ] || [ -z "$GIT_URL_OPENFX_GMIC" ] || [ -z "$GIT_URL_OPENFX_OPENCV" ]; then
    echo "Git URLs not set, please check gitRepositories.sh"
    exit 1
fi

# Setup tmp
if [ ! -d "$TMP_PATH" ]; then
    mkdir -p "$TMP_PATH"
else
# Clean-up any build file
if [ "${DEBUG_SCRIPTS:-}" != "1" ]; then
    rm -rf "${TMP_PATH:?}"/* || true
fi
fi

# Create the build options file, to avoid passing dozens of options between scripts.
# This file is then sourced in each sub-script.
if [ -f "$BUILD_OPTIONS_FILE" ]; then
    rm "$BUILD_OPTIONS_FILE"
fi
ensureBuildOptionFileExists

setBuildOption "PKGOS" "$PKGOS"
setBuildOption "BITS" "$BITS"
setBuildOption "TMP_PATH" "$TMP_PATH"
setBuildOption "TMP_BINARIES_PATH" "${TMP_PATH}/tmp_deploy"
setBuildOption "REMOTE_URL" "${REMOTE_URL:-}"
setBuildOption "REMOTE_USER" "${REMOTE_USER:-}"
setBuildOption "REMOTE_PREFIX" "${REMOTE_PREFIX:-}"

setBuildOption "GIT_URL_NATRON" "$GIT_URL_NATRON"
setBuildOption "GIT_URL_OPENFX_IO" "$GIT_URL_OPENFX_IO"
setBuildOption "GIT_URL_OPENFX_MISC" "$GIT_URL_OPENFX_MISC"
setBuildOption "GIT_URL_OPENFX_ARENA" "$GIT_URL_OPENFX_ARENA"
setBuildOption "GIT_URL_OPENFX_GMIC" "$GIT_URL_OPENFX_GMIC"
setBuildOption "GIT_URL_OPENFX_OPENCV" "$GIT_URL_OPENFX_OPENCV"

BUILDS_ARCHIVE_PATH="$CWD/builds_archive"

BUILD_ARCHIVE_DIRECTORY="$BUILDS_ARCHIVE_PATH/$BUILD_NAME/$BUILD_NUMBER"

if [ ! -d "$BUILD_ARCHIVE_DIRECTORY" ]; then
    mkdir -p "$BUILD_ARCHIVE_DIRECTORY"
fi

setBuildOption "BUILD_NAME" "${BUILD_NAME:-}"
setBuildOption "EXTRA_PYTHON_MODULES_SCRIPT" "${EXTRA_PYTHON_MODULES_SCRIPT:-}"
setBuildOption "CURRENT_DATE" "$CURRENT_DATE"
setBuildOption "DEBUG_SCRIPTS" "$DEBUG_SCRIPTS"
setBuildOption "DISABLE_BREAKPAD" "${DISABLE_BREAKPAD:-}"
setBuildOption "NATRON_LICENSE" "$NATRON_LICENSE"
setBuildOption "BUILD_ARCHIVE_DIRECTORY" "$BUILD_ARCHIVE_DIRECTORY"

# Determine Natron build type
NATRON_BUILD_CONFIG=""
if [ "$IS_GIT_URL_NATRON_REPO" != "1" ]; then
    TYPE="PLUGIN_CI"
else
    if [ ! -z "${RELEASE_TAG:-}" ]; then
        TYPE="RELEASE"
        if [ "$NATRON_DEV_STATUS" != "ALPHA" ] && [ "$NATRON_DEV_STATUS" != "BETA" ] && [ "$NATRON_DEV_STATUS" != "RC" ] && [ "$NATRON_DEV_STATUS" != "CUSTOM" ] && [ "$NATRON_DEV_STATUS" != "STABLE" ]; then
            echo "Invalid NATRON_DEV_STATUS=$NATRON_DEV_STATUS, it must be either [ALPHA, BETA, RC, CUSTOM, STABLE]"
            exit 1
        fi
        NATRON_BUILD_CONFIG="$NATRON_DEV_STATUS"
        setBuildOption "NATRON_BUILD_CONFIG" "$NATRON_DEV_STATUS"
        setBuildOption "NATRON_BUILD_NUMBER" "$NATRON_BUILD_NUMBER"
        setBuildOption "NATRON_CUSTOM_BUILD_USER_NAME" "${NATRON_CUSTOM_BUILD_USER_NAME:-}"
    elif [ ! -z "${SNAPSHOT_COMMIT:-}" ] || [ ! -z "${SNAPSHOT_BRANCH:-}" ]; then
        TYPE="SNAPSHOT"
        NATRON_BUILD_CONFIG="SNAPSHOT"
        setBuildOption "NATRON_BUILD_CONFIG" "SNAPSHOT"
    else
        TYPE="NATRON_CI"
        NATRON_BUILD_CONFIG="SNAPSHOT"
        setBuildOption "NATRON_BUILD_CONFIG" "SNAPSHOT"
    fi
fi

# Can be set to relwithdebinfo, release, debug
setBuildOption "COMPILE_TYPE" "$COMPILE_TYPE"
setBuildOption "NATRON_EXTRA_QMAKE_FLAGS" "${NATRON_EXTRA_QMAKE_FLAGS:-}"

setBuildOption "BUILD_TYPE" "$TYPE"

# Set the url for Natron to GIT_URL if this is the build that triggered it, otherwise use Github by default
if [ "$IS_GIT_URL_NATRON_REPO" = "1" ]; then
    setBuildOption "NATRON_GIT_URL" "$GIT_URL"
else
    setBuildOption "NATRON_GIT_URL" "$GIT_NATRON"
fi


if [ "${UNIT_TESTS:-}" = "true" ]; then
    DO_UTEST=1
fi

if [ -z "${DISABLE_PORTABLE_ARCHIVE:-}" ]; then
    echo "DISABLE_PORTABLE_ARCHIVE not specified, portable archive will be built"
    DISABLE_PORTABLE_ARCHIVE=0
fi
setBuildOption "DISABLE_PORTABLE_ARCHIVE" "$DISABLE_PORTABLE_ARCHIVE"



# Determine commit and branch depending on the build type
if [ "$TYPE" = "RELEASE" ]; then
    # Use the tag as a branch, its commit is already the latest commit on the tag


    if [ -z "${DISABLE_RPM_DEB_PKGS:-}" ]; then
        if [ "$PKGOS" = "Linux" ]; then
            echo "DISABLE_RPM_DEB_PKGS not specified, .rpm and .deb packages will be built"
        fi
        DISABLE_RPM_DEB_PKGS=0
    fi
    setBuildOption "DISABLE_RPM_DEB_PKGS" "$DISABLE_RPM_DEB_PKGS"
    GIT_COMMIT=""
    GIT_BRANCH="tags/${RELEASE_TAG:-}"
    setBuildOption "NATRON_GIT_BRANCH" "$GIT_BRANCH"
    setBuildOption "NATRON_GIT_COMMIT" ""
    setBuildOption "OPENFX_IO_GIT_BRANCH" "tags/Natron-$RELEASE_TAG"
    setBuildOption "OPENFX_IO_GIT_COMMIT" ""
    setBuildOption "OPENFX_MISC_GIT_BRANCH" "tags/Natron-$RELEASE_TAG"
    setBuildOption "OPENFX_MISC_GIT_COMMIT" ""
    setBuildOption "OPENFX_ARENA_GIT_BRANCH" "tags/Natron-$RELEASE_TAG"
    setBuildOption "OPENFX_ARENA_GIT_COMMIT" ""
    setBuildOption "OPENFX_GMIC_GIT_BRANCH" "tags/Natron-$RELEASE_TAG"
    setBuildOption "OPENFX_GMIC_GIT_COMMIT" ""
    setBuildOption "OPENFX_OPENCV_GIT_BRANCH" "tags/Natron-$RELEASE_TAG"
    setBuildOption "OPENFX_OPENCV_GIT_COMMIT" ""
elif [ "$TYPE" = "SNAPSHOT" ] || [ "$TYPE" = "NATRON_CI" ]; then
    # Use the user defined variables
    if [ "$TYPE" = "SNAPSHOT" ]; then
        GIT_COMMIT=${SNAPSHOT_COMMIT:-}
        GIT_BRANCH=${SNAPSHOT_BRANCH:-}
    fi
    setBuildOption "NATRON_GIT_BRANCH" "$GIT_BRANCH"
    setBuildOption "NATRON_GIT_COMMIT" "$GIT_COMMIT"

# Plugin branches will be set after checkout of Natron
elif [ "$TYPE" = "PLUGIN_CI" ]; then
    setBuildOption "NATRON_GIT_BRANCH" ""
    setBuildOption "NATRON_GIT_COMMIT" ""
    setBuildOption "OPENFX_IO_GIT_BRANCH" ""
    setBuildOption "OPENFX_IO_GIT_COMMIT" ""
    setBuildOption "OPENFX_MISC_GIT_BRANCH" ""
    setBuildOption "OPENFX_MISC_GIT_COMMIT" ""
    setBuildOption "OPENFX_ARENA_GIT_BRANCH" ""
    setBuildOption "OPENFX_ARENA_GIT_COMMIT" ""
    setBuildOption "OPENFX_GMIC_GIT_BRANCH" ""
    setBuildOption "OPENFX_GMIC_GIT_COMMIT" ""
    setBuildOption "OPENFX_OPENCV_GIT_BRANCH" ""
    setBuildOption "OPENFX_OPENCV_GIT_COMMIT" ""
    if [ "$GIT_URL" = "$GIT_URL_OPENFX_IO" ]; then
        setBuildOption "OPENFX_IO_GIT_BRANCH" "$GIT_BRANCH"
        setBuildOption "OPENFX_IO_GIT_COMMIT" "$GIT_COMMIT"
    elif [ "$GIT_URL" = "$GIT_URL_OPENFX_MISC" ]; then
        setBuildOption "OPENFX_MISC_GIT_BRANCH" "$GIT_BRANCH"
        setBuildOption "OPENFX_MISC_GIT_COMMIT" "$GIT_COMMIT"
    elif [ "$GIT_URL" = "$GIT_URL_OPENFX_ARENA" ]; then
        setBuildOption "OPENFX_ARENA_GIT_BRANCH" "$GIT_BRANCH"
        setBuildOption "OPENFX_ARENA_GIT_COMMIT" "$GIT_COMMIT"
    elif [ "$GIT_URL" = "$GIT_URL_OPENFX_GMIC" ]; then
        setBuildOption "OPENFX_GMIC_GIT_BRANCH" "$GIT_BRANCH"
        setBuildOption "OPENFX_GMIC_GIT_COMMIT" "$GIT_COMMIT"
    elif [ "$GIT_URL" = "$GIT_URL_OPENFX_OPENCV" ]; then
        setBuildOption "OPENFX_OPENCV_GIT_BRANCH" "$GIT_BRANCH"
        setBuildOption "OPENFX_OPENCV_GIT_COMMIT" "$GIT_COMMIT"
    else
        echo "Unknown plug-in repository URL $GIT_URL"
        exit 1
    fi

fi
# For a CI build, GIT_COMMIT and GIT_BRANCH are already set


setBuildOption "GIT_TRIGGER_URL" "$GIT_URL"
setBuildOption "GIT_TRIGGER_BRANCH" "$GIT_BRANCH"
setBuildOption "GIT_TRIGGER_COMMIT" "$GIT_COMMIT"


# Only use online installer for snapshots or releases
if [ "$TYPE" = "SNAPSHOT" ] || [ "$TYPE" = "RELEASE" ]; then
    WITH_ONLINE_INSTALLER=1
else
    WITH_ONLINE_INSTALLER=0
fi
setBuildOption "WITH_ONLINE_INSTALLER" "$WITH_ONLINE_INSTALLER"


# If 1, source files will be compressed and uploaded with the binary
TAR_SOURCES=0

#
# build
#
FAIL=0

source checkout-repository.sh

# Checkout Natron git
if [ "$IS_GIT_URL_NATRON_REPO" = "1" ]; then
    checkoutRepository "$GIT_URL_NATRON" "Natron" "$NATRON_GIT_BRANCH" "$NATRON_GIT_COMMIT" "NATRON_GIT_COMMIT" "$TAR_SOURCES" || FAIL=$?

    if [ "$FAIL" != "1" ]; then
    NATRON_MAJOR=$(echo "NATRON_VERSION_MAJOR" | $CC -E -P -include "$TMP_PATH/Natron/Global/Macros.h" - | tail -1)
    NATRON_MINOR=$(echo "NATRON_VERSION_MINOR" | $CC -E -P -include "$TMP_PATH/Natron/Global/Macros.h" - | tail -1)
    NATRON_REVISION=$(echo "NATRON_VERSION_REVISION" | $CC -E -P -include "$TMP_PATH/Natron/Global/Macros.h" - | tail -1)

    setBuildOption "NATRON_VERSION_MAJOR" "$NATRON_MAJOR"
    setBuildOption "NATRON_VERSION_MINOR" "$NATRON_MINOR"
    setBuildOption "NATRON_VERSION_REVISION" "$NATRON_REVISION"

    setBuildOption "NATRON_VERSION_SHORT" "${NATRON_MAJOR}.${NATRON_MINOR}"
    setBuildOption "NATRON_VERSION_FULL" "${NATRON_MAJOR}.${NATRON_MINOR}.${NATRON_REVISION}"


    # add breakpad
    cd "$TMP_PATH/Natron"
    rm -rf CrashReporter* BreakpadClient google-breakpad || true
    cp -a "$CWD/../Breakpad/CrashReporter"* "$CWD/../Breakpad/BreakpadClient" "$CWD/../Breakpad/google-breakpad" "$CWD/../Breakpad/breakpadclient.pri" "$CWD/../Breakpad/breakpadpro.pri" .
    fi
    cd "$CWD"
fi


# Determine plug-in branches to build if the Natron build is not a release (we don't have a tag)
if [ "$TYPE" = "SNAPSHOT" ] || [ "$TYPE" = "NATRON_CI" ]; then
    IO_BRANCH="master"
    MISC_BRANCH="master"
    ARENA_BRANCH="master"
    GMIC_BRANCH="master"
    CV_BRANCH="master"
    # If the already checkout-out Natron branch overrides it, do it now
    if [ -f "$TMP_PATH/Natron/Global/plugin-branches.sh" ]; then
    . "$TMP_PATH/Natron/Global/plugin-branches.sh"
    fi


    setBuildOption "OPENFX_IO_GIT_BRANCH" "$IO_BRANCH"
    setBuildOption "OPENFX_IO_GIT_COMMIT" ""
    setBuildOption "OPENFX_MISC_GIT_BRANCH" "$MISC_BRANCH"
    setBuildOption "OPENFX_MISC_GIT_COMMIT" ""
    setBuildOption "OPENFX_ARENA_GIT_BRANCH" "$ARENA_BRANCH"
    setBuildOption "OPENFX_ARENA_GIT_COMMIT" ""
    setBuildOption "OPENFX_GMIC_GIT_BRANCH" "$GMIC_BRANCH"
    setBuildOption "OPENFX_GMIC_GIT_COMMIT" ""
    setBuildOption "OPENFX_OPENCV_GIT_BRANCH" "$CV_BRANCH"
    setBuildOption "OPENFX_OPENCV_GIT_COMMIT" ""
fi

updateBuildOptions

NATRON_VERSION_STRING=""
if [ "$NATRON_BUILD_CONFIG" = "ALPHA" ]; then
    NATRON_VERSION_STRING=$NATRON_VERSION_FULL-alpha-$NATRON_BUILD_NUMBER
elif [ "$NATRON_BUILD_CONFIG" = "BETA" ]; then
    NATRON_VERSION_STRING=$NATRON_VERSION_FULL-beta-$NATRON_BUILD_NUMBER
elif [ "$NATRON_BUILD_CONFIG" = "RC" ]; then
    NATRON_VERSION_STRING=$NATRON_VERSION_FULL-RC$NATRON_BUILD_NUMBER
elif [ "$NATRON_BUILD_CONFIG" = "STABLE" ]; then
    NATRON_VERSION_STRING=$NATRON_VERSION_NUMBER
elif [ "$NATRON_BUILD_CONFIG" = "CUSTOM" ]; then
    NATRON_VERSION_STRING="$NATRON_CUSTOM_BUILD_USER_NAME"
elif [ "$NATRON_BUILD_CONFIG" = "SNAPSHOT" ]; then
    NATRON_VERSION_STRING="${NATRON_GIT_COMMIT:0:7}"
fi

setBuildOption "NATRON_VERSION_STRING" "$NATRON_VERSION_STRING"


# Name of the installer directory, common to all platforms
INSTALLER_BASENAME="Natron"
if [ "$NATRON_BUILD_CONFIG" = "SNAPSHOT" ]; then
    INSTALLER_BASENAME="${INSTALLER_BASENAME}-${NATRON_GIT_BRANCH}-${CURRENT_DATE}"
fi

INSTALLER_BASENAME="${INSTALLER_BASENAME}-${NATRON_VERSION_STRING}-${PKGOS_BITS}"

if [ "$COMPILE_TYPE" = "debug" ]; then
    INSTALLER_BASENAME="${INSTALLER_BASENAME}-debug"
fi
PORTABLE_DIRNAME="${INSTALLER_BASENAME}-no-installer"
setBuildOption "INSTALLER_BASENAME" "$INSTALLER_BASENAME"
setBuildOption "PORTABLE_DIRNAME" "$PORTABLE_DIRNAME"
setBuildOption "TMP_PORTABLE_DIR" "${TMP_BINARIES_PATH}/$PORTABLE_DIRNAME"

updateBuildOptions

# Checkout plug-ins git
if [ ! -z "$OPENFX_IO_GIT_BRANCH" ] && [ "$FAIL" != "1" ]; then
    checkoutRepository "$GIT_URL_OPENFX_IO" "openfx-io" "$OPENFX_IO_GIT_BRANCH" "$OPENFX_IO_GIT_COMMIT" "OPENFX_IO_GIT_BRANCH" "$TAR_SOURCES" || FAIL=$?
fi

if [ ! -z "$OPENFX_MISC_GIT_BRANCH" ] && [ "$FAIL" != "1" ]; then
    checkoutRepository "$GIT_URL_OPENFX_MISC" "openfx-misc" "$OPENFX_MISC_GIT_BRANCH" "$OPENFX_MISC_GIT_COMMIT" "OPENFX_MISC_GIT_BRANCH" "$TAR_SOURCES" || FAIL=$?
fi

if [ ! -z "$OPENFX_ARENA_GIT_BRANCH" ] && [ "$FAIL" != "1" ]; then
    checkoutRepository "$GIT_URL_OPENFX_ARENA" "openfx-arena" "$OPENFX_ARENA_GIT_BRANCH" "$OPENFX_ARENA_GIT_COMMIT" "OPENFX_ARENA_GIT_BRANCH" "$TAR_SOURCES" || FAIL=$?
fi

if [ ! -z "$OPENFX_GMIC_GIT_BRANCH" ] && [ "$FAIL" != "1" ]; then
    checkoutRepository "$GIT_URL_OPENFX_GMIC" "openfx-gmic" "$OPENFX_GMIC_GIT_BRANCH" "$OPENFX_GMIC_GIT_COMMIT" "OPENFX_GMIC_GIT_BRANCH" "$TAR_SOURCES" || FAIL=$?
fi

# Force deactivate of openfx-opencv for now.
OPENFX_OPENCV_GIT_BRANCH=""
if [ ! -z "$OPENFX_OPENCV_GIT_BRANCH" ] && [ "$FAIL" != "1" ]; then
    checkoutRepository "$GIT_URL_OPENFX_OPENCV" "openfx-opencv" "$OPENFX_OPENCV_GIT_BRANCH" "$OPENFX_OPENCV_GIT_COMMIT" "OPENFX_OPENCV_GIT_BRANCH" "$TAR_SOURCES" || FAIL=$?
fi

dumpBuildOptions


# Build plug-ins
if [ "$FAIL" = "0" ]; then
    bash "build-plugins.sh" || FAIL=$?
fi

# Build Natron
if [ "$FAIL" = "0" ] && [ "$IS_GIT_URL_NATRON_REPO" = "1" ]; then
    bash "build-natron.sh" || FAIL=$?
fi

# build installer(s)
#
if [ "$FAIL" = "0" ] && [ "$IS_GIT_URL_NATRON_REPO" = "1" ]; then
    printStatusMessage "Creating installer..."
    bash "build-${PKGOS}-installer.sh" || FAIL=$?
fi


UNIT_TESTS_FAIL=0
if [ "$FAIL" = "0" ]; then
    echo "Build finished! Files are located in $BUILD_ARCHIVE_DIRECTORY"
    # unit tests
    if [ "$IS_GIT_URL_NATRON_REPO" = "1" ] && [ "$DO_UTEST" = "1" ]; then

        env bash runUnitTests.sh || UNIT_TESTS_FAIL=$?
        if [ "$UNIT_TESTS_FAIL" = "0" ]; then
            echo "Unit tests: OK"
        else
            echo "Unit tests: failed ($UNIT_TESTS_FAIL)"
       fi
    fi
fi

if [ -f "$BUILD_OPTIONS_FILE" ]; then
    cp "$BUILD_OPTIONS_FILE" "$BUILD_ARCHIVE_DIRECTORY/"
fi

# Move files to Jenkins artifact directory
if [ "$FAIL" = "0" ]; then
    if [ ! -z "${WORKSPACE:-}" ]; then
        [ -d "${WORKSPACE}" ] || mkdir -p "${WORKSPACE}"
        # Make a sym link of the build to the jenkins artifacts directory 
        ln -s "${BUILD_ARCHIVE_DIRECTORY}" "${WORKSPACE}/jenkins_artifacts"
    fi

    # now limit the size of the archive (size is in megabytes)
    "$INC_PATH/scripts/limitdirsize.sh" "$BUILDS_ARCHIVE_PATH" 8192
fi

# clean up
"$INC_PATH/scripts/cleantmp.sh" > /dev/null 2>&1


# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
