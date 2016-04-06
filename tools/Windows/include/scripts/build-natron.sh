#!/bin/sh
#
# Build Natron for Windows
#
# BUILD_CONFIG=(SNAPSHOT,ALPHA,BETA,RC,STABLE,CUSTOM)
# CUSTOM_BUILD_USER_NAME="Toto" : to be set if BUILD_CONFIG=CUSTOM
# BUILD_NUMBER=X: To be set to indicate the revision number of the build. For example RC1,RC2, RC3 etc...
# DISABLE_BREAKPAD=1: Disable automatic crash report
# Usage: BUILD_CONFIG=SNAPSHOT build-natron.sh BIT "branch"

source `pwd`/common.sh || exit 1

PID=$$



if [ "$1" = "32" ]; then
    BIT=32
    INSTALL_PATH=$INSTALL32_PATH
else
    BIT=64
    INSTALL_PATH=$INSTALL64_PATH
fi

if [ "$1" = "" ]; then
  echo "no bit"
  exit 1
fi

TMP_BUILD_DIR=$TMP_PATH$BIT
# make kill bot
KILLSCRIPT="$TMP_BUILD_DIR/killbot$$.sh"
cat << 'EOF' > "$KILLSCRIPT"
#!/bin/sh
PARENT=$1
sleep 30m
if [ "$PARENT" = "" ]; then
exit 1
fi
PIDS=`ps aux|awk '{print $2}'|grep $PARENT`
if [ "$PIDS" = "$PARENT" ]; then
kill -15 $PARENT
fi
EOF
chmod +x $KILLSCRIPT


# we need $BUILD_CONFIG
if [ -z "$BUILD_CONFIG" ]; then
  echo "Please define BUILD_CONFIG"
  exit 1
fi


# Assume that $GIT_BRANCH is the branch to build, otherwise if empty use the NATRON_GIT_TAG in common.sh, but if BUILD_CONFIG=SNAPSHOT use MASTER_BRANCH
NATRON_BRANCH=$GIT_BRANCH
if [ -z "$NATRON_BRANCH" ]; then
  if [ "$BUILD_CONFIG" = "SNAPSHOT" ]; then
    NATRON_BRANCH=$MASTER_BRANCH
    COMMITS_HASH=$CWD/commits-hash-$NATRON_BRANCH.sh
  else
    NATRON_BRANCH=$NATRON_GIT_TAG
    COMMITS_HASH=$CWD/commits-hash.sh
  fi
else
  COMMITS_HASH=$CWD/commits-hash-$NATRON_BRANCH.sh
fi

echo "===> Using branch $NATRON_BRANCH"

if [ ! -d $INSTALL_PATH ]; then
    if [ -f "$SRC_PATH/Natron-$SDK_VERSION-Windows-$OS-$BIT-SDK.tar.xz" ]; then
        echo "Found binary SDK, extracting ..."
        tar xvJf "$SRC_PATH/Natron-$SDK_VERSION-Windows-$OS-$BIT-SDK.tar.xz" -C "$SDK_PATH/" || exit 1
    else
        echo "Need to build SDK ..."
        env MKJOBS=$MKJOBS TAR_SDK=1 sh $INC_PATH/scripts/build-sdk.sh $BIT || exit 1
    fi
fi

if [ -d $TMP_BUILD_DIR ]; then
    rm -rf $TMP_BUILD_DIR || exit 1
fi
mkdir -p $TMP_BUILD_DIR || exit 1

# Setup env
BOOST_ROOT=$INSTALL_PATH
PYTHON_HOME=$INSTALL_PATH
PYTHON_PATH=$INSTALL_PATH/lib/python2.7
PYTHON_INCLUDE=$INSTALL_PATH/include/python2.7
export BOOST_ROOT PYTHON_HOME PYTHON_PATH PYTHON_INCLUDE

# Install natron
cd $TMP_BUILD_DIR || exit 1


$KILLSCRIPT $PID &
KILLBOT=$!

# clone natron
git clone $GIT_NATRON || exit 1
cd Natron || exit 1

# checkout branch
git checkout $NATRON_BRANCH || exit 1

# if we have a predefined commit use that, else use latest commit from branch
if [ -z "$GIT_COMMIT" ]; then
  git pull origin $NATRON_BRANCH
