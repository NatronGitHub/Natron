#!/bin/bash
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2016 INRIA and Alexandre Gauthier
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

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

# PYV=3 Use Python 3 otherwise use Python 2
# MKJOBS=X: Nb threads, 4 is default as defined in common
# BUILD_CONFIG=(SNAPSHOT,ALPHA,BETA,RC,STABLE,CUSTOM)
# CUSTOM_BUILD_USER_NAME="Toto" : to be set if BUILD_CONFIG=CUSTOM
# BUILD_NUMBER=X: To be set to indicate the revision number of the build. For example RC1,RC2, RC3 etc...
# DISABLE_BREAKPAD=1: Disable automatic crash report
# GIT_BRANCH=xxxx if not defined git tags from common will be used
# GIT_COMMIT=xxxx if not defined latest commit from GIT_BRANCH will be built

source common.sh
source common-buildmaster.sh

QMAKE_FLAGS_EXTRA=()

#QMAKE_FLAGS_EXTRA+=(CONFIG+=silent) # comment for a verbose / non-silent build


PID=$$
if [ "${PIDFILE:-}" != "" ]; then
  echo $PID > "$PIDFILE"
fi

# we need $BUILD_CONFIG
if [ -z "${BUILD_CONFIG:-}" ]; then
    echo "Please define BUILD_CONFIG"
    exit 1
fi

# Assume that $GIT_BRANCH is the branch to build, otherwise if empty use the NATRON_GIT_TAG in common.sh, but if BUILD_CONFIG=SNAPSHOT use MASTER_BRANCH
NATRON_BRANCH="${GIT_BRANCH:-}"
if [ -z "${NATRON_BRANCH:-}" ]; then
    if [ "$BUILD_CONFIG" = "SNAPSHOT" ]; then
        NATRON_BRANCH="${MASTER_BRANCH:-}"
        COMMITS_HASH="$CWD/commits-hash-$NATRON_BRANCH.sh"
    else
        NATRON_BRANCH="${NATRON_GIT_TAG:-}"
        COMMITS_HASH="$CWD/commits-hash.sh"
    fi
else
    COMMITS_HASH="$CWD/commits-hash-$NATRON_BRANCH.sh"
fi

echo "===> Using branch $NATRON_BRANCH"

# Setup tmp
if [ ! -d "$TMP_PATH" ]; then
    mkdir -p "$TMP_PATH"
fi
if [ ! -d "$TMP_PATH/Natron" ]; then
    echo "Error: no Natron source in $TMP_PATH/Natron - execute the checkout-natron.sh script first"
    exit 1
fi
#if [ -d "$TMP_PATH/Natron" ]; then
#    rm -rf "$TMP_PATH/Natron"
#fi

if [ ! -d "$TMP_BINARIES_PATH" ]; then
    mkdir -p "$TMP_BINARIES_PATH"
fi

# Setup env
if [ "$PKGOS" = "Linux" ]; then
    export BOOST_ROOT="$SDK_HOME"
    #export PYTHONHOME="$SDK_HOME"
    export LD_LIBRARY_PATH="$SDK_HOME/lib"
    export PATH="$SDK_HOME/gcc/bin:$SDK_HOME/bin:$HOME/.cabal/bin:$PATH"
    if [ "$ARCH" = "x86_64" ]; then
        export LD_LIBRARY_PATH="$SDK_HOME/gcc/lib64:$LD_LIBRARY_PATH"
    else
        export LD_LIBRARY_PATH="$SDK_HOME/gcc/lib:$LD_LIBRARY_PATH"
    fi
elif [ "$PKGOS" = "Windows" ]; then
#    export PKG_CONFIG_PATH="$SDK_HOME/fontconfig/lib/pkgconfig:$PKG_CONFIG_PATH"
    QTDIR="$SDK_HOME"
elif [ "$PKGOS" = "OSX" ]; then
    QTDIR="$MACPORTS/libexec/qt${QT_VERSION_MAJOR}"
fi
QMAKE="$QTDIR/bin/qmake"

export QTDIR

# python
#if [ "$PYV" = "3" ]; then
#    export PYTHONPATH="$SDK_HOME/lib/python${PYVER}"
    #export PYTHON_INCLUDE="$SDK_HOME/include/python${PYVER}"
#else
#    export PYTHONPATH="$SDK_HOME/lib/python${PYVER}"
    #export PYTHON_INCLUDE="$SDK_HOME/include/python${PYVER}"
#fi

cd "$TMP_PATH"

# kill git if idle for too long
#"$KILLSCRIPT" $PID &
#KILLBOT=$! 


# OpenColorIO-Configs setup
OCIO_CONFIGS_VERSION=""
if [ "$BUILD_CONFIG" = "ALPHA" ] || [ "$BUILD_CONFIG" = "BETA" ] || [ "$BUILD_CONFIG" = "RC" ] || [ "$BUILD_CONFIG" = "STABLE" ]; then
    OCIO_CONFIGS_VERSION=$(echo $NATRON_RELEASE_BRANCH|$GSED 's/RB-//')
else
    case "$NATRON_BRANCH" in
        RB-*)
            OCIO_CONFIGS_VERSION=$(echo $NATRON_BRANCH | sed 's#RB-##;')
            ;;
    esac
fi

if [ "$OCIO_CONFIGS_VERSION" = "" ]; then
    OCIO_CONFIGS_VERSION="3.0"
    echo "Warnning: No OCIO config version found, setting to $OCIO_CONFIGS_VERSION"
fi

OCIO_CONFIGS_URL=https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v${OCIO_CONFIGS_VERSION}.tar.gz

OCIO_CONFIGS_DIR="$SRC_PATH/OpenColorIO-Configs-Natron-v${OCIO_CONFIGS_VERSION}"
if [ ! -d "$OCIO_CONFIGS_DIR" ]; then
    mkdir -p "$SRC_PATH" || true
    $WGET "$OCIO_CONFIGS_URL" -O "$SRC_PATH/OpenColorIO-Configs-Natron-v${OCIO_CONFIGS_VERSION}.tar.gz"
    tar xf "$SRC_PATH/OpenColorIO-Configs-Natron-v${OCIO_CONFIGS_VERSION}.tar.gz" -C "$SRC_PATH"/
    rm -rf "$OCIO_CONFIGS_DIR/aces_1.0.1/baked" || true
    rm -rf "$OCIO_CONFIGS_DIR/aces_1.0.1/python" || true
fi
# set OCIO for Tests
export OCIO="$OCIO_CONFIGS_DIR/nuke-default/config.ocio"

cd Natron

# Get commit
REL_GIT_VERSION=$(git log|head -1|awk '{print $2}')

#Always bump NATRON_DEVEL_GIT, it is only used to version-stamp binaries
NATRON_REL_V="$REL_GIT_VERSION"

NATRON_MAJOR=$(echo "NATRON_VERSION_MAJOR" | $CC -E -P -include "$TMP_PATH/Natron/Global/Macros.h" - | tail -1)
NATRON_MINOR=$(echo "NATRON_VERSION_MINOR" | $CC -E -P -include "$TMP_PATH/Natron/Global/Macros.h" - | tail -1)
NATRON_REVISION=$(echo "NATRON_VERSION_REVISION" | $CC -E -P -include "$TMP_PATH/Natron/Global/Macros.h" - | tail -1)

echo "*** Building Natron ${NATRON_MAJOR}.${NATRON_MINOR}.${NATRON_REVISION} from git commit ${REL_GIT_VERSION}"

if [ ! -f "$COMMITS_HASH" ]; then
    cat <<EOF > "$COMMITS_HASH"
#!/bin/sh
NATRON_DEVEL_GIT=#
IOPLUG_DEVEL_GIT=#
MISCPLUG_DEVEL_GIT=#
ARENAPLUG_DEVEL_GIT=#
GMICPLUG_DEVEL_GIT=#
CVPLUG_DEVEL_GIT=#
NATRON_VERSION_NUMBER=#
EOF
fi

$GSED -e "s/NATRON_DEVEL_GIT=.*/NATRON_DEVEL_GIT=${NATRON_REL_V}/" -e "s/NATRON_VERSION_NUMBER=.*/NATRON_VERSION_NUMBER=${NATRON_MAJOR}.${NATRON_MINOR}.${NATRON_REVISION}/" "$COMMITS_HASH" > "$COMMITS_HASH".new && mv "$COMMITS_HASH".new "$COMMITS_HASH"

