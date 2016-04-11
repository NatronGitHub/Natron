#!/bin/sh

source `pwd`/common.sh

PID=$$
if [ -f $TMP_DIR/natron-snapshot.pid ]; then
  OLDPID=$(cat $TMP_DIR/natron-snapshot.pid)
  RUNNING=$(ps aux|awk '{print $2}'|grep $OLDPID)
  if [ ! -z "$RUNNING" ]; then
    echo "===> Already running"
    exit 1
  fi
fi
echo $PID > $TMP_DIR/natron-snapshot.pid || exit 1

if [ -z "$MASTER_BRANCH" ]; then 
  echo "===> no MASTER_BRANCH defined"
  exit 1
fi

CWD=`pwd`
TMP=$CWD/.autobuild

if [ ! -d $TMP ]; then
  mkdir -p $TMP || exit 1
fi
if [ ! -d $TMP/Natron ]; then
  cd $TMP || exit 1
  git clone $GIT_NATRON || exit 1
#  cd Natron || exit 1
#  git checkout $MASTER_BRANCH || exit 1
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

FINISHED=0
while [ "$FINISHED" = "0" ]; do
source $CWD/common.sh
FAIL=0
git pull origin $MASTER_BRANCH
GIT_BRANCHES=`git branch -a| grep "origin" | sed 's/\// /g;/HEAD/d;/RB-1.1/d;/gh-pages/d;/coverity_scan/d;/workshop/d' | awk '{print $3}'`
for CURRENT_BRANCH in $GIT_BRANCHES; do
echo "=============== $CURRENT_BRANCH ==============="
# make git sync script
GITSCRIPT="/tmp/snapshot-git.sh"
cat << 'EOF' > $GITSCRIPT
#!/bin/sh
if [ "$1" != "" ]; then
  echo "Running git sync ..."
  PID=$$
  echo $PID > /tmp/snapshot-git.pid || exit 1
  git fetch --all || exit 1
  git reset --hard HEAD
  git clean -fd
  git checkout $1
  git pull || exit 1
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

# parse commits hash
if [ ! -f $CWD/commits-hash-${CURRENT_BRANCH}.sh ]; then
  echo "#!/bin/sh" > $CWD/commits-hash-${CURRENT_BRANCH}.sh
  echo "NATRON_DEVEL_GIT=#" >> $CWD/commits-hash-${CURRENT_BRANCH}.sh
  echo "IOPLUG_DEVEL_GIT=#" >> $CWD/commits-hash-${CURRENT_BRANCH}.sh
  echo "MISCPLUG_DEVEL_GIT=#" >> $CWD/commits-hash-${CURRENT_BRANCH}.sh
  echo "ARENAPLUG_DEVEL_GIT=#" >> $CWD/commits-hash-${CURRENT_BRANCH}.sh
  echo "CVPLUG_DEVEL_GIT=#" >> $CWD/commits-hash-${CURRENT_BRANCH}.sh
  echo "NATRON_VERSION_NUMBER=#" >> $CWD/commits-hash-${CURRENT_BRANCH}.sh
  echo "LATEST_SNAPSHOT_BUILD=#" >> $CWD/commits-hash-${CURRENT_BRANCH}.sh
fi
source $CWD/commits-hash-${CURRENT_BRANCH}.sh
if [ -z "$LATEST_SNAPSHOT_BUILD" ]; then
  echo "LATEST_SNAPSHOT_BUILD=#" >> $CWD/commits-hash-${CURRENT_BRANCH}.sh
fi

REPO_SUFFIX=snapshot-$CURRENT_BRANCH
LOGS=$REPO_DIR_PREFIX$REPO_SUFFIX/logs

# check natron
BUILD_NATRON=0
cd $TMP/Natron

$KILLSCRIPT &
KILLBOT=$!
$GITSCRIPT $CURRENT_BRANCH $TMP/Natron
kill -9 $KILLBOT

if [ "$CURRENT_BRANCH" != "$MASTER_BRANCH" ]; then
  echo "===> Checking $CURRENT_BRANCH ..."
  if [ "$NATRON_DEVEL_GIT" != "#" ] && [ "$NATRON_DEVEL_GIT" != "" ]; then
    echo "===> Checking for #snapshot ..."
    GITV_NATRON=`git --no-pager log --pretty=oneline ${NATRON_DEVEL_GIT}..HEAD --grep=#snapshot --since=1.days |head -1|awk '{print $1}'`
    if [ ! -z "$GITV_NATRON" ]; then
      if [ "$GITV_NATRON" != "$LATEST_SNAPSHOT_BUILD" ]; then
        sed -i "s/LATEST_SNAPSHOT_BUILD=.*/LATEST_SNAPSHOT_BUILD=${GITV_NATRON}/" $CWD/commits-hash-${CURRENT_BRANCH}.sh || exit 1
      else
        GITV_NATRON=""
      fi
    fi
  else
    echo "===> first-run, so don't build anything"
    NATRON_LATEST_COMMIT=`git log |head -1|awk '{print $2}'`
    echo "====> $NATRON_LATEST_COMMIT"
    if [ -z "$NATRON_LATEST_COMMIT" ]; then
      echo "===> latest commit is empty, should not happen!"
      NATRON_LATEST_COMMIT="#"
    fi
    sed -i "s/NATRON_DEVEL_GIT=.*/NATRON_DEVEL_GIT=${NATRON_LATEST_COMMIT}/" $CWD/commits-hash-${CURRENT_BRANCH}.sh || exit 1
    GITV_NATRON=""
  fi
else
  GITV_NATRON=`git log |head -1|awk '{print $2}'`
  MASTER_SNAPSHOT=`git --no-pager log --pretty=oneline ${NATRON_DEVEL_GIT}..HEAD --grep=#snapshot --since=1.days |head -1|awk '{print $1}'`
  if [ ! -z "$MASTER_SNAPSHOT" ] && [ "$MASTER_SNAPSHOT" != "$LATEST_SNAPSHOT_BUILD" ]; then
    sed -i "s/LATEST_SNAPSHOT_BUILD=.*/LATEST_SNAPSHOT_BUILD=${MASTER_SNAPSHOT}/" $CWD/commits-hash-${CURRENT_BRANCH}.sh || exit 1
  else
    MASTER_SNAPSHOT=""
  fi
fi

ORIG_NATRON=$NATRON_DEVEL_GIT
echo "==========> Natron $GITV_NATRON vs. $ORIG_NATRON"
if [ "$GITV_NATRON" != "$ORIG_NATRON" -a "$FAIL" != "1" -a "$GITV_NATRON" != "" -a "$GITV_NATRON" != "#" ]; then
  if [ "$GITV_NATRON" != "$LATEST_SNAPSHOT_BUILD" ]; then
    echo "===> Natron update needed!"
    BUILD_NATRON=1
  fi
fi
if [ ! -z "$MASTER_SNAPSHOT" ]; then
  echo "===> MASTER_BRANCH has an snapshot!"
  echo "=====> MASTER_BRANCH $MASTER_SNAPSHOT vs. $ORIG_NATRON"
  if [ "$MASTER_SNAPSHOT" != "$ORIG_NATRON" ] && [ "$MASTER_SNAPSHOT" != "$LATEST_SNAPSHOT_BUILD" ]; then
    echo "===> Natron upate needed!"
    BUILD_NATRON=1
  fi
fi

BUILD_IO=0
if [ "$FAIL" != "1" ]; then
  cd $TMP/openfx-io

  $KILLSCRIPT &
  KILLBOT=$!
  $GITSCRIPT $MASTER_BRANCH
  kill -9 $KILLBOT

  DO_SNAPSHOT_ON_BRANCH=`git --no-pager log --pretty=oneline --grep="#snapshot natron_branch=" --since=1.days | head -1 | sed 's/.*\#//;s/=/ /' | awk '{print $3}'`
  VALID_COMMIT=`git --no-pager log --pretty=oneline --grep="#snapshot natron_branch=" --since=1.days | head -1 | awk '{print $1}'`
  if [ "$VALID_COMMIT" = "$IOPLUG_DEVEL_GIT" ]; then
    DO_SNAPSHOT_ON_BRANCH=""
  fi
  if [ "$CURRENT_BRANCH" = "$DO_SNAPSHOT_ON_BRANCH" ]; then
    echo "=====> Plugin has a #snapshot for $CURRENT_BRANCH !"
    BUILD_NATRON=1
  fi
  if [ "$CURRENT_BRANCH" = "$MASTER_BRANCH" ] && [ "$DO_SNAPSHOT_ON_BRANCH" != "$MASTER_BRANCH" ]; then
    GITV_IO=`git log|head -1|awk '{print $2}'`
    ORIG_IO=$IOPLUG_DEVEL_GIT
    echo "==========> IO $GITV_IO vs. $ORIG_IO"
    if [ "$GITV_IO" != "$ORIG_IO" -a "$FAIL" != "1" ]; then
      echo "===> IO update needed!"
      BUILD_IO=1
    fi
  fi
fi

BUILD_MISC=0
if [ "$FAIL" != "1" ]; then
  cd $TMP/openfx-misc

  $KILLSCRIPT &
  KILLBOT=$!
  $GITSCRIPT $MASTER_BRANCH
  kill -9 $KILLBOT

  DO_SNAPSHOT_ON_BRANCH=`git --no-pager log --pretty=oneline --grep="#snapshot natron_branch=" --since=1.days | head -1 | sed 's/.*\#//;s/=/ /' | awk '{print $3}'`
  VALID_COMMIT=`git --no-pager log --pretty=oneline --grep="#snapshot natron_branch=" --since=1.days | head -1 | awk '{print $1}'`
  if [ "$VALID_COMMIT" = "$MISCPLUG_DEVEL_GIT" ]; then
    DO_SNAPSHOT_ON_BRANCH=""
  fi
  if [ "$CURRENT_BRANCH" = "$DO_SNAPSHOT_ON_BRANCH" ]; then
    echo "=====> Plugin has a #snapshot for $CURRENT_BRANCH !"
    BUILD_NATRON=1
  fi
  if [ "$CURRENT_BRANCH" = "$MASTER_BRANCH" ] && [ "$DO_SNAPSHOT_ON_BRANCH" != "$MASTER_BRANCH" ]; then
    GITV_MISC=`git log|head -1|awk '{print $2}'`
    ORIG_MISC=$MISCPLUG_DEVEL_GIT
    echo "==========> Misc $GITV_MISC vs. $ORIG_MISC"
    if [ "$GITV_MISC" != "$ORIG_MISC" -a "$FAIL" != "1" ]; then
      echo "===> Misc update needed!"
      BUILD_MISC=1
    fi
  fi
