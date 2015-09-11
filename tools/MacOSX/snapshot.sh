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

#Usage:
# cd Natron/tools/MacOSX
# MKJOBS=4 ./snapshot.sh

source $(pwd)/common.sh || exit 1


if [ ! -d $TMP ];then
    mkdir -p $TMP || exit 1
fi

#clone repos to get git version to check against NATRON_WORKSHOP
if [ ! -d $TMP/Natron ];then
    cd $TMP || exit 1
    git clone $GIT_NATRON || exit 1
    cd Natron || exit 1
    git checkout workshop || exit 1
fi

if [ ! -d $TMP/openfx-io ];then
    cd $TMP || exit 1
    git clone $GIT_IO || exit 1
fi

if [ ! -d $TMP/openfx-misc ];then
    cd $TMP || exit 1
    git clone $GIT_MISC || exit 1
fi

if [ ! -d $TMP/openfx-arena ];then
    cd $TMP || exit 1
    git clone $GIT_ARENA || exit 1
fi
#if [ ! -d $TMP/openfx-opencv ]; then
#  cd $TMP || exit 1
#  git clone $GIT_OPENCV || exit 1
#fi

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


#infinite loop with 1min sleeps
while true; do

    source $CWD/commits-hash.sh || exit 1

    FAIL=0

    BUILD_NATRON=0
    BUILD_IO=0
    BUILD_MISC=0

    cd $TMP/Natron
    git fetch || FAIL=1
    git merge origin/workshop || FAIL=1
    GITV_NATRON=$(git log|head -1|awk '{print $2}')
    ORIG_NATRON=$NATRON_DEVEL_GIT
    echo "Natron $GITV_NATRON vs. $ORIG_NATRON"
    if [ "$GITV_NATRON" != "$ORIG_NATRON" ] && [ "$FAIL" != "1" ];then
	echo "Natron update needed"
	BUILD_NATRON=1
    fi

    echo $FAIL

    if [ "$FAIL" != "1" ]; then
	cd $TMP/openfx-io
	git fetch || FAIL=1
	git merge origin/master || FAIL=1
	GITV_IO=$(git log|head -1|awk '{print $2}')
	ORIG_IO=$IOPLUG_DEVEL_GIT
	echo "openfx-io $GITV_IO vs. $ORIG_IO"
	if [ "$GITV_IO" != "$ORIG_IO" ] && [ "$FAIL" != "1" ];then
	    echo "openfx-io update needed"
	    BUILD_IO=1
	fi
    fi

    echo $FAIL

    if [ "$FAIL" != "1" ]; then
	cd $TMP/openfx-misc
	git fetch || FAIL=1
	git merge origin/master || FAIL=1
	GITV_MISC=$(git log|head -1|awk '{print $2}')
	ORIG_MISC=$MISCPLUG_DEVEL_GIT
	echo "openfx-misc $GITV_MISC vs. $ORIG_MISC"
	if [ "$GITV_MISC" != "$ORIG_MISC" ] && [ "$FAIL" != "1" ];then
	    echo "openfx-misc update needed"
	    BUILD_MISC=1
	fi
    fi

    if [ "$FAIL" != "1" ]; then
	cd $TMP/openfx-arena
	git fetch || FAIL=1
	git merge origin/master || FAIL=1
	ARENAV_MISC=$(git log|head -1|awk '{print $2}')
	ORIG_ARENA=$ARENAPLUG_DEVEL_GIT
	echo "openfx-arena $ARENAV_MISC vs. $ORIG_ARENA"
	if [ "$ARENAV_MISC" != "$ORIG_ARENA" ] && [ "$FAIL" != "1" ];then
	    echo "openfx-arena update needed"
	    BUILD_ARENA=1
	fi
    fi


    echo $FAIL

    cd $CWD || FAIL=1
    if [ "$FAIL" != 1 ]; then
	if [ "$BUILD_NATRON" == "1" ] || [ "$BUILD_IO" == "1" ] || [ "$BUILD_MISC" == "1" ] || [ "$BUILD_ARENA" == "1" ]; then
	    env CONFIG=relwithdebinfo BRANCH=workshop MKJOBS=$MKJOBS UPLOAD=1 NO_CLEAN=$NO_CLEAN ./build.sh || FAIL=1
	    echo $FAIL
	fi
    fi

    echo "Idle"
    sleep 60
done
