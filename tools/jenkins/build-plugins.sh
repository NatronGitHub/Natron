#!/bin/bash
#
# Build Natron Plugins

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

source common.sh
source manageBuildOptions.sh
source manageLog.sh

updateBuildOptions

#MAKEFLAGS_VERBOSE="V=1" # verbose build

# Build system of the plug-ins (#1181): PLUGINS_BUILD_SYSTEM=cmake (default) builds openfx-misc,
# openfx-io, openfx-arena and openfx-gmic with their CMake builds (function ofx_cmake_build);
# PLUGINS_BUILD_SYSTEM=make uses their Makefiles (RB-2.6 fallback, e.g. for old release tags).
PLUGINS_BUILD_SYSTEM="${PLUGINS_BUILD_SYSTEM:-cmake}"
case "$PLUGINS_BUILD_SYSTEM" in
    cmake|make) ;;
    *) echo "Error: PLUGINS_BUILD_SYSTEM must be cmake or make, not \"$PLUGINS_BUILD_SYSTEM\""; exit 1;;
esac


BUILD_IO=1
BUILD_MISC=1
BUILD_ARENA=1
BUILD_GMIC=1
BUILD_CV=0

OMP=""
if [ "$COMPILER" = "gcc" ] || [ "$COMPILER" = "clang-omp" ]; then
    # compile DenoiseSharpen, CImg and GMIC with OpenMP support
    OMP="OPENMP=1"
fi

# When debugging script, do not build if ofx bundle exists
if [ "${DEBUG_SCRIPTS:-}" = "1" ]; then

    if [ "$BUILD_IO" = "1" ]; then
        if [ -d "$TMP_BINARIES_PATH/OFX/Plugins/IO.ofx.bundle" ] || [ -d "$TMP_BINARIES_PATH/Natron.app/Contents/Plugins/OFX/Natron/IO.ofx.bundle" ]; then
            BUILD_IO=0
        fi
    fi

    if [ "$BUILD_MISC" = "1" ]; then
        if ([ -d "$TMP_BINARIES_PATH/OFX/Plugins/Misc.ofx.bundle" ] || [ -d "$TMP_BINARIES_PATH/Natron.app/Contents/Plugins/OFX/Natron/Misc.ofx.bundle" ]) && ([ -d "$TMP_BINARIES_PATH/OFX/Plugins/CImg.ofx.bundle" ] || [ -d "$TMP_BINARIES_PATH/Natron.app/Contents/Plugins/OFX/Natron/CImg.ofx.bundle" ]); then
            BUILD_MISC=0
        fi
    fi
    if [ "$BUILD_ARENA" = "1" ]; then
        if [ -d "$TMP_BINARIES_PATH/OFX/Plugins/Arena.ofx.bundle" ] || [ -d "$TMP_BINARIES_PATH/Natron.app/Contents/Plugins/OFX/Natron/Arena.ofx.bundle" ]; then
            BUILD_ARENA=0
        fi
    fi
    if [ "$BUILD_GMIC" = "1" ]; then
        if [ -d "$TMP_BINARIES_PATH/OFX/Plugins/GMIC.ofx.bundle" ] || [ -d "$TMP_BINARIES_PATH/Natron.app/Contents/Plugins/OFX/Natron/GMIC.ofx.bundle" ]; then
            BUILD_GMIC=0
        fi
    fi
fi

if [ ! -d "$TMP_BINARIES_PATH" ]; then
    mkdir -p "$TMP_BINARIES_PATH"
fi

# Setup env
CXXFLAGS_EXTRA=
if [ "$PKGOS" = "Linux" ]; then
    export OPENJPEG_HOME="$SDK_HOME"
    export THIRD_PARTY_TOOLS_HOME="$SDK_HOME"
    export LD_LIBRARY_PATH="$SDK_HOME/lib:$FFMPEG_PATH/lib"
    export PATH="$SDK_HOME/gcc/bin:$SDK_HOME/bin:$SDK_HOME/cmake/bin:$PATH"
    if [ "$ARCH" = "x86_64" ]; then
        export LD_LIBRARY_PATH="$SDK_HOME/gcc/lib64:$LD_LIBRARY_PATH"
    else
        export LD_LIBRARY_PATH="$SDK_HOME/gcc/lib:$LD_LIBRARY_PATH"
    fi
    export C_INCLUDE_PATH="${SDK_HOME}/gcc/include:${SDK_HOME}/include:${FFMPEG_PATH}/include:$LIBRAW_PATH/include"
    export CPLUS_INCLUDE_PATH="${C_INCLUDE_PATH}"

