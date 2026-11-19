#!/bin/bash
#
# Build Natron Plugins
#

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

source common.sh
source common-buildmaster.sh

MAKEFLAGS_VERBOSE="V=1" # verbose build

PID=$$
if [ "${PIDFILE:-}" != "" ]; then
  echo $PID > "$PIDFILE"
fi

# Always use BUILD_CONFIG
if [ -z "${BUILD_CONFIG:-}" ]; then
    echo "Please define BUILD_CONFIG"
    exit 1
fi

# Set to MASTER_BRANCH as default
IO_BRANCH="$MASTER_BRANCH"
MISC_BRANCH="$MASTER_BRANCH"
ARENA_BRANCH="$MASTER_BRANCH"
GMIC_BRANCH="$MASTER_BRANCH"
CV_BRANCH="$MASTER_BRANCH"

# If the already checkout-out Natron branch overrides it, do it now
if [ -f "$TMP_PATH/Natron/Global/plugin-branches.sh" ]; then
    . "$TMP_PATH/Natron/Global/plugin-branches.sh"
fi
     
if [ -z "${GIT_BRANCH:-}"  ]; then
    NATRON_BRANCH="$MASTER_BRANCH"
    if [ "$BUILD_CONFIG" = "SNAPSHOT" ]; then
        COMMITS_HASH="$CWD/commits-hash-$NATRON_BRANCH.sh"
    else
        COMMITS_HASH="$CWD/commits-hash.sh"
    fi
else
    NATRON_BRANCH="$GIT_BRANCH"
    COMMITS_HASH="$CWD/commits-hash-$NATRON_BRANCH.sh"
fi

if [ "$BUILD_CONFIG" != "SNAPSHOT" ]; then
    IO_BRANCH="$IOPLUG_GIT_TAG"
    MISC_BRANCH="$MISCPLUG_GIT_TAG"
    ARENA_BRANCH="$ARENAPLUG_GIT_TAG"
    GMIC_BRANCH="$GMICPLUG_GIT_TAG"
    CV_BRANCH="$CVPLUG_GIT_TAG"
fi

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

if [ ! -d "$TMP_PATH" ]; then
    mkdir -p "$TMP_PATH"
fi

if [ ! -d "$TMP_BINARIES_PATH" ]; then
    mkdir -p "$TMP_BINARIES_PATH"
fi

# Setup env
OIIO_PATH="$SDK_HOME"
CXXFLAGS_EXTRA=
if [ "$PKGOS" = "Linux" ]; then
    if [ -x "$SDK_HOME/bin/qmake" ]; then
        QTDIR="$SDK_HOME"
    elif [ -x "$SDK_HOME/qt4/bin/qmake" ]; then
        QTDIR="$SDK_HOME/qt4"
    elif [ -x "$SDK_HOME/qt5/bin/qmake" ]; then
        QTDIR="$SDK_HOME/qt5"
    else
        echo "Qt cannot be found in $SDK_HOME or $SDK_HOME/qt4 or $SDK_HOME/qt5"
        exit 1
    fi
    export QTDIR
    export BOOST_ROOT="$SDK_HOME"
    export OPENJPEG_HOME="$SDK_HOME"
    export THIRD_PARTY_TOOLS_HOME="$SDK_HOME"
    export LD_LIBRARY_PATH="$SDK_HOME/lib:$FFMPEG_PATH/lib:$LIBRAW_PATH/lib"
    export PATH="$SDK_HOME/gcc/bin:$SDK_HOME/bin:$SDK_HOME/cmake/bin:$PATH"
    if [ "$ARCH" = "x86_64" ]; then
        export LD_LIBRARY_PATH="$SDK_HOME/gcc/lib64:$LD_LIBRARY_PATH"
    else
        export LD_LIBRARY_PATH="$SDK_HOME/gcc/lib:$LD_LIBRARY_PATH"
    fi
    export LD_RUN_PATH="$LD_LIBRARY_PATH"
elif [ "$PKGOS" = "Windows" ]; then
    export QTDIR="$SDK_HOME"
    export BOOST_ROOT="$SDK_HOME"
    export OPENJPEG_HOME="$SDK_HOME"
    export THIRD_PARTY_TOOLS_HOME="$SDK_HOME"
    export GIT_SSL_NO_VERIFY="true"
    BUILDID="-Wl,--build-id"
elif [ "$PKGOS" = "OSX" ]; then
    export QTDIR="$MACPORTS/libexec/qt4"
    export BOOST_ROOT="$SDK_HOME"
    export OPENJPEG_HOME="$SDK_HOME"
    export THIRD_PARTY_TOOLS_HOME="$SDK_HOME"
    # see https://github.com/Homebrew/homebrew-core/issues/32765
    CXXFLAGS_EXTRA="-isysroot `xcodebuild -version -sdk macosx Path 2>/dev/null`"
fi

if [ -d "$TMP_BINARIES_PATH/OFX/Plugins" ]; then
    rm -rf "$TMP_BINARIES_PATH/OFX/Plugins"
fi
mkdir -p "$TMP_BINARIES_PATH/OFX/Plugins"

if [ -d "$TMP_BINARIES_PATH/Plugins" ]; then
    rm -rf "$TMP_BINARIES_PATH/Plugins"
fi
mkdir -p "$TMP_BINARIES_PATH/Plugins"

#ls -la  "$TMP_BINARIES_PATH" "$TMP_BINARIES_PATH/OFX/Plugins" "$TMP_BINARIES_PATH/Plugins"

if [ -z "${BUILD_IO:-}" ]; then
    BUILD_IO=1
fi
if [ -z "${BUILD_MISC:-}" ]; then
    BUILD_MISC=1
