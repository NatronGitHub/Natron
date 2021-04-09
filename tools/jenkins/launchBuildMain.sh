#!/bin/bash
#
# Main script to launch a Natron or plug-in repository build.
#
set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.


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
# RELEASE_TAG: string "x.y.z" indicating the release number. The corresponding tag is "vx.y.z" in the Natron repository and "Natron-x.y.z" in each of the plugins repository. Only indicate this to trigger a release.
# SNAPSHOT_BRANCH: If set and RELEASE_TAG is not set, this describes a specific branch to build as a snapshot
# SNAPSHOT_COMMIT: The commit to build for a snapshot on the given SNAPSHOT_BRANCH. If not set, the latest commit of the branch will be built
# UNIT_TESTS: "false" or "true" (for compat with jenkins) to run the full unit tests suite after the build
# NATRON_LICENSE: (GPL or COMMERCIAL) The license to build Natron with. If GPL then GPL components will be included.
# DISABLE_BREAKPAD: If set to 1, natron will be built without crash reporting capabilities
# COMPILE_TYPE: The type of build to do, i.e in terms of compilation. Valid values are (`relwithdebinfo`, `release`, `debug`). Must be `relwithdebinfo` or `debug` if DISABLE_BREAKPAD is not 1.
# NATRON_DEV_STATUS: ALPHA, BETA, RC, STABLE, CUSTOM: this is useful when doing a release (i.e: if specifying RELEASE_TAG.)
# NATRON_CUSTOM_BUILD_USER_NAME: For a release, Tag the build with a specific user/client name
# NATRON_BUILD_NUMBER: When doing a release this is the number of the release (if doing a rebuild)
# NATRON_EXTRA_QMAKE_FLAGS: Optional qmake flags to pass when building Natron
# BUILD_NAME: Give a name to the build so that it can be found in the archive
# DISABLE_RPM_DEB_PKGS: If set to 1, deb and rpm packages will not be built. Otherwise they are only built for a release
# DISABLE_PORTABLE_ARCHIVE: If set to 1, a portable archive will not be built
# BITS: Windows only: Must indicate if this is a 64 or 32 bits build
# DEBUG_SCRIPTS: If set to 1, binaries from previous build with same options are not cleaned so that the scripts can continue the same compilation
# EXTRA_PYTHON_MODULES_SCRIPT: Path to a python script that should install extra modules with pip or easy_install.
# MKJOBS: number of parallel build jobs
#
# The options above are set by custom parameters in the build configuration
# https://wiki.jenkins-ci.org/display/JENKINS/Git+Plugin#GitPlugin-Jenkins,GITpluginandWindows
#
# GIT_URL, GIT_BRANCH, GIT_COMMIT are set from the git jenkins plug-in automatically
# If GIT_URL is an unknown Natron repository, GIT_URL_IS_NATRON=1 can be used to force a Natron build.
# BUILD_NUMBER is also given by jenkins, see https://wiki.jenkins-ci.org/display/JENKINS/Building+a+software+project#Buildingasoftwareproject-JenkinsSetEnvironmentVariables
# The above options must be set if calling this script manually. If GIT_COMMIT is omitted, the latest commit of GIT_BRANCH is actually built
#
#
# Script debug parameters:
#
# BUILD_FROM: first step of the build
# BUILD_TO: last step of the build
#
#### STEP 1: checkout sources
if [ "${BUILD_FROM:-1}" -le 1 ] && [ "${BUILD_TO:-99}" -ge 1 ]; then
    BUILD_1=true
else
    BUILD_1=false
fi
#### STEP2: build plugins
if [ "${BUILD_FROM:-1}" -le 2 ] && [ "${BUILD_TO:-99}" -ge 2 ]; then
    BUILD_2=true
else
    BUILD_2=false
fi
#### STEP3: build natron
if [ "${BUILD_FROM:-1}" -le 3 ] && [ "${BUILD_TO:-99}" -ge 3 ]; then
    BUILD_3=true
else
    BUILD_3=false
fi
#### STEP4: build installer
if [ "${BUILD_FROM:-1}" -le 4 ] && [ "${BUILD_TO:-99}" -ge 4 ]; then
    BUILD_4=true
else
    BUILD_4=false
fi
#### STEP5: unit tests
if [ "${BUILD_FROM:-1}" -le 5 ] && [ "${BUILD_TO:-99}" -ge 5 ]; then
    BUILD_5=true
