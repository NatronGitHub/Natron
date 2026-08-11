#!/bin/bash

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

# GIT_COMMIT=xxxx If doing a ci or snapshot build, must be set to the commit to build
# GIT_BRANCH=xxxx Used if GIT_COMMIT is not set, it points to the branch (or tag if BUILD_CONFIG=STABLE) to build
# REPO: Must point to the git URL of the Natron repo to use

source common.sh
source manageBuildOptions.sh
source manageLog.sh

updateBuildOptions

function checkoutRepository() {
    REPO_URL="$1"
    REPO_LOCAL_DIRNAME="$2"
    REPO_BRANCH="$3"
    REPO_COMMIT="$4"
    COMMIT_BUILD_OPTION_NAME="$5"
    TAR_SOURCES=$6
    if [ -z "$REPO_URL" ]; then
        echo "Please define REPO"
        return 1
    fi

    printStatusMessage "===> Cloning $REPO_URL on branch $REPO_BRANCH, commit $REPO_COMMIT"

    if [ "${DEBUG_SCRIPTS:-}" != "1" ] && [ -d "$TMP_PATH/$REPO_LOCAL_DIRNAME" ]; then
        rm -rf "$TMP_PATH/$REPO_LOCAL_DIRNAME"
    fi

    # clone
    if [ ! -d "$TMP_PATH" ]; then
        mkdir -p "$TMP_PATH"
    fi
    cd "$TMP_PATH"
    if [ -z "$REPO_COMMIT" ]; then
        branch="$REPO_BRANCH"
    else
        branch="$REPO_COMMIT"
    fi
    echo branch=$branch BRANCH=$REPO_BRANCH COMMIT=$REPO_COMMIT
    deep=false
    if [ ! -d "$REPO_LOCAL_DIRNAME" ]; then
        # slow version:
        if "$deep" && [ -z "$REPO_COMMIT" ]; then
            $TIMEOUT 1800 $GIT clone "$REPO_URL" -b "${REPO_BRANCH}" "$REPO_LOCAL_DIRNAME"
        elif [ -n "$REPO_COMMIT" ]; then
            # A commit is not a ref, so it cannot be given to clone -b: git fails
            # with "Remote branch <sha> not found in upstream origin". Clone the
            # branch, or the default branch when none was given, and fetch the
            # commit itself below.
            if [ -n "$REPO_BRANCH" ]; then
                $TIMEOUT 1800 $GIT clone --depth 1 -b "${REPO_BRANCH#tags/}" "$REPO_URL" "$REPO_LOCAL_DIRNAME"
            else
                $TIMEOUT 1800 $GIT clone --depth 1 "$REPO_URL" "$REPO_LOCAL_DIRNAME"
            fi
        else
            # fast version (depth=1):
            $TIMEOUT 1800 $GIT clone --depth 1 -b "${branch#tags/}" "$REPO_URL" "$REPO_LOCAL_DIRNAME"
        fi
    fi

    cd "$REPO_LOCAL_DIRNAME"

    if [ -z "$REPO_COMMIT" ]; then
        REPO_COMMIT=$(git rev-parse HEAD)
        echo "done, got commit $REPO_COMMIT"
    else
        # The clone is shallow and holds only the branch tip, so ask for the
        # pinned commit before checking it out. Needs the full 40 character sha:
        # GitHub rejects an abbreviated one with "not our ref".
        $TIMEOUT 1800 $GIT fetch --depth 1 origin "$REPO_COMMIT"
        $GIT checkout "$REPO_COMMIT"
    fi


    # the snapshot are always built with latest version
    # slow version:
    #$TIMEOUT 1800 $GIT submodule update --init --recursive --remote
    # fast version (depth=1):
    $TIMEOUT 1800 $GIT submodule update --depth 1 --init --recursive --remote

    # copy tarball of sources pruned of git files
    if [ "${TAR_SOURCES:-}" = "1" ]; then
        SRC_DIR_NAME="$REPO_LOCAL_DIRNAME-$REPO_BRANCH-$REPO_COMMIT"
        cp -a "$REPO_LOCAL_DIRNAME" "$SRC_DIR_NAME"
        (cd "$SRC_DIR_NAME"; find . -type d -name .git -exec rm -rf {} \;)
        tar cvvJf "$SRC_PATH/$SRC_DIR_NAME.tar.xz" "$SRC_DIR_NAME"
        rm -rf "$SRC_DIR_NAME"
    fi

    if [ -n "$REPO_COMMIT" ] && [ -n "$COMMIT_BUILD_OPTION_NAME" ]; then
        setBuildOption "$COMMIT_BUILD_OPTION_NAME" "$REPO_COMMIT"
    fi

    cd $CWD
    return 0
}

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