fi
if [ -z "${BUILD_ARENA:-}" ]; then
    BUILD_ARENA=1
fi
if [ -z "${BUILD_GMIC:-}" ]; then
    BUILD_GMIC=1
fi
BUILD_CV=0

BUILD_MODE=""
if [ "${DEBUG_MODE:-}" = "1" ]; then
    # CI build have to be build with CONFIG=relwithdebinfo, or the
    # installer build for plugins won't work (deployment on osx, which
    # copies libraries, is only
    # done in release and relwithdebinfo configs).
    # 
    #BUILD_MODE=debug
    BUILD_MODE=relwithdebinfo
    OPTFLAG=-O
else
    BUILD_MODE=relwithdebinfo
    # -O3, not -Ofast: -ffinite-math-only breaks the plug-ins' NaN/Inf guards
    # (std::isnan/std::isfinite are folded to constants).
    OPTFLAG=-O3
fi

# Build system of the plug-ins (#1181): PLUGINS_BUILD_SYSTEM=cmake (default) builds openfx-misc,
# openfx-io, openfx-arena and openfx-gmic with their CMake builds (function ofx_cmake_build, as
# tools/jenkins/build-plugins.sh), or with their Makefiles where the checkout has no CMake build
# (the release tags, see plugins_use_cmake); PLUGINS_BUILD_SYSTEM=make uses the Makefiles for all
# of them (RB-2.6 fallback).
PLUGINS_BUILD_SYSTEM="${PLUGINS_BUILD_SYSTEM:-cmake}"
case "$PLUGINS_BUILD_SYSTEM" in
    cmake|make) ;;
    *) echo "Error: PLUGINS_BUILD_SYSTEM must be cmake or make, not \"$PLUGINS_BUILD_SYSTEM\""; exit 1;;
esac
# CMake build type: always RelWithDebInfo, as the Makefiles' CONFIG (BUILD_MODE); DEBUG_MODE=1 gives
# -O1 -g, as the Makefiles' OPTFLAG=-O. Not CMake's Debug: openfx's ofx_deploy_bundle only deploys
# release configurations, so Debug bundles would still reference the MacPorts libraries.
PLUGINS_CMAKE_BUILD_TYPE=RelWithDebInfo
if [ "${DEBUG_MODE:-}" = "1" ]; then
    PLUGINS_DEBUG=1
    PLUGINS_CMAKE_RELWITHDEBINFO_FLAGS="-O1 -g -DNDEBUG"
else
    PLUGINS_DEBUG=0
    PLUGINS_CMAKE_RELWITHDEBINFO_FLAGS="-O3 -g -DNDEBUG"
fi

# Whether to build the plug-in repository in the current directory (named $1) with its CMake
# build: yes unless PLUGINS_BUILD_SYSTEM=make, or unless the checkout has no CMake build that uses
# openfx's helpers (ofx_deploy_bundle). The release builds check out the *PLUG_GIT_TAG tags of
# common-buildmaster.sh (Natron-2.3.13), which predate the CMake builds: those repositories are
# built with their Makefiles, with a message. (tools/jenkins/build-plugins.sh stops instead.)
function plugins_use_cmake() {
    if [ "$PLUGINS_BUILD_SYSTEM" = "make" ]; then
        return 1
    fi
    if [ -f CMakeLists.txt ] && grep -q 'ofx_deploy_bundle' CMakeLists.txt; then
        return 0
    fi
    echo "Warning: $1 in $(pwd) ($(git describe --tags --always 2>/dev/null || echo unknown version)) has no CMake build with openfx's helpers (ofx_deploy_bundle): building it with its Makefiles"
    return 1
}

