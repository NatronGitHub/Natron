#!/bin/sh
#
# Auto build Natron through cron
#

cd /root/natron-linux || exit 1

# Update
git fetch --all
git merge origin/master

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
cd /root/natron-linux
bash snapshot.sh >/tmp/natron-build.log 2>&1
EOF
