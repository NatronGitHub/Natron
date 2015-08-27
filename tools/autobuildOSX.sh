#!/bin/bash
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

#script to be executed in the root of Natron repository

PKGOS=OSX
BIT=Universal
CWD=`pwd`
TMP=$CWD/.autobuild
GIT_NATRON=https://github.com/MrKepzie/Natron.git
GIT_IO=https://github.com/MrKepzie/openfx-io.git
GIT_MISC=https://github.com/devernay/openfx-misc.git

if [ ! -d $TMP ];then
  mkdir -p $TMP || exit 1
fi

#clone repos to get git version to check against NATRON_WORKSHOP
if [ ! -d $TMP/Natron ];then
  cd $TMP || exit 1
  git clone $GIT_NATRON || exit 1
  touch NATRON_WORKSHOP || exit 1
  cd Natron || exit 1
  git checkout workshop || exit 1
fi

if [ ! -d $TMP/openfx-io ];then
  cd $TMP || exit 1
  git clone $GIT_IO || exit 1
  touch IO_WORKSHOP || exit 1
fi

if [ ! -d $TMP/openfx-misc ];then
  cd $TMP || exit 1
  git clone $GIT_MISC || exit 1
  touch MISC_WORKSHOP || exit 1
fi

#infinite loop with 1min sleeps
while true; do

FAIL=0

BUILD_NATRON=0
BUILD_IO=0
BUILD_MISC=0

cd $TMP/Natron
git fetch || FAIL=1
git merge origin/workshop || FAIL=1
GITV_NATRON=$(git log|head -1|awk '{print $2}')
ORIG_NATRON=$(cat ../NATRON_WORKSHOP)
echo "Natron $GITV_NATRON vs. $ORIG_NATRON"
if [ "$GITV_NATRON" != "$ORIG_NATRON" ] && [ "$FAIL" != "1" ];then
 echo "Natron update needed"
 BUILD_NATRON=1
 echo $GITV_NATRON > ../NATRON_WORKSHOP | FAIL=1
fi

echo $FAIL

cd $TMP/openfx-io
git fetch || FAIL=1
git merge origin/master || FAIL=1
GITV_IO=$(git log|head -1|awk '{print $2}')
ORIG_IO=$(cat ../IO_WORKSHOP)
echo "openfx-io $GITV_IO vs. $ORIG_IO"
if [ "$GITV_IO" != "$ORIG_IO" ] && [ "$FAIL" != "1" ];then
 echo "openfx-io update needed"
 BUILD_IO=1
 echo $GITV_IO > ../IO_WORKSHOP | FAIL=1
fi

echo $FAIL

cd $TMP/openfx-misc
git fetch || FAIL=1
git merge origin/master || FAIL=1
GITV_MISC=$(git log|head -1|awk '{print $2}')
ORIG_MISC=$(cat ../MISC_WORKSHOP)
echo "openfx-misc $GITV_MISC vs. $ORIG_MISC"
if [ "$GITV_MISC" != "$ORIG_MISC" ] && [ "$FAIL" != "1" ];then
 echo "openfx-misc update needed"
 BUILD_MISC=1
 echo $GITV_MISC > ../MISC_WORKSHOP | FAIL=1
fi

echo $FAIL

cd $CWD || exit 1
if [ "$FAIL" != 1 ]; then
if [ "$BUILD_NATRON" == "1" ] || [ "$BUILD_IO" == "1" ] || [ "$BUILD_MISC" == "1" ]; then
  ./tools/packageOSX.sh  || FAIL=1
  echo $FAIL
fi
fi

if [ "$FAIL" != "1" ]; then
  rm -rf build
  rm -rf repo
  mkdir repo
  DMGNAME=Natron-snapshots-$GITV_NATRON.dmg 
  mv Natron.dmg repo/$DMGNAME || FAIL=1 
  if [ "$FAIL" != "1" ]; then
    #rsync -avz -e ssh --delete repo kepzlol@frs.sourceforge.net:/home/frs/project/natron/snapshots/$PKGOS$BIT
    rsync -avz -e ssh repo/ mrkepzie@vps163799.ovh.net:../www/downloads.natron.fr/Mac/snapshots
  fi
fi

echo "Idle"
sleep 60
done