# Build and install the plug-in repository in the current directory with its CMake build into
# $TMP_BINARIES_PATH/OFX/Plugins, where the Makefile builds copy their bundles too. Same options as
# ofx_cmake_build in tools/jenkins/build-plugins.sh (keep them in sync): build type, C++ standard
# (C++17 with Qt5, C++20 with Qt6), licence, OpenMP, macOS deployment target and bundle
# deployment. Extra arguments are passed to cmake at configure time, after these.
function ofx_cmake_build() {
    # shellcheck disable=SC2206 # (split on purpose, see above)
    local cxx_words=($CXX)
    local cxx="${cxx_words[0]}"
    local cxx_flags=""
    local cxx_std=17
    local i w
    if [ "${QT_VERSION_MAJOR:-5}" -ge 6 ]; then
        cxx_std=20
    fi
    for ((i = 1; i < ${#cxx_words[@]}; i++)); do
        w="${cxx_words[$i]}"
        case "$w" in
            -std=*) echo "Info: plug-ins built with OFX_CXX_STANDARD=$cxx_std, not CXX's $w (C++17 with Qt5, C++20 with Qt6)";;
            *) cxx_flags="$cxx_flags $w";;
        esac
    done
    local openmp=OFF
    if [ "$COMPILER" = "gcc" ] || [ "$COMPILER" = "clang-omp" ]; then
        openmp=ON
    elif [ "$PKGOS" = "OSX" ] && "$cxx" --version 2>/dev/null | grep -q '^Apple clang' \
         && [ -f "${MACPORTS:-/opt/local}/include/libomp/omp.h" ]; then
        openmp=ON
    fi
    if [ "$openmp" = "OFF" ] && [ "$PKGOS" = "OSX" ]; then
        # the plug-ins that use OpenMP (CImg, G'MIC, DenoiseSharpen...) would be much slower
        if [ "$PLUGINS_DEBUG" = "1" ]; then
            echo "Warning: ************************************************************"
            echo "Warning: building the plug-ins in $(pwd) WITHOUT OpenMP: $cxx (COMPILER=$COMPILER) has no OpenMP, and ${MACPORTS:-/opt/local}/include/libomp/omp.h is missing (sudo port install libomp)"
            echo "Warning: ************************************************************"
        else
            echo "Error: the plug-ins in $(pwd) would be built without OpenMP: $cxx (COMPILER=$COMPILER) has no OpenMP, and ${MACPORTS:-/opt/local}/include/libomp/omp.h is missing (sudo port install libomp)"
            exit 1
        fi
    fi
    # MinGW gcc: large objects; openfx's add_ofx_plugin only adds -Wa,-mbig-obj to plug-in targets
    local bigobj=""
    if [ "$PKGOS" = "Windows" ] && [ "$COMPILER" = "gcc" ]; then
        bigobj=" -Wa,-mbig-obj"
    fi
    local prefix_path="$SDK_HOME" install_prefix="$TMP_BINARIES_PATH"
    if [ "$PKGOS" = "Windows" ]; then
        prefix_path="$(cygpath -m "$prefix_path")"
        install_prefix="$(cygpath -m "$install_prefix")"
    fi
    local args=(
        -S . -B build
        -DCMAKE_BUILD_TYPE="$PLUGINS_CMAKE_BUILD_TYPE"
        -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="$PLUGINS_CMAKE_RELWITHDEBINFO_FLAGS"
        -DCMAKE_CXX_COMPILER="$cxx"
        -DCMAKE_CXX_FLAGS="${cxx_flags# } ${CXXFLAGS_EXTRA}${bigobj}"
        -DOFX_CXX_STANDARD="$cxx_std"
        -DOFX_USE_OPENMP="$openmp"
        -DCMAKE_INSTALL_PREFIX="$install_prefix"
        -DPLUGIN_INSTALLDIR="OFX/Plugins"
        -DCMAKE_PREFIX_PATH="$prefix_path"
        -DBUILD_TESTING=OFF
    )
    if grep -q 'OFX_LICENSE' CMakeLists.txt; then
        # the licence, where the CMake build has the option (openfx-arena, openfx-gmic, and
        # openfx-misc since #171; not openfx-io, which gets the licence's FFmpeg and LibRaw)
        args+=(-DOFX_LICENSE="$NATRON_LICENSE")
    fi
    if [ -n "${BUILDID:-}" ]; then
        args+=(-DCMAKE_SHARED_LINKER_FLAGS="${BUILDID}" -DCMAKE_MODULE_LINKER_FLAGS="${BUILDID}")
    fi
    if [ "$PKGOS" = "OSX" ]; then
        args+=(-DOFX_LIBOMP_ROOT="${MACPORTS:-/opt/local}")
        # the deployment target of common.sh (14.0 for all the macOS bundles, #1347), as the
        # Makefiles, which read MACOSX_DEPLOYMENT_TARGET too
        if [ -n "${MACOSX_DEPLOYMENT_TARGET:-}" ]; then
            args+=(-DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOSX_DEPLOYMENT_TARGET")
        fi
        args+=(-DOFX_DEPLOY_BUNDLES=ON)
        if [ "$PLUGINS_DEBUG" != "1" ]; then
            args+=(-DOFX_DEPLOY_STRICT_MINOS=ON)
        fi
    fi
    if command -v ninja >/dev/null 2>&1; then
        args+=(-G Ninja)
    elif [ "$PKGOS" = "Windows" ]; then
        args+=(-G "MSYS Makefiles")
    fi
    rm -rf build
    mkdir -p build
    set -x
    cmake "${args[@]}" "$@" 2>&1 | tee build/configure.log
    local configure_status="${PIPESTATUS[0]}"
    set +x
    if [ "$configure_status" != 0 ]; then
        echo "Error: cmake configure failed in $(pwd)"
        exit 1
    fi
    if grep -q "not building (libraries not found)" build/configure.log; then
        grep "not building (libraries not found)" build/configure.log
        echo "Error: plug-ins would be missing from the bundle in $(pwd): install the libraries above"
        exit 1
    fi
    if grep -q "OpenMP not found" build/configure.log; then
        echo "Error: OpenMP was requested but not found in $(pwd) (or pass -DOFX_USE_OPENMP=OFF)"
        exit 1
    fi
    set -x
    cmake --build build -j"${MKJOBS}"
    cmake --install build
    set +x
}

# MISC
if [ "$BUILD_MISC" = "1" ]; then
    #ls -la  "$TMP_BINARIES_PATH" "$TMP_BINARIES_PATH/OFX/Plugins" "$TMP_BINARIES_PATH/Plugins"
    cd "$TMP_PATH"
    if [ -d openfx-misc ]; then
        rm -rf openfx-misc || true
    fi
    if [ -d openfx-misc ]; then
        sleep 25 
        rm -rf openfx-misc
    fi

    #ls -la  "$TMP_BINARIES_PATH" "$TMP_BINARIES_PATH/OFX/Plugins" "$TMP_BINARIES_PATH/Plugins"
    $TIMEOUT 1800 $GIT clone "$GIT_MISC"
    cd openfx-misc

    #$KILLSCRIPT $PID &
    #KILLBOT=$!

    $TIMEOUT 1800 $GIT checkout "${MISC_BRANCH}"
    $TIMEOUT 1800 $GIT submodule update -i --recursive
    if [ "$MISC_BRANCH" = "$MASTER_BRANCH" ]; then
        # the snapshots are always built with the latest version of submodules
        $TIMEOUT 1800 $GIT submodule update -i --recursive --remote
        #$TIMEOUT 1800 $GIT submodule foreach git pull origin master
    fi

    #kill -9 $KILLBOT

    #make -C CImg CImg.h

    MISC_GIT_VERSION=$(git log|head -1|awk '{print $2}')

    #ls -la  "$TMP_BINARIES_PATH" "$TMP_BINARIES_PATH/OFX/Plugins" "$TMP_BINARIES_PATH/Plugins"
    # mksrc
    if [ "${MKSRC:-}" = "1" ]; then
        cd ..
        cp -a openfx-misc "openfx-misc-$MISC_GIT_VERSION"
        (cd "openfx-misc-$MISC_GIT_VERSION";find . -type d -name .git -exec rm -rf {} \;)
        tar cvvJf "$SRC_PATH/openfx-misc-$MISC_GIT_VERSION.tar.xz" "openfx-misc-$MISC_GIT_VERSION"
        rm -rf "openfx-misc-$MISC_GIT_VERSION"
        cd openfx-misc
    fi

    #Always bump git commit, it is only used to version-stamp binaries
    MISC_V=$MISC_GIT_VERSION
    $GSED -e "s/MISCPLUG_DEVEL_GIT=.*/MISCPLUG_DEVEL_GIT=${MISC_V}/" --in-place "$COMMITS_HASH"

    llvmversion="$(${LLVM_PATH}/bin/llvm-config --version)"
    case "$llvmversion" in
        2.*)
            llvmsystemlibs=""
            ;;
        3.[0-4]*)
            llvmsystemlibs=""
            ;;
        *)
            llvmsystemlibs="--system-libs"
            ;;
    esac
    llvmcomponents="engine mcjit mcdisassembler"
    llvmlinkstatic=""
    case "$llvmversion" in
        [2-9].*|1[0-4].*)
            ;;
        *)
            # LLVM >= 15 (Mesa 25.0.7 OSMesa): link the LLVM components used by llvmpipe (llvm_modules
            # in Mesa's meson.build), and the static libraries even if LLVM's default is shared.
            llvmcomponents="bitwriter engine mcdisassembler mcjit core executionengine scalaropts transformutils instcombine coroutines native passes"
            llvmlinkstatic="--link-static"
            ;;
    esac
    LLVM_LIB="$(${LLVM_PATH}/bin/llvm-config ${llvmlinkstatic} --ldflags --libs ${llvmcomponents} ${llvmsystemlibs}| tr '\n' ' ')"
    MESALIB=""
    GLULIB=""
    MESALIB="-lOSMesa"
    if [ -s "$OSMESA_PATH/lib/libMangledOSMesa32.a" ]; then
        MESALIB="-lMangledOSMesa32"
    fi
    GLULIB="-lMangledGLU"

    #ls -la  "$TMP_BINARIES_PATH" "$TMP_BINARIES_PATH/OFX/Plugins" "$TMP_BINARIES_PATH/Plugins"
    # Build static on Windows so that openfx-misc are usable on other hosts without DLL's and manifests (note Shadertoy will require osmesa dll)
    # Could/Should also be done on OSX?
    # On Linux this is not the best solution, as Nuke forces gcc, so static won't help. as-is the plugins should work in Nuke10+ (CY2015),
    # but will not work on older versions. The best solution on Nuke9 and lower is to build the plugins using RHEL devtoolset (gcc4.7).
    #if [ "$PKGOS" = "Windows" ]; then
    #    EXTRA_LDFLAGS_OFXMISC="-static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -lgomp -lwinpthread -Wl,-Bdynamic"
    #fi

    if plugins_use_cmake openfx-misc; then
        # Shadertoy is its own bundle, with OSMesa (the make path's LDFLAGS_MESA)
        misc_osmesa="$OSMESA_PATH"
        misc_llvm="$LLVM_PATH"
        misc_sdk="$SDK_HOME"
        misc_llvm_lib="$LLVM_LIB"
        if [ "$PKGOS" = "Windows" ]; then
            misc_osmesa="$(cygpath -m "$OSMESA_PATH")"
            misc_llvm="$(cygpath -m "$LLVM_PATH")"
            misc_sdk="$(cygpath -m "$SDK_HOME")"
            misc_llvm_lib="${LLVM_LIB//-L${LLVM_PATH}/-L${misc_llvm}}"
        fi
        # BUILD_DEBUGPROXY: not shipped (its default is ON in Debug builds)
        ofx_cmake_build \
            -DBUILD_DEBUGPROXY=OFF \
            -DUSE_OSMESA=ON \
            -DOSMESA_PATH="${misc_osmesa}" \
            -DLLVM_PATH="${misc_llvm}" \
            -DOSMESA_INCLUDES="${misc_osmesa}/include" \
            -DOSMESA_LIBRARIES="-L${misc_osmesa}/lib ${GLULIB} ${MESALIB} -L${misc_sdk}/lib -lz -L${misc_llvm}/lib ${misc_llvm_lib}"
    else
        #ls -la  "$TMP_BINARIES_PATH" "$TMP_BINARIES_PATH/OFX/Plugins" "$TMP_BINARIES_PATH/Plugins"
        #set -x
        #make -C Shadertoy $MAKEFLAGS_VERBOSE CXXFLAGS_MESA="-DHAVE_OSMESA" OSMESA_PATH="${OSMESA_PATH}" LDFLAGS_MESA="-L${OSMESA_PATH}/lib -L${LLVM_PATH}/lib ${GLULIB} ${MESALIB} $LLVM_LIB" CXX="$CXX" CONFIG="${BUILD_MODE}" OPTFLAG="${OPTFLAG}" LDFLAGS_ADD="${BUILDID:-} ${EXTRA_LDFLAGS_OFXMISC:-}" -j"${MKJOBS}" BITS="${BITS}" HAVE_CIMG=0
        #set +x
        # first, build everything except CImg (including OpenGL plugins)
        OMP=""
        if [ "$COMPILER" = "gcc" ] || [ "$COMPILER" = "clang-omp" ]; then
            # compile DenoiseSharpen and CImg with OpenMP support
            OMP="OPENMP=1"
        fi

        env \
            CXXFLAGS_MESA="-DHAVE_OSMESA" \
            OSMESA_PATH="${OSMESA_PATH}" \
            LDFLAGS_MESA="-L${OSMESA_PATH}/lib ${GLULIB} ${MESALIB} -lz -L${LLVM_PATH}/lib ${LLVM_LIB}" \
            CXX="$CXX" \
            CONFIG="${BUILD_MODE}" \
            OPTFLAG="${OPTFLAG}" \
            BITS="${BITS}" \
            LDFLAGS_ADD="${BUILDID:-} ${EXTRA_LDFLAGS_OFXMISC:-}" \
            HAVE_CIMG=0 \
            ${OMP} \
            LICENSE="$NATRON_LICENSE" \
            CXXFLAGS_EXTRA="${CXXFLAGS_EXTRA}" \
            make -j"${MKJOBS}" "$MAKEFLAGS_VERBOSE"

        #ls -la  "$TMP_BINARIES_PATH" "$TMP_BINARIES_PATH/OFX/Plugins" "$TMP_BINARIES_PATH/Plugins"
        
        # extract CImg.h
        make -C CImg LICENSE="$NATRON_LICENSE" CImg.h
        
        # build CImg.ofx
        if [ "$COMPILER" = "clang" ] && [ -n "${GXX:-}" ]; then
            # Building with Apple clang (no OpenMP available), but GCC is available too!
            # libSupport was compiled by clang, now clean it to build it again with gcc
            make -j"${MKJOBS}" \
                 CXX="$CXX" CONFIG="${BUILD_MODE}" OPTFLAG="${OPTFLAG}" BITS="${BITS}" HAVE_CIMG=0 clean
            # build CImg with OpenMP support, but statically link libgomp (see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=31400)
            env \
                CXX="$CXX" \
                CONFIG="${BUILD_MODE}" \
                OPTFLAG="${OPTFLAG}" \
                BITS="${BITS}" \
                LDFLAGS_ADD="${BUILDID:-} ${EXTRA_LDFLAGS_OFXMISC:-} -static-libgcc" \
                HAVE_CIMG=1 \
                OPENMP=1 \
                LICENSE="$NATRON_LICENSE" \
                CXXFLAGS_EXTRA="${CXXFLAGS_EXTRA}" \
                make -j"${MKJOBS}" "$MAKEFLAGS_VERBOSE" -C CImg
        else
            # build CImg with OpenMP support (no OpenGL required)
            env \
                CXX="$CXX" \
                CONFIG="${BUILD_MODE}" \
                OPTFLAG="${OPTFLAG}" \
                BITS="${BITS}" \
                LDFLAGS_ADD="${BUILDID:-} ${EXTRA_LDFLAGS_OFXMISC:-}" \
                HAVE_CIMG=1 \
                ${OMP} \
                LICENSE="$NATRON_LICENSE" \
                CXXFLAGS_EXTRA="${CXXFLAGS_EXTRA}" \
                make -j"${MKJOBS}" "$MAKEFLAGS_VERBOSE" -C CImg
        fi
        #ls -la  "$TMP_BINARIES_PATH" "$TMP_BINARIES_PATH/OFX/Plugins" "$TMP_BINARIES_PATH/Plugins"
        cp -a ./*/*-*-*/*.ofx.bundle "$TMP_BINARIES_PATH/OFX/Plugins/"
    fi
    echo "$MISC_V" > "$TMP_BINARIES_PATH/OFX/Plugins/Misc.ofx.bundle-version.txt"
    ls -la  "$TMP_BINARIES_PATH/OFX/Plugins"
    cd "$TMP_PATH"
    #rm -rf openfx-misc || true