else
  git checkout $GIT_COMMIT
fi

# Update submodule
git submodule update -i --recursive || exit 1

# the snapshot are always built with latest version
if [ "$NATRON_BRANCH" = "$MASTER_BRANCH" ]; then
  git submodule foreach git pull origin master
fi

kill -o $KILLBOT

REL_GIT_VERSION=`git log|head -1|awk '{print $2}'`

#Always bump NATRON_DEVEL_GIT, it is only used to version-stamp binaries
NATRON_REL_V="$REL_GIT_VERSION"

NATRON_MAJOR=`grep "define NATRON_VERSION_MAJOR" $TMP_BUILD_DIR/Natron/Global/Macros.h | awk '{print $3}'`
NATRON_MINOR=`grep "define NATRON_VERSION_MINOR" $TMP_BUILD_DIR/Natron/Global/Macros.h | awk '{print $3}'`
NATRON_REVISION=`grep "define NATRON_VERSION_REVISION" $TMP_BUILD_DIR/Natron/Global/Macros.h | awk '{print $3}'`

if [ ! -f "$COMMITS_HASH" ]; then
cat <<EOF > "$COMMITS_HASH"
#!/bin/sh
NATRON_DEVEL_GIT=#
IOPLUG_DEVEL_GIT=#
MISCPLUG_DEVEL_GIT=#
ARENAPLUG_DEVEL_GIT=#
CVPLUG_DEVEL_GIT=#
NATRON_VERSION_NUMBER=#
EOF
fi

sed -i "s/NATRON_DEVEL_GIT=.*/NATRON_DEVEL_GIT=${NATRON_REL_V}/" $COMMITS_HASH || exit 1
sed -i "s/NATRON_VERSION_NUMBER=.*/NATRON_VERSION_NUMBER=${NATRON_MAJOR}.${NATRON_MINOR}.${NATRON_REVISION}/" $COMMITS_HASH || exit 1


# Plugins git hash
IO_VERSION_FILE=$INSTALL_PATH/Plugins/IO.ofx.bundle-version.txt
MISC_VERSION_FILE=$INSTALL_PATH/Plugins/Misc.ofx.bundle-version.txt
ARENA_VERSION_FILE=$INSTALL_PATH/Plugins/Arena.ofx.bundle-version.txt

if [ -f "$IO_VERSION_FILE" ]; then
  IO_GIT_HASH=`cat ${IO_VERSION_FILE}`
fi
if [ -f "$MISC_VERSION_FILE" ]; then
  MISC_GIT_HASH=`cat ${MISC_VERSION_FILE}`
fi
if [ -f "$ARENA_VERSION_FILE" ]; then
  ARENA_GIT_HASH=`cat ${ARENA_VERSION_FILE}`
fi


#Make sure GitVersion.h is in sync with NATRON_GIT_VERSION
cat "$INC_PATH/natron/GitVersion.h" | sed -e "s#__BRANCH__#${NATRON_BRANCH}#;s#__COMMIT__#${REL_GIT_VERSION}#;s#__IO_COMMIT__#${IO_GIT_HASH}#;s#__MISC_COMMIT__#${MISC_GIT_HASH}#;s#__ARENA_COMMIT__#${ARENA_GIT_HASH}#" > Global/GitVersion.h || exit 1

if [ "$PYV" = "3" ]; then
    cat "$INC_PATH/natron/config_py3.pri" > config.pri || exit 1
else
    cat "$INC_PATH/natron/config.pri" > config.pri || exit 1
fi

rm -rf build
mkdir build || exit 1
cd build || exit 1

if [ "$BUILD_CONFIG" = "SNAPSHOT" ]; then
    EXTRA_QMAKE_FLAG="CONFIG+=snapshot"
elif [ "$BUILD_CONFIG" = "ALPHA" ]; then
	EXTRA_QMAKE_FLAG="CONFIG+=alpha  BUILD_NUMBER=$BUILD_NUMBER"
	if [ -z "$BUILD_NUMBER" ]; then
		echo "You must supply a BUILD_NUMBER when BUILD_CONFIG=ALPHA"
		exit 1
	fi

