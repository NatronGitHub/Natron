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
# Autobuild for Natron
#

#Usage snapshot.sh noThreads
#Options:
#NO_LOOP=1: Only build once and then exits.

#Easier to debug
set -x

source `pwd`/common.sh || exit 1

PID=$$
if [ -f $TMP_DIR/natron-build-snapshot.pid ]; then
  OLDPID=`cat $TMP_DIR/natron-build-snapshot.pid`
  PIDS=`ps aux|awk '{print $2}'`
  for i in $PIDS;do
    if [ "$i" == "$OLDPID" ]; then
      echo "already running ..."
      exit 1
    fi
  done
fi
echo $PID > $TMP_DIR/natron-build-snapshot.pid || exit 1


CWD=`pwd`
TMP=$CWD/.autobuild

if [ ! -f $CWD/commits-hash.sh ]; then
    touch $CWD/commits-hash.sh
    echo "#!/bin/sh" >> $CWD/commits-hash.sh
    echo "NATRON_DEVEL_GIT=#" >> $CWD/commits-hash.sh
    echo "IOPLUG_DEVEL_GIT=#" >> $CWD/commits-hash.sh
    echo "MISCPLUG_DEVEL_GIT=#" >> $CWD/commits-hash.sh
    echo "ARENAPLUG_DEVEL_GIT=#" >> $CWD/commits-hash.sh
    echo "CVPLUG_DEVEL_GIT=#" >> $CWD/commits-hash.sh
    echo "NATRON_VERSION_NUMBER=#" >> $CWD/commits-hash.sh
fi

source $CWD/commits-hash.sh



REPO_SUFFIX=snapshot
LOGS=$REPO_DIR_PREFIX$REPO_SUFFIX/logs


if [ "$1" != "" ]; then
    JOBS=$1
else
    #Default to 4 threads
    JOBS=$DEFAULT_MKJOBS
fi

if [ ! -d $LOGS ]; then
  mkdir -p $LOGS || exit 1
fi
if [ ! -d $TMP ]; then
  mkdir -p $TMP || exit 1
fi
if [ ! -d $TMP/Natron ]; then
  cd $TMP || exit 1
  git clone $GIT_NATRON || exit 1
  cd Natron || exit 1
  git checkout workshop || exit 1
fi
if [ ! -d $TMP/openfx-io ]; then
  cd $TMP || exit 1
  git clone $GIT_IO || exit 1
fi
if [ ! -d $TMP/openfx-misc ]; then
  cd $TMP || exit 1
  git clone $GIT_MISC || exit 1
fi
if [ ! -d $TMP/openfx-arena ]; then
  cd $TMP || exit 1
  git clone $GIT_ARENA || exit 1
fi
#if [ ! -d $TMP/openfx-opencv ]; then
#  cd $TMP || exit 1
#  git clone $GIT_OPENCV || exit 1
#fi

FINISHED=0
while [ "$FINISHED" == "0" ]
do

source $CWD/common.sh
source $CWD/commits-hash.sh

#Sync all scripts except snapshot.sh
git pull origin workshop

FAIL=0
echo "Running ..."

BUILD_NATRON=0
cd $TMP/Natron 
git fetch --all || FAIL=1
git merge origin/workshop || FAIL=1
GITV_NATRON=`git log|head -1|awk '{print $2}'`
ORIG_NATRON=$NATRON_DEVEL_GIT
echo "Natron $GITV_NATRON vs. $ORIG_NATRON"
if [ "$GITV_NATRON" != "$ORIG_NATRON" ] && [ "$FAIL" != "1" ]; then
  echo "Natron update needed"
  BUILD_NATRON=1
fi
BUILD_IO=0
if [ "$FAIL" != "1" ]; then
  cd $TMP/openfx-io
  git fetch --all || FAIL=1
  git merge origin/master || FAIL=1
  GITV_IO=`git log|head -1|awk '{print $2}'`
  ORIG_IO=$IOPLUG_DEVEL_GIT
  echo "IO $GITV_IO vs. $ORIG_IO"
  if [ "$GITV_IO" != "$ORIG_IO" ] && [ "$FAIL" != "1" ]; then
    echo "IO update needed"
    BUILD_IO=1
  fi
fi
BUILD_MISC=0
if [ "$FAIL" != "1" ]; then
  cd $TMP/openfx-misc
  git fetch --all || FAIL=1
  git merge origin/master || FAIL=1
  GITV_MISC=`git log|head -1|awk '{print $2}'`
  ORIG_MISC=$MISCPLUG_DEVEL_GIT
  echo "Misc $GITV_MISC vs. $ORIG_MISC"
  if [ "$GITV_MISC" != "$ORIG_MISC" ] && [ "$FAIL" != "1" ]; then
    echo "Misc update needed"
    BUILD_MISC=1
  fi
fi
BUILD_ARENA=0
if [ "$FAIL" != "1" ]; then
  cd $TMP/openfx-arena
  git fetch --all || FAIL=1
  git merge origin/master || FAIL=1
  GITV_ARENA=`git log|head -1|awk '{print $2}'`
  ORIG_ARENA=$ARENAPLUG_DEVEL_GIT
  echo "Arena $GITV_ARENA vs. $ORIG_ARENA"
  if [ "$GITV_ARENA" != "$ORIG_ARENA" ] && [ "$FAIL" != "1" ]; then
    echo "Arena update needed"
    BUILD_ARENA=1
  fi
fi
BUILD_OPENCV=0
#if [ "$FAIL" != "1" ]; then
#  cd $TMP/openfx-opencv
#  git fetch --all || FAIL=1
#  git merge origin/master || FAIL=1
#  GITV_CV=`git log|head -1|awk '{print $2}'`
#  ORIG_CV=$CVPLUG_DEVEL_GIT
#  echo "CV $GITV_CV vs. $ORIG_CV"
#  if [ "$GITV_CV" != "$ORIG_CV" ] && [ "$FAIL" != "1" ]; then
#    echo "CV update needed"
#    BUILD_OPENCV=1
#  fi
#fi

cd $CWD || exit 1
if [ "$FAIL" != "1" ]; then
  if [ "$BUILD_NATRON" == "1" ] || [ "$BUILD_IO" == "1" ] || [ "$BUILD_MISC" == "1" ] || [ "$BUILD_ARENA" == "1" ] || [ "$BUILD_OPENCV" == "1" ]; then
      NATRON_LICENSE=GPL OFFLINE_INSTALLER=1 SYNC=1 NOCLEAN=1 SNAPSHOT=1 sh build.sh workshop $JOBS
  fi
fi

if [ "$NO_LOOP" == "1" ]; then
	FINISHED=1
else
	echo "Idle ..."
	sleep 60
fi
done