fi

# IO
if [ "$BUILD_IO" = "1" ]; then
    cd "$TMP_PATH"

    #$KILLSCRIPT $PID &
    #KILLBOT=$!

    if [ -d openfx-io ]; then
        rm -rf openfx-io || true
    fi
    if [ -d openfx-io ]; then
        sleep 25
        rm -rf openfx-io
    fi

    $TIMEOUT 1800 $GIT clone "$GIT_IO"
    cd openfx-io
    $TIMEOUT 1800 $GIT checkout "${IO_BRANCH}"
    $TIMEOUT 1800 $GIT submodule update -i --recursive
    if [ "$IO_BRANCH" = "$MASTER_BRANCH" ]; then
        # the snapshots are always built with the latest version of submodules
        $TIMEOUT 1800 $GIT submodule update -i --recursive --remote
        #$TIMEOUT 1800 $GIT submodule foreach git pull origin master
    fi

    #kill -9 $KILLBOT || true

    IO_GIT_VERSION=$(git log|head -1|awk '{print $2}')

    # mksrc
    if [ "${MKSRC:-}" = "1" ]; then
        cd ..
        cp -a openfx-io "openfx-io-$IO_GIT_VERSION"
        (cd "openfx-io-$IO_GIT_VERSION"; find . -type d -name .git -exec rm -rf {} \;)
        tar cvvJf "$SRC_PATH/openfx-io-$IO_GIT_VERSION.tar.xz" "openfx-io-$IO_GIT_VERSION"
        rm -rf "openfx-io-$IO_GIT_VERSION"
        cd openfx-io
    fi

    #Always bump git commit, it is only used to version-stamp binaries
    IO_V=$IO_GIT_VERSION
    $GSED -e "s/IOPLUG_DEVEL_GIT=.*/IOPLUG_DEVEL_GIT=${IO_V}/" --in-place "$COMMITS_HASH"

    if plugins_use_cmake openfx-io; then
        # the FFmpeg and LibRaw of the licence (common.sh), as the make path gets them through
        # pkg-config
        io_seexpr="$SDK_HOME"
        io_ffmpeg_pc="${FFMPEG_PATH}/lib/pkgconfig"
        io_prefix="${SDK_HOME};${LIBRAW_PATH};${FFMPEG_PATH}"
        if [ "$PKGOS" = "Windows" ]; then
            io_seexpr="$(cygpath -m "$io_seexpr")"
            io_ffmpeg_pc="$(cygpath -m "$io_ffmpeg_pc")"
            io_prefix="$(cygpath -m "$SDK_HOME");$(cygpath -m "$LIBRAW_PATH");$(cygpath -m "$FFMPEG_PATH")"
        fi
        io_saved_pkg_config_path="${PKG_CONFIG_PATH:-}"
        export PKG_CONFIG_PATH="${LIBRAW_PATH}/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
        ofx_cmake_build -DSEEXPR_HOME="$io_seexpr" -DFFMPEG_PKG_CONFIG_PATH="$io_ffmpeg_pc" \
            -DOFX_IO_REQUIRE_ALL=ON -DCMAKE_PREFIX_PATH="$io_prefix"
        export PKG_CONFIG_PATH="$io_saved_pkg_config_path"
    else
        make -j"${MKJOBS}" $MAKEFLAGS_VERBOSE OIIO_HOME="$OIIO_PATH" SEEXPR_HOME="${SDK_HOME}" \
             CXX="$CXX" CONFIG="${BUILD_MODE}" BITS="${BITS}" OPTFLAG="${OPTFLAG}" LDFLAGS_ADD="${BUILDID:-}" CXXFLAGS_EXTRA="${CXXFLAGS_EXTRA}"
        #ls -la  "$TMP_BINARIES_PATH" "$TMP_BINARIES_PATH/OFX/Plugins" "$TMP_BINARIES_PATH/Plugins"
        cp -a ./*/*-*-*/*.ofx.bundle "$TMP_BINARIES_PATH/OFX/Plugins/"
    fi
    echo "$IO_V" > "$TMP_BINARIES_PATH/OFX/Plugins/IO.ofx.bundle-version.txt"
    cd "$TMP_PATH"
    #rm -rf openfx-io  || true
