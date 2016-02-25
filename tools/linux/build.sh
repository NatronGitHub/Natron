#!/bin/sh
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2016 INRIA and Alexandre Gauthier
#
# Natron is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Natron is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****

#
# Build and package Natron for Linux
#
# (options) sh build.sh (threads, 4 is default)
# 
# Options (optional):
# NOPKG=1 : Don't build installer/repo
# NOCLEAN=1 : Don't remove sdk installation
# REBUILD_SDK=1 : Trigger rebuild of sdk
# NOBUILD=1 : Don't build anything
# SYNC=1 : Sync files with server
# ONLY_NATRON=1 : Don't build plugins
# ONLY_PLUGINS=1 : Don't build natron
# IO=1 : Enable io plug
# MISC=1 : Enable misc plug
# ARENA=1 : Enable arena plug
# CV=1 : Enable cv plug
# DISABLE_BREAKPAD=1: Disable automatic crash report
# OFFLINE_INSTALLER=1: Build offline installer in addition to the online installer
# BUILD_CONFIG=(SNAPSHOT,ALPHA,BETA,RC,STABLE,CUSTOM)
# CUSTOM_BUILD_USER_NAME="Toto" : to be set if BUILD_CONFIG=CUSTOM
# BUILD_NUMBER=X: To be set to indicate the revision number of the build. For example RC1,RC2, RC3 etc...
# TARSRC=1 : tar sources
# NATRON_LICENSE=(GPL,COMMERCIAL)
# UNIT_TESTS=1 : do unit tests

# USAGE example: BUILD_CONFIG=STABLE BUILD_NUMBER=1 NATRON_LICENSE=GPL BUILD_CONFIG=SNAPSHOT build2.sh workshop<branch> 8<noThreads>

source `pwd`/common.sh || exit 1

PID=$$
# make kill bot
KILLSCRIPT="/tmp/killbot$$.sh"
cat << 'EOF' > "$KILLSCRIPT"
#!/bin/sh
PARENT=$1
sleep 30m
if [ "$PARENT" = "" ]; then
  exit 1
fi
PIDS=`ps aux|awk '{print $2}'|grep $PARENT`
if [ "$PIDS" = "$PARENT" ]; then
  kill -15 $PARENT
fi
EOF
chmod +x $KILLSCRIPT

if [ "$OS" = "GNU/Linux" ]; then
    PKGOS=Linux
else
    echo "Linux-only!"
    exit 1
fi

if [ "$1" = "workshop" ]; then
    BRANCH=$1
    REPO_SUFFIX=snapshot
else
    REPO_SUFFIX=release
fi

if [ "$2" != "" ]; then
    JOBS=$2
else
    #Default to 4 threads
    JOBS=$DEFAULT_MKJOBS
fi

if [ "$NATRON_LICENSE" != "GPL" -a "$NATRON_LICENSE" != "COMMERCIAL" ]; then
    echo "Please select a License with NATRON_LICENSE=(GPL,COMMERCIAL)"
    exit 1
fi

if [ -z "$BUILD_CONFIG" ]; then
	echo "You must select a BUILD_CONFIG".
	exit 1
fi

if [ "$NOCLEAN" != "1" ]; then
    rm -rf $INSTALL_PATH
fi
if [ "$REBUILD_SDK" = "1" ]; then
    rm -f "$SRC_PATH"/Natron*SDK.tar.xz
fi

if [ -z "$IO" ]; then
    IO=1
fi
if [ -z "$MISC" ]; then
    MISC=1
fi
if [ -z "$ARENA" ]; then
    ARENA=1
fi
if [ -z "$CV" ]; then
    CV=1
fi
if [ -z "$OFFLINE_INSTALLER" ]; then
    OFFLINE_INSTALLER=1
fi

REPO_DIR="$REPO_DIR_PREFIX$REPO_SUFFIX"

LOGS="$REPO_DIR/logs"
rm -rf "$LOGS"
if [ ! -d "$LOGS" ]; then
    mkdir -p "$LOGS" || exit 1
fi

FAIL=0

if [ ! -f "$CWD/commits-hash.sh" ]; then
    cat <<EOF > "${CWD}/commits-hash.sh"
#!/bin/sh
NATRON_DEVEL_GIT=#
IOPLUG_DEVEL_GIT=#
MISCPLUG_DEVEL_GIT=#
ARENAPLUG_DEVEL_GIT=#
CVPLUG_DEVEL_GIT=#
NATRON_VERSION_NUMBER=#
EOF
fi


