#!/bin/bash

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
set -x # Print commands and their arguments as they are executed.

# Ensure we have the build master ssh key to clone/update repository
# Note: This is now done by keychain (see ~/.bash_profile)
#. ensure-ssh-identity.sh
if [ -f "$HOME/.ssh/id_rsa_build_master" ]; then
    keychainstatus=0
    eval `keychain -q --eval --agents ssh id_rsa_build_master` || keychainstatus=1
    if [ $keychainstatus != 0 ]; then
        echo "Restarting ssh-agent"
        keychain -k
        eval `keychain --eval --agents ssh id_rsa_build_master`
    fi
fi

source common.sh
source manageBuildOptions.sh


if [ -z "${BUILD_NAME:-}" ]; then
    echo "You forgot to set BUILD_NAME to the value that was given to the build to upload."
    exit 1
fi
if [ -z "${BUILD_NUMBER:-}" ]; then
    echo "You forgot to set BUILD_NUMBER to the value that was given to the build to upload."
    exit 1
fi
if [ -z "${REMOTE_USER:-}" ]; then
    echo "You forgot to set REMOTE_USER"
    exit 1
fi
if [ -z "${REMOTE_URL:-}" ]; then
    echo "You forgot to set REMOTE_URL"
    exit 1
fi
if [ -z "${REMOTE_PREFIX:-}" ]; then
    echo "You forgot to set REMOTE_PREFIX"
    exit 1
fi
if [ -z "${BITS:-}" ]; then
    echo "Must set BITS to either 32, 64 or Universal"
    exit 1
fi

BUILD_DIR="$CWD/builds_archive/$BUILD_NAME/$BUILD_NUMBER"

if [ ! -d "$BUILD_DIR" ]; then
    echo "$BUILD_DIR does not exist"
    exit 1
fi


# Source the build options
source "$BUILD_DIR/$BUILD_OPTIONS_BASENAME"

REMOTE_PATH="$REMOTE_PREFIX"
if [ "$BUILD_TYPE" != "RELEASE" ] && [ "$BUILD_TYPE" != "SNAPSHOT" ]; then
    if [ "${FORCE_UPLOAD:-}" != "1" ]; then
        echo "This build is not a snapshot or release so it will not be built."
        exit 1
    else
        echo "This build is not a snapshot or release but FORCE_UPLOAD=1 was passed. Uploading in the $REMOTE_PATH/other_builds directory"
        REMOTE_PATH="$REMOTE_PATH/other_builds"
    fi
elif [ "$BUILD_TYPE" = "RELEASE" ]; then
    echo "This build is a release, uploading to $REMOTE_PATH/releases"
    REMOTE_PATH="$REMOTE_PATH/releases"
elif [ "$BUILD_TYPE" = "SNAPSHOT" ]; then
    echo "This build is a snapshot, uploading to $REMOTE_PATH/snapshots"
    REMOTE_PATH="$REMOTE_PATH/snapshots"
fi

REMOTE_PATH="$REMOTE_PATH/$PKGOS/$BITS/$BUILD_NAME"
REMOTE_BUILD_DIR="$REMOTE_PATH/$BUILD_NUMBER"


# Create directory on the remote
ssh "${REMOTE_USER}@${REMOTE_URL}" "mkdir -p $REMOTE_BUILD_DIR && chmod 775 -R ${REMOTE_PREFIX}" || true


# Limit directory size of a build name to 1GiB.
ssh "${REMOTE_USER}@${REMOTE_URL}" "sh limitdirsize.sh $REMOTE_BUILD_DIR 1024" || true


# Upload online installer packages. All build with the same BUILD_NAME will share updates
if [ -d "$BUILD_DIR/online_installer" ]; then
    rsync_remote "$BUILD_DIR/online_installer/packages" "$REMOTE_PATH/" --delete
    for f in "$BUILD_DIR/online_installer/"*-online*; do
	rsync_remote "$f" "$REMOTE_PATH/" --delete
    done
fi

# Upload build options
rsync_remote "$BUILD_DIR/$BUILD_OPTIONS_BASENAME" "$REMOTE_BUILD_DIR/"

# OSX always has the compressed non installer (.dmg), but other platforms may not have it.
if [ -d "$BUILD_DIR/compressed_no_installer" ]; then
    for f in "$BUILD_DIR/compressed_no_installer/"*; do
	rsync_remote "$f" "$REMOTE_BUILD_DIR/"
    done
fi

# Only windows and linux have an installer for now
if [ "$PKGOS" = "Linux" ] || [ "$PKGOS" = "Windows" ]; then
    if [ -d "$BUILD_DIR/offline_installer" ]; then
        for f in "$BUILD_DIR/offline_installer/"*; do
	    rsync_remote "$f" "$REMOTE_BUILD_DIR/"
	done
    fi
fi

# Linux may have deb or rpm
if [ "$PKGOS" = "Linux" ]; then
    if [ -d "$BUILD_DIR/rpm_package" ]; then
        for f in "$BUILD_DIR/rpm_package/"*.rpm; do
	    rsync_remote "$f" "$REMOTE_BUILD_DIR/"
	done
    fi
    if [ -d "$BUILD_DIR/deb_package" ]; then
        for f in "$BUILD_DIR/deb_package/"*.deb; do
	    rsync_remote "$f" "$REMOTE_BUILD_DIR/"
	done
    fi
fi

# Upload symbols to the main symbols store
if [ -d "$BUILD_DIR/symbols" ]; then
    for f in "$BUILD_DIR/symbols/"*; do
	rsync_remote "$f" "symbols/"
    done
fi


# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:

