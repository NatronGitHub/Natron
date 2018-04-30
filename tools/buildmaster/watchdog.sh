#!/bin/bash
#
# Buildmaster watchdog
#

QID=${1:-}
PARENT=${2:-}

if [ "$QID" = "" ]; then
  echo "No QID"
  exit
fi

CWD=`pwd`
if [ -f "$CWD/common.sh" ]; then
  source "$CWD/common.sh"
  source "$CWD/common-buildmaster.sh"
else
  echo "No common.sh"
  exit 
fi

if [ ! -d "$TMP_PATH" ]; then
  mkdir -p "$TMP_PATH"
fi

sleep 10
echo "===> Woof! Fido is awake and watching build $QID for parent $PARENT"

NATRON_PID_FILE="$QUEUE_LOG_PATH/natron-$QID-$QOS.pid"
PLUGINS_PID_FILE="$QUEUE_LOG_PATH/plugins-$QID-$QOS.pid"
INSTALLER_PID_FILE="$QUEUE_LOG_PATH/installer-$QID-$QOS.pid"


function isRunning() {
  IS_RUNNING=""
  if [ "$PKGOS" != "Windows" ]; then
    IS_RUNNING=`ps -p ${PARENT} | awk '{print $1}' | grep ${PARENT} | tail -1`
  else
    IS_RUNNING=`ps -s -p ${PARENT} | awk '{print $1}' | grep ${PARENT} | tail -1`
  fi
  echo $IS_RUNNING
}

FINISHED=0
while [ "$FINISHED" = "0" ]; do
  QTMP="$TMP_PATH/watchdog-query.$$"
  STATUS=0
  $MYSQL "SELECT id,${QOS} FROM queue WHERE id=${QID} LIMIT 1;" | sed 1d > $QTMP
  while read -r job_id job_status ; do
    if [ "$job_id" = "$QID" ] && [ "$job_status" -gt 0 ]; then
      STATUS=$job_status
    fi
  done < $QTMP
  rm -f "$QTMP"

  if [ "$PARENT" != "" ]; then
    IS_PARENT_RUNNING=`isRunning $PARENT`
    if [ "$IS_PARENT_RUNNING" != "$PARENT" ]; then
      echo "===> buildmaster is not running anymore, kamikaze!"
      STATUS=0
      updateStatus $QID 0
    fi
  fi
  if [ "$STATUS" = 0 ]; then
    NATRON_PID=""
    if [ -f "$NATRON_PID_FILE" ]; then
      NATRON_PID=`cat $NATRON_PID_FILE`
    fi
    PLUGINS_PID=""
    if [ -f "$PLUGINS_PID_FILE" ]; then
      PLUGINS_PID=`cat $PLUGINS_PID_FILE`
    fi
    INSTALLER_PID=""
    if [ -f "$INSTALLER_PID_FILE" ]; then 
      INSTALLER_PID=`cat $INSTALLER_PID_FILE`
    fi
    if [ "$PLUGINS_PID" != "" ]; then
      IS_PLUGINS_RUNNING=`isRunning $PLUGINS_PID`
      if [ "$IS_PLUGINS_RUNNING" = "$PLUGINS_PID" ]; then
        kill -15 $PLUGINS_PID
      fi
    fi
    if [ "$NATRON_PID" != "" ]; then
      IS_NATRON_RUNNING=`isRunning $NATRON_PID`
      if [ "$IS_NATRON_RUNNING" = "$NATRON_PID" ]; then
        kill -15 $NATRON_PID
      fi
    fi
    if [ "$INSTALLER_PID" != "" ]; then
      IS_INSTALLER_RUNNING=`isRunning $INSTALLER_PID`
      if [ "$IS_INSTALLER_RUNNING" = "$INSTALLER_PID" ]; then
        kill -15 $INSTALLER_PID
      fi
    fi
    FINISHED=1
  elif [ "$STATUS" = 2 ]; then
    echo "===> Fido should not be running anymore, kamikaze!"
    FINISHED=1
  fi
  if [ "$FINISHED" = 0 ]; then
    sleep 25
  fi
done
rm -rf $TMP_PATH/* || true
exit 0
