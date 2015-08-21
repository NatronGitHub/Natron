#!/bin/sh
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