elif [ "$PKGOS" = "Windows" ]; then
    export OPENJPEG_HOME="$SDK_HOME"
    export THIRD_PARTY_TOOLS_HOME="$SDK_HOME"
    export GIT_SSL_NO_VERIFY="true"
    # https://github.com/msys2/MINGW-packages/issues/10761
    # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70129
    # export C_INCLUDE_PATH="${SDK_HOME}/gcc/include:${SDK_HOME}/include:${FFMPEG_PATH}/include:$LIBRAW_PATH/include"
    export C_INCLUDE_PATH="${FFMPEG_PATH}/include:${LIBRAW_PATH}/include"
    export CPLUS_INCLUDE_PATH="${C_INCLUDE_PATH}"
    BUILDID="-Wl,--build-id"
elif [ "$PKGOS" = "OSX" ]; then
    # see https://github.com/Homebrew/homebrew-core/issues/32765
    CXXFLAGS_EXTRA="-isysroot `xcodebuild -version -sdk macosx Path 2>/dev/null`"
fi

if [ "${DEBUG_SCRIPTS:-}" != "1" ] && [ -d "$TMP_BINARIES_PATH/OFX/Plugins" ]; then
    rm -rf "$TMP_BINARIES_PATH/OFX/Plugins"
fi

if [ ! -d "$TMP_BINARIES_PATH/OFX/Plugins" ]; then
    mkdir -p "$TMP_BINARIES_PATH/OFX/Plugins"
fi

if [ -d "$TMP_BINARIES_PATH/Plugins" ]; then
    rm -rf "$TMP_BINARIES_PATH/Plugins"
fi
mkdir -p "$TMP_BINARIES_PATH/Plugins"


# CMake build type of the plug-ins (ofx_cmake_build), before COMPILE_TYPE=debug is rewritten
# below for the Makefiles. On macOS, openfx's ofx_deploy_bundle only deploys release
# configurations, as the Makefiles: Debug bundles still reference the MacPorts libraries.
case "$COMPILE_TYPE" in
    debug) PLUGINS_CMAKE_BUILD_TYPE=Debug;;
    relwithdebinfo) PLUGINS_CMAKE_BUILD_TYPE=RelWithDebInfo;;
    *) PLUGINS_CMAKE_BUILD_TYPE=Release;;
esac

if [ "$COMPILE_TYPE" = "debug" ]; then
    # Makefiles: CI build have to be build with CONFIG=relwithdebinfo, or the
    # installer build for plugins won't work (deployment on osx, which
    # copies libraries, is only
    # done in release and relwithdebinfo configs).
    # 
    #COMPILE_TYPE=debug
    COMPILE_TYPE=relwithdebinfo
    OPTFLAG=-O
else
    # Makefile builds (PLUGINS_BUILD_SYSTEM=make): -O3, like the CMake builds. Not -Ofast: its
    # -ffinite-math-only lets NaN and Inf through std::max/std::min and folds
    # std::isnan/std::isfinite to constants, so the plug-ins' NaN/Inf guards don't work.
    OPTFLAG=-O3
fi
# OPTFLAG is ignored by the CMake builds (ofx_cmake_build): they use -O3, with -g for
# relwithdebinfo, and conformant IEEE floating point.

CP_OR_MV="cp -a"
if [ "${MINIMIZE_DISK_USAGE:-}" = "1" ]; then
    # Move large files instead of copying them to minimize disk usage.
    CP_OR_MV=mv
fi

function maybe_cleanup_binary_files() {
    if [ "${MINIMIZE_DISK_USAGE:-}" = "1" ]; then
        echo "Cleaning up files to minimize disk usage..."

        rm -fv `find . -name *.o`
        rm -fv `find . -name *.ofx`
    fi
}


# Whether to build the plug-in repository in the current directory (named $1) with its CMake
# build: yes unless PLUGINS_BUILD_SYSTEM=make. A checkout without a CMake build that uses openfx's
# helpers (ofx_deploy_bundle) is an error, rather than a silent fallback to its Makefiles.
function plugins_use_cmake() {
    if [ "$PLUGINS_BUILD_SYSTEM" = "make" ]; then
        return 1
    fi
    if [ -f CMakeLists.txt ] && grep -q 'ofx_deploy_bundle' CMakeLists.txt; then
        return 0
    fi
    echo "Error: $1 in $(pwd) has no CMake build with openfx's helpers (ofx_deploy_bundle): use a newer $1, or build the plug-ins with their Makefiles (PLUGINS_BUILD_SYSTEM=make)"
    exit 1
}