if [ "$NOBUILD" != "1" ]; then
    if [ "$ONLY_NATRON" != "1" ]; then
        log="$LOGS/plugins.$PKGOS$BIT.$TAG.log"
        echo -n "Building Plugins (log in $log)..."
        env MKJOBS=$JOBS MKSRC=${TARSRC} BUILD_CONFIG=${BUILD_CONFIG} CUSTOM_BUILD_USER_NAME=${CUSTOM_BUILD_USER_NAME} BUILD_NUMBER=$BUILD_NUMBER BUILD_CV=$CV BUILD_IO=$IO BUILD_MISC=$MISC BUILD_ARENA=$ARENA sh "$INC_PATH/scripts/build-plugins.sh" $BRANCH >& "$log" || FAIL=1
        if [ "$FAIL" != "1" ]; then
            echo OK
        else
            echo ERROR
            echo "BUILD__ERROR" >> $log
            cat "$log"
        fi
    fi
    if [ "$FAIL" != "1" -a "$ONLY_PLUGINS" != "1" ]; then
        log="$LOGS/natron.$PKGOS$BIT.$TAG.log"
        echo -n "Building Natron (log in $log)..."
        env MKJOBS=$JOBS MKSRC=${TARSRC} BUILD_CONFIG=${BUILD_CONFIG} CUSTOM_BUILD_USER_NAME=${CUSTOM_BUILD_USER_NAME} BUILD_NUMBER=$BUILD_NUMBER DISABLE_BREAKPAD=$DISABLE_BREAKPAD sh "$INC_PATH/scripts/build-natron.sh" $BRANCH >& "$log" || FAIL=1
        if [ "$FAIL" != "1" ]; then
            echo OK
        else
            echo ERROR
            echo "BUILD__ERROR" >> $log
            cat "$log"
        fi
    fi
fi

if [ "$NOPKG" != "1" -a "$FAIL" != "1" ]; then
    log="$LOGS/installer.$PKGOS$BIT.$TAG.log"
    echo -n "Building Packages (log in $log)... "
    env OFFLINE=${OFFLINE_INSTALLER} DISABLE_BREAKPAD=$DISABLE_BREAKPAD NOTGZ=1 sh "$INC_PATH/scripts/build-installer.sh" $BRANCH >& "$log" || FAIL=1
    if [ "$FAIL" != "1" ]; then
        echo OK
    else
        echo ERROR
        echo "BUILD__ERROR" >> $log
        cat "$log"
    fi 
fi

if [ "$UNIT_TESTS" = "1" ]; then
  log="$LOGS/unit_tests.$PKGOS$BIT.$TAG.log"
  echo "Running unit tests (log in $log) ..."
  if [ ! -d "$CWD/Natron-Tests" ]; then
    cd $CWD || exit 1
    git clone $GIT_UNIT || exit 1
    cd Natron-Tests || exit 1
  else
    cd $CWD/Natron-Tests || exit 1
    git pull
  fi
  sh runOnServer.sh >& "$log"
fi

if [ "$BRANCH" = "workshop" ]; then
    ONLINE_REPO_BRANCH=snapshots
else
    ONLINE_REPO_BRANCH=releases
fi

BIT_SUFFIX=bit
BIT_TAG="$BIT$BIT_SUFFIX"

$KILLSCRIPT $PID &
KILLBOT=$!

if [ "$SYNC" = "1" -a "$FAIL" != "1" ]; then
    echo "Syncing packages ... "
    rsync -avz --progress --delete --verbose -e ssh "$REPO_DIR/packages/" "$REPO_DEST/$PKGOS/$ONLINE_REPO_BRANCH/$BIT_TAG/packages"

    rsync -avz --progress  --verbose -e ssh "$REPO_DIR/installers/" "$REPO_DEST/$PKGOS/$ONLINE_REPO_BRANCH/$BIT_TAG/files" 

  # Symbols
  echo "sync symbols ..."
  rsync -avz --progress --verbose -e ssh "$INSTALL_PATH/symbols/" "${REPO_DEST}/symbols/"
fi

if [ "$NO_LOG_SYNC" != "1" ]; then

#Always upload logs, even upon failure
rsync -avz --progress --delete --verbose -e ssh "$LOGS/" "$REPO_DEST/$PKGOS/$ONLINE_REPO_BRANCH/$BIT_TAG/logs"

fi

kill -9 $KILLBOT

if [ "$FAIL" = "1" ]; then
    exit 1
else
    exit 0
fi

rm -f $KILLSCRIPT