else
    BUILD_5=false
fi
#### STEP6: archive artifacts and cleanup (which happens even if build fails)
if [ "${BUILD_FROM:-1}" -le 6 ] && [ "${BUILD_TO:-99}" -ge 6 ]; then
    BUILD_6=true
else
    BUILD_6=false
fi

echo "-----------------------------------------------------------------------"
echo "NATRON JENKINS BUILDMASTER"
echo "-----------------------------------------------------------------------"
echo "BUILD_NUMBER                      : \"${BUILD_NUMBER:-}\""
echo "BUILD_NAME                        : \"${BUILD_NAME:-}\""
echo "WORKSPACE                         : \"${WORKSPACE:-}\""
echo "GIT_URL                           : \"${GIT_URL:-}\""
echo "GIT_URL_IS_NATRON                 : \"${GIT_URL_IS_NATRON:-}\""
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
echo "REMOTE_URL                        : \"${REMOTE_URL:-}\""
echo "REMOTE_USER                       : \"${REMOTE_USER:-}\""
echo "REMOTE_PREFIX                     : \"${REMOTE_PREFIX:-}\""
echo "BUILD_FROM                        : \"${BUILD_FROM:-}\""
echo "BUILD_TO                          : \"${BUILD_TO:-}\""
echo "BUILD_1 (checkout sources)        : \"${BUILD_1:-}\""
echo "BUILD_2 (plugins)                 : \"${BUILD_2:-}\""
echo "BUILD_3 (natron)                  : \"${BUILD_3:-}\""
echo "BUILD_4 (installer)               : \"${BUILD_4:-}\""
echo "BUILD_5 (unit tests)              : \"${BUILD_5:-}\""
echo "BUILD_6 (archive and cleanup)     : \"${BUILD_6:-}\""
echo "MKJOBS                            : \"${MKJOBS:-}\""
echo "NOUPDATE (disable scripts update) : \"${NOUPDATE:-}\""
echo "-----------------------------------------------------------------------"

if [ "${NATRON_LICENSE:-}" != "GPL" ] && [ "${NATRON_LICENSE:-}" != "COMMERCIAL" ]; then
    echo "Please select a License with NATRON_LICENSE=(GPL,COMMERCIAL)"
    exit 1
fi

if [ -z "${BUILD_NAME:-}" ] || [ -z "${BUILD_NUMBER:-}" ]; then
    echo "Must set BUILD_NAME and BUILD_NUMBER"
    exit 1
fi

if [ -z "${WORKSPACE:-}" ] || [ ! -d "${WORKSPACE}" ]; then
    echo "WORKSPACE must be set to an existing directory on the local filesystem."
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
if [ -f "$HOME/.ssh/id_rsa_build_master" ]; then
    keychainstatus=0
    eval $(keychain -q --eval --agents ssh id_rsa_build_master) || keychainstatus=1
    if [ $keychainstatus != 0 ]; then
        echo "Restarting ssh-agent"
        keychain -k
        eval $(keychain --eval --agents ssh id_rsa_build_master)
    fi
fi

# check that TIMEOUT works
$TIMEOUT 1 true && echo "$TIMEOUT works"
#check that SED works
echo "Testing $GSED"
echo "$GSED does not work" | $GSED -e "s/does not work/works/"
# check that curl works
$CURL --silent --head http://www.google.com > /dev/null && echo "$CURL works"

if [ -n "${GIT_BRANCH:+}" ]; then
    GIT_BRANCH=$(echo "$GIT_BRANCH" | sed 's#origin/##')
fi
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
if [ -n "${WORKSPACE:-}" ]; then
    if [ -d "${WORKSPACE}/jenkins_artifacts" ]; then
        rm -rf "${WORKSPACE}/jenkins_artifacts" || true
    fi
fi

# Do we need to build Natron ? No if the script was not invoked from a Natron CI project
if [ -z "${GIT_URL_IS_NATRON:-}" ]; then
    GIT_URL_IS_NATRON=0
    isNatronRepo "$GIT_URL" && GIT_URL_IS_NATRON=1
fi


if [ "${GIT_URL_IS_NATRON:-}" = "1" ]; then
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

BUILDS_ARCHIVE_PATH="$WORKSPACE/builds_archive"

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
echo "RELEASE_TAG is ${RELEASE_TAG:-(unset)}"
echo "SNAPSHOT_COMMIT is ${SNAPSHOT_COMMIT:-(unset)}"
echo "SNAPSHOT_BRANCH is ${SNAPSHOT_BRANCH:-(unset)}"
if [ "${GIT_URL_IS_NATRON:-}" != "1" ]; then
    TYPE="PLUGIN_CI"