# Build and install the plug-in repository in the current directory with its CMake build
# (openfx's add_ofx_plugin/ofx_deploy_bundle helpers), into $TMP_BINARIES_PATH/OFX/Plugins, where
# the Makefile builds copy their bundles too.
# Common options: the build type (PLUGINS_CMAKE_BUILD_TYPE), the C++ standard (C++17 with Qt5,
# C++20 with Qt6), the licence (OFX_LICENSE=$NATRON_LICENSE, where supported), OpenMP, and on macOS the deployment
# target and the bundle deployment (OFX_DEPLOY_BUNDLES).
# Extra arguments are passed to cmake at configure time, after these (so they win). The configure
# output is checked: the build fails if a plug-in group was dropped because its libraries were not
# found, or if OpenMP was requested but not found.
# Paths given as arguments must already be in the form cmake expects (on Windows: cygpath -m).
function ofx_cmake_build() {
    # $CXX may carry flags (e.g. "clang++-mp-22 -stdlib=libc++ -std=c++20"): CMake needs the
    # compiler alone, and the other flags go to CMAKE_CXX_FLAGS. Its -std=... is dropped: the C++
    # standard of the plug-ins follows the policy (C++17 with Qt5, C++20 with Qt6), through
    # OFX_CXX_STANDARD. Limitation: $CXX is split on spaces, so the compiler path must not
    # contain spaces, and a launcher (e.g. "ccache clang++") is not supported.
    local cxx_words=($CXX)
    local cxx="${cxx_words[0]}"
    local cxx_flags=""
    local cxx_std=17
    local i w
    if [ "${QT_VERSION_MAJOR:-5}" -ge 6 ]; then
        cxx_std=20
    fi
    # (index loop: macOS /bin/bash 3.2 with set -u rejects expanding an empty array slice)
    for ((i = 1; i < ${#cxx_words[@]}; i++)); do
        w="${cxx_words[$i]}"
        case "$w" in
            -std=*) echo "Info: plug-ins built with OFX_CXX_STANDARD=$cxx_std, not CXX's $w (C++17 with Qt5, C++20 with Qt6)";;
            *) cxx_flags="$cxx_flags $w";;
        esac
    done
    # OpenMP (ofx_target_openmp): with gcc and MacPorts clang (COMPILER=gcc or clang-omp), and
    # with Apple clang from MacPorts' libomp (OFX_LIBOMP_ROOT). A repository may override it.
    local openmp=OFF
    if [ "$COMPILER" = "gcc" ] || [ "$COMPILER" = "clang-omp" ]; then
        openmp=ON
    elif [ "$PKGOS" = "OSX" ] && "$cxx" --version 2>/dev/null | grep -q '^Apple clang' \
         && [ -f "${MACPORTS:-/opt/local}/include/libomp/omp.h" ]; then
        openmp=ON
    fi
    local prefix_path="$SDK_HOME" install_prefix="$TMP_BINARIES_PATH"
    if [ "$PKGOS" = "Windows" ]; then
        # cmake.exe and ninja are native programs: pass Windows paths (untested here)
        prefix_path="$(cygpath -m "$prefix_path")"
        install_prefix="$(cygpath -m "$install_prefix")"
    fi
    local args=(
        -S . -B build
        -DCMAKE_BUILD_TYPE="$PLUGINS_CMAKE_BUILD_TYPE"
        # -O3 also with debug info (CMake's RelWithDebInfo default is -O2 -g; Release is -O3)
        -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O3 -g -DNDEBUG"
        -DCMAKE_CXX_COMPILER="$cxx"
        -DCMAKE_CXX_FLAGS="${cxx_flags# } ${CXXFLAGS_EXTRA}"
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
        # Apple clang has no OpenMP runtime: MacPorts' libomp (only used with Apple clang)
        args+=(-DOFX_LIBOMP_ROOT="${MACPORTS:-/opt/local}")
        if [ -n "${MACOSX_DEPLOYMENT_TARGET:-}" ]; then
            args+=(-DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOSX_DEPLOYMENT_TARGET")
        fi
        # make the bundles self-contained, as the Makefiles' release builds do (osxDeploy.sh)
        args+=(-DOFX_DEPLOY_BUNDLES=ON)
        if [ "$PLUGINS_CMAKE_BUILD_TYPE" != "Debug" ]; then
            # fail if a bundled library needs a newer macOS than the deployment target
            # (an "unused variable" warning where the repository's openfx is older)
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
    # Don't ship incomplete bundles: openfx-io drops a plug-in group whose libraries are not
    # found (STATUS message only), and ofx_target_openmp only warns when OpenMP is missing.
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
if [ "$BUILD_MISC" = "1" ] && [ -d "$TMP_PATH/openfx-misc" ]; then

    printStatusMessage "Building openfx-misc..."

    cd "$TMP_PATH/openfx-misc"

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

    # Build static on Windows so that openfx-misc are usable on other hosts without DLL's and manifests (note Shadertoy will require osmesa dll)
    # Could/Should also be done on OSX?
    # On Linux this is not the best solution, as Nuke forces gcc, so static won't help. as-is the plugins should work in Nuke10+ (CY2015),
    # but will not work on older versions. The best solution on Nuke9 and lower is to build the plugins using RHEL devtoolset (gcc4.7).
    #if [ "$PKGOS" = "Windows" ]; then
    #    EXTRA_LDFLAGS_OFXMISC="-static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -lgomp -lwinpthread -Wl,-Bdynamic"
    #fi

    # openfx-misc builds with openfx's CMake helpers since openfx-misc#152. The legacy Apple
    # clang + GXX setup (CImg built by gcc for OpenMP) only exists in the make path: the CMake
    # build uses MacPorts' libomp with Apple clang.
    if plugins_use_cmake openfx-misc; then
        echo "Info: build openfx-misc using cmake..."
        # Shadertoy is its own bundle, with OSMesa: the same OSMesa/LLVM link line as the make
        # path (LDFLAGS_MESA below)
        misc_osmesa="$OSMESA_PATH"
        misc_llvm="$LLVM_PATH"
        misc_sdk="$SDK_HOME"
        misc_llvm_lib="$LLVM_LIB"
        if [ "$PKGOS" = "Windows" ]; then
            # cmake.exe is a native program: pass Windows paths (untested here)
            misc_osmesa="$(cygpath -m "$OSMESA_PATH")"
            misc_llvm="$(cygpath -m "$LLVM_PATH")"
            misc_sdk="$(cygpath -m "$SDK_HOME")"
            misc_llvm_lib="${LLVM_LIB//-L${LLVM_PATH}/-L${misc_llvm}}"
        fi
        ofx_cmake_build \
            -DUSE_OSMESA=ON \
            -DOSMESA_PATH="${misc_osmesa}" \
            -DLLVM_PATH="${misc_llvm}" \
            -DOSMESA_INCLUDES="${misc_osmesa}/include" \
            -DOSMESA_LIBRARIES="-L${misc_osmesa}/lib ${GLULIB} ${MESALIB} -L${misc_sdk}/lib -lz -L${misc_llvm}/lib ${misc_llvm_lib}"
        echo "Info: build openfx-misc using cmake... done!"
    else
        echo "Info: build openfx-misc using make..."
        #set -x
        #make -C Shadertoy V=1 CXXFLAGS_MESA="-DHAVE_OSMESA" OSMESA_PATH="${OSMESA_PATH}" LDFLAGS_MESA="-L${OSMESA_PATH}/lib -L${LLVM_PATH}/lib ${GLULIB} ${MESALIB} $LLVM_LIB" CXX="$CXX" CONFIG="${COMPILE_TYPE}" OPTFLAG="${OPTFLAG}" LDFLAGS_ADD="${BUILDID:-} ${EXTRA_LDFLAGS_OFXMISC:-}" -j"${MKJOBS}" BITS="${BITS}" HAVE_CIMG=0
        #set +x
        # first, build everything except CImg (including OpenGL plugins)

        echo "Info: make all except CImg."

        set -x
        env \
            OSMESA_PATH="${OSMESA_PATH}" \
            LLVM_PATH="${LLVM_PATH}" \
            CXXFLAGS_MESA="-I${OSMESA_PATH}/include" \
            LDFLAGS_MESA="-L${OSMESA_PATH}/lib ${GLULIB} ${MESALIB} -L${SDK_HOME}/lib -lz -L${LLVM_PATH}/lib ${LLVM_LIB}" \
            CXX="$CXX" \
            CONFIG="${COMPILE_TYPE}" \
            OPTFLAG="${OPTFLAG}" \
            BITS="${BITS}" \
            LDFLAGS_ADD="${BUILDID:-} ${EXTRA_LDFLAGS_OFXMISC:-}" \
            HAVE_CIMG=0 \
            ${OMP} \
            LICENSE="$NATRON_LICENSE" \
            CXXFLAGS_EXTRA="-DHAVE_OSMESA ${CXXFLAGS_EXTRA}" \
            make -j"${MKJOBS}" ${MAKEFLAGS_VERBOSE:-}
        set +x

        echo "Info: make CImg."

        # extract CImg.h
        make -C CImg LICENSE="$NATRON_LICENSE" CImg.h
        
        # build CImg.ofx

        set -x
        if [ "$COMPILER" = "clang" ] && [ -n "${GXX:-}" ]; then
            # Building with Apple clang (no OpenMP available), but GCC is available too!
            # libSupport was compiled by clang, now clean it to build it again with gcc
            env \
                CXX="$CXX" CONFIG="${COMPILE_TYPE}" OPTFLAG="${OPTFLAG}" BITS="${BITS}" HAVE_CIMG=0 \
                make -j"${MKJOBS}" clean
            # build CImg with OpenMP support, but statically link libgomp (see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=31400)
            env \
                CXX="$CXX" \
                CONFIG="${COMPILE_TYPE}" \
                OPTFLAG="${OPTFLAG}" \
                BITS="${BITS}" \
                LDFLAGS_ADD="${BUILDID:-} ${EXTRA_LDFLAGS_OFXMISC:-} -static-libgcc" \
                HAVE_CIMG=1 \
                OPENMP=1 \
                LICENSE="$NATRON_LICENSE" \
                CXXFLAGS_EXTRA="${CXXFLAGS_EXTRA}" \
                make -j"${MKJOBS}" ${MAKEFLAGS_VERBOSE:-} -C CImg
        else
            # build CImg with OpenMP support (no OpenGL required)
            env \
                CXX="$CXX" \
                CONFIG="${COMPILE_TYPE}" \
                OPTFLAG="${OPTFLAG}" \
                BITS="${BITS}" \
                LDFLAGS_ADD="${BUILDID:-} ${EXTRA_LDFLAGS_OFXMISC:-}" \
                HAVE_CIMG=1 \
                ${OMP} \
                LICENSE="$NATRON_LICENSE" \
                CXXFLAGS_EXTRA="${CXXFLAGS_EXTRA}" \
                make -j"${MKJOBS}" ${MAKEFLAGS_VERBOSE:-} -C CImg
        fi
        ${CP_OR_MV} ./*/*-*-*/*.ofx.bundle "$TMP_BINARIES_PATH/OFX/Plugins/"
        set +x
        echo "Info: build openfx-misc using make... done!"
    fi

    maybe_cleanup_binary_files

    cd "$TMP_PATH"
    #rm -rf openfx-misc || true
fi

# IO
if [ "$BUILD_IO" = "1" ] && [ -d "$TMP_PATH/openfx-io" ]; then

    printStatusMessage "Building openfx-io..."
    cd "$TMP_PATH/openfx-io"

    #Always bump git commit, it is only used to version-stamp binaries

    # openfx-io builds with openfx's CMake helpers (add_ofx_plugin, ofx_deploy_bundle) since
    # openfx-io#102.
    if plugins_use_cmake openfx-io; then
        echo "Info: build openfx-io using cmake..."
        io_seexpr="$SDK_HOME"
        io_ffmpeg_pc="${FFMPEG_PKG_CONFIG_PATH:-}"
        io_prefix_args=()
        if [ "$PKGOS" != "OSX" ]; then
            # Linux and Windows: the FFmpeg and LibRaw of the license (common.sh), as the
            # make path gets them through C_INCLUDE_PATH and pkg-config
            io_ffmpeg_pc="${io_ffmpeg_pc:-${FFMPEG_PATH}/lib/pkgconfig}"
            io_prefix_args+=(-DCMAKE_PREFIX_PATH="${SDK_HOME};${LIBRAW_PATH};${FFMPEG_PATH}")
        fi
        if [ "$PKGOS" = "Windows" ]; then
            # cmake.exe is a native program: pass Windows paths (untested here)
            io_seexpr="$(cygpath -m "$io_seexpr")"
            [ -n "$io_ffmpeg_pc" ] && io_ffmpeg_pc="$(cygpath -m "$io_ffmpeg_pc")"
            io_prefix_args=(-DCMAKE_PREFIX_PATH="$(cygpath -m "$SDK_HOME");$(cygpath -m "$LIBRAW_PATH");$(cygpath -m "$FFMPEG_PATH")")
        fi
        # OFX_IO_REQUIRE_ALL (requested from openfx-io): fail on missing plug-in groups; an
        # "unused variable" warning until openfx-io has it (ofx_cmake_build also checks the log)
        io_saved_pkg_config_path="${PKG_CONFIG_PATH:-}"
        if [ "$PKGOS" != "OSX" ]; then
            # LibRaw's libraw_r.pc (only for this build, not for the next plug-ins)
            export PKG_CONFIG_PATH="${LIBRAW_PATH}/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
        fi
        ofx_cmake_build -DSEEXPR_HOME="$io_seexpr" -DFFMPEG_PKG_CONFIG_PATH="$io_ffmpeg_pc" \
            -DOFX_IO_REQUIRE_ALL=ON ${io_prefix_args[@]+"${io_prefix_args[@]}"}
        export PKG_CONFIG_PATH="$io_saved_pkg_config_path"
        echo "Info: build openfx-io using cmake... done!"
    else
        echo "Info: build openfx-io using make..."
        set -x
        env \
            OIIO_HOME="$SDK_HOME" \
            SEEXPR_HOME="${SDK_HOME}" \
            CXX="$CXX" \
            CONFIG="${COMPILE_TYPE}" \
            OPTFLAG="${OPTFLAG}" \
            BITS="${BITS}" \
            LDFLAGS_ADD="${BUILDID:-}" \
            CXXFLAGS_EXTRA="${CXXFLAGS_EXTRA}" \
            make -j"${MKJOBS}" ${MAKEFLAGS_VERBOSE:-}
        ${CP_OR_MV} ./*/*-*-*/*.ofx.bundle "$TMP_BINARIES_PATH/OFX/Plugins/"
        set +x
        echo "Info: build openfx-io using make... done!"
    fi

    maybe_cleanup_binary_files

    cd "$TMP_PATH"
    #rm -rf openfx-io  || true
fi

# ARENA
if [ "$BUILD_ARENA" = "1" ] && [ -d "$TMP_PATH/openfx-arena" ]; then

    printStatusMessage "Building openfx-arena..."
    cd "$TMP_PATH/openfx-arena"

    # openfx-arena builds with openfx's CMake helpers since openfx-arena#72.
    if plugins_use_cmake openfx-arena; then
        echo "Info: build openfx-arena using cmake..."
        # Same ImageMagick as the make path: the Magick++ found by pkg-config (ImageMagick 6;
        # the SDK's magick7 is not used, see the commented-out IM=7 in the make path), and
        # OpenMP only if that ImageMagick was built with it (the Makefiles keep its -fopenmp,
        # which the CMake build drops in favour of OFX_USE_OPENMP). Not static on Windows
        # (no MINGW_STATIC), as the make path.
        arena_openmp=OFF
        case " $(pkg-config Magick++ --cflags --libs 2>/dev/null) " in
            *" -fopenmp "*) arena_openmp=ON;;
        esac
        arena_args=(-DOFX_LICENSE="$NATRON_LICENSE" -DOFX_USE_OPENMP="$arena_openmp")
        if [ "$NATRON_LICENSE" = "COMMERCIAL" ]; then
            # no AudioCurve: libsox links GPL libraries (libmad, libid3tag; libao on Windows)
            arena_args+=(-DAUDIO=OFF)
        fi
        ofx_cmake_build ${arena_args[@]+"${arena_args[@]}"}
        echo "Info: build openfx-arena using cmake... done!"
    else
        if [ "$PKGOS" = "OSX" ]; then
            $GSED -e s/-lgomp// -i.orig Makefile.master
        fi

        if [ "$PKGOS" = "Windows" ]; then
            ISWIN=1
        fi
        arena_make_audio=""
        if [ "$NATRON_LICENSE" = "COMMERCIAL" ]; then
            # no AudioCurve (libsox links GPL libraries). On the command line: Bundle/Makefile's
            # "AUDIO = ON" (when pkg-config finds sox) overrides the environment.
            arena_make_audio="AUDIO=OFF"
        fi

        echo "Info: build openfx-arena using make..."
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

        set -x
        env \
            MINGW="${ISWIN:-}" \
            LICENSE="$NATRON_LICENSE" \
            ${ARENA_FLAGS:-} \
            CXX="$CXX" \
            CONFIG="${COMPILE_TYPE}" \
            OPTFLAG="${OPTFLAG}" \
            BITS="${BITS}" \
            LDFLAGS_ADD="${BUILDID:-}" \
            CXXFLAGS_EXTRA="${CXXFLAGS_EXTRA}" \
            make -j"${MKJOBS}" ${MAKEFLAGS_VERBOSE:-} ${arena_make_audio:-}

        ${CP_OR_MV} ./*/*-*-*/*.ofx.bundle "$TMP_BINARIES_PATH/OFX/Plugins/"
        set +x

        echo "Info: build openfx-arena using make... done!"
    fi

    maybe_cleanup_binary_files

    cd "$TMP_PATH"
    #rm -rf openfx-arena || true
fi

# GMIC
if [ "$BUILD_GMIC" = "1" ] && [ -d "$TMP_PATH/openfx-gmic" ]; then
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

    printStatusMessage "Building openfx-gmic..."
    cd "$TMP_PATH/openfx-gmic"

    # openfx-gmic builds with openfx's CMake helpers since openfx-gmic#61.
    if plugins_use_cmake openfx-gmic; then
        echo "Info: build openfx-gmic using cmake..."
        # The CMake build downloads the G'MIC sources (from each mirror in turn, checking their
        # sha256) into GMIC_DOWNLOAD_DIR: the download cache $SRC_PATH, which outlives the
        # openfx-gmic checkout, so a tarball already there is not downloaded again.
        # OFX_LICENSE=COMMERCIAL (ofx_cmake_build) gives pocketfft instead of FFTW, and only the
        # G'MIC filters under CeCILL-C.
        gmic_download_dir="$SRC_PATH"
        mkdir -p "$gmic_download_dir"
        if [ "$PKGOS" = "Windows" ]; then
            # cmake.exe is a native program: pass Windows paths (untested here)
            gmic_download_dir="$(cygpath -m "$gmic_download_dir")"
        fi
        gmic_cxxflags_extra="${CXXFLAGS_EXTRA}"
        if [ "$COMPILER" = "gcc" ]; then
            # (the --param options are gcc's)
            gmic_cxxflags_extra="$GMIC_CXXFLAGS_EXTRA ${CXXFLAGS_EXTRA}"
        fi
        CXXFLAGS_EXTRA="$gmic_cxxflags_extra" MKJOBS="$GMIC_MKJOBS" \
            ofx_cmake_build -DGMIC_DOWNLOAD_DIR="$gmic_download_dir"
        echo "Info: build openfx-gmic using cmake... done!"
    else
        if [ "$PKGOS" = "Windows" ]; then
            ISWIN=1
        fi

        # gmic.eu may refuse downloads (HTTP 403 seen in 2026), and the Makefile's curl saves the
        # error page as the tarball. Pre-fetch the G'MIC sources (official site, then the MacPorts
        # distfiles mirror), verifying the checksum. Only for versions whose checksum is known here:
        # for others, the openfx-gmic Makefile downloads and verifies the sources itself.
        GMICVER=$(sed -n 's/^GMICVER *= *//p' Makefile | head -1)
        GMICSRC=$(sed -n 's/^GMICSRC *= *//p' Makefile | head -1 | sed "s/\$(GMICVER)/${GMICVER}/")
        GMIC_SHA256=
        case "$GMICSRC" in
            gmic_2.8.4.tar.gz) GMIC_SHA256=b2a279bdf981595b30d8d7828fabf9a620cde06eae8db345a8cddf2160c25379 ;; # MacPorts gmic 2.8.4
        esac
        gmic_src_ok() { # file
            gzip -t "$1" 2>/dev/null || return 1
            [ -n "$GMIC_SHA256" ] || return 0
            if command -v sha256sum >/dev/null 2>&1; then
                [ "$(sha256sum "$1" | cut -d' ' -f1)" = "$GMIC_SHA256" ]
            elif command -v shasum >/dev/null 2>&1; then
                [ "$(shasum -a 256 "$1" | cut -d' ' -f1)" = "$GMIC_SHA256" ]
            else
                echo "Warning: neither sha256sum nor shasum found, cannot verify $1"
                return 1
            fi
        }
        if [ -n "$GMICSRC" ] && [ -n "$GMIC_SHA256" ] && ! gmic_src_ok "$GMICSRC"; then
            rm -f "$GMICSRC"
            for url in "https://gmic.eu/files/source/$GMICSRC" ${GMIC_SHA256:+"https://distfiles.macports.org/gmic/$GMICSRC"}; do
                if curl -f --retry 3 -L -s -S -o "$GMICSRC.tmp" "$url" && gmic_src_ok "$GMICSRC.tmp"; then
                    mv "$GMICSRC.tmp" "$GMICSRC"
                    echo "Info: fetched $GMICSRC from $url"
                    break
                fi
                echo "Warning: could not fetch a valid $GMICSRC from $url"
                rm -f "$GMICSRC.tmp"
            done
            if [ -n "$GMIC_SHA256" ] && [ ! -f "$GMICSRC" ]; then
                echo "Error: could not download $GMICSRC"
                exit 1
            fi
        fi

        if [ "$NATRON_LICENSE" = "COMMERCIAL" ] && [ ! -f license.mk ]; then
            # An openfx-gmic without license.mk ignores LICENSE= and links FFTW (GPL), whose DLLs
            # the COMMERCIAL Windows installer leaves out (genDllVersions.sh).
            echo "Error: this openfx-gmic has no license.mk, so its Makefiles can't make a COMMERCIAL build (it would link FFTW, which is GPL): use a newer openfx-gmic"
            exit 1
        fi
        echo "Info: build openfx-gmic using make..."

        # build GMIC.ofx
        set -x
        if [ "$COMPILER" = "clang" ] && [ -n "${GXX:-}" ]; then
            # Building with Apple clang (no OpenMP available), but GCC is available too!
            env \
                CXX="$GXX" \
                CONFIG="${COMPILE_TYPE}" \
                OPTFLAG="${OPTFLAG}" \
                BITS="${BITS}" \
                LDFLAGS_ADD="${BUILDID:-} -static-libgcc" \
                OPENMP=1 \
                LICENSE="$NATRON_LICENSE" \
                CXXFLAGS_EXTRA="$GMIC_CXXFLAGS_EXTRA ${CXXFLAGS_EXTRA}" \
                make -j"${GMIC_MKJOBS}" ${MAKEFLAGS_VERBOSE:-}
        else
            # build with OpenMP support (no OpenGL required)
            env \
                CXX="$CXX" \
                CONFIG="${COMPILE_TYPE}" \
                OPTFLAG="${OPTFLAG}" \
                BITS="${BITS}" \
                LDFLAGS_ADD="${BUILDID:-}" \
                ${OMP} \
                LICENSE="$NATRON_LICENSE" \
                CXXFLAGS_EXTRA="$GMIC_CXXFLAGS_EXTRA ${CXXFLAGS_EXTRA}" \
                make -j"${GMIC_MKJOBS}" ${MAKEFLAGS_VERBOSE:-}
        fi
        ${CP_OR_MV} ./*/*-*-*/*.ofx.bundle "$TMP_BINARIES_PATH/OFX/Plugins/"
        set +x
        echo "Info: build openfx-gmic using make... done!"
    fi
    maybe_cleanup_binary_files

    cd "$TMP_PATH"
    #rm -rf openfx-gmic || true
fi

# OPENCV
if [ "$BUILD_CV" = "1" ] && [ -d "$TMP_PATH/openfx-opencv" ]; then

    printStatusMessage "Building openfx-opencv..."
    cd "$TMP_PATH/openfx-opencv"

    #Always bump git commit, it is only used to version-stamp binaries
    CV_V="$CV_GIT_VERSION"
    setBuildOption "OPENFX_OPENCV_GIT_COMMIT" "${CV_V}"
    setBuildOption "OPENFX_OPENCV_GIT_BRANCH" "${CV_BRANCH}"
    cd opencv2fx
    env \
        CXX="$CXX" \
        CONFIG="${COMPILE_TYPE}" \
        OPTFLAG="${OPTFLAG}" \
        BITS="${BITS}" \
        LDFLAGS_ADD="${BUILDID:-}" \
        CXXFLAGS_EXTRA="${CXXFLAGS_EXTRA}" \
        make -j"${MKJOBS}" ${MAKEFLAGS_VERBOSE:-}
    ${CP_OR_MV} ./*/*-"${BITS}"-*/*.ofx.bundle "$TMP_BINARIES_PATH/OFX/Plugins/"

    maybe_cleanup_binary_files

fi

#rm -f $KILLSCRIPT || true

echo "Done!"

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