elif [ "$BUILD_CONFIG" = "BETA" ]; then
	if [ -z "$BUILD_NUMBER" ]; then
		echo "You must supply a BUILD_NUMBER when BUILD_CONFIG=BETA"
		exit 1
	fi
	EXTRA_QMAKE_FLAG="CONFIG+=beta  BUILD_NUMBER=$BUILD_NUMBER"
elif [ "$BUILD_CONFIG" = "RC" ]; then
	if [ -z "$BUILD_NUMBER" ]; then
		echo "You must supply a BUILD_NUMBER when BUILD_CONFIG=RC"
		exit 1
	fi
	EXTRA_QMAKE_FLAG="CONFIG+=RC BUILD_NUMBER=$BUILD_NUMBER"
elif [ "$BUILD_CONFIG" = "STABLE" ]; then
	EXTRA_QMAKE_FLAG="CONFIG+=stable"
elif [ "$BUILD_CONFIG" = "CUSTOM" ]; then
	if [ -z "$CUSTOM_BUILD_USER_NAME" ]; then
		echo "You must supply a CUSTOM_BUILD_USER_NAME when BUILD_CONFIG=CUSTOM"
		exit 1
	fi
	EXTRA_QMAKE_FLAG="CONFIG+=stable BUILD_USER_NAME=$CUSTOM_BUILD_USER_NAME"
fi

if [ "$DISABLE_BREAKPAD" = "1" ]; then
    CONFIG_BREAKPAD_FLAG="CONFIG+=disable-breakpad"
fi

$INSTALL_PATH/bin/qmake -r CONFIG+=relwithdebinfo ${EXTRA_QMAKE_FLAG} ${CONFIG_BREAKPAD_FLAG} CONFIG+=silent CONFIG+=${BIT}bit DEFINES+=QT_NO_DEBUG_OUTPUT ../Project.pro || exit 1
make -j${MKJOBS} || exit 1

cp App/release/Natron.exe $INSTALL_PATH/bin/ || exit 1
cp Renderer/release/NatronRenderer.exe $INSTALL_PATH/bin/ || exit 1
if [ "$DISABLE_BREAKPAD" != "1" ]; then
    cp CrashReporter/release/NatronCrashReporter.exe $INSTALL_PATH/bin/ || exit 1
    cp CrashReporterCLI/release/NatronRendererCrashReporter.exe $INSTALL_PATH/bin/ || exit 1
fi

#If OpenColorIO-Configs do not exist, download them
if [ ! -d "$SRC_PATH/OpenColorIO-Configs" ]; then
    mkdir -p "$SRC_PATH"
    wget "$GIT_OCIO_CONFIG_TAR" -O "$SRC_PATH/OpenColorIO-Configs.tar.gz" || exit 1
    (cd "$SRC_PATH"; tar xf OpenColorIO-Configs.tar.gz) || exit 1
    rm "$SRC_PATH/OpenColorIO-Configs.tar.gz" || exit 1
    mv "$SRC_PATH/OpenColorIO-Configs"* "$SRC_PATH/OpenColorIO-Configs" || exit 1
	rm -rf "$SRC_PATH/OpenColorIO-Configs/aces_1.0.1/baked"
	rm -rf "$SRC_PATH/OpenColorIO-Configs/aces_1.0.1/python"
fi

cp -a "$SRC_PATH/OpenColorIO-Configs" "$INSTALL_PATH/share/" || exit 1
mkdir -p "$INSTALL_PATH/docs/natron" || exit 1
cp ../LICENSE.txt ../README* ../BUGS* ../CONTRI* ../Documentation/* "$INSTALL_PATH/docs/natron/"
mkdir -p "$INSTALL_PATH/share/stylesheets" || exit 1
cp ../Gui/Resources/Stylesheets/mainstyle.qss "$INSTALL_PATH/share/stylesheets/" || exit 1
mkdir -p $INSTALL_PATH/share/pixmaps || exit 1
cp ../Gui/Resources/Images/natronIcon256_linux.png "$INSTALL_PATH/share/pixmaps/" || exit 1
echo "$NATRON_REL_V" > "$INSTALL_PATH/docs/natron/VERSION" || exit 1

rm -rf $INSTALL_PATH/PyPlugs
mkdir -p $INSTALL_PATH/PyPlugs || exit 1
cp ../Gui/Resources/PyPlugs/* $INSTALL_PATH/PyPlugs/ || exit 1


rm -f $KILLSCRIPT

echo "Done!"

