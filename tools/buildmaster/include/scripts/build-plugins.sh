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
    # Let us benefit from maximum optimization for release builds,
    # includng non-conformant IEEE floating-point computation.
    OPTFLAG=-Ofast
fi

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
    LLVM_LIB="$(${LLVM_PATH}/bin/llvm-config --ldflags --libs engine mcjit mcdisassembler ${llvmsystemlibs}| tr '\n' ' ')"
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

    if [ "$CMAKE_PLUGINS" = "1" ] && [ -f CMakeLists.txt ] && [ -d openfx/cmake ]; then
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
        rm ../DenoiseSharpen/DenoiseWavelet.cpp || true
        env CXX="$CXX" LDFLAGS="$CMAKE_LDFLAGS" cmake .. ${MSYS:-} -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX="$TMP_BINARIES_PATH" -DBITS="${BITS}" -DUSE_OSMESA=1 -DOSMESA_INCLUDES="${OSMESA_PATH}/include" -DOSMESA_LIBRARIES="-L${OSMESA_PATH}/lib -L${LLVM_PATH}/lib ${GLULIB} ${MESALIB} $LLVM_LIB" ${UNIVERSAL:-}
        make -j"${MKJOBS}"
        make install
    else
        #ls -la  "$TMP_BINARIES_PATH" "$TMP_BINARIES_PATH/OFX/Plugins" "$TMP_BINARIES_PATH/Plugins"
        #set -x
        #make -C Shadertoy $MAKEFLAGS_VERBOSE CXXFLAGS_MESA="-DHAVE_OSMESA" OSMESA_PATH="${OSMESA_PATH}" LDFLAGS_MESA="-L${OSMESA_PATH}/lib -L${LLVM_PATH}/lib ${GLULIB} ${MESALIB} $LLVM_LIB" CXX="$CXX" CONFIG="${BUILD_MODE}" OPTFLAG="${OPTFLAG}" LDFLAGS_ADD="${BUILDID:-} ${EXTRA_LDFLAGS_OFXMISC:-}" -j"${MKJOBS}" BITS="${BITS}" HAVE_CIMG=0
        #set +x
        # first, build everything except CImg (including OpenGL plugins)
        OMP=""
        if [ "$COMPILER" = "gcc" ] || [ "$COMPILER" = "clang-omp" ]; then
            # compile DenoiseSharpen with OpenMP support
            OMP="OPENMP=1"
        fi
        make -j"${MKJOBS}" $MAKEFLAGS_VERBOSE CXXFLAGS_MESA="-DHAVE_OSMESA" OSMESA_PATH="${OSMESA_PATH}" LDFLAGS_MESA="-L${OSMESA_PATH}/lib -L${LLVM_PATH}/lib ${GLULIB} ${MESALIB} $LLVM_LIB" HAVE_CIMG=0 \
             CXX="$CXX" CONFIG="${BUILD_MODE}" BITS="${BITS}" OPTFLAG="${OPTFLAG}" LDFLAGS_ADD="${BUILDID:-} ${EXTRA_LDFLAGS_OFXMISC:-}" ${OMP}

        #ls -la  "$TMP_BINARIES_PATH" "$TMP_BINARIES_PATH/OFX/Plugins" "$TMP_BINARIES_PATH/Plugins"
        # extract CImg.h
        make -C CImg CImg.h
        # build CImg.ofx
        #ls -la  "$TMP_BINARIES_PATH" "$TMP_BINARIES_PATH/OFX/Plugins" "$TMP_BINARIES_PATH/Plugins"
        if [ "$COMPILER" = "gcc" ] || [ "$COMPILER" = "clang-omp" ]; then
            # build CImg with OpenMP support (no OpenGL required)
            make -j"${MKJOBS}" $MAKEFLAGS_VERBOSE -C CImg OPENMP=1 \
                 CXX="$CXX" CONFIG="${BUILD_MODE}" BITS="${BITS}" OPTFLAG="${OPTFLAG}" LDFLAGS_ADD="${BUILDID:-} ${EXTRA_LDFLAGS_OFXMISC:-}"
        elif [ -n "${GXX:-}" ]; then
            # Building with clang, but GCC is available too!
            # libSupport was compiled by clang, now clean it to build it again with gcc
            make $MAKEFLAGS_VERBOSE CONFIG="${BUILD_MODE}" OPTFLAG="${OPTFLAG}" -j"${MKJOBS}" BITS="${BITS}" HAVE_CIMG=0 clean
            # build CImg with OpenMP support, but statically link libgomp (see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=31400)
            make -j"${MKJOBS}" $MAKEFLAGS_VERBOSE -C CImg OPENMP=1 \
                 CXX="$GXX" CONFIG="${BUILD_MODE}" BITS="${BITS}" OPTFLAG="${OPTFLAG}" LDFLAGS_ADD="${BUILDID:-} ${EXTRA_LDFLAGS_OFXMISC:-} -static-libgcc"
        else
            # build CImg without OpenMP
            make -j"${MKJOBS}" $MAKEFLAGS_VERBOSE -C CImg \
                 CXX="$CXX" CONFIG="${BUILD_MODE}" BITS="${BITS}" OPTFLAG="${OPTFLAG}" LDFLAGS_ADD="${BUILDID:-} ${EXTRA_LDFLAGS_OFXMISC:-}"
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

    if [ "$CMAKE_PLUGINS" = "1" ] && [ -f CMakeLists.txt ] && [ -d openfx/cmake ]; then
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
        env CXX="$CXX" LDFLAGS="$CMAKE_LDFLAGS" cmake .. ${MSYS:-} -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX="$TMP_BINARIES_PATH" -DBITS="${BITS}" -DOpenImageIO_INCLUDE_DIR="${IO_DEPENDS_PATH}/include" -DOpenImageIO_LIBRARY="${IO_DEPENDS_PATH}/${IO_OIIO_LIB}" -DSEEXPR_INCLUDE_DIRS="${IO_DEPENDS_PATH}/include" -DSEEXPR_LIBRARIES="${IO_DEPENDS_PATH}/${IO_SEEXPR_LIB}" ${UNIVERSAL:-}
        make -j"${MKJOBS}"
        make install
    else
        make -j"${MKJOBS}" $MAKEFLAGS_VERBOSE OIIO_HOME="$OIIO_PATH" SEEXPR_HOME="${SDK_HOME}" \
             CXX="$CXX" CONFIG="${BUILD_MODE}" BITS="${BITS}" OPTFLAG="${OPTFLAG}" LDFLAGS_ADD="${BUILDID:-}"
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

    if [ "$PKGOS" = "OSX" ]; then
        $GSED -e s/-lgomp// -i.orig Makefile.master
    fi

    #Always bump git commit, it is only used to version-stamp binaries
    ARENA_V=$ARENA_GIT_VERSION
    $GSED -e "s/ARENAPLUG_DEVEL_GIT=.*/ARENAPLUG_DEVEL_GIT=${ARENA_V}/" --in-place "$COMMITS_HASH"

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

    make -j"${MKJOBS}" $MAKEFLAGS_VERBOSE MINGW="${ISWIN:-}" LICENSE="$NATRON_LICENSE" ${ARENA_FLAGS:-} \
         CXX="$CXX" CONFIG="${BUILD_MODE}" BITS="${BITS}" OPTFLAG="${OPTFLAG}" LDFLAGS_ADD="${BUILDID:-}"
    cp -a ./*/*-*-*/*.ofx.bundle "$TMP_BINARIES_PATH/OFX/Plugins/"
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

    if [ "$COMPILER" = "gcc" ] || [ "$COMPILER" = "clang-omp" ]; then
        # build with OpenMP support
        make -j"${GMIC_MKJOBS}" $MAKEFLAGS_VERBOSE OPENMP=1 \
             CXX="$CXX" CONFIG="${BUILD_MODE}" OPTFLAG="${OPTFLAG}" BITS="${BITS}" LDFLAGS_ADD="${BUILDID:-}" CXXFLAGS_EXTRA="$GMIC_CXXFLAGS_EXTRA"
    elif [ -n "${GXX:-}" ]; then
        # Building with clang, but GCC is available too!
        make -j"${GMIC_MKJOBS}" $MAKEFLAGS_VERBOSE OPENMP=1 \
             CXX="$GXX" CONFIG="${BUILD_MODE}" OPTFLAG="${OPTFLAG}" BITS="${BITS}" LDFLAGS_ADD="${BUILDID:-} -static-libgcc" CXXFLAGS_EXTRA="$GMIC_CXXFLAGS_EXTRA"
    else
        # build without OpenMP
        make -j"${GMIC_MKJOBS}" $MAKEFLAGS_VERBOSE \
             CXX="$CXX" CONFIG="${BUILD_MODE}" OPTFLAG="${OPTFLAG}" BITS="${BITS}" LDFLAGS_ADD="${BUILDID:-}" CXXFLAGS_EXTRA="$GMIC_CXXFLAGS_EXTRA"
    fi
    cp -a ./*/*-*-*/*.ofx.bundle "$TMP_BINARIES_PATH/OFX/Plugins/"
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
