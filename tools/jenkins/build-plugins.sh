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

# Enable to use cmake to build openfx-misc instead of its Makefiles (old, untested CMake path).
# openfx-io uses its CMake build by default (set IO_USE_MAKE=1 to use its Makefiles).
CMAKE_PLUGINS=0


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


if [ "$COMPILE_TYPE" = "debug" ]; then
    # CI build have to be build with CONFIG=relwithdebinfo, or the
    # installer build for plugins won't work (deployment on osx, which
    # copies libraries, is only
    # done in release and relwithdebinfo configs).
    # 
    #COMPILE_TYPE=debug
    COMPILE_TYPE=relwithdebinfo
    OPTFLAG=-O
else
    # Let us benefit from maximum optimization for release builds,
    # including non-conformant IEEE floating-point computation.
    OPTFLAG=-Ofast
fi

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

    if [ "$CMAKE_PLUGINS" = "1" ] && [ -f CMakeLists.txt ] && [ -d openfx/cmake ]; then
        echo "Info: build openfx-misc using cmake..."
        # Use CMake if available
        mkdir build
        cd build
        CMAKE_LDFLAGS=""
        if [ "$PKGOS" = "OSX" ] && [ "$BITS" = "Universal" ]; then
            UNIVERSAL='-DCMAKE_OSX_ARCHITECTURES="x86_64;i386"'
        elif [ "$PKGOS" = "Windows" ]; then
            MSYS='-G"MSYS Makefiles"'
            CMAKE_LDFLAGS="-static -Wl,--build-id"
        fi
        rm ../DenoiseSharpen/DenoiseWavelet.cpp || true
        set -x
        env CXX="$CXX" LDFLAGS="$CMAKE_LDFLAGS" cmake .. ${MSYS:-} -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DBITS="${BITS}" -DUSE_OSMESA=1 -DOSMESA_INCLUDES="${OSMESA_PATH}/include" -DOSMESA_LIBRARIES="-L${OSMESA_PATH}/lib -L${LLVM_PATH}/lib ${GLULIB} ${MESALIB} $LLVM_LIB" ${UNIVERSAL:-}
        make -j"${MKJOBS}"
        make install
        set +x
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
            CXXFLAGS_EXTRA="-DHAVE_OSMESA ${CXXFLAGS_EXTRA}" \
            make -j"${MKJOBS}" ${MAKEFLAGS_VERBOSE:-}
        set +x

        echo "Info: make CImg."

        # extract CImg.h
        make -C CImg CImg.h
        
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
    # openfx-io#102. Use them when available, unless IO_USE_MAKE=1 forces the Makefiles.
    if [ "${IO_USE_MAKE:-}" != "1" ] && [ -f CMakeLists.txt ] && grep -q 'ofx_deploy_bundle' CMakeLists.txt; then
        echo "Info: build openfx-io using cmake..."
        # $CXX may carry flags (e.g. "clang++-mp-22 -stdlib=libc++ -std=c++20"): CMake needs the
        # compiler alone, the -std=... becomes OFX_CXX_STANDARD, and the other flags go to
        # CMAKE_CXX_FLAGS.
        io_cxx_words=($CXX)
        io_cxx="${io_cxx_words[0]}"
        io_cxx_flags=""
        io_cxx_std=""
        # (index loop: macOS /bin/bash 3.2 with set -u rejects expanding an empty array slice)
        for ((i = 1; i < ${#io_cxx_words[@]}; i++)); do
            w="${io_cxx_words[$i]}"
            case "$w" in
                -std=c++1z|-std=gnu++1z|-std=c++17|-std=gnu++17) io_cxx_std=17;;
                -std=c++2a|-std=gnu++2a|-std=c++20|-std=gnu++20) io_cxx_std=20;;
                -std=c++2b|-std=gnu++2b|-std=c++23|-std=gnu++23) io_cxx_std=23;;
                -std=*) echo "Error: unsupported C++ standard in CXX=$CXX"; exit 1;;
                *) io_cxx_flags="$io_cxx_flags $w";;
            esac
        done
        if [ -z "$io_cxx_std" ]; then
            # C++ standard policy: C++17 with Qt5, C++20 with Qt6
            if [ "${QT_VERSION_MAJOR:-5}" -ge 6 ]; then io_cxx_std=20; else io_cxx_std=17; fi
        fi
        io_cc_words=(${CC:-})
        io_cmake_args=(
            -S . -B build
            -DCMAKE_BUILD_TYPE="$([ "$COMPILE_TYPE" = "relwithdebinfo" ] && echo RelWithDebInfo || echo Release)"
            -DCMAKE_CXX_COMPILER="$io_cxx"
            -DCMAKE_CXX_FLAGS="${io_cxx_flags# } ${CXXFLAGS_EXTRA}"
            -DOFX_CXX_STANDARD="$io_cxx_std"
            -DCMAKE_INSTALL_PREFIX="$TMP_BINARIES_PATH"
            -DPLUGIN_INSTALLDIR="OFX/Plugins"
            -DCMAKE_PREFIX_PATH="$SDK_HOME"
            -DSEEXPR_HOME="$SDK_HOME"
            -DFFMPEG_PKG_CONFIG_PATH="${FFMPEG_PKG_CONFIG_PATH:-}"
            -DBUILD_TESTING=OFF
        )
        if [ -n "${io_cc_words[0]:-}" ]; then
            io_cmake_args+=(-DCMAKE_C_COMPILER="${io_cc_words[0]}")
        fi
        if [ -n "${BUILDID:-}" ]; then
            io_cmake_args+=(-DCMAKE_SHARED_LINKER_FLAGS="${BUILDID}" -DCMAKE_MODULE_LINKER_FLAGS="${BUILDID}")
        fi
        if [ "$PKGOS" = "OSX" ]; then
            # make the bundle self-contained, as the Makefile's release builds do (osxDeploy.sh)
            io_cmake_args+=(-DOFX_DEPLOY_BUNDLES=ON)
        fi
        if command -v ninja >/dev/null 2>&1; then
            io_cmake_args+=(-G Ninja)
        elif [ "$PKGOS" = "Windows" ]; then
            io_cmake_args+=(-G "MSYS Makefiles")
        fi
        rm -rf build
        set -x
        cmake "${io_cmake_args[@]}"
        cmake --build build -j"${MKJOBS}"
        cmake --install build
        set +x
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

    if [ "$PKGOS" = "OSX" ]; then
        $GSED -e s/-lgomp// -i.orig Makefile.master
    fi

    if [ "$PKGOS" = "Windows" ]; then
        ISWIN=1
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
        make -j"${MKJOBS}" ${MAKEFLAGS_VERBOSE:-}

    ${CP_OR_MV} ./*/*-*-*/*.ofx.bundle "$TMP_BINARIES_PATH/OFX/Plugins/"
    set +x

    maybe_cleanup_binary_files

    cd "$TMP_PATH"
    #rm -rf openfx-arena || true
    echo "Info: build openfx-arena using make... done!"
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
            CXXFLAGS_EXTRA="$GMIC_CXXFLAGS_EXTRA ${CXXFLAGS_EXTRA}" \
            make -j"${GMIC_MKJOBS}" ${MAKEFLAGS_VERBOSE:-}
    fi
    ${CP_OR_MV} ./*/*-*-*/*.ofx.bundle "$TMP_BINARIES_PATH/OFX/Plugins/"
    set +x
    maybe_cleanup_binary_files

    cd "$TMP_PATH"
    #rm -rf openfx-gmic || true
    echo "Info: build openfx-gmic using make... done!"
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