fi

BUILD_ARENA=0
if [ "$FAIL" != "1" ]; then
  cd $TMP/openfx-arena

  $KILLSCRIPT &
  KILLBOT=$!
  $GITSCRIPT $MASTER_BRANCH
  kill -9 $KILLBOT

  DO_SNAPSHOT_ON_BRANCH=`git --no-pager log --pretty=oneline --grep="#snapshot natron_branch=" --since=1.days | head -1 | sed 's/.*\#//;s/=/ /' | awk '{print $3}'`
  VALID_COMMIT=`git --no-pager log --pretty=oneline --grep="#snapshot natron_branch=" --since=1.days | head -1 | awk '{print $1}'`
  if [ "$VALID_COMMIT" = "$ARENAPLUG_DEVEL_GIT" ]; then
    DO_SNAPSHOT_ON_BRANCH=""
  fi
  if [ "$CURRENT_BRANCH" = "$DO_SNAPSHOT_ON_BRANCH"  ]; then
    echo "=====> Plugin has a #snapshot for $CURRENT_BRANCH !"
    BUILD_NATRON=1
  fi
  if [ "$CURRENT_BRANCH" = "$MASTER_BRANCH" ] && [ "$DO_SNAPSHOT_ON_BRANCH" != "$MASTER_BRANCH" ]; then
    GITV_ARENA=`git log|head -1|awk '{print $2}'`
    ORIG_ARENA=$ARENAPLUG_DEVEL_GIT
    echo "==========> Arena $GITV_ARENA vs. $ORIG_ARENA"
    if [ "$GITV_ARENA" != "$ORIG_ARENA" -a "$FAIL" != "1" ]; then
      echo "===> Arena update needed!"
      BUILD_ARENA=1
    fi
  fi
fi

rm -f $GITSCRIPT $KILLSCRIPT

cd $CWD || exit 1
if [ "$FAIL" != "1" ]; then


if [ "$CURRENT_BRANCH" != "$MASTER_BRANCH" ]; then
  if [ "$BUILD_NATRON" = "1" ];  then
    if [ "$CURRENT_BRANCH" = "$DO_SNAPSHOT_ON_BRANCH" ]; then
      GITV_NATRON=""
    else
      sed -i "s/NATRON_DEVEL_GIT=.*/NATRON_DEVEL_GIT=${GITV_NATRON}/" $CWD/commits-hash-$CURRENT_BRANCH.sh || exit 1
    fi
    echo "==========> BUILDING FROM $CURRENT_BRANCH BRANCH ..."
    UNIT_TEST=0 GIT_BRANCH=$CURRENT_BRANCH GIT_COMMIT=$GITV_NATRON NATRON_LICENSE=$NATRON_LICENSE OFFLINE_INSTALLER=1 SYNC=1 BUILD_CONFIG=SNAPSHOT sh build.sh
  fi
else
  if [ "$BUILD_NATRON" = "1" -o "$BUILD_IO" = "1" -o "$BUILD_MISC" = "1" -o "$BUILD_ARENA" = "1" -o "$BUILD_CV" = "1" ]; then
    if [ ! -z "$MASTER_SNAPSHOT" ]; then
      DO_SYNC=1
      NOMKINSTALL=0
      DOTEST=0
    else
      NOMKINSTALL=1
      DO_SYNC=0
      DOTEST=0
    fi
    if [ "$MASTER_BRANCH" = "$DO_SNAPSHOT_ON_BRANCH" ]; then
      DO_SYNC=1
      NOMKINSTALL=0
      DOTEST=0
      MASTER_SNAPSHOT=""
    fi

    echo "==========> BUILDING FROM $CURRENT_BRANCH BRANCH ..."
    UNIT_TEST=$DOTEST NO_INSTALLER=$NOMKINSTALL GIT_BRANCH=$CURRENT_BRANCH GIT_COMMIT=$MASTER_SNAPSHOT NATRON_LICENSE=$NATRON_LICENSE OFFLINE_INSTALLER=1 SYNC=$DO_SYNC BUILD_CONFIG=SNAPSHOT sh build.sh
  fi
fi


fi

done # end branch loop
if [ "$NO_LOOP" = "1" ]; then
  FINISHED=1
else
  echo "Idle ..."
  sleep 60
fi
done # end main loop