else
    if [ -n "${RELEASE_TAG:-}" ]; then
        TYPE="RELEASE"
        if [ "${NATRON_DEV_STATUS:-}" != "ALPHA" ] && [ "${NATRON_DEV_STATUS:-}" != "BETA" ] && [ "${NATRON_DEV_STATUS:-}" != "RC" ] && [ "${NATRON_DEV_STATUS:-}" != "CUSTOM" ] && [ "${NATRON_DEV_STATUS:-}" != "STABLE" ]; then
            echo "Invalid NATRON_DEV_STATUS=${NATRON_DEV_STATUS:-(unset)}, it must be either [ALPHA, BETA, RC, CUSTOM, STABLE]"
            exit 1
        fi
        NATRON_BUILD_CONFIG="$NATRON_DEV_STATUS"
        setBuildOption "NATRON_BUILD_CONFIG" "$NATRON_DEV_STATUS"
        setBuildOption "NATRON_BUILD_NUMBER" "$NATRON_BUILD_NUMBER"
        setBuildOption "NATRON_CUSTOM_BUILD_USER_NAME" "${NATRON_CUSTOM_BUILD_USER_NAME:-}"
    elif [ -n "${SNAPSHOT_COMMIT:-}" ] || [ -n "${SNAPSHOT_BRANCH:-}" ]; then
        TYPE="SNAPSHOT"
        NATRON_BUILD_CONFIG="SNAPSHOT"
        setBuildOption "NATRON_BUILD_CONFIG" "SNAPSHOT"
    else
        TYPE="NATRON_CI"
        NATRON_BUILD_CONFIG="SNAPSHOT"
        setBuildOption "NATRON_BUILD_CONFIG" "SNAPSHOT"
    fi
fi
echo "TYPE is $TYPE"

# Can be set to relwithdebinfo, release, debug
setBuildOption "COMPILE_TYPE" "$COMPILE_TYPE"
setBuildOption "NATRON_EXTRA_QMAKE_FLAGS" "${NATRON_EXTRA_QMAKE_FLAGS:-}"

setBuildOption "BUILD_TYPE" "$TYPE"

# Set the url for Natron to GIT_URL if this is the build that triggered it, otherwise use Github by default
if [ "${GIT_URL_IS_NATRON:-}" = "1" ]; then
    setBuildOption "NATRON_GIT_URL" "$GIT_URL"
else
    setBuildOption "NATRON_GIT_URL" "$GIT_URL_NATRON"
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
    GIT_BRANCH="tags/v${RELEASE_TAG:-}"
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
if [ "$TYPE" != "SNAPSHOT" ] && [ "$TYPE" != "RELEASE" ]; then
    WITH_ONLINE_INSTALLER=0
else
    WITH_ONLINE_INSTALLER="${WITH_ONLINE_INSTALLER:-0}"
fi

setBuildOption "WITH_ONLINE_INSTALLER" "${WITH_ONLINE_INSTALLER}"


# If 1, source files will be compressed and uploaded with the binary
TAR_SOURCES=0

#
# build
#
FAIL=0

source checkout-repository.sh

