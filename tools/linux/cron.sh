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
# Auto build Natron through cron
#

cd /root/Natron/tools/linux || exit 1

# Update
#git fetch --all
#git merge origin/workshop

# set PID, don't start if running
PID=$$
if [ -f /tmp/natron-cron.pid ]; then
  OLDPID=$(cat /tmp/natron-cron.pid)
  PIDS=$(ps aux|awk '{print $2}')
  for i in $PIDS;do
    if [ "$i" == "$OLDPID" ]; then
      echo "already running ..."
      exit 1
    fi
  done
fi
echo $PID > /tmp/natron-cron.pid || exit 1

# Build (if changes)
scl enable devtoolset-2 - << \EOF
cd /root/Natron/tools/linux
bash snapshot.sh >/tmp/natron-build.log 2>&1
EOF