fi

# ARENA
if [ "$BUILD_ARENA" = "1" ]; then
    cd "$TMP_PATH"

    #$KILLSCRIPT $PID &
    #KILLBOT=$!

    if [ -d openfx-arena ]; then
        rm -rf openfx-arena || true
    fi
    if [ -d openfx-arena ]; then
        sleep 25
        rm -rf openfx-arena
    fi

    $TIMEOUT 1800 $GIT clone "$GIT_ARENA"
    cd openfx-arena
    $TIMEOUT 1800 $GIT checkout "${ARENA_BRANCH}"
    $TIMEOUT 1800 $GIT submodule update -i --recursive
    if [ "$ARENA_BRANCH" = "$MASTER_BRANCH" ]; then
        # the snapshots are always built with the latest version of submodules
        if true; then
            $TIMEOUT 1800 $GIT submodule update -i --recursive --remote
            #$TIMEOUT 1800 $GIT submodule foreach git pull origin master
        else
            echo "Warning: openfx-arena submodules not updated..."
        fi
    fi
    
    #kill -9 $KILLBOT || true

    ARENA_GIT_VERSION=$(git log|head -1|awk '{print $2}')

    # mksrc
    if [ "${MKSRC:-}" = "1" ]; then
        cd ..
        cp -a openfx-arena "openfx-arena-$ARENA_GIT_VERSION"
        (cd "openfx-arena-$ARENA_GIT_VERSION"; find . -type d -name .git -exec rm -rf {} \;)
        tar cvvJf "$SRC_PATH/openfx-arena-$ARENA_GIT_VERSION.tar.xz" "openfx-arena-$ARENA_GIT_VERSION"
        rm -rf "openfx-arena-$ARENA_GIT_VERSION"
        cd openfx-arena
    fi

    #Always bump git commit, it is only used to version-stamp binaries
    ARENA_V=$ARENA_GIT_VERSION
    $GSED -e "s/ARENAPLUG_DEVEL_GIT=.*/ARENAPLUG_DEVEL_GIT=${ARENA_V}/" --in-place "$COMMITS_HASH"

    # ImageMagick 7 (MAGICK7_PREFIX, as tools/jenkins/build-plugins.sh): its pkg-config files come
    # first, only for this build; IM=7 when the Magick++ found is ImageMagick 7.
    arena_saved_pkg_config_path="${PKG_CONFIG_PATH:-}"
    if [ -n "${MAGICK7_PREFIX:-}" ]; then
        export PKG_CONFIG_PATH="${MAGICK7_PREFIX}/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
    fi
    arena_magick_version="$(pkg-config --modversion Magick++ 2>/dev/null || true)"
    arena_im=""
    case "$arena_magick_version" in
        7.*) arena_im=7;;
        *) echo "Warning: openfx-arena is built with ImageMagick ${arena_magick_version:-(none)}, not ImageMagick 7 (set MAGICK7_PREFIX)";;
    esac
    if plugins_use_cmake openfx-arena; then
        # OpenMP only if ImageMagick was built with it (the Makefiles keep its -fopenmp)
        arena_openmp=OFF
        case " $(pkg-config Magick++ --cflags --libs --static 2>/dev/null) " in
            *" -fopenmp "*) arena_openmp=ON;;
        esac
        arena_args=(-DOFX_USE_OPENMP="$arena_openmp" -DIM="$arena_im")
        if [ "$NATRON_LICENSE" = "COMMERCIAL" ]; then
            # no AudioCurve: libsox links GPL libraries (libmad, libid3tag; libao on Windows)
            arena_args+=(-DAUDIO=OFF)
        fi
        ofx_cmake_build "${arena_args[@]}"
    else
        if [ "$PKGOS" = "OSX" ]; then
            $GSED -e s/-lgomp// -i.orig Makefile.master
        fi

        if [ "$PKGOS" = "Windows" ]; then
            ISWIN=1
        fi

        # Arena 2.1 breaks compat with older releases, check if this is a 2.1(+) build
        ARENA_CHECK=$(cat Bundle/Makefile|grep lodepng)
        if [ "$ARENA_CHECK" != "" ]; then
            #if [ "$PKGOS" != "OSX" ]; then
            #  export PKG_CONFIG_PATH="${SDK_HOME}/magick7/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
            #else
            #  export PKG_CONFIG_PATH="/opt/magick/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
            #fi
            #ARENA_FLAGS="IM=7"
            # Fetch external depends before build
            make -C Bundle lodepng.h
        fi

        arena_make_audio=""
        if [ "$NATRON_LICENSE" = "COMMERCIAL" ]; then
            # no AudioCurve (libsox links GPL libraries). On the command line: Bundle/Makefile's
            # "AUDIO = ON" (when pkg-config finds sox) overrides the environment.
            arena_make_audio="AUDIO=OFF"
        fi
        make -j"${MKJOBS}" $MAKEFLAGS_VERBOSE MINGW="${ISWIN:-}" LICENSE="$NATRON_LICENSE" ${ARENA_FLAGS:-} ${arena_im:+IM=$arena_im} ${arena_make_audio} \
             CXX="$CXX" CONFIG="${BUILD_MODE}" BITS="${BITS}" OPTFLAG="${OPTFLAG}" LDFLAGS_ADD="${BUILDID:-}" CXXFLAGS_EXTRA="${CXXFLAGS_EXTRA}"
        cp -a ./*/*-*-*/*.ofx.bundle "$TMP_BINARIES_PATH/OFX/Plugins/"
    fi
    export PKG_CONFIG_PATH="$arena_saved_pkg_config_path"
    echo "$ARENA_V" > "$TMP_BINARIES_PATH/OFX/Plugins/Arena.ofx.bundle-version.txt"
    cd "$TMP_PATH"
    #rm -rf openfx-arena || true
