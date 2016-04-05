#!/bin/sh

source `pwd`/common.sh

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

# parse commits hash
if [ ! -f $CWD/commits-hash-${CURRENT_BRANCH}.sh ]; then
    echo "#!/bin/sh" > $CWD/commits-hash-${CURRENT_BRANCH}.sh
    echo "NATRON_DEVEL_GIT=#" >> $CWD/commits-hash-${CURRENT_BRANCH}.sh
    echo "IOPLUG_DEVEL_GIT=#" >> $CWD/commits-hash-${CURRENT_BRANCH}.sh
    echo "MISCPLUG_DEVEL_GIT=#" >> $CWD/commits-hash-${CURRENT_BRANCH}.sh
    echo "ARENAPLUG_DEVEL_GIT=#" >> $CWD/commits-hash-${CURRENT_BRANCH}.sh
    echo "CVPLUG_DEVEL_GIT=#" >> $CWD/commits-hash-${CURRENT_BRANCH}.sh
    echo "NATRON_VERSION_NUMBER=#" >> $CWD/commits-hash-${CURRENT_BRANCH}.sh
fi
source $CWD/commits-hash-${CURRENT_BRANCH}.sh

REPO_SUFFIX=snapshot-$CURRENT_BRANCH
LOGS=$REPO_DIR_PREFIX$REPO_SUFFIX/logs

# check natron
BUILD_NATRON=0
cd $TMP/Natron

git fetch --all
git reset --hard HEAD
git clean -fd
git checkout $CURRENT_BRANCH
git pull

if [ "$CURRENT_BRANCH" != "$MASTER_BRANCH" ]; then
  echo "===> Checking $CURRENT_BRANCH ..."
  if [ "$NATRON_DEVEL_GIT" != "#" ] && [ "$NATRON_DEVEL_GIT" != "" ]; then
    echo "===> Checking for #snapshot ..."
    GITV_NATRON=`git --no-pager log --pretty=oneline ${NATRON_DEVEL_GIT}..HEAD --grep=#snapshot --since=1.days |head -1|awk '{print $1}'`
  else
    echo "===> first-run, so don't build anything"
    NATRON_LATEST_COMMIT=`git log |head -1|awk '{print $2}'`
    echo "====> $NATRON_LATEST_COMMIT"
    if [ -z "$NATRON_LATEST_COMMIT" ]; then
      echo "===> latest commit is empty, should not happen!"
      NATRON_LATEST_COMMIT="#"
    fi
    sed -i "" -e "s/NATRON_DEVEL_GIT=.*/NATRON_DEVEL_GIT=${NATRON_LATEST_COMMIT}/" $CWD/commits-hash-${CURRENT_BRANCH}.sh || exit 1
    GITV_NATRON=""
  fi
else
  GITV_NATRON=`git log |head -1|awk '{print $2}'`
  MASTER_SNAPSHOT=`git --no-pager log --pretty=oneline ${NATRON_DEVEL_GIT}..HEAD --grep=#snapshot --since=1.days |head -1|awk '{print $1}'`
fi

ORIG_NATRON=$NATRON_DEVEL_GIT
echo "==========> Natron $GITV_NATRON vs. $ORIG_NATRON"
if [ "$GITV_NATRON" != "$ORIG_NATRON" -a "$FAIL" != "1" -a "$GITV_NATRON" != "" -a "$GITV_NATRON" != "#" ]; then
  echo "===> Natron update needed!"
  BUILD_NATRON=1
fi
if [ ! -z "$MASTER_SNAPSHOT" ]; then
  echo "===> MASTER_BRANCH has an snapshot!"
  echo "=====> MASTER_BRANCH $MASTER_SNAPSHOT vs. $ORIG_NATRON"
  if [ "$MASTER_SNAPSHOT" != "$ORIG_NATRON" ]; then
    echo "===> Natron upate needed!"
    BUILD_NATRON=1
  fi
fi

BUILD_IO=0
if [ "$CURRENT_BRANCH" = "$MASTER_BRANCH" ]; then
if [ "$FAIL" != "1" ]; then
  cd $TMP/openfx-io

  git fetch --all 
  git reset --hard HEAD
  git clean -fd
  git checkout $MASTER_BRANCH
  git pull 

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
if [ "$CURRENT_BRANCH" = "$MASTER_BRANCH" ]; then
if [ "$FAIL" != "1" ]; then
  cd $TMP/openfx-misc

  git fetch --all
  git reset --hard HEAD
  git clean -fd
  git checkout $MASTER_BRANCH
  git pull

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
if [ "$CURRENT_BRANCH" = "$MASTER_BRANCH" ]; then
if [ "$FAIL" != "1" ]; then
  cd $TMP/openfx-arena

  git fetch --all
  git reset --hard HEAD
  git clean -fd
  git checkout $MASTER_BRANCH
  git pull

  GITV_ARENA=`git log|head -1|awk '{print $2}'`
  ORIG_ARENA=$ARENAPLUG_DEVEL_GIT
  echo "==========> Arena $GITV_ARENA vs. $ORIG_ARENA"
  if [ "$GITV_ARENA" != "$ORIG_ARENA" -a "$FAIL" != "1" ]; then
    echo "===> Arena update needed!"
    BUILD_ARENA=1
  fi
fi
fi

cd $CWD || exit 1
if [ "$FAIL" != "1" ]; then


if [ "$CURRENT_BRANCH" != "$MASTER_BRANCH" ]; then
  if [ "$BUILD_NATRON" = "1" ];  then
    sed -i "s/NATRON_DEVEL_GIT=.*/NATRON_DEVEL_GIT=${GITV_NATRON}/" $CWD/commits-hash-$CURRENT_BRANCH.sh || exit 1
    env COMMIT=$GITV CONFIG=relwithdebinfo BRANCH=$CURRENT_BRANCH BUILD_CONFIG=SNAPSHOT MKJOBS=$MKJOBS UPLOAD=1 NO_CLEAN=$NO_CLEAN ./build.sh
  fi
else
  if [ "$BUILD_NATRON" = "1" -o "$BUILD_IO" = "1" -o "$BUILD_MISC" = "1" -o "$BUILD_ARENA" = "1" -o "$BUILD_CV" = "1" ]; then

    if [ ! -z "$MASTER_SNAPSHOT" ]; then
      DO_SYNC=1
    else
      DO_SYNC=0
    fi

    env CONFIG=relwithdebinfo BRANCH=$CURRENT_BRANCH BUILD_CONFIG=SNAPSHOT MKJOBS=$MKJOBS UPLOAD=$DO_SYNC NO_CLEAN=$NO_CLEAN ./build.sh
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