# Checkout Natron git
if [ "${GIT_URL_IS_NATRON:-}" = "1" ]; then
    if $BUILD_1; then
        checkoutRepository "$GIT_URL_NATRON" "Natron" "$NATRON_GIT_BRANCH" "$NATRON_GIT_COMMIT" "NATRON_GIT_COMMIT" "$TAR_SOURCES" || FAIL=$?
    fi

    if [ "$FAIL" != "1" ]; then
    NATRON_MAJOR=$(echo "NATRON_VERSION_MAJOR" | $CC -E -P -include "$TMP_PATH/Natron/Global/Macros.h" - | tail -1)
    NATRON_MINOR=$(echo "NATRON_VERSION_MINOR" | $CC -E -P -include "$TMP_PATH/Natron/Global/Macros.h" - | tail -1)
    NATRON_REVISION=$(echo "NATRON_VERSION_REVISION" | $CC -E -P -include "$TMP_PATH/Natron/Global/Macros.h" - | tail -1)

    setBuildOption "NATRON_VERSION_MAJOR" "$NATRON_MAJOR"
    setBuildOption "NATRON_VERSION_MINOR" "$NATRON_MINOR"
    setBuildOption "NATRON_VERSION_REVISION" "$NATRON_REVISION"

    setBuildOption "NATRON_VERSION_SHORT" "${NATRON_MAJOR}.${NATRON_MINOR}"
    setBuildOption "NATRON_VERSION_FULL" "${NATRON_MAJOR}.${NATRON_MINOR}.${NATRON_REVISION}"
    fi
    cd "$CWD"
    # Update build scripts, except launchBuildMain.sh, which is being executed
    # That way, the SDK doesn't need to be updated when eg build-plugins.sh or buil-Linux-installer.sh is updated
    if [ "$TMP_PATH/Natron/tools/jenkins/launchBuildMain.sh" -nt "launchBuildMain.sh" ] && ! cmp "$TMP_PATH/Natron/tools/jenkins/launchBuildMain.sh" "launchBuildMain.sh" >/dev/null 2>&1 && [ -z "${LAUNCHBUILDMAIN_UPDATED+x}" ]; then
        echo "Warning: launchBuildMain.sh has changed since the last SDK build. SDK may need to be rebuilt. Restarting after a 5s pause."
        sleep 5
        if [ -f ./launchBuildMain.sh.orig ]; then
            rm -f ./launchBuildMain.sh.orig || true
        fi
        mv ./launchBuildMain.sh ./launchBuildMain.sh.orig || true
        cp "$TMP_PATH/Natron/tools/jenkins/launchBuildMain.sh" ./launchBuildMain.sh || true
        exec env LAUNCHBUILDMAIN_UPDATED=1 ./launchBuildMain.sh "$*"
    fi
    if [ -z "${NOUPDATE:+x}" ]; then
        (cd "$TMP_PATH/Natron/tools/jenkins"; tar --exclude launchBuildMain.sh -cf - .) | tar xf -
    fi
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
    NATRON_VERSION_STRING="${NATRON_VERSION_FULL}-alpha-$NATRON_BUILD_NUMBER"
elif [ "$NATRON_BUILD_CONFIG" = "BETA" ]; then
    NATRON_VERSION_STRING="${NATRON_VERSION_FULL}-beta-$NATRON_BUILD_NUMBER"
elif [ "$NATRON_BUILD_CONFIG" = "RC" ]; then
    NATRON_VERSION_STRING="$NATRON_VERSION_FULL-RC$NATRON_BUILD_NUMBER"
elif [ "$NATRON_BUILD_CONFIG" = "STABLE" ]; then
    NATRON_VERSION_STRING="$RELEASE_TAG"
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

INSTALLER_BASENAME="${INSTALLER_BASENAME}-${NATRON_VERSION_STRING}-${PKGOS}-${BITS}"

if [ "$COMPILE_TYPE" = "debug" ]; then
    INSTALLER_BASENAME="${INSTALLER_BASENAME}-debug"
fi
PORTABLE_DIRNAME="${INSTALLER_BASENAME}-no-installer"

if [ "$PKGOS" = "Windows" ] && [ -f "/Setup.exe" ]; then
    # Windows portable has installer, don't append "no-installer"
    PORTABLE_DIRNAME="$INSTALLER_BASENAME"
fi

setBuildOption "INSTALLER_BASENAME" "$INSTALLER_BASENAME"
setBuildOption "PORTABLE_DIRNAME" "$PORTABLE_DIRNAME"
setBuildOption "TMP_PORTABLE_DIR" "${TMP_BINARIES_PATH}/$PORTABLE_DIRNAME"

updateBuildOptions

# Checkout plug-ins git
if $BUILD_1 && [ -n "$OPENFX_IO_GIT_BRANCH" ] && [ "$FAIL" != "1" ]; then
    checkoutRepository "$GIT_URL_OPENFX_IO" "openfx-io" "$OPENFX_IO_GIT_BRANCH" "$OPENFX_IO_GIT_COMMIT" "OPENFX_IO_GIT_BRANCH" "$TAR_SOURCES" || FAIL=$?
fi

if $BUILD_1 && [ -n "$OPENFX_MISC_GIT_BRANCH" ] && [ "$FAIL" != "1" ]; then
    checkoutRepository "$GIT_URL_OPENFX_MISC" "openfx-misc" "$OPENFX_MISC_GIT_BRANCH" "$OPENFX_MISC_GIT_COMMIT" "OPENFX_MISC_GIT_BRANCH" "$TAR_SOURCES" || FAIL=$?
