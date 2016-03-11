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
# Autobuild for Natron
#

#Usage snapshot.sh noThreads
#Options:
#NO_LOOP=1: Only build once and then exits.
#NATRON_LICENSE=(GPL,COMMERCIAL)
#Usage example: MKJOBS=8 NATRON_LICENSE=GPL sh snapshot.sh

#Easier to debug
set -x

source `pwd`/common.sh || exit 1

PID=$$
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
while [ "$FINISHED" = "0" ]; do

source $CWD/common.sh
source $CWD/commits-hash.sh

#Sync all scripts except snapshot.sh
git pull origin workshop

FAIL=0
echo "Running ..."



# make git sync
GITSCRIPT="/tmp/snapshot-git.sh"
cat << 'EOF' > $GITSCRIPT
#!/bin/sh
if [ "$1" != "" ]; then
  echo "Running git sync ..."
  PID=$$
  echo $PID > /tmp/snapshot-git.pid || exit 1
  git fetch --all || exit 1
  git merge origin/${1} || exit 1
fi
EOF
chmod +x $GITSCRIPT

# make kill bot
KILLSCRIPT="/tmp/killbot$$.sh"
cat << 'EOF' > "$KILLSCRIPT"
#!/bin/sh
sleep 30m
PARENT=`cat /tmp/snapshot-git.pid`
if [ "$PARENT" = "" ]; then
  exit 1
fi
PIDS=`ps aux|awk '{print $2}'|grep $PARENT`
if [ "$PIDS" = "$PARENT" ]; then
  kill -15 $PARENT
fi
EOF
chmod +x $KILLSCRIPT


BUILD_NATRON=0
cd $TMP/Natron 

$KILLSCRIPT &
KILLBOT=$!
$GITSCRIPT workshop 
kill -9 $KILLBOT

GITV_NATRON=`git log|head -1|awk '{print $2}'`
ORIG_NATRON=$NATRON_DEVEL_GIT
echo "Natron $GITV_NATRON vs. $ORIG_NATRON"
if [ "$GITV_NATRON" != "$ORIG_NATRON" -a "$FAIL" != "1" ]; then
  echo "Natron update needed"
  BUILD_NATRON=1
fi
BUILD_IO=0
if [ "$FAIL" != "1" ]; then
  cd $TMP/openfx-io

  $KILLSCRIPT &
  KILLBOT=$!
  $GITSCRIPT master 
  kill -9 $KILLBOT

  GITV_IO=`git log|head -1|awk '{print $2}'`
  ORIG_IO=$IOPLUG_DEVEL_GIT
  echo "IO $GITV_IO vs. $ORIG_IO"
  if [ "$GITV_IO" != "$ORIG_IO" -a "$FAIL" != "1" ]; then
    echo "IO update needed"
    BUILD_IO=1
  fi
fi
BUILD_MISC=0
if [ "$FAIL" != "1" ]; then
  cd $TMP/openfx-misc

  $KILLSCRIPT &
  KILLBOT=$!
  $GITSCRIPT master 
  kill -9 $KILLBOT

  GITV_MISC=`git log|head -1|awk '{print $2}'`
  ORIG_MISC=$MISCPLUG_DEVEL_GIT
  echo "Misc $GITV_MISC vs. $ORIG_MISC"
  if [ "$GITV_MISC" != "$ORIG_MISC" -a "$FAIL" != "1" ]; then
    echo "Misc update needed"
    BUILD_MISC=1
  fi
fi
BUILD_ARENA=0
if [ "$FAIL" != "1" ]; then
  cd $TMP/openfx-arena

  $KILLSCRIPT &
  KILLBOT=$!
  $GITSCRIPT master 
  kill -9 $KILLBOT

  GITV_ARENA=`git log|head -1|awk '{print $2}'`
  ORIG_ARENA=$ARENAPLUG_DEVEL_GIT
  echo "Arena $GITV_ARENA vs. $ORIG_ARENA"
  if [ "$GITV_ARENA" != "$ORIG_ARENA" -a "$FAIL" != "1" ]; then
    echo "Arena update needed"
    BUILD_ARENA=1
  fi
fi
BUILD_CV=0
#if [ "$FAIL" != "1" ]; then
#  cd $TMP/openfx-opencv
#  git fetch --all || FAIL=1
#  git merge origin/master || FAIL=1
#  GITV_CV=`git log|head -1|awk '{print $2}'`
#  ORIG_CV=$CVPLUG_DEVEL_GIT
#  echo "CV $GITV_CV vs. $ORIG_CV"
#  if [ "$GITV_CV" != "$ORIG_CV" -a "$FAIL" != "1" ]; then
#    echo "CV update needed"
#    BUILD_CV=1
#  fi
#fi

rm -f $GITSCRIPT $KILLSCRIPT

cd $CWD || exit 1
if [ "$FAIL" != "1" ]; then
  if [ "$BUILD_NATRON" = "1" -o "$BUILD_IO" = "1" -o "$BUILD_MISC" = "1" -o "$BUILD_ARENA" = "1" -o "$BUILD_CV" = "1" ]; then
      NATRON_LICENSE=$NATRON_LICENSE OFFLINE_INSTALLER=1 SYNC=1 NOCLEAN=1 BUILD_CONFIG=SNAPSHOT sh build.sh workshop $JOBS
  fi
fi

if [ "$NO_LOOP" = "1" ]; then
	FINISHED=1
else
	echo "Idle ..."
	sleep 60
fi
done

