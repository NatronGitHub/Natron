#!/bin/bash

# This script saves all options of the current build so that we can retrieve these values at difference places across scripts
if [ -z "$1" ]; then
    echo "No argument"
    exit 1
fi

if [ -f "$1" ]; then
    exit 0
fi

cat <<EOF > "$1"
#!/bin/sh

# Build type can be RELEASE, SNAPSHOT, NATRON_CI, PLUGIN_CI
BUILD_TYPE=""

# The name identifying the project (e.g: natron_github_master)
BUILD_NAME=""

# Where the final build artifacts (binaries) are stored
BUILD_ARCHIVE_DIRECTORY=""

# Set to 1 to also build a online installer for SNAPSHOT and RELEASE builds (only available for Windows and Linux).
WITH_ONLINE_INSTALLER=""

# The date at which the build started
CURRENT_DATE=""

# If set to 1, deb and rpm packages will never be built. Default is to build when NATRON_BUILD_CONFIG=STABLE.
DISABLE_RPM_DEB_PKGS=""

# If set to 1, a portable archive will not be built
DISABLE_PORTABLE_ARCHIVE=""

# This mode is useful when debugging scripts: it doesn't remove any cloned repo or binary so that we don't redo
# tasks that take time
DEBUG_SCRIPTS=""

# The Natron license (either GPL or COMMERCIAL). When COMMERCIAL, GPL components are not included
NATRON_LICENSE=""
# The Natron build config (SNAPSHOT,ALPHA,BETA,RC,STABLE,CUSTOM)
NATRON_BUILD_CONFIG=""
NATRON_BUILD_NUMBER=""
NATRON_CUSTOM_BUILD_USER_NAME=""

# If set to 1, natron will be built without crash reporting capabilities
DISABLE_BREAKPAD=""


# Url of the git that triggered the build
GIT_TRIGGER_URL=""
GIT_TRIGGER_BRANCH=""
GIT_TRIGGER_COMMIT=""

# Urls of the repositories to build
GIT_URL_NATRON=""
GIT_URL_OPENFX_IO=""
GIT_URL_OPENFX_MISC=""
GIT_URL_OPENFX_ARENA=""
GIT_URL_OPENFX_GMIC=""
GIT_URL_OPENFX_OPENCV=""

NATRON_GIT_BRANCH=""

OPENFX_IO_GIT_BRANCH=""
OPENFX_MISC_GIT_BRANCH=""
OPENFX_ARENA_GIT_BRANCH=""
OPENFX_GMIC_GIT_BRANCH=""
OPENFX_OPENCV_GIT_BRANCH=""


# Set by checkout-natron.sh
NATRON_GIT_COMMIT=""

NATRON_VERSION_MAJOR=""
NATRON_VERSION_MINOR=""
NATRON_VERSION_REVISION=""

# Major.Minor
NATRON_VERSION_SHORT=""

# Major.Minor.Rev
NATRON_VERSION_FULL=""

# Used to decorate installer name
NATRON_VERSION_STRING=""

# Set by build-plugins.sh
OPENFX_IO_GIT_COMMIT=""
OPENFX_MISC_GIT_COMMIT=""
OPENFX_ARENA_GIT_COMMIT=""
OPENFX_GMIC_GIT_COMMIT=""
OPENFX_OPENCV_GIT_COMMIT=""


# Either [relwithdebinfo, release, debug]
COMPILE_TYPE=""

# Extra qmake flags
NATRON_EXTRA_QMAKE_FLAGS=""

# Extra Python modules install script
EXTRA_PYTHON_MODULES_SCRIPT=""

# OS name
PKGOS=""

# 32, 64 or universal
BITS=""

# Temporary directory where the binaries are built
TMP_PATH=""

# The basename of the installer
INSTALLER_BASENAME=""

# Path where binaries are stored after compilation.
# The build-plugin and build-natron scripts copy binaries to this folder
# which is then read by build-$PKGOS-installer to produce the portable install directory
TMP_BINARIES_PATH=""

# Directory name of the portable installation
PORTABLE_DIRNAME=""

# Path of the portable installation where we can launch the unit tests
# in a safe environment
TMP_PORTABLE_DIR=""

# The name of the host where to upload the online installer. This will be used
# to configure the online installer to select the location where to check for updates.
REMOTE_URL=""

# The user that should connect to the REMOTE_URL when uploading with rsync
REMOTE_USER=""

# The prefix location on the server where to upload the artifacts, relative to
# REMOTE_USER@REMOTE_URL
REMOTE_PREFIX=""

EOF

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:

