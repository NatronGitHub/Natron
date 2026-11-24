#!/bin/bash
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2013-2018 INRIA and Alexandre Gauthier
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

# DISABLE_BREAKPAD=1: Disable automatic crash report
# NATRON_BUILD_SYSTEM: cmake (the default) or qmake (deprecated, kept on RB-2.6 as a
#   fallback until the CMake-built installers are shown to be equivalent, see #1181)
# NATRON_EXTRA_CMAKE_FLAGS: extra arguments for cmake (CMake build), e.g. "-DNATRON_FAST_MATH=ON"
# NATRON_EXTRA_QMAKE_FLAGS: extra arguments for qmake (qmake build)

source common.sh
source manageBuildOptions.sh
source manageLog.sh
updateBuildOptions

NATRON_BUILD_SYSTEM="${NATRON_BUILD_SYSTEM:-cmake}"
case "$NATRON_BUILD_SYSTEM" in
    cmake|qmake) ;;
    *)
        echo "Error: NATRON_BUILD_SYSTEM must be cmake or qmake, not \"$NATRON_BUILD_SYSTEM\""
        exit 1
        ;;
esac
if [ "$NATRON_BUILD_SYSTEM" = "cmake" ] && [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    # launchBuildMain.sh sets DISABLE_BREAKPAD=1 for CMake builds
    echo "Error: the CMake build has no Breakpad crash reporter (#1183): set DISABLE_BREAKPAD=1, or NATRON_BUILD_SYSTEM=qmake"
    exit 1
fi

NO_BUILD=0
# If Natron is already built, exit when debugging scripts
if [ "${DEBUG_SCRIPTS:-}" = "1" ]; then
    if [ -f "$TMP_BINARIES_PATH/bin/Natron" ] || [ -f "$TMP_BINARIES_PATH/Natron.app/Contents/MacOS/Natron" ]; then
        NO_BUILD=1
    fi
fi


# Setup env
if [ "$PKGOS" = "Linux" ]; then
    #export PYTHONHOME="$SDK_HOME"
    export LD_LIBRARY_PATH="$SDK_HOME/lib:$SDK_HOME/qt${QT_VERSION_MAJOR}/lib"
    export PATH="$SDK_HOME/bin:$SDK_HOME/qt${QT_VERSION_MAJOR}/bin:$HOME/.cabal/bin:$PATH"
    export C_INCLUDE_PATH="${SDK_HOME}/include:${SDK_HOME}/qt${QT_VERSION_MAJOR}/include"
    export CPLUS_INCLUDE_PATH="${C_INCLUDE_PATH}"
fi
# QMAKE is resolved by common.sh, which knows about version-suffixed qmake names
# (set NATRON_QMAKE to override it).
QMAKE="${QMAKE:-$QTDIR/bin/qmake}"

cd "$TMP_PATH"

# OpenColorIO-Configs setup
OCIO_CONFIGS_VERSION=${OCIO_CONFIGS_VERSION:-}
if [ "$NATRON_BUILD_CONFIG" = "ALPHA" ] || [ "$NATRON_BUILD_CONFIG" = "BETA" ] || [ "$NATRON_BUILD_CONFIG" = "RC" ] || [ "$NATRON_BUILD_CONFIG" = "STABLE" ]; then
    OCIO_CONFIGS_VERSION=$(echo $NATRON_GIT_BRANCH | sed 's#tags/v##;' | cut -c1-3)
elif [ "$OCIO_CONFIGS_VERSION" = "" ]; then
    case "$NATRON_GIT_BRANCH" in
        *RB-*)
            OCIO_CONFIGS_VERSION=$(echo $NATRON_GIT_BRANCH | sed 's#.*RB-##;')
            ;;
    esac
fi

if [ "$OCIO_CONFIGS_VERSION" = "" ]; then
    OCIO_CONFIGS_VERSION="3.0"
    echo "Warning: No OCIO config version found, setting to $OCIO_CONFIGS_VERSION"
fi

OCIO_CONFIGS_URL=https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v${OCIO_CONFIGS_VERSION}.tar.gz

OCIO_CONFIGS_DIR="$SRC_PATH/OpenColorIO-Configs-Natron-v${OCIO_CONFIGS_VERSION}"
if [ ! -d "$OCIO_CONFIGS_DIR" ]; then
    mkdir -p "$SRC_PATH" || true
    $CURL "$OCIO_CONFIGS_URL" --output "$SRC_PATH/OpenColorIO-Configs-Natron-v${OCIO_CONFIGS_VERSION}.tar.gz"
    tar xf "$SRC_PATH/OpenColorIO-Configs-Natron-v${OCIO_CONFIGS_VERSION}.tar.gz" -C "$SRC_PATH"/
    rm -rf "$OCIO_CONFIGS_DIR/aces_1.0.1/baked" || true
    rm -rf "$OCIO_CONFIGS_DIR/aces_1.0.1/python" || true
fi

cd Natron

# color
if [ ! -d "OpenColorIO-Configs" ]; then
    ln -sf "$OCIO_CONFIGS_DIR" OpenColorIO-Configs
fi


echo "===> Building Natron $NATRON_GIT_COMMIT from $NATRON_GIT_BRANCH using $MKJOBS threads."

if [ -n "$OPENFX_IO_GIT_COMMIT" ]; then
    IO_GIT_HASH="$OPENFX_IO_GIT_COMMIT"
else
    IO_GIT_HASH="$OPENFX_IO_GIT_BRANCH"
fi
if [ -n "$OPENFX_MISC_GIT_COMMIT" ]; then
    MISC_GIT_HASH="$OPENFX_MISC_GIT_COMMIT"
else
    MISC_GIT_HASH="$OPENFX_MISC_GIT_BRANCH"
fi
if [ -n "$OPENFX_ARENA_GIT_COMMIT" ]; then
    ARENA_GIT_HASH="$OPENFX_ARENA_GIT_COMMIT"
else
    ARENA_GIT_HASH="$OPENFX_ARENA_GIT_BRANCH"
fi
if [ -n "$OPENFX_GMIC_GIT_COMMIT" ]; then
    GMIC_GIT_HASH="$OPENFX_GMIC_GIT_COMMIT"
else
    GMIC_GIT_HASH="$OPENFX_GMIC_GIT_BRANCH"
fi


if [ "${IO_GIT_HASH:-}" = "" ] || [ "${MISC_GIT_HASH:-}" = "" ] || [ "${ARENA_GIT_HASH:-}" = "" ] || [ "${GMIC_GIT_HASH:-}" = "" ]; then
    echo "PLUGINS GIT COMMIT MISSING, did you call build-plugins.sh first?"
fi

#Update GitVersion to have the correct hash
$GSED "s#__BRANCH__#${NATRON_GIT_BRANCH}#;s#__COMMIT__#${NATRON_GIT_COMMIT}#;s#__IO_COMMIT__#${IO_GIT_HASH:-}#;s#__MISC_COMMIT__#${MISC_GIT_HASH:-}#;s#__ARENA_COMMIT__#${ARENA_GIT_HASH:-}#;s#__GMIC_COMMIT__#${GMIC_GIT_HASH:-}#" "$INC_PATH/natron/GitVersion.h" > Global/GitVersion.h

if [ "$NATRON_BUILD_SYSTEM" = "qmake" ]; then
    # add config based on python version
    if [ "$NATRON_VERSION_MAJOR" -ge "3" ]; then
        cat "$INC_PATH/natron/${PKGOS}_natron3.pri" > config.pri
    else
        cat "$INC_PATH/natron/${PKGOS}.pri" > config.pri
    fi

    echo "*** config.pri:"
    echo "========================================================================"
    cat config.pri
    echo "========================================================================"
fi

# Generate pyside bindings (cmake generates these during the build, qmake can't)
if [ "$NATRON_BUILD_SYSTEM" = "qmake" ] && [ "$QT_VERSION_MAJOR" = 5 ]; then
    UNIX_PYTHON_HOME="${PYTHON_HOME}"
    UNIX_SDK_HOME="${SDK_HOME}"
    case "$system" in
    Linux)

        ;;
    Msys|MSYS_NT-*|MINGW64_NT-*|MINGW32_NT-*|UCRT64_NT-*|CLANG64_NT-*|CLANGARM64_NT-*)
        UNIX_PYTHON_HOME="$(cygpath -u "${PYTHON_HOME}")"
        UNIX_SDK_HOME="$(cygpath -u "${SDK_HOME}")"
        ;;
    Darwin)
        # Check for a missing link in the MacPorts package
        [ ! -f ${PYTHON_HOME}/lib/python${PYVER}/site-packages/shiboken2_generator/shiboken2-${PYVER} ] && (echo "Error: broken MacPort install"; echo "Please execute:"; echo "sudo ln -s shiboken2 ${PYTHON_HOME}/lib/python${PYVER}/site-packages/shiboken2_generator/shiboken2-${PYVER}"; echo "and relaunch."; exit 1)
        ;;
    *)
        ;;
    esac

    rm Engine/Qt${QT_VERSION_MAJOR}/NatronEngine/* Gui/Qt${QT_VERSION_MAJOR}/NatronGui/* || true
    SHIBOKEN_INCLUDE_PATHS="-I. -I./Engine -I./Global -Ilibs/OpenFX/include -I${UNIX_SDK_HOME}/include -I${QTDIR}/include -I${QTDIR}/include/QtCore -I${UNIX_PYTHON_HOME}/include/python${PYVER} -I${UNIX_PYTHON_HOME}/include/PySide2 -I${UNIX_PYTHON_HOME}/include/PySide2/QtCore -I${UNIX_PYTHON_HOME}/include/PySide2/QtGui -I${UNIX_PYTHON_HOME}/lib/python${PYVER}/site-packages/PySide2/include -I${UNIX_PYTHON_HOME}/lib/python${PYVER}/site-packages/PySide2/include/QtCore -I${UNIX_PYTHON_HOME}/lib/python${PYVER}/site-packages/PySide2/include/QtGui"
    SHIBOKEN_TYPESYSTEM_PATHS="-T${UNIX_PYTHON_HOME}/share/PySide2/typesystems -T${UNIX_PYTHON_HOME}/lib/python${PYVER}/site-packages/PySide2/typesystems"
    shiboken2 -std=c++17 --avoid-protected-hack --enable-pyside-extensions ${SHIBOKEN_INCLUDE_PATHS} ${SHIBOKEN_TYPESYSTEM_PATHS} --output-directory=Engine/Qt${QT_VERSION_MAJOR} Engine/PySide2_Engine_Python.h  Engine/typesystem_engine.xml

    shiboken2 -std=c++17 --avoid-protected-hack --enable-pyside-extensions ${SHIBOKEN_INCLUDE_PATHS} -I${QTDIR}/include/QtWidgets -I${QTDIR}/include/QtGui -I${QTDIR}/include/QtCore  ${SHIBOKEN_TYPESYSTEM_PATHS} -T./Engine -T./Shiboken --output-directory=Gui/Qt${QT_VERSION_MAJOR} Gui/PySide2_Gui_Python.h  Gui/typesystem_natronGui.xml

    python3 tools/utils/sourceCleanup.py Engine/typesystem_engine.xml Engine/Qt${QT_VERSION_MAJOR}
    python3 tools/utils/sourceCleanup.py Gui/typesystem_natronGui.xml Gui/Qt${QT_VERSION_MAJOR}

fi

OCIO_CONFIGS=()
if [ "$OCIO_CONFIGS_VERSION" = "2.0" ]; then
    OCIO_CONFIGS+=(blender nuke-default)
else
    OCIO_CONFIGS+=(blender natron nuke-default)
fi

RES_DIR="$TMP_BINARIES_PATH/Resources"

if [ "$NATRON_BUILD_SYSTEM" = "cmake" ]; then
    # CMake build: configure, build, "cmake --install" with the PORTABLE layout into a
    # staging directory, then copy the result to the flat layout that the installer scripts
    # read from $TMP_BINARIES_PATH: bin/ (with Info.plist and PkgInfo on macOS), Resources/,
    # PyPlugs/, natron_convert_project.py, docs/natron/, and on macOS the DMG background.
    # CMake generates the Python bindings during the build.
    CMAKE_BUILD_DIR="$TMP_PATH/Natron-cmake-build"
    CMAKE_INSTALL_DIR="$TMP_PATH/Natron-cmake-install"
    # cmake is a native program on Windows: give it Windows paths
    cmake_path() {
        if [ "$PKGOS" = "Windows" ]; then cygpath -m "$1"; else echo "$1"; fi
    }

    case "$COMPILE_TYPE" in
        debug) CMAKE_BUILD_TYPE=Debug;;
        relwithdebinfo) CMAKE_BUILD_TYPE=RelWithDebInfo;;
        *) CMAKE_BUILD_TYPE=Release;;
    esac

    # $CC, $CXX and $OBJECTIVE_CXX may carry flags (e.g. "clang++-mp-22 -stdlib=libc++ -std=c++17",
    # -std=c++20 with Qt6; see compiler-common.sh):
    # CMake needs the compiler alone, and the flags in CMAKE_<LANG>_FLAGS. A -std=... flag is
    # dropped: CMakeLists.txt sets the C++ standard (C++17 with Qt5, C++20 with Qt6, from NATRON_QT6,
    # with GNU extensions: -std=gnu++17/gnu++20).
    # The compiler path must not contain spaces.
    split_compiler() { # split_compiler VAR "compiler flags...": sets VAR_EXE and VAR_FLAGS
        local words i w flags=""
        read -r -a words <<< "$2"
        for ((i = 1; i < ${#words[@]}; i++)); do
            w="${words[$i]}"
            case "$w" in
                -std=*) ;;
                *) flags="$flags $w";;
            esac
        done
        printf -v "$1_EXE" '%s' "${words[0]}"
        printf -v "$1_FLAGS" '%s' "${flags# }"
    }
    split_compiler CMAKE_CC "$CC"
    split_compiler CMAKE_CXX "$CXX"
    split_compiler CMAKE_OBJCXX "$OBJECTIVE_CXX"

    PYTHON_EXE="$PY_EXE"
    if [ -x "$SDK_HOME/bin/python${PYVER}" ]; then
        PYTHON_EXE="$SDK_HOME/bin/python${PYVER}"
    elif [ -x "$SDK_HOME/bin/python${PYVER}.exe" ]; then
        # MSYS2: keep the .exe, so that "cygpath -m" names an existing file for cmake
        PYTHON_EXE="$SDK_HOME/bin/python${PYVER}.exe"
    fi

    OCIO_CONFIGS_LIST="${OCIO_CONFIGS[*]}"
    CMAKE_FLAGS=(
        -S "$(cmake_path "$PWD")"
        -B "$(cmake_path "$CMAKE_BUILD_DIR")"
        -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
        -DCMAKE_C_COMPILER="$CMAKE_CC_EXE"
        -DCMAKE_CXX_COMPILER="$CMAKE_CXX_EXE"
        -DCMAKE_C_FLAGS="${CMAKE_CC_FLAGS} ${BF:-}"
        -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS} ${BF:-}"
        -DCMAKE_PREFIX_PATH="$(cmake_path "$QTDIR");$(cmake_path "$SDK_HOME")"
        -DPython3_EXECUTABLE="$(cmake_path "$PYTHON_EXE")"
        -DCMAKE_INSTALL_PREFIX="$(cmake_path "$CMAKE_INSTALL_DIR")"
        -DNATRON_INSTALL_LAYOUT=PORTABLE
        -DNATRON_OCIO_CONFIGS_DIR="$(cmake_path "$OCIO_CONFIGS_DIR")"
        -DNATRON_OCIO_CONFIGS="${OCIO_CONFIGS_LIST// /;}"
        -DNATRON_BUILD_TESTS=ON
    )
    if [ "$QT_VERSION_MAJOR" -ge 6 ]; then
        CMAKE_FLAGS+=(-DNATRON_QT6=ON)
    fi
    if [ "$CMAKE_BUILD_TYPE" = "RelWithDebInfo" ]; then
        # -O3 also with debug info (CMake's RelWithDebInfo default is -O2 -g)
        CMAKE_FLAGS+=(-DCMAKE_C_FLAGS_RELWITHDEBINFO="-O3 -g -DNDEBUG" -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O3 -g -DNDEBUG")
    fi

    # setup version (qmake: CONFIG+=snapshot/alpha/beta/RC/stable, BUILD_NUMBER, BUILD_USER_NAME)
    case "$NATRON_BUILD_CONFIG" in
        ALPHA|BETA|RC)
            if [ -z "${NATRON_BUILD_NUMBER:-}" ]; then
                echo "You must supply a NATRON_BUILD_NUMBER when NATRON_BUILD_CONFIG=$NATRON_BUILD_CONFIG"
                exit 1
            fi
            CMAKE_FLAGS+=(-DNATRON_BUILD_CONFIG="$NATRON_BUILD_CONFIG" -DNATRON_BUILD_NUMBER="$NATRON_BUILD_NUMBER")
            ;;
        SNAPSHOT|STABLE)
            CMAKE_FLAGS+=(-DNATRON_BUILD_CONFIG="$NATRON_BUILD_CONFIG")
            ;;
        CUSTOM)
            if [ -z "${NATRON_CUSTOM_BUILD_USER_NAME:-}" ]; then
                echo "You must supply a NATRON_CUSTOM_BUILD_USER_NAME when NATRON_BUILD_CONFIG=CUSTOM"
                exit 1
            fi
            CMAKE_FLAGS+=(-DNATRON_BUILD_CONFIG=CUSTOM -DNATRON_CUSTOM_BUILD_USER_NAME="$NATRON_CUSTOM_BUILD_USER_NAME")
            ;;
    esac

    # OpenMP (ceres and openMVG, for the tracker), as with qmake
    if [ "$COMPILER" = "clang" ]; then
        CMAKE_FLAGS+=(-DNATRON_USE_OPENMP=OFF)
    else
        CMAKE_FLAGS+=(-DNATRON_USE_OPENMP=ON)
    fi

    if [ "$COMPILE_TYPE" != "debug" ]; then
        # qmake: CONFIG+=noassertions. No fast math (the release builds use -O3 without it):
        # -ffinite-math-only defeats NaN and infinity checks (see #1277). Add
        # -DNATRON_FAST_MATH=ON to NATRON_EXTRA_CMAKE_FLAGS to get it.
        CMAKE_FLAGS+=(-DNATRON_NO_ASSERTIONS=ON)
    fi

    if [ "$PKGOS" = "OSX" ]; then
        CMAKE_FLAGS+=(
            -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOSX_DEPLOYMENT_TARGET"
            -DCMAKE_OBJCXX_COMPILER="$CMAKE_OBJCXX_EXE"
            -DCMAKE_OBJCXX_FLAGS="$CMAKE_OBJCXX_FLAGS"
        )
    fi

    if command -v ninja >/dev/null 2>&1; then
        CMAKE_FLAGS+=(-G Ninja)
    elif [ "$PKGOS" = "Windows" ]; then
        CMAKE_FLAGS+=(-G "MSYS Makefiles")
    fi

    # Extra cmake flags passed from the command line (split on spaces, no globbing)
    read -r -a CMAKE_FLAGS_EXTRA <<< "${NATRON_EXTRA_CMAKE_FLAGS:-}"
    # (index loop: macOS /bin/bash 3.2 with set -u rejects expanding an empty array)
    for ((i = 0; i < ${#CMAKE_FLAGS_EXTRA[@]}; i++)); do
        CMAKE_FLAGS+=("${CMAKE_FLAGS_EXTRA[$i]}")
    done

    if [ "$NO_BUILD" != "1" ]; then
        printStatusMessage "Building Natron with CMake..."
        rm -rf "$CMAKE_BUILD_DIR"
        set -x
        env CFLAGS= CXXFLAGS= cmake "${CMAKE_FLAGS[@]}"
        cmake --build "$(cmake_path "$CMAKE_BUILD_DIR")" -j"${MKJOBS}"
        set +x
    fi

    rm -rf "$CMAKE_INSTALL_DIR"
    cmake --install "$(cmake_path "$CMAKE_BUILD_DIR")"
    cmake --install "$(cmake_path "$CMAKE_BUILD_DIR")" --component Tests

    if [ "${MINIMIZE_DISK_USAGE:-}" = "1" ]; then
        echo "Cleaning up files to minimize disk usage..."
        find "$CMAKE_BUILD_DIR" -type f \( -name '*.o' -o -name '*.obj' -o -name '*.a' \) -delete
    fi

    # stage the installed files into the layout the installer scripts expect
    rm -rf "$TMP_BINARIES_PATH"/Natron* "$TMP_BINARIES_PATH/PyPlugs" "$RES_DIR/OpenColorIO-Configs" || true
    rm -f "$TMP_BINARIES_PATH"/bin/Natron* "$TMP_BINARIES_PATH"/bin/natron-python* "$TMP_BINARIES_PATH"/bin/Tests* || true
    mkdir -p "$TMP_BINARIES_PATH/bin" "$RES_DIR" "$TMP_BINARIES_PATH/docs/natron"
    if [ "$PKGOS" = "OSX" ]; then
        CMAKE_APP_CONTENTS="$CMAKE_INSTALL_DIR/Natron.app/Contents"
        cp -a "$CMAKE_APP_CONTENTS/MacOS/." "$TMP_BINARIES_PATH/bin/"
        # build-OSX-installer.sh moves Info.plist and PkgInfo from bin/ to Contents/
        cp "$CMAKE_APP_CONTENTS/Info.plist" "$CMAKE_APP_CONTENTS/PkgInfo" "$TMP_BINARIES_PATH/bin/"
        cp -a "$CMAKE_APP_CONTENTS/Resources/." "$RES_DIR/"
        cp -a "$CMAKE_APP_CONTENTS/Plugins/PyPlugs" "$TMP_BINARIES_PATH/PyPlugs"
        cp "$CMAKE_APP_CONTENTS/Plugins/natron_convert_project.py" "$TMP_BINARIES_PATH/"
        cp LICENSE.txt "$TMP_BINARIES_PATH/docs/natron/"
        cp Gui/Resources/Images/splashscreen.* "$TMP_BINARIES_PATH/"
    else
        cp -a "$CMAKE_INSTALL_DIR/bin/." "$TMP_BINARIES_PATH/bin/"
        cp -a "$CMAKE_INSTALL_DIR/Resources/." "$RES_DIR/"
        cp -a "$CMAKE_INSTALL_DIR/Plugins/PyPlugs" "$TMP_BINARIES_PATH/PyPlugs"
        cp "$CMAKE_INSTALL_DIR/Plugins/natron_convert_project.py" "$TMP_BINARIES_PATH/"
        cp -a "$CMAKE_INSTALL_DIR/docs/natron/." "$TMP_BINARIES_PATH/docs/natron/"
    fi
    echo "*** $RES_DIR/OpenColorIO-Configs now contains:"
    ls "$RES_DIR/OpenColorIO-Configs"

    TEST_BINARY_TO_RUN="$TMP_BINARIES_PATH/bin/Tests"
    if [ "$PKGOS" = "Windows" ]; then
        TEST_BINARY_TO_RUN="${TEST_BINARY_TO_RUN}.exe"
    elif [ "$PKGOS" = "OSX" ]; then
        # the installed Tests has the relative RPATHs of the app bundle, which only resolve once
        # build-OSX-installer.sh has bundled the libraries (it runs Tests again from there):
        # run the one in the build tree, whose RPATHs point to the libraries it was linked with
        TEST_BINARY_TO_RUN="$CMAKE_BUILD_DIR/Tests/Tests"
    fi
else
# qmake build (NATRON_BUILD_SYSTEM=qmake, deprecated): not indented, to keep it
# identical to the previous version of this script. It ends at "end of the qmake build".

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


# Get extra qmake flags passed from command line
QMAKE_FLAGS_EXTRA=($NATRON_EXTRA_QMAKE_FLAGS)

# Do not make the build silent so we can check that all flags are correctly passed at compile time. Only useful to debug the build.
QMAKE_FLAGS_EXTRA+=(CONFIG+=silent)


# setup version
if [ "$NATRON_BUILD_CONFIG" = "SNAPSHOT" ]; then
    QMAKE_FLAGS_EXTRA+=(CONFIG+=snapshot)
elif [ "$NATRON_BUILD_CONFIG" = "ALPHA" ]; then
    QMAKE_FLAGS_EXTRA+=(CONFIG+=alpha  BUILD_NUMBER="$NATRON_BUILD_NUMBER")
    if [ -z "${BUILD_NUMBER:-}" ]; then
        echo "You must supply a BUILD_NUMBER when NATRON_BUILD_CONFIG=ALPHA"
        exit 1
    fi
elif [ "$NATRON_BUILD_CONFIG" = "BETA" ]; then
    if [ -z "${NATRON_BUILD_NUMBER:-}" ]; then
        echo "You must supply a NATRON_BUILD_NUMBER when NATRON_BUILD_CONFIG=BETA"
        exit 1
    fi
    QMAKE_FLAGS_EXTRA+=(CONFIG+=beta  BUILD_NUMBER="$NATRON_BUILD_NUMBER")
elif [ "$NATRON_BUILD_CONFIG" = "RC" ]; then
    if [ -z "${NATRON_BUILD_NUMBER:-}" ]; then
        echo "You must supply a NATRON_BUILD_NUMBER when NATRON_BUILD_CONFIG=RC"
        exit 1
    fi
    QMAKE_FLAGS_EXTRA+=(CONFIG+=RC BUILD_NUMBER="$NATRON_BUILD_NUMBER")
elif [ "$NATRON_BUILD_CONFIG" = "STABLE" ]; then
    QMAKE_FLAGS_EXTRA+=(CONFIG+=stable)
elif [ "$NATRON_BUILD_CONFIG" = "CUSTOM" ]; then
    if [ -z "${NATRON_CUSTOM_BUILD_USER_NAME:-}" ]; then
        echo "You must supply a NATRON_CUSTOM_BUILD_USER_NAME when NATRON_BUILD_CONFIG=CUSTOM"
        exit 1
    fi
    QMAKE_FLAGS_EXTRA+=(CONFIG+=stable BUILD_USER_NAME="$NATRON_CUSTOM_BUILD_USER_NAME")
fi

if [ "$PYV" = "3" ]; then
    PYO="PYTHON_CONFIG=python${PYVER}${PYTHON_ABIFLAGS}-config"
    QMAKE_FLAGS_EXTRA+=(CONFIG+=python3)
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

if [ -n "${OSMESA_PREFIX:-}" ]; then
    # OSMESA_PATH and LLVM_PATH were set from OSMESA_PREFIX and LLVM_PREFIX by common.sh
    :
elif [ -d "$CUSTOM_BUILDS_PATH/osmesa" ]; then
    LLVM_PATH="$CUSTOM_BUILDS_PATH/llvm"
    OSMESA_PATH="$CUSTOM_BUILDS_PATH/osmesa"
elif [ -d "/opt/osmesa" ]; then
    LLVM_PATH="/opt/llvm"
    OSMESA_PATH="/opt/osmesa"
fi

QMAKE_FLAGS_EXTRA+=(CONFIG+=enable-osmesa OSMESA_PATH="$OSMESA_PATH" LLVM_PATH="$LLVM_PATH" CONFIG+=enable-cairo)


# mac compiler
if [ "$PKGOS" = "OSX" ]; then
    if [ "$COMPILER" = "clang" ] || [ "$COMPILER" = "clang-omp" ]; then
        SPEC=macx-clang
    else
        SPEC=macx-g++
    fi
    QBITS=$(echo "${BITS}" | awk '{print tolower($0)}')
    QMAKE_FLAGS_EXTRA+=(-spec "$SPEC" CONFIG+="${QBITS}" QMAKE_MACOSX_DEPLOYMENT_TARGET="$MACOSX_DEPLOYMENT_TARGET")
elif [ "$PKGOS" = "Windows" ]; then
    QMAKE_FLAGS_EXTRA+=(-spec win32-g++)
fi

if [ "$COMPILE_TYPE" != "debug" ]; then
    # No CONFIG+=fast (-Ofast), as in the CMake build: its -ffinite-math-only
    # defeats NaN and infinity checks (see #1277).
    QMAKE_FLAGS_EXTRA+=(CONFIG+=noassertions)
fi

# build
if [ "$NO_BUILD" != "1" ]; then
    printStatusMessage "Building Natron..."
    echo "env CFLAGS=\"${BF:-}\" CXXFLAGS=\"${BF:-}\" \"$QMAKE\" -r CONFIG+=\"$COMPILE_TYPE\" QMAKE_CC=\"$CC\" QMAKE_CXX=\"$CXX\" QMAKE_LINK=\"$CXX\" QMAKE_OBJECTIVE_CC=\"$OBJECTIVE_CC\" QMAKE_OBJECTIVE_CXX=\"$OBJECTIVE_CXX\" ${QMAKE_FLAGS_EXTRA[*]} ${PYO:-} ../Project.pro"
    env CFLAGS="${BF:-}" CXXFLAGS="${BF:-}" "$QMAKE" -r CONFIG+="$COMPILE_TYPE" QMAKE_CC="$CC" QMAKE_CXX="$CXX" QMAKE_LINK="$CXX" QMAKE_OBJECTIVE_CC="$OBJECTIVE_CC" QMAKE_OBJECTIVE_CXX="$OBJECTIVE_CXX" "${QMAKE_FLAGS_EXTRA[@]}" ${PYO:-} "$srcdir"/Project.pro
    make -j"${MKJOBS}"
    make -j"${MKJOBS}" -C Tests
    if [ "$PKGOS" = "OSX" ]; then
    # the app bundle is wrong when building in parallel
        make -C App clean
        make -C App
    fi
fi

if [ ! -d "$TMP_BINARIES_PATH/bin" ]; then
    mkdir -p "$TMP_BINARIES_PATH/bin"
fi



# install app(s)
if [ "$NO_BUILD" != "1" ]; then
    rm -rf "$TMP_BINARIES_PATH"/Natron* || true
    rm -f "$TMP_BINARIES_PATH"/bin/Natron* "$TMP_BINARIES_PATH/bin/Tests" || true
fi

NATRON_CONVERTER="NatronProjectConverter"
NATRON_TEST="Tests"
NATRON_BIN="Natron"
NATRON_PYTHON_BIN="natron-python"
RENDERER_BIN="NatronRenderer"
CRASHGUI="NatronCrashReporter"
CRASHCLI="NatronRendererCrashReporter"
if [ "$PKGOS" = "Windows" ]; then
    WIN_BIN_TYPE=release
    if [ "${COMPILE_TYPE}" = "debug" ]; then
       WIN_BIN_TYPE=debug
    fi
    NATRON_CONVERTER="$WIN_BIN_TYPE/${NATRON_CONVERTER}.exe"
    NATRON_PYTHON_BIN="$WIN_BIN_TYPE/${NATRON_PYTHON_BIN}.exe"
    NATRON_TEST="$WIN_BIN_TYPE/${NATRON_TEST}.exe"
    NATRON_BIN="$WIN_BIN_TYPE/${NATRON_BIN}.exe"
    RENDERER_BIN="$WIN_BIN_TYPE/${RENDERER_BIN}.exe"
    CRASHGUI="$WIN_BIN_TYPE/${CRASHGUI}.exe"
    CRASHCLI="$WIN_BIN_TYPE/${CRASHCLI}.exe"
fi


if [ "$PKGOS" = "OSX" ]; then
    MAC_APP_PATH=/Natron.app/Contents/MacOS
    MAC_INFOPLIST=/Natron.app/Contents/Info.plist
    MAC_PKGINFO=/Natron.app/Contents/PkgInfo
    MAC_CRASH_PATH=/NatronCrashReporter.app/Contents/MacOS
fi

CP_OR_MV=cp
if [ "${MINIMIZE_DISK_USAGE:-}" = "1" ]; then
    echo "Cleaning up files to minimize disk usage..."

    # Free up some disk space before copying.
    DIRS_TO_CLEAN="Engine Gui Tests libs/openMVG libs/ceres libs/libmv"

    for dir in ${DIRS_TO_CLEAN}; do
        if [ "$PKGOS" = "Windows" ]; then
            rm -vf ${dir}/${WIN_BIN_TYPE}/{*.o,*.a}
        else
            rm -vf ${dir}/{*.o,*.a}
        fi

        if [ -d ${dir}/pch ]; then
            rm -rv ${dir}/pch
        fi
    done

    # Move large files instead of copying them to minimize disk usage.
    CP_OR_MV=mv
fi


cp Tests/$NATRON_TEST "$TMP_BINARIES_PATH/bin/"
TEST_BINARY_TO_RUN="$TMP_BINARIES_PATH/bin/Tests"
if [ "$PKGOS" = "Windows" ]; then
    TEST_BINARY_TO_RUN="${TEST_BINARY_TO_RUN}.exe"
fi

${CP_OR_MV} App${MAC_APP_PATH:-}/$NATRON_BIN "$TMP_BINARIES_PATH/bin/"

# copy Info.plist and PkgInfo
if [ "$PKGOS" = "OSX" ]; then
    ls -lR App/Natron.app
    # Note: Info.plist generation only works if compiling in the sources,
    # NOT in a "build" subdir (at least in qt4).
    cp "App${MAC_INFOPLIST}" "$TMP_BINARIES_PATH/bin/"
    cp "App${MAC_PKGINFO}" "$TMP_BINARIES_PATH/bin/"
fi

${CP_OR_MV} Renderer/$RENDERER_BIN "$TMP_BINARIES_PATH/bin/"

if [ -f ProjectConverter/$NATRON_CONVERTER ]; then
    cp ProjectConverter/$NATRON_CONVERTER "${TMP_BINARIES_PATH}/bin/"
fi

if [ -f PythonBin/$NATRON_PYTHON_BIN ]; then
    ${CP_OR_MV} PythonBin/$NATRON_PYTHON_BIN "${TMP_BINARIES_PATH}/bin/"
fi

mkdir -p "$TMP_BINARIES_PATH/docs/natron" || true
cp "$srcdir"/LICENSE.txt "$TMP_BINARIES_PATH/docs/natron/"

# install crashapp(s)
if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    ${CP_OR_MV} CrashReporter${MAC_CRASH_PATH:-}/$CRASHGUI "$TMP_BINARIES_PATH/bin/"
    ${CP_OR_MV} CrashReporterCLI/$CRASHCLI "$TMP_BINARIES_PATH/bin/"
fi

if [ ! -d "$RES_DIR" ]; then
    mkdir -p "$RES_DIR"
fi

# copy everything else we need
rm -rf "$RES_DIR/OpenColorIO-Configs" || true
mkdir -p "$RES_DIR/OpenColorIO-Configs"

echo "*** Copying OpenColorIO profiles from $OCIO_CONFIGS_DIR to $RES_DIR/OpenColorIO-Configs (profiles: ${OCIO_CONFIGS[*]})"
for profile in "${OCIO_CONFIGS[@]}"; do
    if [ -d "$OCIO_CONFIGS_DIR/${profile}" ]; then
        cp -a "$OCIO_CONFIGS_DIR/${profile}" "$RES_DIR/OpenColorIO-Configs/"
    else
        echo "Error: $profile not found!"
        exit 1
    fi
done
echo "*** $RES_DIR/OpenColorIO-Configs now contains:"
ls "$RES_DIR/OpenColorIO-Configs"

mkdir -p "$RES_DIR/stylesheets"
cp "$srcdir"/Gui/Resources/Stylesheets/mainstyle.qss "$TMP_BINARIES_PATH/Resources/stylesheets/"

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
cp "$srcdir"/tools/natron_convert_project.py "$TMP_BINARIES_PATH/"
if [ "$PKGOS" = "Linux" ] || [ "$PKGOS" = "Windows" ]; then
    mkdir -p "$RES_DIR/etc/fonts/conf.d"
    cp "$srcdir"/Gui/Resources/etc/fonts/fonts.conf "$RES_DIR/etc/fonts/"
    #cp "$srcdir"/Gui/Resources/share/fontconfig/conf.avail/* "$RES_DIR/etc/fonts/conf.d/"
    cp "$srcdir"/Gui/Resources/etc/fonts/conf.d/* "$RES_DIR/etc/fonts/conf.d/"
fi

fi # end of the qmake build

set -x
# set OCIO for Tests
export OCIO="$OCIO_CONFIGS_DIR/nuke-default/config.ocio"
export NATRON_PLUGIN_PATH="$TMP_BINARIES_PATH/PyPlugs"
export OFX_PLUGIN_PATH="$TMP_BINARIES_PATH/OFX/Plugins"
env
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
    if [ "${ARCH:-$(uname -m)}" = aarch64 ]; then
        # valgrind is about 10 times slower on aarch64 than on x86_64: the suite doesn't fit in
        # the time limit (the x86_64 build runs it under valgrind)
        env LD_LIBRARY_PATH="$SDK_HOME/lib:$FFMPEG_PATH/lib:$LIBRAW_PATH/lib:$QTDIR/lib" $TIMEOUT -s KILL 1800 ${TEST_BINARY_TO_RUN}
    else
        # DeepGraphRender.VeryDeepChain (a 5000-node graph) takes 7 minutes without valgrind and
        # more than the whole 30-minute limit with it: it runs in CI (ci.yml), without valgrind
        env GTEST_FILTER=-DeepGraphRender.VeryDeepChain LD_LIBRARY_PATH="$SDK_HOME/lib:$FFMPEG_PATH/lib:$LIBRAW_PATH/lib:$QTDIR/lib" $TIMEOUT -s KILL 1800 valgrind --tool=memcheck --suppressions="$INC_PATH/natron/valgrind-python${PYV}.supp" ${TEST_BINARY_TO_RUN}
    fi
    rm -f lib || true
    # ITS NOT POSSIBLE TO RUN THE WIN TESTS HERE, DO IT IN THE INSTALLER SCRIPT
elif [ "$PKGOS" = "Windows" ]; then
    mkdir tmpdll
    pushd tmpdll
    cp "$OFX_PLUGIN_PATH"/*.ofx.bundle/Contents/*/*.ofx .
    for f in *.ofx; do
        dll="${f%\.ofx}.dll"
        mv "$f" "$dll"
        echo "Dependencies for $f:"
        env PATH="$FFMPEG_PATH/bin:$LIBRAW_PATH/bin:$QTDIR/bin:$SDK_HOME/osmesa/lib:$SDK_HOME/bin:${PATH:-}" ldd "$dll"
    done
    popd
    rm -rf tmpdll
    #rm -rf /c/Users/NatronWin/AppData/Local/INRIA/Natron &> /dev/null || true
    rm -rf "$LOCALAPPDATA\\INRIA\\Natron"* &> /dev/null || true
    testfail=0
    # MSYS2's Python loads extension modules with LOAD_LIBRARY_SEARCH_DEFAULT_DIRS, which ignores
    # PATH, and Tests.exe is not next to the DLLs: PYTHONLEGACYWINDOWSDLLLOADING=1 (an MSYS2
    # Python extension) restores the PATH search, as in ci.yml (#1147)
    env PYTHONHOME="$SDK_HOME" PYTHONLEGACYWINDOWSDLLLOADING=1 PATH="$FFMPEG_PATH/bin:$LIBRAW_PATH/bin:$QTDIR/bin:$SDK_HOME/osmesa/lib:$SDK_HOME/bin:${PATH:-}" $TIMEOUT -s KILL 1800 "${TEST_BINARY_TO_RUN}" || testfail=1
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
    if [ "$MACOSX_DEPLOYMENT_TARGET" = "10.6" ]; then
        if [ "$COMPILER" = "gcc" ]; then
            env DYLD_LIBRARY_PATH="$SDK_HOME"/lib/libgcc:/System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/ImageIO.framework/Versions/A/Resources:"$SDK_HOME"/lib $TIMEOUT -s KILL 1800 ${TEST_BINARY_TO_RUN}
        else
            # be more tolerant to test fails on 10.6
            $TIMEOUT -s KILL 1800 ${TEST_BINARY_TO_RUN} || true
        fi
    else
        # Tests exit with a segfault on Qt5
        $TIMEOUT -s KILL 1800 ${TEST_BINARY_TO_RUN} || [ "$QT_VERSION_MAJOR" = 5 ]
    fi
fi
set +x

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
