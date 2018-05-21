#!/bin/bash
#
# Natron Buildmaster
#
# Usage: 
# export MKJOBS=XX
# export NATRON_LICENSE=XX
# ./buildmaster.sh
#
# DO NOT USE ENV XXXXXX BEFORE BUILDMASTER.SH AND DO NOT RUN SH BUILDMASTER.SH
#

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

DISABLE_BREAKPAD=1 # comment to re-enable breakpad
export DISABLE_BREAKPAD

BM_PID=$$
FINISHED=0
RESTART=0
CHECKSUM=""
while [ "$FINISHED" = "0" ]; do

    # common
    #
    source common.sh
    source common-buildmaster.sh
    if [ "${TMP_PATH:-}" = "" ]; then
        echo "Failed to source common.sh, something is wrong!"
        exit 1
    fi

    # refresh credentials (-q = silent)
    keychainstatus=0
    eval `keychain -q --eval --agents ssh id_rsa_build_master` || keychainstatus=1
    if [ $keychainstatus != 0 ]; then
        echo "Restarting ssh-agent"
        keychain -k
        eval `keychain --eval --agents ssh id_rsa_build_master`
    fi

    # add tmp
    rm -rf "${TMP_PATH}" || true
    if [ ! -d "$TMP_PATH" ]; then
        mkdir -p "$TMP_PATH"
    fi

    # settings
    #
    # You can have servers that only build specific builds, like one just for RELEASES etc.
    # 'settings.sh':
    # DISABLE_RELEASE=1|0
    # DISABLE_SNAPSHOT=1|0
    # DISABLE_CI=1|0
    #
    # queue limit can be changed in 'settings.sh':
    # QUEUE_LIMIT=X (default 20)
    #
    # SERVERNAME=XXX
    #
    if [ -f settings.sh ]; then
        source settings.sh
    fi

    if [ "${QUEUE_LIMIT:-}" = "" ]; then
        QUEUE_LIMIT=20
    fi

    QUEUE="$TMP_PATH/queue.$$"
    CONFIG="$TMP_PATH/config.$$"
    GOT_RELEASE=""
    GOT_SNAPSHOT=""
    GOT_CI=""
    COMMIT=""
    BRANCH=""
    TYPE=""
    REPO=""
    QUEUE_ID=""
    REALTIME_LOGS=""
    RUN_UNIT_TESTS=1

    # update scripts
    #
    $GIT checkout common.sh || true
    $TIMEOUT 1800 $GIT pull --rebase --autostash || true
    $TIMEOUT 1800 $GIT submodule update -i --recursive || true

    # cleanup
    #
    rm -rf "$CWD"/qbuild_*_"${BITS}" || true

    # if an update to this script was found restart
    if [ "$PKGOS" != "OSX" ]; then
        MKSUM=$(md5sum "$0"|awk '{print $1}')
    else
        MKSUM=$(md5 "$0"|awk '{print $4}')
    fi
    echo "===> buildmaster checksum is $MKSUM"
    if [ ! -z "$CHECKSUM" ] && [ ! -z "$MKSUM" ]; then
        #echo "===> buildmaster $MKSUM vs. $CHECKSUM"
        if [ "$MKSUM" != "$CHECKSUM" ]; then
             echo "===> buildmaster has changed, we need to restart"
             RESTART=1
             FINISHED=1
         fi
    fi
    CHECKSUM=$MKSUM


    # Get config
    #
    $MYSQL "SELECT config_key,config_value FROM config;"|sed 1d > "$CONFIG" || true
    while read -r config_key config_value ; do
        if [ "$config_key" = "realtime_logs" ]; then
            REALTIME_LOGS="$config_value"
        elif [ "$config_key" = "run_unit_tests" ]; then
            RUN_UNIT_TESTS="$config_value"
        fi
    done < "$CONFIG"

    # Get queue and find a job
    # Priority: RELEASE, SNAPSHOT, CI
    #
    $MYSQL "SELECT id,commit_timestamp,commit_repo,commit_branch,commit_id,commit_type FROM queue WHERE ${QOS}=0 ORDER BY queue_order DESC LIMIT ${QUEUE_LIMIT};"|sed 1d > "$QUEUE" || true
    while read -r job_id job_date job_repo job_branch job_commit job_type ; do
        if [ ! -z "$job_commit" ] && [ -z "$job_type" ]; then
            job_type=$job_commit
            job_commit=""
        fi
        if [ "$job_type" = "RELEASE" ] && [ "$GOT_RELEASE" = "" ] && [ "${DISABLE_RELEASE:-}" != "1" ]; then
            REPO="$job_repo"
            GOT_RELEASE="1"
            COMMIT="$job_commit"
            BRANCH="$job_branch"
            QUEUE_ID=$job_id
        elif [ "$job_type" = "SNAPSHOT" ] && [ "$GOT_SNAPSHOT" = "" ] && [ "$GOT_RELEASE" = "" ] && [ "${DISABLE_SNAPSHOT:-}" != "1" ]; then
            REPO="$job_repo"
            GOT_SNAPSHOT="1"
            COMMIT="$job_commit"
            BRANCH="$job_branch"
            QUEUE_ID=$job_id
        elif [ "$job_type" = "CI" ] && [ "$GOT_CI" = "" ] && [ "$GOT_SNAPSHOT" = "" ] && [ "$GOT_RELEASE" = "" ] && [ "${DISABLE_CI:-}" != "1" ]; then
            REPO="$job_repo"
            GOT_CI="1"
            COMMIT="$job_commit"
            BRANCH="$job_branch"
            QUEUE_ID=$job_id
        fi
    done < "$QUEUE"

    # check if we found a job
    #
    if [ "$GOT_RELEASE" = "1" ]; then
        TYPE="RELEASE"
    elif [ "$GOT_SNAPSHOT" = "1" ]; then
        TYPE="SNAPSHOT"
    elif [ "$GOT_CI" = "1" ]; then
        TYPE="CI"
    fi

    if [ "$TYPE" = "" ]; then
        pingServer "$SERVERNAME" 0
    fi

    # we have a job ...
    # TODO: need to parse a build number from alpha/beta/rc
    #
    if [ "$TYPE" != "" ] && [ "$BRANCH" != "" ] && [ "$QUEUE_ID" != "" ]; then
        echo "Started build $QUEUE_ID type $TYPE/$REPO on branch $BRANCH using commit $COMMIT ..."

        # take job (first come, first serve)
        #
        JOBDATE=$(date "+%Y-%m-%d %H:%M:%S")
        $MYSQL "UPDATE queue SET ${QOS}_timestamp='${JOBDATE}' WHERE id=${QUEUE_ID};" || true
        updateStatus "$QUEUE_ID" 1
        updateStatusLog "$QUEUE_ID" ""

        # start watchdog
        #
        NATRON_LICENSE=$NATRON_LICENSE "$CWD"/watchdog.sh "$QUEUE_ID" "$BM_PID" &
        WATCHDOG_PID=$!

        # debug?
        # 
        DEBUG_BUILD=0
        #if [ "$TYPE" = "CI" ]; then
        #    DEBUG_BUILD=1
        #fi

        # add tmp
        if [ ! -d "$TMP_PATH" ]; then
            mkdir -p "$TMP_PATH"
        fi 
        # setup logs
        #
        PID_NATRON="$QUEUE_LOG_PATH/natron-$QUEUE_ID-$QOS.pid"
        PID_PLUGINS="$QUEUE_LOG_PATH/plugins-$QUEUE_ID-$QOS.pid"
        PID_INSTALLER="$QUEUE_LOG_PATH/installer-$QUEUE_ID-$QOS.pid"
        PLOG="$QUEUE_LOG_PATH/plugins_log-$QUEUE_ID-$QOS.log"
        NLOG="$QUEUE_LOG_PATH/natron_log-$QUEUE_ID-$QOS.log"
        ILOG="$QUEUE_LOG_PATH/installer_log-$QUEUE_ID-$QOS.log"
        SLOG1="$QUEUE_LOG_PATH/sync1_log-$QUEUE_ID-$QOS.log"
        SLOG2="$QUEUE_LOG_PATH/sync2_log-$QUEUE_ID-$QOS.log"
        SLOG3="$QUEUE_LOG_PATH/sync3_log-$QUEUE_ID-$QOS.log"
        BLOG="$QUEUE_LOG_PATH/build-$QUEUE_ID-$QOS.log"
        ULOG="$QUEUE_LOG_PATH/unit-$QUEUE_ID-$QOS.log"
        RULOG="$QUEUE_LOG_PATH/runit-$QUEUE_ID-$QOS.log"
        UDIR="$QUEUE_LOG_PATH/ufail-$QUEUE_ID-$QOS"

        rm -f "$PID_NATRON" "$PID_PLUGINS" "$PID_INSTALLER" "$PLOG" "$NLOG" "$ILOG" "$SLOG1" "$SLOG2" "$SLOG3" "$BLOG" "$ULOG" "$RULOG" > /dev/null 2>&1 || true
        FAIL=0

        if [ -n "${SERVERNAME+x}" ]; then
            pingServer "$SERVERNAME" "$QUEUE_ID"
        fi

        # remove existing dir on osx
        #
        if [ "$PKGOS" = "OSX" ] && [ -d "$TMP_BINARIES_PATH" ]; then
            rm -rf "$TMP_BINARIES_PATH" || FAIL=$?
        fi

        # update tags if release
        # TODO: also bump files in LATEST_VERSION_README.txt
        if [ "$FAIL" = "0" ] && [ "$TYPE" = "RELEASE" ] && ([ "$REPO" = "Natron" ] || [ "$REPO" = "$GIT_NATRON_REPO_LABEL" ] || [ "$REPO" = "$GIT_NATRON_GFORGE_REPO_LABEL" ]); then
            RELEASE_BRANCH=$(echo "$BRANCH" | $GSED 's#tags/##;' | cut -c1-3)
            PLUGIN_COMMIT=$(echo "$BRANCH" | $GSED 's#tags/#tags/Natron-#;')
            COMMON="$CWD/common-buildmaster.sh"
            sed -e "s/NATRON_RELEASE_BRANCH=.*/NATRON_RELEASE_BRANCH=RB-${RELEASE_BRANCH}/" -e "s#NATRON_GIT_TAG=.*#NATRON_GIT_TAG=${BRANCH}#" -e "s#IOPLUG_GIT_TAG=.*#IOPLUG_GIT_TAG=${PLUGIN_COMMIT}#" -e "s#MISCPLUG_GIT_TAG=.*#MISCPLUG_GIT_TAG=${PLUGIN_COMMIT}#" -e "s#ARENAPLUG_GIT_TAG=.*#ARENAPLUG_GIT_TAG=${PLUGIN_COMMIT}#" -e "s#GMICPLUG_GIT_TAG=.*#GMICPLUG_GIT_TAG=${PLUGIN_COMMIT}#" -e "s#CVPLUG_GIT_TAG=.*#CVPLUG_GIT_TAG=${PLUGIN_COMMIT}#" "$COMMON" > "$COMMON".new && mv "$COMMON".new "$COMMON" || FAIL=$?
        fi

        # checkout natron
        #
        if [ "$FAIL" = "0" ] && ([ "$REPO" = "Natron" ] || [ "$REPO" = "$GIT_NATRON_REPO_LABEL" ] || [ "$REPO" = "$GIT_NATRON_GFORGE_REPO_LABEL" ]); then
            NATRON_GIT_REPO="$GIT_NATRON"
            if [ "$REPO" = "$GIT_NATRON_GFORGE_REPO_LABEL" ]; then
                NATRON_GIT_REPO="$GIT_NATRON_GFORGE"
            fi
            if [ "$TYPE" = "CI" ] || [ "$TYPE" = "SNAPSHOT" ]; then
                updateStatusLog "$QUEUE_ID" "Checking out Natron ..."
                echo "$(date '+%Y-%m-%d %H:%M:%S') *** START Natron checkout from $REPO=$NATRON_GIT_REPO with GIT_BRANCH=$BRANCH GIT_COMMIT=$COMMIT BUILD_CONFIG=SNAPSHOT DEBUG_MODE=$DEBUG_BUILD" >> "$PLOG"
                env REPO="$NATRON_GIT_REPO" GIT_BRANCH="$BRANCH" GIT_COMMIT="$COMMIT" BUILD_CONFIG=SNAPSHOT DEBUG_MODE="$DEBUG_BUILD" QID="${QUEUE_ID}" PIDFILE="$PID_NATRON" bash "$INC_PATH/scripts/checkout-natron.sh" >> "$PLOG" 2>&1 || FAIL=$?
                echo "$(date '+%Y-%m-%d %H:%M:%S') *** END Natron checkout from $REPO=$NATRON_GIT_REPO with GIT_BRANCH=$BRANCH GIT_COMMIT=$COMMIT BUILD_CONFIG=SNAPSHOT DEBUG_MODE=$DEBUG_BUILD -> $FAIL" >> "$PLOG"
            elif [ "$TYPE" = "RELEASE" ]; then
                updateStatusLog "$QUEUE_ID" "Checking out Natron ..."
                echo "$(date '+%Y-%m-%d %H:%M:%S') *** START Natron checkout from $REPO=$NATRON_GIT_REPO with BUILD_CONFIG=STABLE" >> "$PLOG"
                env REPO="$NATRON_GIT_REPO" BUILD_CONFIG=STABLE BUILD_NUMBER=1 QID="${QUEUE_ID}" PIDFILE="$PID_NATRON" bash "$INC_PATH/scripts/checkout-natron.sh" >> "$PLOG" 2>&1 || FAIL=$?
                echo "$(date '+%Y-%m-%d %H:%M:%S') *** END Natron checkout from $REPO=$NATRON_GIT_REPO with BUILD_CONFIG=STABLE -> $FAIL" >> "$PLOG"
            fi
            if [ "$FAIL" != "0" ]; then
                updateStatus "$QUEUE_ID" 3
                updateStatusLog "$QUEUE_ID" "Failed to checkout Natron!"
                #updateLog "$QUEUE_ID $QOS: Failed to checkout Natron!"
                #buildFailed ${QUEUE_ID} $QOS
            else
                updateStatusLog "$QUEUE_ID" "Checking out Natron ... OK!"
            fi
        fi

        # build plugins
        #
        if [ "$FAIL" = "0" ]; then
            if [ "$TYPE" = "CI" ] || [ "$TYPE" = "SNAPSHOT" ]; then
                if [ "$TYPE" = "SNAPSHOT" ] || ([ "$REPO" = "Natron" ] || [ "$REPO" = "$GIT_NATRON_REPO_LABEL" ] || [ "$REPO" = "$GIT_NATRON_GFORGE_REPO_LABEL" ]); then
                    updateStatusLog "$QUEUE_ID" "Building plugins ..."
                    echo "$(date '+%Y-%m-%d %H:%M:%S') *** START plugins build with GIT_BRANCH=$BRANCH BUILD_CONFIG=SNAPSHOT" >> "$PLOG"
                    env GIT_BRANCH="$BRANCH" BUILD_CONFIG=SNAPSHOT QID="${QUEUE_ID}" PIDFILE="$PID_PLUGINS" bash "$INC_PATH/scripts/build-plugins.sh" >> "$PLOG" 2>&1 || FAIL=$?
                    echo "$(date '+%Y-%m-%d %H:%M:%S') *** END plugins build with GIT_BRANCH=$BRANCH BUILD_CONFIG=SNAPSHOT -> $FAIL" >> "$PLOG"
                elif [ "$TYPE" = "CI" ] && ([ "$REPO" != "Natron" ] && [ "$REPO" != "$GIT_NATRON_REPO_LABEL" ] && [ "$REPO" != "$GIT_NATRON_GFORGE_REPO_LABEL" ]); then
                    updateStatusLog "$QUEUE_ID" "Building $REPO ..."
                    DO_IO=0
                    DO_MISC=0
                    DO_ARENA=0
                    DO_GMIC=0
                    if [ "$REPO" = "openfx-io" ]; then
                        DO_IO=1
                    elif [ "$REPO" = "openfx-misc" ]; then
                        DO_MISC=1
                    elif [ "$REPO" = "openfx-arena" ]; then
                        DO_ARENA=1
                    elif [ "$REPO" = "openfx-gmic" ]; then
                        DO_GMIC=1
                    fi
                    echo "$(date '+%Y-%m-%d %H:%M:%S') *** START plugins build with BUILD_IO=$DO_IO BUILD_MISC=$DO_MISC BUILD_ARENA=$DO_ARENA BUILD_GMIC=$DO_GMIC GIT_BRANCH=$BRANCH GIT_COMMIT=$COMMIT BUILD_CONFIG=SNAPSHOT" >> "$PLOG"
                    env BUILD_IO="$DO_IO" BUILD_MISC="$DO_MISC" BUILD_ARENA="$DO_ARENA" BUILD_GMIC="$DO_GMIC" GIT_BRANCH="$BRANCH" GIT_COMMIT="$COMMIT" BUILD_CONFIG=SNAPSHOT QID="${QUEUE_ID}" PIDFILE="$PID_PLUGINS" bash "$INC_PATH/scripts/build-plugins.sh" >> "$PLOG" 2>&1 || FAIL=$?
                    echo "$(date '+%Y-%m-%d %H:%M:%S') *** END plugins build with BUILD_IO=$DO_IO BUILD_MISC=$DO_MISC BUILD_ARENA=$DO_ARENA BUILD_GMIC=$DO_GMIC GIT_BRANCH=$BRANCH GIT_COMMIT=$COMMIT BUILD_CONFIG=SNAPSHOT -> $FAIL" >> "$PLOG"
                fi
            elif [ "$TYPE" = "RELEASE" ]; then
                updateStatusLog "$QUEUE_ID" "Building plugins ..."
                echo "$(date '+%Y-%m-%d %H:%M:%S') *** START plugins build with BUILD_CONFIG=STABLE" >> "$PLOG"
                env BUILD_CONFIG=STABLE BUILD_NUMBER=1 QID="${QUEUE_ID}" PIDFILE="$PID_PLUGINS" bash "$INC_PATH/scripts/build-plugins.sh" >> "$PLOG" 2>&1 || FAIL=$?
                echo "$(date '+%Y-%m-%d %H:%M:%S') *** END plugins build with BUILD_CONFIG=STABLE -> $FAIL" >> "$PLOG"
            fi

            # update status
            #
            if [ "$FAIL" != "0" ]; then
                updateStatus "$QUEUE_ID" 3
                FAILED_WHAT="plugins!"
                if [ "$REPO" != "Natron" ] && [ "$REPO" != "$GIT_NATRON_REPO_LABEL" ] && [ "$REPO" != "$GIT_NATRON_GFORGE_REPO_LABEL" ]; then
                    FAILED_WHAT=$REPO
                fi
                updateStatusLog "$QUEUE_ID" "Failed to build $FAILED_WHAT"
                #updateLog "$QUEUE_ID $QOS: Failed to build $FAILED_WHAT!"
                #buildFailed ${QUEUE_ID} $QOS
            else
                updateStatusLog "$QUEUE_ID" "Building plugins ... OK!"
            fi
        fi

        # build natron
        #
        if [ "$FAIL" = "0" ] && ([ "$REPO" = "Natron" ] || [ "$REPO" = "$GIT_NATRON_REPO_LABEL" ] || [ "$REPO" = "$GIT_NATRON_GFORGE_REPO_LABEL" ]); then
            if [ "$TYPE" = "CI" ] || [ "$TYPE" = "SNAPSHOT" ]; then
                updateStatusLog "$QUEUE_ID" "Building Natron ..."
                echo "$(date '+%Y-%m-%d %H:%M:%S') *** START natron build with GIT_BRANCH=$BRANCH GIT_COMMIT=$COMMIT BUILD_CONFIG=SNAPSHOT DEBUG_MODE=$DEBUG_BUILD" >> "$NLOG"
                env GIT_BRANCH="$BRANCH" GIT_COMMIT="$COMMIT" BUILD_CONFIG=SNAPSHOT DEBUG_MODE="$DEBUG_BUILD" QID="${QUEUE_ID}" PIDFILE="$PID_NATRON" bash "$INC_PATH/scripts/build-natron.sh" >> "$NLOG" 2>&1 || FAIL=$?
                echo "$(date '+%Y-%m-%d %H:%M:%S') *** END natron build with GIT_BRANCH=$BRANCH GIT_COMMIT=$COMMIT BUILD_CONFIG=SNAPSHOT DEBUG_MODE=$DEBUG_BUILD -> $FAIL" >> "$NLOG"
            elif [ "$TYPE" = "RELEASE" ]; then
                updateStatusLog "$QUEUE_ID" "Building Natron ..."
                echo "$(date '+%Y-%m-%d %H:%M:%S') *** START natron build with BUILD_CONFIG=STABLE" >> "$NLOG"
                env BUILD_CONFIG=STABLE BUILD_NUMBER=1 QID="${QUEUE_ID}" PIDFILE="$PID_NATRON" bash "$INC_PATH/scripts/build-natron.sh" >> "$NLOG" 2>&1 || FAIL=$?
                echo "$(date '+%Y-%m-%d %H:%M:%S') *** END natron build with BUILD_CONFIG=STABLE -> $FAIL" >> "$NLOG"
            fi
            if [ "$FAIL" != "0" ]; then
                updateStatus "$QUEUE_ID" 3
                updateStatusLog "$QUEUE_ID" "Failed to build Natron!"
                #updateLog "$QUEUE_ID $QOS: Failed to build Natron!"
                #buildFailed ${QUEUE_ID} $QOS
            else
                updateStatusLog "$QUEUE_ID" "Building Natron ... OK!"
            fi
        fi

        # build installer(s)
        #
        if [ "$FAIL" = "0" ] && ([ "$REPO" = "Natron" ] || [ "$REPO" = "$GIT_NATRON_REPO_LABEL" ] || [ "$REPO" = "$GIT_NATRON_GFORGE_REPO_LABEL" ]); then
            updateStatusLog "$QUEUE_ID" "Building installer(s) ..."
            if [ "$TYPE" = "CI" ]; then
                echo "$(date '+%Y-%m-%d %H:%M:%S') *** START installer build with GIT_BRANCH=$BRANCH BUILD_CONFIG=SNAPSHOT DEBUG_MODE=$DEBUG_BUILD" >> "$ILOG"
                env GIT_BRANCH="$BRANCH" BUILD_CONFIG=SNAPSHOT DEBUG_MODE="$DEBUG_BUILD" QID="${QUEUE_ID}" PIDFILE="$PID_INSTALLER" bash "$INC_PATH/scripts/build-${PKGOS}-installer.sh" >> "$ILOG" 2>&1 || FAIL=$?
                echo "$(date '+%Y-%m-%d %H:%M:%S') *** END installer build with GIT_BRANCH=$BRANCH BUILD_CONFIG=SNAPSHOT DEBUG_MODE=$DEBUG_BUILD -> $FAIL" >> "$ILOG"
            elif [ "$TYPE" = "SNAPSHOT" ]; then
                echo "$(date '+%Y-%m-%d %H:%M:%S') *** START installer build with GIT_BRANCH=$BRANCH TAR_BUILD=1 OFFLINE=1 BUILD_CONFIG=SNAPSHOT" >> "$ILOG"
                env GIT_BRANCH="$BRANCH" TAR_BUILD=1 OFFLINE=1 BUILD_CONFIG=SNAPSHOT QID="${QUEUE_ID}" PIDFILE="$PID_INSTALLER" bash "$INC_PATH/scripts/build-${PKGOS}-installer.sh" >> "$ILOG" 2>&1 || FAIL=$?
                echo "$(date '+%Y-%m-%d %H:%M:%S') *** END installer build with GIT_BRANCH=$BRANCH TAR_BUILD=1 OFFLINE=1 BUILD_CONFIG=SNAPSHOT -> $FAIL" >> "$ILOG"
            elif [ "$TYPE" = "RELEASE" ]; then
                RPM_BUILD=0
                if $(gpg --list-keys | fgrep build@natron.fr > /dev/null); then
                    RPM_BUILD=1
                fi
                echo "$(date '+%Y-%m-%d %H:%M:%S') *** START installer build with TAR_BUILD=1 DEB_BUILD=1 RPM_BUILD=$RPM_BUILD OFFLINE=1 BUILD_CONFIG=STABLE BUILD_NUMBER=1" >> "$ILOG"
                if [ $RPM_BUILD -eq 0 ]; then
                    echo "Warning: RPM build disabled, because the GnuPG key build@natron.fr was lost"
                fi
                env TAR_BUILD=1 DEB_BUILD=1 RPM_BUILD="$RPM_BUILD" OFFLINE=1 BUILD_CONFIG=STABLE BUILD_NUMBER=1 QID="${QUEUE_ID}" PIDFILE="$PID_INSTALLER" bash "$INC_PATH/scripts/build-${PKGOS}-installer.sh" >> "$ILOG" 2>&1 || FAIL=$?
                echo "$(date '+%Y-%m-%d %H:%M:%S') *** END installer build with TAR_BUILD=1 DEB_BUILD=1 RPM_BUILD=1 OFFLINE=1 BUILD_CONFIG=STABLE BUILD_NUMBER=1 -> $FAIL" >> "$ILOG"
            fi
            if [ "$FAIL" != "0" ]; then
                updateStatus "$QUEUE_ID" 3
                updateStatusLog "$QUEUE_ID" "Failed to build installer(s)!"
                #updateLog "$QUEUE_ID $QOS: Failed to build installer(s)!"
                #buildFailed ${QUEUE_ID} $QOS
            else
                updateStatusLog "$QUEUE_ID" "Building installer(s) ... OK!"
            fi
        fi

        # run unit tests
        # TODO: move to include/scripts/unit-test.sh
        if [ "$FAIL" = "0" ] && ([ "$REPO" = "Natron" ] || [ "$REPO" = "$GIT_NATRON_REPO_LABEL" ]  || [ "$REPO" = "$GIT_NATRON_GFORGE_REPO_LABEL" ]) && [ "$RUN_UNIT_TESTS" = 1 ]; then #  && [ "$BRANCH" != "master" ]
            updateStatusLog "$QUEUE_ID" "Updating unit tests ..."
            echo "$(date '+%Y-%m-%d %H:%M:%S') *** START updating unit tests" >> "$ILOG"

            TESTDIR="Natron-Tests${BITS}"
            if [ "$PKGOS" = "Windows" ] && [ "${BITS}" = "64" ]; then
                # sometimes NatronRenderer just hangs. Try taskkill first, then tskill if it fails because of a message like:
                # $ taskkill -f -im NatronRenderer-bin.exe -t
                # ERROR: The process with PID 3260 (child process of PID 3816) could not be terminated.
                # Reason: There is no running instance of the task.
                # mapfile use: see https://github.com/koalaman/shellcheck/wiki/SC2207
                mapfile -t processes < <(taskkill -f -im NatronRenderer-bin.exe -t 2>&1 |grep "ERROR: The process with PID"| awk '{print $6}' || true)
                for p in "${processes[@]}"; do
                    tskill "$p" || true
                done
            fi
            # cleanup tests (TEMPORARY) the following lines should be commented most of the time
            if [ -d "$CWD/$TESTDIR" ]; then
                echo "$(date '+%Y-%m-%d %H:%M:%S') *** CLEAN unit tests - please disable in the script" >> "$ILOG"
                echo "$(date '+%Y-%m-%d %H:%M:%S') *** CLEAN unit tests - please disable in the script" >> "$ULOG"
                rm -rf "${CWD:?}/$TESTDIR"
            fi
            
            if [ ! -d "$CWD/$TESTDIR" ]; then
                cd "$CWD" && $TIMEOUT 1800 $GIT clone "$GIT_UNIT" "$TESTDIR" && cd "$TESTDIR" || FAIL=$?
            else
                cd "$CWD/$TESTDIR" && $TIMEOUT 1800 $GIT pull --rebase --autostash || FAIL=$?
            fi
            if [ "$FAIL" != "0" ]; then
                echo "$(date '+%Y-%m-%d %H:%M:%S') *** END updating unit tests -> FAIL" >> "$ILOG"
                updateStatus "$QUEUE_ID" 3
                updateStatusLog "$QUEUE_ID" "Failed to update unit tests!"
            else
                echo "$(date '+%Y-%m-%d %H:%M:%S') *** END updating unit tests -> OK" >> "$ILOG"
                updateStatusLog "$QUEUE_ID" "Updating unit tests ... OK!"
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
                fi
                UNIT_TMP="$CWD"/unit_tmp_${BITS}
                if [ -d "$UNIT_TMP" ]; then
                    rm -rf "$UNIT_TMP"
                fi
                mkdir -p "$UNIT_TMP"
                    
                if [ "$PKGOS" = "Linux" ] && [ "${BITS}" = "64" ]; then
                #if [ "$PKGOS" = "Linux" ]; then
                    updateStatusLog "$QUEUE_ID" "Running unit tests ..."
                    if  [ "${BITS}" = "64" ]; then
                        l=64
                    else
                        l=""
                    fi
                    export FONTCONFIG_PATH="$TMP_BINARIES_PATH/Resources/etc/fonts/fonts.conf" # use the fonts.conf shipped with Natron, not $SDK_HOME/etc/fonts/fonts.conf
                    export LD_LIBRARY_PATH="${SDK_HOME}/gcc/lib${l}:${SDK_HOME}/lib:${FFMPEG_PATH}/lib:${LIBRAW_PATH}/lib"
                    export CPATH="${SDK_HOME}/gcc/include:${SDK_HOME}/include:${FFMPEG_PATH}/include:${LIBRAW_PATH}/include"
                    export PATH="${SDK_HOME}/bin:$PATH"
                    rm -rf ~/.cache/INRIA/Natron || true
                    mkdir -p ~/.cache/INRIA/Natron/{ViewerCache,DiskCache} || true
                    echo "$(date '+%Y-%m-%d %H:%M:%S') *** START unit tests" >> "$ILOG"
                    echo "$(date '+%Y-%m-%d %H:%M:%S') *** START unit tests" >> "$ULOG"
                    ocio="$TMP_BINARIES_PATH/Resources/OpenColorIO-Configs/blender/config.ocio"
                     if [ ! -f "$ocio" ]; then
                        echo "*** Error: OCIO file $ocio is missing" >> "$ULOG"
                    fi
                   env NATRON_PLUGIN_PATH="$TMP_BINARIES_PATH/PyPlugs" OFX_PLUGIN_PATH="$TMP_BINARIES_PATH/OFX/Plugins" OCIO="$ocio" FFMPEG="$FFMPEG_PATH/bin/ffmpeg" COMPARE="$SDK_HOME/bin/idiff"  $TIMEOUT -s KILL 7200 bash runTests.sh "$TMP_BINARIES_PATH/bin/NatronRenderer" >> "$ULOG" 2>&1 || FAIL=$?
                    echo "$(date '+%Y-%m-%d %H:%M:%S') *** END unit tests -> $FAIL" >> "$ILOG"
                    echo "$(date '+%Y-%m-%d %H:%M:%S') *** END unit tests -> $FAIL" >> "$ULOG"
                    FAIL=0
                #elif [ "$PKGOS" = "Windows" ] && [ "${BITS}" = "64" ]; then
                elif [ "$PKGOS" = "Windows" ] && [ "${BITS}" = "64" ]; then
                    updateStatusLog "$QUEUE_ID" "Running unit tests ..."
                    cp -a "$TMP_PATH"/Natron-installer/packages/*/data/* "$UNIT_TMP"/ 
                    export FONTCONFIG_PATH="$UNIT_TMP/Resources/etc/fonts/fonts.conf"
                    rm -rf "$LOCALAPPDATA\\INRIA\\Natron" || true
                    mkdir -p "$LOCALAPPDATA\\INRIA\\Natron\\cache\\"{ViewerCache,DiskCache} || true
                    echo "$(date '+%Y-%m-%d %H:%M:%S') *** START unit tests" >> "$ILOG"
                    echo "$(date '+%Y-%m-%d %H:%M:%S') *** START unit tests" >> "$ULOG"
                    # idiff needs libraw dll which is not in the same dir as other dlls
                    if [ ! -f "${TMP_BINARIES_PATH}/bin/libraw_r-16.dll" ]; then
                        cp "${LIBRAW_PATH}/bin/libraw_r-16.dll" "${TMP_BINARIES_PATH}/bin/"
                    fi
                    #echo "*** Debug:"
                    #ls -l "$TMP_BINARIES_PATH" "$TMP_BINARIES_PATH/Resources" "$TMP_BINARIES_PATH/Resources/OpenColorIO-Configs"
                    ocio="$TMP_BINARIES_PATH/Resources/OpenColorIO-Configs/blender/config.ocio"
                    if [ ! -f "$ocio" ]; then
                        echo "*** Error: OCIO file $ocio is missing" >> "$ULOG"
                    fi
                    env PATH="$UNIT_TMP/bin:${TMP_BINARIES_PATH}/bin:${FFMPEG_PATH}/bin:${LIBRAW_PATH}/bin:$PATH" NATRON_PLUGIN_PATH="$TMP_BINARIES_PATH/PyPlugs" OFX_PLUGIN_PATH="$TMP_BINARIES_PATH/OFX/Plugins" OCIO="$ocio" FFMPEG="$FFMPEG_PATH/bin/ffmpeg.exe" COMPARE="${SDK_HOME}/bin/idiff.exe" $TIMEOUT -s KILL 7200 bash runTests.sh "$UNIT_TMP/bin/NatronRenderer.exe" >> "$ULOG" 2>&1 || FAIL=$?
                    echo "$(date '+%Y-%m-%d %H:%M:%S') *** END unit tests -> $FAIL" >> "$ILOG"
                    echo "$(date '+%Y-%m-%d %H:%M:%S') *** END unit tests -> $FAIL" >> "$ULOG"
                    # sometimes NatronRenderer just hangs. Try taskkill first, then tskill if it fails because of a message like:
                    # $ taskkill -f -im NatronRenderer-bin.exe -t
                    # ERROR: The process with PID 3260 (child process of PID 3816) could not be terminated.
                    # Reason: There is no running instance of the task.
                    # mapfile use: see https://github.com/koalaman/shellcheck/wiki/SC2207
                    mapfile -t processes < <(taskkill -f -im NatronRenderer-bin.exe -t 2>&1 |grep "ERROR: The process with PID"| awk '{print $6}' || true)
                    for p in "${processes[@]}"; do
                        tskill "$p" || true
                    done
                    FAIL=0
                elif [ "$PKGOS" = "OSX" ]; then
                    updateStatusLog "$QUEUE_ID" "Running unit tests ..."
                    if [ "$MACOSX_DEPLOYMENT_TARGET" = "10.6" ] && [ "$COMPILER" = "gcc" ]; then
                        export DYLD_LIBRARY_PATH="$MACPORTS"/lib/libgcc:/System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/ImageIO.framework/Versions/A/Resources:"$MACPORTS"/lib
                    fi
                    rm -rf "$HOME/Library/Caches/INRIA/Natron" || true
                    mkdir -p "$HOME/Library/Caches/INRIA/Natron"/{ViewerCache,DiskCache} || true
                    echo "$(date '+%Y-%m-%d %H:%M:%S') *** START unit tests" >> "$ILOG"
                    echo "$(date '+%Y-%m-%d %H:%M:%S') *** START unit tests" >> "$ULOG"
                    ocio="$TMP_BINARIES_PATH/Natron.app/Contents/Resources/OpenColorIO-Configs/blender/config.ocio"
                    if [ ! -f "$ocio" ]; then
                        echo "*** Error: OCIO file $ocio is missing" >> "$ULOG"
                    fi
                    env NATRON_PLUGIN_PATH="$TMP_BINARIES_PATH/Natron.app/Contents/Plugins/PyPlugs" OFX_PLUGIN_PATH="$TMP_BINARIES_PATH/Natron.app/Contents/Plugins/OFX" OCIO="$ocio" FFMPEG="$FFMPEG_PATH/bin/ffmpeg" COMPARE="idiff" $TIMEOUT -s KILL 7200 bash runTests.sh "$TMP_BINARIES_PATH/Natron.app/Contents/MacOS/NatronRenderer-bin" >> "$ULOG" 2>&1 || FAIL=$?
                    echo "$(date '+%Y-%m-%d %H:%M:%S') *** END unit tests -> $FAIL" >> "$ILOG"
                    echo "$(date '+%Y-%m-%d %H:%M:%S') *** END unit tests -> $FAIL" >> "$ULOG"
                    FAIL=0
                fi
                rm -rf "$UNIT_TMP"
                # collect result
                if [ -f "result.txt" ]; then
                    mv result.txt "$RULOG"
                fi
                if [ -d "failed" ]; then
                    mv failed "$UDIR"
                fi
            fi
            cd "$CWD"
        fi

        # prep for sync
        #
        if [ "$FAIL" = "0" ] && ([ "$REPO" = "Natron" ] || [ "$REPO" = "$GIT_NATRON_REPO_LABEL" ] || [ "$REPO" = "$GIT_NATRON_GFORGE_REPO_LABEL" ]); then
            if [ "$TYPE" = "RELEASE" ]; then
                #
                # releases are put in a staging area until they are published (controlled from the build queue on natron.fr)
                #
                # buildmaster@server:staging/ID/
                # ├── files
                # │   ├── Linux
                # │   │   ├── 32 (if publish, files will be moved to buildmaster@server:files/Linux/32/releases/)
                # │   │   └── 64 (if publish, files will be moved to buildmaster@server:files/Linux/64/releases/)
                # │   ├── OSX
                # │   │   └── Universal (if publish, files will be moved to buildmaster@server:files/OSX/Universal/releases/)
                # │   └── Windows
                # │       ├── 32 (if publish, files will be moved to buildmaster@server:files/Windows/32/releases/)
                # │       └── 64 (if publish, files will be moved to buildmaster@server:files/Windows/64/releases/)
                # ├── packages
                # │   ├── Linux
                # │   │   ├── 32 (if publish, files will be moved to buildmaster@server:packages/Linux/32/releases/BRANCH)
                # │   │   └── 64 (if publish, files will be moved to buildmaster@server:packages/Linux/64/releases/BRANCH)
                # │   └── Windows
                # │       ├── 32 (if publish, files will be moved to buildmaster@server:packages/Windows/32/releases/BRANCH)
                # │       └── 64 (if publish, files will be moved to buildmaster@server:packages/Windows/64/releases/BRANCH)
                # └── symbols (if publish, files will be moved to buildmaster@server:symbols/)
                #
                QUEUE_PREFIX="staging/${QUEUE_ID}"
                QUEUE_FILES_UPLOAD_PATH="${QUEUE_PREFIX}/files/${PKGOS}/${BITS}"
                QUEUE_PACKAGES_UPLOAD_PATH="${QUEUE_PREFIX}/packages/${PKGOS}/${BITS}"
                QUEUE_SYMBOLS_UPLOAD_PATH="${QUEUE_PREFIX}/symbols"
                ssh "$QUEUE_URL" "mkdir -p '${QUEUE_PREFIX}/symbols' '${QUEUE_PREFIX}/files/${PKGOS}/${BITS}' '${QUEUE_PREFIX}/packages/${PKGOS}/${BITS}' && chmod 775 -R '${QUEUE_PREFIX}'" || true
            elif [ "$TYPE" = "SNAPSHOT" ]; then
                #
                # snapshots are published directly, we should probably also put these in a staging area until all platforms has been uploaded, then auto publish(?)
                #
                # buildmaster@server
                # ├── files
                # │   ├── Linux
                # │   │   ├── 32
                # │   │   │   └── snapshots [ sub-folder for each branch ]
                # │   │   └── 64
                # │   │       └── snapshots [ sub-folder for each branch ]
                # │   ├── OSX
                # │   │   └── Universal
                # │   │       └── snapshots [ sub-folder for each branch]
                # │   └── Windows
                # │       ├── 32
                # │       │   └── snapshots [ sub-folder for each branch ]
                # │       └── 64
                # │           └── snapshots [ sub-folder for each branch ]
                # ├── packages
                # │   ├── Linux
                # │   │   ├── 32
                # │   │   │   └── snapshots [ sub-folder for each branch ]
                # │   │   └── 64
                # │   │       └── snapshots [ sub-folder for each branch ]
                # │   └── Windows
                # │       ├── 32
                # │       │   └── snapshots [ sub-folder for each branch ]
                # │       └── 64
                # │           └── snapshots [ sub-folder for each branch ]
                # └── symbols
                #
                QUEUE_FILES_UPLOAD_PATH="files/${PKGOS}/${BITS}/snapshots/${BRANCH}"
                QUEUE_PACKAGES_UPLOAD_PATH="packages/${PKGOS}/${BITS}/snapshots/${BRANCH}"
                ssh "$QUEUE_URL" "mkdir -p {files,packages}/'${PKGOS}/${BITS}/snapshots/${BRANCH}' && chmod 775 -R {files,packages}/'${PKGOS}/${BITS}/snapshots/${BRANCH}'" || true
            fi
        fi

        # sync files
        #
        if [ "$FAIL" = "0" ] && ([ "$REPO" = "Natron" ] || [ "$REPO" = "$GIT_NATRON_REPO_LABEL" ]); then # do not upload gforge builds
            if [ "$TYPE" = "RELEASE" ] || [ "$TYPE" = "SNAPSHOT" ]; then
                updateStatusLog "$QUEUE_ID" "Sync files ..."
                if [ "$PKGOS" != "OSX" ]; then
                    rsync_remote "$QUEUE_TMP_PATH${QUEUE_ID}_${BITS}/installers/" "$QUEUE_FILES_UPLOAD_PATH/" >& "$SLOG2" || FAIL=$?
                    if [ "$FAIL" = "0" ]; then
                        rsync_remote "$QUEUE_TMP_PATH${QUEUE_ID}_${BITS}/packages/" "$QUEUE_PACKAGES_UPLOAD_PATH/" --delete >& "$SLOG1" || FAIL=$?
                    fi
                else
                    rsync_remote "$TMP_BINARIES_PATH/files/" "$QUEUE_FILES_UPLOAD_PATH/" >& "$SLOG2" || FAIL=$?
                    if [ "$FAIL" = "0" ]; then
                        touch "$SLOG1"
                    fi
                fi
                if [ "$FAIL" = "0" ]; then
                    rsync_remote "$TMP_BINARIES_PATH/symbols/" "$QUEUE_SYMBOLS_UPLOAD_PATH/" >& "$SLOG3" || FAIL=$?
                fi
            fi
            if [ "$FAIL" != "0" ]; then
                updateStatus "$QUEUE_ID" 3
                updateStatusLog "$QUEUE_ID" "Failed to sync files!"
                #updateLog "$QUEUE_ID $QOS: Failed to sync files!"
                #buildFailed ${QUEUE_ID} $QOS
            else
                updateStatusLog "$QUEUE_ID" "Sync files ... OK!"
            fi
        fi

        # update status
        #
        if [ "$FAIL" = "0" ]; then
            updateStatus "$QUEUE_ID" 2
            updateStatusLog "$QUEUE_ID" "Complete"
            #updateLog "$QUEUE_ID $QOS: Build complete"
            #buildOK ${QUEUE_ID} $QOS
        fi

        # kill watchdog
        #
        kill "$WATCHDOG_PID" || true
        sleep 2
        kill -9 "$WATCHDOG_PID" || true

        # sync final log
        #
        cat "$PLOG" "$NLOG" "$ILOG" "$SLOG1" "$SLOG2" "$SLOG3" > "$BLOG" 2>&1 || true
        if [ -f "$BLOG".gz ]; then
            rm -f "$BLOG".gz
        fi
        gzip -9 -f "$BLOG"
        if [ -f "$ULOG" ]; then
            if [ -f "$ULOG".gz ]; then
                rm -f "$ULOG".gz
            fi
            gzip -9 -f "$ULOG"
            rsync_remote "$ULOG".gz "$QUEUE_LOG_UPLOAD_PATH" > /dev/null 2>&1 || true
        fi
        if [ -f "$RULOG" ]; then
            rsync_remote "$RULOG" "$QUEUE_LOG_UPLOAD_PATH" > /dev/null 2>&1 || true
        fi
        rsync_remote "$BLOG".gz "$QUEUE_LOG_UPLOAD_PATH/" > /dev/null 2>&1 || true
        if [ -d "$UDIR" ]; then
            rsync_remote "$UDIR" "$QUEUE_LOG_UPLOAD_PATH/" > /dev/null 2>&1 || true
        fi
        rm -f "$PID_NATRON" "$PID_PLUGINS" "$PID_INSTALLER" "$PLOG" "$NLOG" "$ILOG" "$SLOG1" "$SLOG2" "$SLOG3" "$BLOG" "$ULOG" "$RULOG" || true

        # limit log dir size to 10 Megs
        "$INC_PATH/scripts/limitdirsize.sh" "$QUEUE_LOG_PATH" 10

        # clean up
        rm -rf "$TMP_BINARIES_PATH" || true
        rm -rf "$TMPDIR"/*.ntp || true # remains from projectconverter
        "$INC_PATH/scripts/cleantmp.sh" > /dev/null 2>&1
        
    fi # END OF VALID JOB

    # cleanup
    #mv "$TMP_PATH" "${TMP_PATH}.$$"
    rm -rf "${TMP_PATH}" || true

    rm -f "$QUEUE" || true
    rm -f "$CONFIG" || true

    if [ -n "${SERVERNAME+x}" ]; then
        pingServer "$SERVERNAME" 0
    fi

    echo "Idle ..."
    if [ -z "$TYPE" ]; then
        sleep 60
    else
        sleep 10
    fi

done # END OF MAIN LOOP

# Restart script on changes
if [ "$RESTART" = 1 ]; then
    echo "===> Restart buildmaster ..."
    exec "$0"
fi

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