BUILD_MODE=""
if [ "${DEBUG_MODE:-}" = "1" ]; then
    # CI build have to be build with CONFIG=relwithdebinfo, or the
    # installer build for plugins won't work (deployment on osx, which
    # copies libraries, is only
    # done in release and relwithdebinfo configs).
    # 
    #BUILD_MODE=debug
    BUILD_MODE=relwithdebinfo
else
    BUILD_MODE=relwithdebinfo
fi

echo "===> Building Natron $NATRON_REL_V from $NATRON_BRANCH using $MKJOBS threads."

# Plugins git hash
IO_VERSION_FILE="$TMP_BINARIES_PATH/OFX/Plugins/IO.ofx.bundle-version.txt"
MISC_VERSION_FILE="$TMP_BINARIES_PATH/OFX/Plugins/Misc.ofx.bundle-version.txt"
ARENA_VERSION_FILE="$TMP_BINARIES_PATH/OFX/Plugins/Arena.ofx.bundle-version.txt"
GMIC_VERSION_FILE="$TMP_BINARIES_PATH/OFX/Plugins/GMIC.ofx.bundle-version.txt"

if [ -f "$IO_VERSION_FILE" ]; then
    IO_GIT_HASH=$(cat "${IO_VERSION_FILE}")
fi
if [ -f "$MISC_VERSION_FILE" ]; then
    MISC_GIT_HASH=$(cat "${MISC_VERSION_FILE}")
fi
if [ -f "$ARENA_VERSION_FILE" ]; then
    ARENA_GIT_HASH=$(cat "${ARENA_VERSION_FILE}")
fi
if [ -f "$GMIC_VERSION_FILE" ]; then
    GMIC_GIT_HASH=$(cat "${GMIC_VERSION_FILE}")
fi

if [ "${IO_GIT_HASH:-}" = "" ] || [ "${MISC_GIT_HASH:-}" = "" ] || [ "${ARENA_GIT_HASH:-}" = "" ] || [ "${GMIC_GIT_HASH:-}" = "" ]; then
    echo "PLUGINS GIT COMMIT MISSING!!!"
fi

#Update GitVersion to have the correct hash
$GSED "s#__BRANCH__#${NATRON_BRANCH}#;s#__COMMIT__#${REL_GIT_VERSION}#;s#__IO_COMMIT__#${IO_GIT_HASH:-}#;s#__MISC_COMMIT__#${MISC_GIT_HASH:-}#;s#__ARENA_COMMIT__#${ARENA_GIT_HASH:-}#;s#__GMIC_COMMIT__#${GMIC_GIT_HASH:-}#" "$INC_PATH/natron/GitVersion.h" > Global/GitVersion.h

# add config based on python version
if [ "$NATRON_MAJOR" -ge "3" ]; then
    cat "$INC_PATH/natron/${PKGOS}_natron3.pri" > config.pri
else
    if [ "$PYV" = "3" ]; then
        cat "$INC_PATH/natron/${PKGOS}_py3.pri" > config.pri
    else
        cat "$INC_PATH/natron/${PKGOS}.pri" > config.pri
    fi
fi

if [ "${SDK_VERSION:-}" = "CY2016" ] || [ "${SDK_VERSION:-}" = "CY2017" ]; then
    cat "$INC_PATH/natron/${SDK_VERSION}.pri" > config.pri
    rm -f Engine/NatronEngine/* Gui/NatronGui/*
    shiboken2 --avoid-protected-hack --enable-pyside-extensions --include-paths="../Engine:../Global:${SDK_HOME}/include:${SDK_HOME}/include/PySide2"  --typesystem-paths="${SDK_HOME}/share/PySide2/typesystems" --output-directory=Engine Engine/Pyside_Engine_Python.h  Engine/typesystem_engine.xml
    shiboken2 --avoid-protected-hack --enable-pyside-extensions --include-paths="../Engine:../Gui:../Global:${SDK_HOME}/include:${SDK_HOME}/include/PySide2"  --typesystem-paths="${SDK_HOME}/share/PySide2/typesystems:Engine" --output-directory=Gui Gui/Pyside_Gui_Python.h  Gui/typesystem_natronGui.xml
    sh tools/utils/runPostShiboken.sh
fi

echo "*** config.pri:"
echo "========================================================================"
cat config.pri
echo "========================================================================"


# setup build dir
if [ "${QMAKE_BUILD_SUBDIR:-}" = "1" ]; then
    # disabled by default, because it does not always work right, eg for Info.plist creation on macOS
    # Since we delete the sources after the build, we really don't care
    rm -rf build || true
    mkdir build
    cd build
    srcdir=..
else
    srcdir=.
fi

# setup version
if [ "$BUILD_CONFIG" = "SNAPSHOT" ]; then
    QMAKE_FLAGS_EXTRA+=(CONFIG+=snapshot)
elif [ "$BUILD_CONFIG" = "ALPHA" ]; then
    QMAKE_FLAGS_EXTRA+=(CONFIG+=alpha  BUILD_NUMBER="$BUILD_NUMBER")
    if [ -z "${BUILD_NUMBER:-}" ]; then
        echo "You must supply a BUILD_NUMBER when BUILD_CONFIG=ALPHA"
        exit 1
    fi
elif [ "$BUILD_CONFIG" = "BETA" ]; then
    if [ -z "${BUILD_NUMBER:-}" ]; then
        echo "You must supply a BUILD_NUMBER when BUILD_CONFIG=BETA"
        exit 1
    fi
    QMAKE_FLAGS_EXTRA+=(CONFIG+=beta  BUILD_NUMBER="$BUILD_NUMBER")
elif [ "$BUILD_CONFIG" = "RC" ]; then
    if [ -z "${BUILD_NUMBER:-}" ]; then
        echo "You must supply a BUILD_NUMBER when BUILD_CONFIG=RC"
        exit 1
    fi
    QMAKE_FLAGS_EXTRA+=(CONFIG+=RC BUILD_NUMBER="$BUILD_NUMBER")
elif [ "$BUILD_CONFIG" = "STABLE" ]; then
    QMAKE_FLAGS_EXTRA+=(CONFIG+=stable)
elif [ "$BUILD_CONFIG" = "CUSTOM" ]; then
    if [ -z "${CUSTOM_BUILD_USER_NAME:-}" ]; then
        echo "You must supply a CUSTOM_BUILD_USER_NAME when BUILD_CONFIG=CUSTOM"
        exit 1
    fi
    QMAKE_FLAGS_EXTRA+=(CONFIG+=stable BUILD_USER_NAME="$CUSTOM_BUILD_USER_NAME")
fi

if [ "$PYV" = "3" ]; then
    PYO="PYTHON_CONFIG=python${PYVER}m-config"
fi

# use breakpad?
if [ "${DISABLE_BREAKPAD:-}" = "1" ]; then
    QMAKE_FLAGS_EXTRA+=(CONFIG-=enable-breakpad)
else
    QMAKE_FLAGS_EXTRA+=(CONFIG+=enable-breakpad)
fi

if [ "$COMPILER" = "clang" ]; then
    QMAKE_FLAGS_EXTRA+=(CONFIG-=openmp)
else
    QMAKE_FLAGS_EXTRA+=(CONFIG+=openmp)
fi

QMAKE_FLAGS_EXTRA+=(CONFIG+=enable-osmesa OSMESA_PATH="$OSMESA_PATH" LLVM_PATH="$LLVM_PATH" CONFIG+=enable-cairo)


# mac compiler
if [ "$PKGOS" = "OSX" ]; then
    if [ "$COMPILER" = "clang" ] || [ "$COMPILER" = "clang-omp" ]; then
        SPEC=unsupported/macx-clang
    else
        SPEC=macx-g++
    fi
    QBITS=$(echo "${BITS}" | awk '{print tolower($0)}')
    QMAKE_FLAGS_EXTRA+=(-spec "$SPEC" CONFIG+="${QBITS}" QMAKE_MACOSX_DEPLOYMENT_TARGET="$MACOSX_DEPLOYMENT_TARGET")
fi

if [ "${DEBUG_MODE:-}" != "1" ]; then
    QMAKE_FLAGS_EXTRA+=(CONFIG+=noassertions CONFIG+=fast)
fi

# build
echo "env CFLAGS=\"${BF:-}\" CXXFLAGS=\"${BF:-}\" \"$QMAKE\" -r CONFIG+=\"$BUILD_MODE\" QMAKE_CC=\"$CC\" QMAKE_CXX=\"$CXX\" QMAKE_LINK=\"$CXX\" QMAKE_OBJECTIVE_CC=\"$OBJECTIVE_CC\" QMAKE_OBJECTIVE_CXX=\"$OBJECTIVE_CXX\" ${QMAKE_FLAGS_EXTRA[*]} ${PYO:-} "$srcdir"/Project.pro"
env CFLAGS="${BF:-}" CXXFLAGS="${BF:-}" "$QMAKE" -r CONFIG+="$BUILD_MODE" QMAKE_CC="$CC" QMAKE_CXX="$CXX" QMAKE_LINK="$CXX" QMAKE_OBJECTIVE_CC="$OBJECTIVE_CC" QMAKE_OBJECTIVE_CXX="$OBJECTIVE_CXX" "${QMAKE_FLAGS_EXTRA[@]}" ${PYO:-} "$srcdir"/Project.pro

make -j"${MKJOBS}"
make -j"${MKJOBS}" -C Tests
if [ "$PKGOS" = "OSX" ]; then
    # the app bundle is wrong when building in parallel
    make -C App clean
    make -C App
fi

# install app(s)
rm -f "$TMP_BINARIES_PATH"/Natron* || true
rm -f "$TMP_BINARIES_PATH"/bin/Natron* "$TMP_BINARIES_PATH/bin/Tests" || true

NATRON_CONVERTER="NatronProjectConverter"
NATRON_TEST="Tests"
NATRON_BIN="Natron"
NATRON_PYTHON_BIN="natron-python"
RENDERER_BIN="NatronRenderer"
CRASHGUI="NatronCrashReporter"
CRASHCLI="NatronRendererCrashReporter"
if [ "$PKGOS" = "Windows" ]; then
    WIN_BIN_TYPE=release
    #if [ "${DEBUG_MODE}" = "1" ]; then
    #    WIN_BIN_TYPE=debug
    #fi
    NATRON_CONVERTER="$WIN_BIN_TYPE/${NATRON_CONVERTER}.exe"
    NATRON_PYTHON_BIN="$WIN_BIN_TYPE/${NATRON_PYTHON_BIN}.exe"
    NATRON_TEST="$WIN_BIN_TYPE/${NATRON_TEST}.exe"
    NATRON_BIN="$WIN_BIN_TYPE/${NATRON_BIN}.exe"
    RENDERER_BIN="$WIN_BIN_TYPE/${RENDERER_BIN}.exe"
    CRASHGUI="$WIN_BIN_TYPE/${CRASHGUI}.exe"
    CRASHCLI="$WIN_BIN_TYPE/${CRASHCLI}.exe"
fi

if [ ! -d "$TMP_BINARIES_PATH/bin" ]; then
    mkdir -p "$TMP_BINARIES_PATH/bin"
fi

if [ "$PKGOS" = "OSX" ]; then
    MAC_APP_PATH=/Natron.app/Contents/MacOS
    MAC_INFOPLIST=/Natron.app/Contents/Info.plist
    MAC_PKGINFO=/Natron.app/Contents/PkgInfo
    MAC_CRASH_PATH=/NatronCrashReporter.app/Contents/MacOS
fi


#rm -f $TMP_BINARIES_PATH/bin/{$NATRON_TEST,$NATRON_BIN,$RENDERER_BIN,$CRASHGUI,$CRASHCLI,$NATRON_CONVERTER} || true
#if [ "$PKGOS" = "Windows" ]; then
#    rm -f "$TMP_BINARIES_PATH"/bin/Natron*exe || true
#fi

echo "${NATRON_MAJOR}.${NATRON_MINOR}" > "$TMP_BINARIES_PATH/natron-version.txt"
echo "${NATRON_MAJOR}.${NATRON_MINOR}.${NATRON_REVISION}" > "$TMP_BINARIES_PATH/natron-fullversion.txt"

cp Tests/$NATRON_TEST "$TMP_BINARIES_PATH/bin/"
cp App${MAC_APP_PATH:-}/$NATRON_BIN "$TMP_BINARIES_PATH/bin/"

# copy Info.plist and PkgInfo
if [ "$PKGOS" = "OSX" ]; then
    ls -lR App/Natron.app
    # Note: Info.plist generation only works if compiling in the sources,
    # NOT in a "build" subdir (at least in qt4).
    cp "App${MAC_INFOPLIST}" "$TMP_BINARIES_PATH/bin/"
    cp "App${MAC_PKGINFO}" "$TMP_BINARIES_PATH/bin/"
fi

cp Renderer/$RENDERER_BIN "$TMP_BINARIES_PATH/bin/"

if [ -f ProjectConverter/$NATRON_CONVERTER ]; then
    cp ProjectConverter/$NATRON_CONVERTER "${TMP_BINARIES_PATH}/bin/"
fi

if [ -f PythonBin/$NATRON_PYTHON_BIN ]; then
    cp PythonBin/$NATRON_PYTHON_BIN "${TMP_BINARIES_PATH}/bin/"
fi

mkdir -p "$TMP_BINARIES_PATH/docs/natron" || true
cp "$srcdir"/LICENSE.txt "$TMP_BINARIES_PATH/docs/natron/"

# install crashapp(s)
if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    cp CrashReporter${MAC_CRASH_PATH:-}/$CRASHGUI "$TMP_BINARIES_PATH/bin/"
    cp CrashReporterCLI/$CRASHCLI "$TMP_BINARIES_PATH/bin/"
fi

RES_DIR="$TMP_BINARIES_PATH/Resources"
if [ ! -d "$RES_DIR" ]; then
    mkdir -p "$RES_DIR"
fi

# copy everything else we need
rm -rf "$RES_DIR/OpenColorIO-Configs" || true
mkdir -p "$RES_DIR/OpenColorIO-Configs"

OCIO_CONFIGS=()
if [ "$OCIO_CONFIGS_VERSION" = "2.0" ]; then
    OCIO_CONFIGS+=(blender nuke-default)
else
    OCIO_CONFIGS+=(blender natron nuke-default)
fi

echo "*** Copying OpenColorIO profiles from $OCIO_CONFIGS_DIR to $RES_DIR/OpenColorIO-Configs (profiles: ${OCIO_CONFIGS[*]})"
for profile in "${OCIO_CONFIGS[@]}"; do
    if [ -d "$OCIO_CONFIGS_DIR/${profile}" ]; then
        cp -a "$OCIO_CONFIGS_DIR/${profile}" "$RES_DIR/OpenColorIO-Configs/"
    else
        echo "$profile NOT FOUND! THIS SHOULD NOT HAPPEN"
    fi
done
echo "*** $RES_DIR/OpenColorIO-Configs now contains:"
ls -l "$RES_DIR/OpenColorIO-Configs" "$RES_DIR/OpenColorIO-Configs/blender/config.ocio"

mkdir -p "$RES_DIR/stylesheets"
cp "$srcdir"/Gui/Resources/Stylesheets/mainstyle.qss "$RES_DIR/stylesheets/"

if [ "$PKGOS" != "OSX" ]; then
    mkdir -p "$RES_DIR/pixmaps" || true
fi

if [ "$PKGOS" = "Linux" ]; then
    cp "$srcdir"/Gui/Resources/Images/natronIcon256_linux.png "$RES_DIR/pixmaps/"
    cp "$srcdir"/Gui/Resources/Images/natronProjectIcon_linux.png "$RES_DIR/pixmaps/"
elif [ "$PKGOS" = "Windows" ]; then
    cp "$srcdir"/Gui/Resources/Images/natronProjectIcon_windows.ico "$RES_DIR/pixmaps/"
elif [ "$PKGOS" = "OSX" ]; then
    cp "$srcdir"/Gui/Resources/Images/*.icns "$RES_DIR/"
    cp "$srcdir"/Gui/Resources/Images/splashscreen.* "$TMP_BINARIES_PATH/"
    #cp -a App/Natron.app/Contents/Resources/etc "$RES_DIR/"
    # quickfix
    cp -a "$srcdir"/Gui/Resources/etc "$RES_DIR/"
fi

rm -rf "$TMP_BINARIES_PATH/PyPlugs" || true
mkdir -p "$TMP_BINARIES_PATH/PyPlugs"
cp "$srcdir"/Gui/Resources/PyPlugs/* "$TMP_BINARIES_PATH/PyPlugs/"
if [ "$PKGOS" = "Linux" ]; then
    mkdir -p "$RES_DIR/etc/fonts/conf.d"
    cp "$srcdir"/Gui/Resources/etc/fonts/fonts.conf "$RES_DIR/etc/fonts/"
    #cp "$srcdir"/Gui/Resources/share/fontconfig/conf.avail/* "$RES_DIR/etc/fonts/conf.d/"
    cp "$srcdir"/Gui/Resources/etc/fonts/conf.d/* "$RES_DIR/etc/fonts/conf.d/"
fi

export NATRON_PLUGIN_PATH="$TMP_BINARIES_PATH/PyPlugs"
export OFX_PLUGIN_PATH="$TMP_BINARIES_PATH/OFX/Plugins"
if [ "$PKGOS" = "Linux" ]; then
    export FONTCONFIG_FILE="$RES_DIR"/etc/fonts/fonts.conf
elif [ "$PKGOS" = "OSX" ]; then
    export FONTCONFIG_FILE="$RES_DIR"/etc/fonts/fonts.conf
fi
if [ "$PKGOS" = "Linux" ]; then
    rm -rf "$HOME/.cache/INRIA/Natron"* &> /dev/null || true
    ln -sf "$SDK_HOME"/lib .
    # Note: Several suppression files can be passed to valgrind.
    # There is an automatic tool to generate libc/libstdc++/Qt
    # suppressions at https://github.com/AlekSi/valgrind-suppressions
    env LD_LIBRARY_PATH="$SDK_HOME/gcc/lib:$SDK_HOME/gcc/lib64:$SDK_HOME/lib:$FFMPEG_PATH/lib:$LIBRAW_PATH/lib:$QTDIR/lib" $TIMEOUT -s KILL 1800 valgrind --tool=memcheck --suppressions="$INC_PATH/natron/valgrind-python.supp" Tests/Tests
    #env LD_LIBRARY_PATH="$SDK_HOME/gcc/lib:$SDK_HOME/gcc/lib64:$SDK_HOME/lib:$FFMPEG_PATH/lib:$LIBRAW_PATH/lib:$QTDIR/lib" $TIMEOUT -s KILL 1800 Tests/Tests
    rm -f lib || true
    # ITS NOT POSSIBLE TO RUN THE WIN TESTS HERE, DO IT IN THE INSTALLER SCRIPT

elif [ "$PKGOS" = "Windows" ]; then
    #rm -rf /c/Users/NatronWin/AppData/Local/INRIA/Natron &> /dev/null || true
    rm -rf "$LOCALAPPDATA\\INRIA\\Natron"* &> /dev/null || true
    mkdir tmpdll
    pushd tmpdll
    #ls -R "$OFX_PLUGIN_PATH"
    cp "$OFX_PLUGIN_PATH"/*.ofx.bundle/Contents/*/*.ofx .
    for f in *.ofx; do
        dll="${f%\.ofx}.dll"
        mv "$f" "$dll"
        echo "Dependencies for $f:"
        env PATH="$FFMPEG_PATH/bin:$LIBRAW_PATH/bin:$QTDIR/bin:$OSMESA_PATH/lib:$SDK_HOME/bin:${PATH:-}" ldd "$dll"
    done
    popd
    rm -rf tmpdll
    testfail=0
    env PYTHONHOME="$SDK_HOME" PATH="$FFMPEG_PATH/bin:$LIBRAW_PATH/bin:$QTDIR/bin:$OSMESA_PATH/lib:$SDK_HOME/bin:${PATH:-}" $TIMEOUT -s KILL 1800 Tests/$WIN_BIN_TYPE/Tests.exe || testfail=1
    if [ $testfail != 0 ]; then
        echo "*** WARNING: Natron tests FAILED"
    fi
    # sometimes Tests.exe just hangs. Try taskkill first, then tskill if it fails because of a message like:
    # $ taskkill -f -im Tests.exe -t
    # ERROR: The process with PID 3260 (child process of PID 3816) could not be terminated.
    # Reason: There is no running instance of the task.
    # mapfile use: see https://github.com/koalaman/shellcheck/wiki/SC2207
    mapfile -t processes < <(taskkill -f -im Tests.exe -t 2>&1 |grep "ERROR: The process with PID"| awk '{print $6}' || true)
    for p in "${processes[@]}"; do
        tskill "$p" || true
    done


