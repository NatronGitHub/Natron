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
# End running build scripts
#
TMP_DIR=/tmp

if [ -f $TMP_DIR/natron-build-sdk.pid ]; then
  OLDPID=$(cat $TMP_DIR/natron-build-sdk.pid)
  PIDS=$(ps aux|awk '{print $2}')
  for i in $PIDS;do
    if [ "$i" == "$OLDPID" ]; then
      echo "Killing build-sdk.sh ..."
      kill -15 $i
    fi
  done
fi
if [ -f $TMP_DIR/natron-build-app.pid ]; then
  OLDPID=$(cat $TMP_DIR/natron-build-app.pid)
  PIDS=$(ps aux|awk '{print $2}')
  for i in $PIDS;do
    if [ "$i" == "$OLDPID" ]; then
      echo "Killing build-natron.sh ..."
      kill -15 $i
    fi
  done
fi
if [ -f $TMP_DIR/natron-build-plugins.pid ]; then
  OLDPID=$(cat $TMP_DIR/natron-build-plugins.pid)
  PIDS=$(ps aux|awk '{print $2}')
  for i in $PIDS;do
    if [ "$i" == "$OLDPID" ]; then
      echo "Killing build-plugins.sh ..."
      kill -15 $i
    fi
  done
fi
if [ -f $TMP_DIR/natron-build-installer.pid ]; then
  OLDPID=$(cat $TMP_DIR/natron-build-installer.pid)
  PIDS=$(ps aux|awk '{print $2}')
  for i in $PIDS;do
    if [ "$i" == "$OLDPID" ]; then
      echo "Killing build-installer.sh ..."
      kill -15 $i
    fi
  done
fi
if [ -f $TMP_DIR/natron-build.pid ]; then
  OLDPID=$(cat $TMP_DIR/natron-build.pid)
  PIDS=$(ps aux|awk '{print $2}')
  for i in $PIDS;do
    if [ "$i" == "$OLDPID" ]; then
      echo "Killing build.sh ..."
      kill -15 $i
    fi
  done
fi
if [ -f $TMP_DIR/natron-build-snapshot.pid ]; then
  OLDPID=$(cat $TMP_DIR/natron-build-snapshot.pid)
  PIDS=$(ps aux|awk '{print $2}')
  for i in $PIDS;do
    if [ "$i" == "$OLDPID" ]; then
      echo "Killing snapshot.sh ..."
      kill -15 $i
    fi
  done
fi
if [ -f $TMP_DIR/natron-cron.pid ]; then
  OLDPID=$(cat $TMP_DIR/natron-cron.pid)
  PIDS=$(ps aux|awk '{print $2}')
  for i in $PIDS;do
    if [ "$i" == "$OLDPID" ]; then
      echo "Killing cron.sh ..."
      kill -15 $i
    fi
  done
fi

