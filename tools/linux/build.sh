#!/bin/sh
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2015 INRIA and Alexandre Gauthier
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
# OFFLINE_INSTALLER=1: Build offline installer in addition to the online installer
# SNAPSHOT=1 : Tag build as snapshot
# TARSRC=1 : tar sources
# NATRON_LICENSE=(GPL,COMMERCIAL)

# USAGE example: NATRON_LICENSE=GPL build2.sh workshop<branch> 8<noThreads>

source `pwd`/common.sh || exit 1

PID=$$
if [ -f $TMP_DIR/natron-build.pid ]; then
    OLDPID=`cat $TMP_DIR/natron-build.pid`
    PIDS=`ps aux|awk '{print $2}'`
    for i in $PIDS;do
        if [ "$i" = "$OLDPID" ]; then
            echo "already running ..."
            exit 1
        fi
    done
fi
echo $PID > $TMP_DIR/natron-build.pid || exit 1

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
    if [ "$ONLY_PLUGINS" != "1" ]; then
        log="$LOGS/natron.$PKGOS$BIT.$TAG.log"
        echo -n "Building Natron (log in $log)..."
        env MKJOBS=$JOBS MKSRC=${TARSRC} BUILD_SNAPSHOT=${SNAPSHOT} sh "$INC_PATH/scripts/build-natron.sh" $BRANCH >& "$log" || FAIL=1
        if [ "$FAIL" != "1" ]; then
            echo OK
        else
            echo ERROR
            sleep 2
            cat "$log"
        fi
    fi
    if [ "$FAIL" != "1" -a "$ONLY_NATRON" != "1" ]; then
        log="$LOGS/plugins.$PKGOS$BIT.$TAG.log"
        echo -n "Building Plugins (log in $log)..."
        env MKJOBS=$JOBS MKSRC=${TARSRC} BUILD_CV=$CV BUILD_IO=$IO BUILD_MISC=$MISC BUILD_ARENA=$ARENA sh "$INC_PATH/scripts/build-plugins.sh" $BRANCH >& "$log" || FAIL=1
        if [ "$FAIL" != "1" ]; then
            echo OK
        else
            echo ERROR
            sleep 2
            cat 
        fi  
    fi
fi

if [ "$NOPKG" != "1" -a "$FAIL" != "1" ]; then
    log="$LOGS/installer.$PKGOS$BIT.$TAG.log"
    echo -n "Building Packages (log in $log)... "
    env OFFLINE=${OFFLINE_INSTALLER} NOTGZ=1 sh "$INC_PATH/scripts/build-installer.sh" $BRANCH >& "$log" || FAIL=1
    if [ "$FAIL" != "1" ]; then
        echo OK
    else
        echo ERROR
        sleep 2
        cat "$log"
    fi 
fi

if [ "$BRANCH" = "workshop" ]; then
    ONLINE_REPO_BRANCH=snapshots
else
    ONLINE_REPO_BRANCH=releases
fi

BIT_SUFFIX=bit
BIT_TAG="$BIT$BIT_SUFFIX"

if [ "$SYNC" = "1" -a "$FAIL" != "1" ]; then
    echo "Syncing packages ... "
    rsync -avz --progress --delete --verbose -e ssh "$REPO_DIR/packages/" "$REPO_DEST/$PKGOS/$ONLINE_REPO_BRANCH/$BIT_TAG/packages"

    rsync -avz --progress  --verbose -e ssh "$REPO_DIR/installers/" "$REPO_DEST/$PKGOS/$ONLINE_REPO_BRANCH/$BIT_TAG/files"
fi

#Always upload logs, even upon failure
rsync -avz --progress --delete --verbose -e ssh "$LOGS/" "$REPO_DEST/$PKGOS/$ONLINE_REPO_BRANCH/$BIT_TAG/logs"

if [ "$FAIL" = "1" ]; then
    exit 1
else
    exit 0
fi