fi

if $BUILD_1 && [ -n "$OPENFX_ARENA_GIT_BRANCH" ] && [ "$FAIL" != "1" ]; then
    checkoutRepository "$GIT_URL_OPENFX_ARENA" "openfx-arena" "$OPENFX_ARENA_GIT_BRANCH" "$OPENFX_ARENA_GIT_COMMIT" "OPENFX_ARENA_GIT_BRANCH" "$TAR_SOURCES" || FAIL=$?
fi

if $BUILD_1 && [ -n "$OPENFX_GMIC_GIT_BRANCH" ] && [ "$FAIL" != "1" ]; then
    checkoutRepository "$GIT_URL_OPENFX_GMIC" "openfx-gmic" "$OPENFX_GMIC_GIT_BRANCH" "$OPENFX_GMIC_GIT_COMMIT" "OPENFX_GMIC_GIT_BRANCH" "$TAR_SOURCES" || FAIL=$?
fi

# Force deactivate of openfx-opencv for now.
OPENFX_OPENCV_GIT_BRANCH=""
if $BUILD_1 && [ -n "$OPENFX_OPENCV_GIT_BRANCH" ] && [ "$FAIL" != "1" ]; then
    checkoutRepository "$GIT_URL_OPENFX_OPENCV" "openfx-opencv" "$OPENFX_OPENCV_GIT_BRANCH" "$OPENFX_OPENCV_GIT_COMMIT" "OPENFX_OPENCV_GIT_BRANCH" "$TAR_SOURCES" || FAIL=$?
fi

dumpBuildOptions


# Build plug-ins
if $BUILD_2 && [ "$FAIL" = "0" ]; then
    bash "build-plugins.sh" || FAIL=$?
fi
if [ "$FAIL" -ne "0" ]; then
    echo "build-plugins.sh failed with status $FAIL"
    exit $FAIL
fi

# Build Natron
if $BUILD_3 && [ "$FAIL" = "0" ] && [ "${GIT_URL_IS_NATRON:-}" = "1" ]; then
    bash "build-natron.sh" || FAIL=$?
fi
if [ "$FAIL" -ne "0" ]; then
    echo "build-natron.sh failed with status $FAIL"
    exit $FAIL
fi

# build installer(s)
#
if $BUILD_4 && [ "$FAIL" = "0" ] && [ "${GIT_URL_IS_NATRON:-}" = "1" ]; then
    printStatusMessage "Creating installer..."
    bash "build-${PKGOS}-installer.sh" || FAIL=$?
fi
if [ "$FAIL" -ne "0" ]; then
    echo "build-${PKGOS}-installer.sh failed with status $FAIL"
    exit $FAIL
fi


UNIT_TESTS_FAIL=0
if $BUILD_5 && [ "$FAIL" = "0" ]; then
    echo "Build finished! Files are located in $BUILD_ARCHIVE_DIRECTORY"
    # unit tests
    if [ "${GIT_URL_IS_NATRON:-}" = "1" ] && [ "$DO_UTEST" = "1" ]; then

        env bash runUnitTests.sh || UNIT_TESTS_FAIL=$?
        if [ "$UNIT_TESTS_FAIL" = "0" ]; then
            echo "Unit tests: OK"
        else
            echo "Unit tests: failed ($UNIT_TESTS_FAIL)"
       fi
    fi
fi

if $BUILD_6; then
if [ -f "$BUILD_OPTIONS_FILE" ]; then
    cp "$BUILD_OPTIONS_FILE" "$BUILD_ARCHIVE_DIRECTORY/"
fi

# Move files to Jenkins artifact directory
if [ "$FAIL" = "0" ]; then
    if [ -n "${WORKSPACE:-}" ]; then
        [ -d "${WORKSPACE}" ] || mkdir -p "${WORKSPACE}"
        # Make a sym link of the build to the jenkins artifacts directory 
        ln -s "${BUILD_ARCHIVE_DIRECTORY}" "${WORKSPACE}/jenkins_artifacts"
    fi

    # now limit the size of the archive (size is in megabytes)
    "$INC_PATH/scripts/limitdirsize.sh" "$BUILDS_ARCHIVE_PATH" 8192
fi

# clean up
"$INC_PATH/scripts/cleantmp.sh" > /dev/null 2>&1
fi

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