elif [ "$PKGOS" = "OSX" ]; then
    rm -rf  "$HOME/Library/Caches/INRIA/Natron"* &> /dev/null || true
    # setting DYLD_LIBRARY_PATH breaks many things. We get this error on 10.9 Mavericks, because /opt/local/lib/libiconv.2.dylib and /usr/lib/libiconv.2.dylib do not contain the same symbols
    #dyld: Symbol not found: _iconv
    #  Referenced from: /usr/lib/libcups.2.dylib
    #  Expected in: /opt/local/lib/libiconv.2.dylib
    # in /usr/lib/libcups.2.dylib
    #Trace/BPT trap: 5
    # However, The universal build on OS X 10.6 requires this
    if [ "$MACOSX_DEPLOYMENT_TARGET" = "10.6" ] && [ "$COMPILER" = "gcc" ]; then
        env DYLD_LIBRARY_PATH="$MACPORTS"/lib/libgcc:/System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/ImageIO.framework/Versions/A/Resources:"$MACPORTS"/lib $TIMEOUT -s KILL 1800 Tests/Tests
    else
        $TIMEOUT -s KILL 1800 Tests/Tests
    fi
fi

######################
# Make documentation

cd "$srcdir"/Documentation
# generating the docs requires a recent pandoc, else a spurious "{ width=10% }" will appear after each plugin logo
# pandoc 1.19.1 runs OK, but MacPorts has an outdated version (1.12.4.2), and Ubuntu 16LTS has 1.16.0.2
# For now, we regenerate the docs from a CI or SNAPSHOT build on Fred's mac, using:
# cd Development/Natron/Documentation
# env FONTCONFIG_FILE=/Applications/Natron.app/Contents/Resources/etc/fonts/fonts.conf OCIO=/Applications/Natron.app/Contents/Resources/OpenColorIO-Configs/nuke-default/config.ocio OFX_PLUGIN_PATH=/Applications/Natron.app/Contents/Plugins bash "$srcdir"/tools/genStaticDocs.sh /Applications/Natron.app/Contents/MacOS/NatronRenderer  $TMPDIR/natrondocs .
GENDOCS=0
if [ "$GENDOCS" = 1 ] && command -v pandoc > /dev/null 2>&1; then
    
    # generate the plugins doc first
    if [ "$PKGOS" = "Linux" ]; then
        env LD_LIBRARY_PATH="$SDK_HOME/gcc/lib:$SDK_HOME/gcc/lib64:$SDK_HOME/lib:$FFMPEG_PATH/lib:$LIBRAW_PATH/lib:$QTDIR/lib" "$srcdir"/tools/genStaticDocs.sh "$TMP_BINARIES_PATH"/bin/NatronRenderer "$TMPDIR/natrondocs$$" .
    elif [ "$PKGOS" = "Windows" ]; then
        echo "TODO: generate doc on windows"
        true
    elif [ "$PKGOS" = "OSX" ]; then
        env DYLD_LIBRARY_PATH=/System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/ImageIO.framework/Versions/A/Resources:"$MACPORTS"/lib "$srcdir"/tools/genStaticDocs.sh "$TMP_BINARIES_PATH"/bin/NatronRenderer "$TMPDIR/natrondocs$$" .
    fi
