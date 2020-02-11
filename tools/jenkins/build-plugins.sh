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

# Enable to use cmake to build instead of regular Makefiles. Untested for a while
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
    export BOOST_ROOT="$SDK_HOME"
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
    export BOOST_ROOT="$SDK_HOME"
    export OPENJPEG_HOME="$SDK_HOME"
    export THIRD_PARTY_TOOLS_HOME="$SDK_HOME"
    export GIT_SSL_NO_VERIFY="true"
    export C_INCLUDE_PATH="${SDK_HOME}/gcc/include:${SDK_HOME}/include:${FFMPEG_PATH}/include:$LIBRAW_PATH/include"
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
    LLVM_LIB="$(${LLVM_PATH}/bin/llvm-config --ldflags --libs engine mcjit mcdisassembler ${llvmsystemlibs}| tr '\n' ' ')"
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
        env CXX="$CXX" LDFLAGS="$CMAKE_LDFLAGS" cmake .. ${MSYS:-} -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DBITS="${BITS}" -DUSE_OSMESA=1 -DOSMESA_INCLUDES="${OSMESA_PATH}/include" -DOSMESA_LIBRARIES="-L${OSMESA_PATH}/lib -L${LLVM_PATH}/lib ${GLULIB} ${MESALIB} $LLVM_LIB" ${UNIVERSAL:-}
        make -j"${MKJOBS}"
        make install
        echo "Info: build openfx-misc using cmake... done!"
    else
        echo "Info: build openfx-misc using make..."
        #set -x
        #make -C Shadertoy V=1 CXXFLAGS_MESA="-DHAVE_OSMESA" OSMESA_PATH="${OSMESA_PATH}" LDFLAGS_MESA="-L${OSMESA_PATH}/lib -L${LLVM_PATH}/lib ${GLULIB} ${MESALIB} $LLVM_LIB" CXX="$CXX" CONFIG="${COMPILE_TYPE}" OPTFLAG="${OPTFLAG}" LDFLAGS_ADD="${BUILDID:-} ${EXTRA_LDFLAGS_OFXMISC:-}" -j"${MKJOBS}" BITS="${BITS}" HAVE_CIMG=0
        #set +x
        # first, build everything except CImg (including OpenGL plugins)

        echo "Info: make all except CImg."

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

        echo "Info: make CImg."

        # extract CImg.h
        make -C CImg CImg.h
        
        # build CImg.ofx

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
        cp -a ./*/*-*-*/*.ofx.bundle "$TMP_BINARIES_PATH/OFX/Plugins/"
        echo "Info: build openfx-misc using make... done!"
    fi
    cd "$TMP_PATH"
    #rm -rf openfx-misc || true
fi

# IO
if [ "$BUILD_IO" = "1" ] && [ -d "$TMP_PATH/openfx-io" ]; then

    printStatusMessage "Building openfx-io..."
    cd "$TMP_PATH/openfx-io"

    #Always bump git commit, it is only used to version-stamp binaries

    if [ "$CMAKE_PLUGINS" = "1" ] && [ -f CMakeLists.txt ] && [ -d openfx/cmake ]; then
        echo "Info: build openfx-io using cmake..."
        # Use CMake if available
        mkdir build
        cd build
        CMAKE_LDFLAGS=""
        if [ "$PKGOS" = "OSX" ]; then
            UNIVERSAL='-DCMAKE_OSX_ARCHITECTURES="x86_64;i386"'
        elif [ "$PKGOS" = "Windows" ]; then
            MSYS='-G"MSYS Makefiles"'
            CMAKE_LDFLAGS="-static -Wl,--build-id"
        fi
        IO_DEPENDS_PATH=""
        IO_OIIO_LIB=""
        IO_SEEXPR_LIB=""
        if [ "$PKGOS" = "OSX" ]; then
            IO_DEPENDS_PATH="${MACPORTS}"
            IO_OIIO_LIB="lib/libOpenImageIO.dylib"
            IO_SEEXPR_LIB="lib/libSeExpr.dylib"
        else
            IO_DEPENDS_PATH="${SDK_HOME}"
            IO_OIIO_LIB="lib/libOpenImageIO.so"
            IO_SEEXPR_LIB="lib/libSeExpr.so"
            if [ "$PKGOS" = "Windows" ]; then
                 IO_OIIO_LIB="bin/libOpenImageIO.dll"
                 IO_SEEXPR_LIB="bin/libSeExpr.dll"
            fi
        fi
        env CXX="$CXX" LDFLAGS="$CMAKE_LDFLAGS" cmake .. ${MSYS:-} -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DBITS="${BITS}" -DOpenImageIO_INCLUDE_DIR="${IO_DEPENDS_PATH}/include" -DOpenImageIO_LIBRARY="${IO_DEPENDS_PATH}/${IO_OIIO_LIB}" -DSEEXPR_INCLUDE_DIRS="${IO_DEPENDS_PATH}/include" -DSEEXPR_LIBRARIES="${IO_DEPENDS_PATH}/${IO_SEEXPR_LIB}" ${UNIVERSAL:-}
        make -j"${MKJOBS}"
        make install
        echo "Info: build openfx-io using cmake... done!"
    else
        echo "Info: build openfx-io using make..."
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
        cp -a ./*/*-*-*/*.ofx.bundle "$TMP_BINARIES_PATH/OFX/Plugins/"
        echo "Info: build openfx-io using make... done!"
    fi
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

    pkg-config sox && ARENA_AUDIO="-DAUDIO=ON"
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
        make -j"${MKJOBS}" ${MAKEFLAGS_VERBOSE:-} ${ARENA_AUDIO:-}

    cp -a ./*/*-*-*/*.ofx.bundle "$TMP_BINARIES_PATH/OFX/Plugins/"
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

    echo "Info: build openfx-gmic using make..."

    # build GMIC.ofx
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
    cp -a ./*/*-*-*/*.ofx.bundle "$TMP_BINARIES_PATH/OFX/Plugins/"
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
