#!/bin/sh
#
# Build Natron for Windows
#
# BUILD_CONFIG=(SNAPSHOT,ALPHA,BETA,RC,STABLE,CUSTOM)
# CUSTOM_BUILD_USER_NAME="Toto" : to be set if BUILD_CONFIG=CUSTOM
# BUILD_NUMBER=X: To be set to indicate the revision number of the build. For example RC1,RC2, RC3 etc...
# Usage: BUILD_CONFIG=SNAPSHOT build-natron.sh BIT "branch"

source `pwd`/common.sh || exit 1


if [ "$1" = "32" ]; then
    BIT=32
    INSTALL_PATH=$INSTALL32_PATH
else
    BIT=64
    INSTALL_PATH=$INSTALL64_PATH
fi

TMP_BUILD_DIR=$TMP_PATH$BIT


#Assume that $1 is the branch to build, otherwise if empty use the NATRON_GIT_TAG in common.sh
NATRON_BRANCH=$2
if [ -z "$NATRON_BRANCH" ]; then
    NATRON_BRANCH=$NATRON_GIT_TAG
fi

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

git clone $GIT_NATRON || exit 1
cd Natron || exit 1
git checkout $NATRON_BRANCH || exit 1
git pull origin $NATRON_BRANCH
git submodule update -i --recursive || exit 1
if [ "$NATRON_BRANCH" = "workshop" ]; then
    # the snapshots are always built with the latest version of submodules
    git submodule foreach git pull origin master
fi

REL_GIT_VERSION=`git log|head -1|awk '{print $2}'`

# mksrc
if [ "$MKSRC" = "1" ]; then
    cd .. || exit 1
    cp -a Natron "Natron-$REL_GIT_VERSION" || exit 1
    (cd "Natron-$REL_GIT_VERSION";find . -type d -name .git -exec rm -rf {} \;)
    tar cvvJf "$SRC_PATH/Natron-$REL_GIT_VERSION.tar.xz" "Natron-$REL_GIT_VERSION" || exit 1
    rm -rf "Natron-$REL_GIT_VERSION" || exit 1
    cd Natron || exit 1 
fi

#Always bump NATRON_DEVEL_GIT, it is only used to version-stamp binaries
NATRON_REL_V="$REL_GIT_VERSION"

sed -i "s/NATRON_DEVEL_GIT=.*/NATRON_DEVEL_GIT=${NATRON_REL_V}/" "$CWD/commits-hash.sh" || exit 1

NATRON_MAJOR=`grep "define NATRON_VERSION_MAJOR" $TMP_BUILD_DIR/Natron/Global/Macros.h | awk '{print $3}'`
NATRON_MINOR=`grep "define NATRON_VERSION_MINOR" $TMP_BUILD_DIR/Natron/Global/Macros.h | awk '{print $3}'`
NATRON_REVISION=`grep "define NATRON_VERSION_REVISION" $TMP_BUILD_DIR/Natron/Global/Macros.h | awk '{print $3}'`
sed -i "s/NATRON_VERSION_NUMBER=.*/NATRON_VERSION_NUMBER=${NATRON_MAJOR}.${NATRON_MINOR}.${NATRON_REVISION}/" "$CWD/commits-hash.sh" || exit 1

echo
echo "Building Natron $NATRON_REL_V from $NATRON_BRANCH against SDK $SDK_VERSION on Windows-$BIT using $MKJOBS threads."
echo
sleep 2

#Make sure GitVersion.h is in sync with NATRON_GIT_VERSION
cat "$INC_PATH/natron/GitVersion.h" | sed -e "s#__BRANCH__#${NATRON_BRANCH}#;s#__COMMIT__#${REL_GIT_VERSION}#" > Global/GitVersion.h || exit 1

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
#EXTRA_QMAKE_FLAG="CONFIG+=gbreakpad" Enable when ready
$INSTALL_PATH/bin/qmake -r CONFIG+=relwithdebinfo CONFIG+=silent ${EXTRA_QMAKE_FLAG} CONFIG+=${BIT}bit DEFINES+=QT_NO_DEBUG_OUTPUT ../Project.pro || exit 1
make -j${MKJOBS} || exit 1

cp App/release/Natron.exe $INSTALL_PATH/bin/ || exit 1
cp Renderer/release/NatronRenderer.exe $INSTALL_PATH/bin/ || exit 1
if [ -f CrashReporter/release/NatronCrashReporter.exe ]; then
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
fi

cp -a "$SRC_PATH/OpenColorIO-Configs" "$INSTALL_PATH/share/" || exit 1
mkdir -p "$INSTALL_PATH/docs/natron" || exit 1
cp ../LICENSE.txt ../README* ../BUGS* ../CONTRI* ../Documentation/* "$INSTALL_PATH/docs/natron/"
mkdir -p "$INSTALL_PATH/share/stylesheets" || exit 1
cp ../Gui/Resources/Stylesheets/mainstyle.qss "$INSTALL_PATH/share/stylesheets/" || exit 1
mkdir -p $INSTALL_PATH/share/pixmaps || exit 1
cp ../Gui/Resources/Images/natronIcon256_linux.png "$INSTALL_PATH/share/pixmaps/" || exit 1
echo "$NATRON_REL_V" > "$INSTALL_PATH/docs/natron/VERSION" || exit 1

echo "Done!"