fi

# GMIC
if [ "$BUILD_GMIC" = "1" ]; then
    # compiling gmic requires a lot of memory
    GMIC_MKJOBS="${MKJOBS}" # 1 # "${MKJOBS}"
    # also limit memory usage in case we are using gcc
    GMIC_CXXFLAGS_EXTRA=
    if [ "$COMPILER" = "gcc" ] || [ -n "${GXX:-}" ]; then
        # see :
        # https://web.archive.org/web/20061106204547/http://hostingfu.com/article/compiling-with-gcc-on-low-memory-vps
        # http://jkroon.blogs.uls.co.za/it/scriptingprogramming/preventing-gcc-from-trashing-the-system  
        GMIC_CXXFLAGS_EXTRA="--param ggc-min-expand=20 --param ggc-min-heapsize=32768"
    fi
                                 
    cd "$TMP_PATH"

    #$KILLSCRIPT $PID &
    #KILLBOT=$!

    if [ -d openfx-gmic ]; then
        rm -rf openfx-gmic || true
    fi
    if [ -d openfx-gmic ]; then
        sleep 25
        rm -rf openfx-gmic
    fi

    $TIMEOUT 1800 $GIT clone "$GIT_GMIC"
    cd openfx-gmic
    $TIMEOUT 1800 $GIT checkout "${GMIC_BRANCH}"
    $TIMEOUT 1800 $GIT submodule update -i --recursive
    if [ "$IO_BRANCH" = "$MASTER_BRANCH" ]; then
        # the snapshots are always built with the latest version of submodules
        $TIMEOUT 1800 $GIT submodule update -i --recursive --remote
        #$TIMEOUT 1800 $GIT submodule foreach git pull origin master
    fi

    #kill -9 $KILLBOT || true

    GMIC_GIT_VERSION=$(git log|head -1|awk '{print $2}')

    # mksrc
    if [ "${MKSRC:-}" = "1" ]; then
        cd ..
        cp -a openfx-gmic "openfx-gmic-$GMIC_GIT_VERSION"
        (cd "openfx-gmic-$GMIC_GIT_VERSION"; find . -type d -name .git -exec rm -rf {} \;)
        tar cvvJf "$SRC_PATH/openfx-gmic-$GMIC_GIT_VERSION.tar.xz" "openfx-gmic-$GMIC_GIT_VERSION"
        rm -rf "openfx-gmic-$GMIC_GIT_VERSION"
        cd openfx-gmic
    fi

    #Always bump git commit, it is only used to version-stamp binaries
    GMIC_V=$GMIC_GIT_VERSION
    $GSED -e "s/GMICPLUG_DEVEL_GIT=.*/GMICPLUG_DEVEL_GIT=${GMIC_V}/" --in-place "$COMMITS_HASH"

    if plugins_use_cmake openfx-gmic; then
        # The CMake build downloads the G'MIC sources (from each mirror in turn, checking their
        # sha256) into the download cache $SRC_PATH, which outlives the checkout.
        # OFX_LICENSE=COMMERCIAL gives pocketfft instead of FFTW, and only the CeCILL-C filters.
        gmic_download_dir="$SRC_PATH"
        mkdir -p "$gmic_download_dir"
        if [ "$PKGOS" = "Windows" ]; then
            gmic_download_dir="$(cygpath -m "$gmic_download_dir")"
        fi
        gmic_cxxflags_extra="${CXXFLAGS_EXTRA}"
        if [ "$COMPILER" = "gcc" ]; then
            # (the --param options are gcc's)
            gmic_cxxflags_extra="$GMIC_CXXFLAGS_EXTRA ${CXXFLAGS_EXTRA}"
        fi
        gmic_args=(-DGMIC_DOWNLOAD_DIR="$gmic_download_dir")
        if [ "$PKGOS" = "Linux" ] && grep -q 'GMIC_STATIC_DEPS' CMakeLists.txt; then
            # link the SDK's shared zlib and libpng: GMIC_STATIC_DEPS (default ON on Linux) needs
            # static archives built with -fPIC, which the Natron SDK doesn't provide
            gmic_args+=(-DGMIC_STATIC_DEPS=OFF)
        fi
        CXXFLAGS_EXTRA="$gmic_cxxflags_extra" MKJOBS="$GMIC_MKJOBS" \
            ofx_cmake_build "${gmic_args[@]}"
    else
        if [ "$COMPILER" = "gcc" ] || [ "$COMPILER" = "clang-omp" ]; then
            # build with OpenMP support
            make -j"${GMIC_MKJOBS}" $MAKEFLAGS_VERBOSE LICENSE="$NATRON_LICENSE" OPENMP=1 \
                 CXX="$CXX" CONFIG="${BUILD_MODE}" OPTFLAG="${OPTFLAG}" BITS="${BITS}" LDFLAGS_ADD="${BUILDID:-}" CXXFLAGS_EXTRA="$GMIC_CXXFLAGS_EXTRA ${CXXFLAGS_EXTRA}"
        elif [ -n "${GXX:-}" ]; then
            # Building with clang, but GCC is available too!
            make -j"${GMIC_MKJOBS}" $MAKEFLAGS_VERBOSE LICENSE="$NATRON_LICENSE" OPENMP=1 \
                 CXX="$GXX" CONFIG="${BUILD_MODE}" OPTFLAG="${OPTFLAG}" BITS="${BITS}" LDFLAGS_ADD="${BUILDID:-} -static-libgcc" CXXFLAGS_EXTRA="$GMIC_CXXFLAGS_EXTRA ${CXXFLAGS_EXTRA}"
        else
            # build without OpenMP
            make -j"${GMIC_MKJOBS}" $MAKEFLAGS_VERBOSE LICENSE="$NATRON_LICENSE" \
                 CXX="$CXX" CONFIG="${BUILD_MODE}" OPTFLAG="${OPTFLAG}" BITS="${BITS}" LDFLAGS_ADD="${BUILDID:-}" CXXFLAGS_EXTRA="$GMIC_CXXFLAGS_EXTRA ${CXXFLAGS_EXTRA}"
        fi
        cp -a ./*/*-*-*/*.ofx.bundle "$TMP_BINARIES_PATH/OFX/Plugins/"
    fi
    echo "$GMIC_V" > "$TMP_BINARIES_PATH/OFX/Plugins/GMIC.ofx.bundle-version.txt"
    cd "$TMP_PATH"
    #rm -rf openfx-gmic  || true