fi

SPHINX_BIN=""
if [ "$PKGOS" = "Windows" ]; then
    #SPHINX_BIN="sphinx-build2.exe" # fails with 'failed to create process (\mingw64\bin\python2.exe "C:\msys64\mingw64\bin\sphinx-build2-script.py").'
    SPHINX_BIN="sphinx-build2-script.py"
elif [ "$PKGOS" = "OSX" ]; then
    SPHINX_BIN="sphinx-build-${PYVER}"
else
    SPHINX_BIN="sphinx-build"
fi

if [ -d "$RES_DIR/docs" ]; then
    rm -rf "$RES_DIR/docs"
fi
mkdir -p "$RES_DIR/docs"

$SPHINX_BIN -b html source html
cp -a html "$RES_DIR/docs/"

GENPDF=0
if [ "$GENPDF" = 1 ]; then
    $SPHINX_BIN -b latex source pdf
    pushd pdf
    # we have to run twice pdflatex, then makeindex, then pdflatex again
    pdflatex -shell-escape -interaction=nonstopmode -file-line-error Natron.tex | grep ".*:[0-9]*:.*"
    pdflatex -shell-escape -interaction=nonstopmode -file-line-error Natron.tex | grep ".*:[0-9]*:.*"
    makeindex Natron
    pdflatex -shell-escape -interaction=nonstopmode -file-line-error Natron.tex | grep ".*:[0-9]*:.*"
    popd
    cp pdf/Natron.pdf "$RES_DIR/docs/"
fi

#rm -f $KILLSCRIPT || true

echo "Done!"

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