fi

# OPENCV
if [ "$BUILD_CV" = "1" ]; then
    cd "$TMP_PATH"

    if [ -d openfx-opencv ]; then
        rm -rf openfx-opencv
    fi
    $TIMEOUT 1800 $GIT clone "$GIT_OPENCV"
    cd openfx-opencv
    $TIMEOUT 1800 $GIT checkout "${CV_BRANCH}"
    $TIMEOUT 1800 $GIT submodule update -i --recursive
    if [ "$CV_BRANCH" = "$MASTER_BRANCH" ]; then
        # the snapshots are always built with the latest version of submodules
        $TIMEOUT 1800 $GIT submodule update -i --recursive --remote
        #$TIMEOUT 1800 $GIT submodule foreach git pull origin master
    fi

    CV_GIT_VERSION=$(git log|head -1|awk '{print $2}')

    # mksrc
    if [ "${MKSRC:-}" = "1" ]; then
        cd ..
        cp -a openfx-opencv "openfx-opencv-$CV_GIT_VERSION"
        (cd "openfx-opencv-$CV_GIT_VERSION"; find . -type d -name .git -exec rm -rf {} \;)
        tar cvvJf "$SRC_PATH/openfx-opencv-$CV_GIT_VERSION.tar.xz" "openfx-opencv-$CV_GIT_VERSION"
        rm -rf "openfx-opencv-$CV_GIT_VERSION"
        cd openfx-opencv
    fi

    #Always bump git commit, it is only used to version-stamp binaries
    CV_V="$CV_GIT_VERSION"
    $GSED -e "s/CVPLUG_DEVEL_GIT=.*/CVPLUG_DEVEL_GIT=${CV_V}/" --in-place "$COMMITS_HASH"


    cd opencv2fx
    make -j"${MKJOBS}" $MAKEFLAGS_VERBOSE \
             CXX="$CXX" CONFIG="${BUILD_MODE}" BITS="${BITS}" LDFLAGS_ADD="${BUILDID:-}"
    cp -a ./*/*-"${BITS}"-*/*.ofx.bundle "$TMP_BINARIES_PATH/OFX/Plugins/"
fi

#rm -f $KILLSCRIPT || true

echo "Done!"

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
