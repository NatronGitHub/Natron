#!/bin/bash

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

# C++ standard: C++17 with Qt5, C++20 with Qt6 (RB-2.6 policy; RB-2.7: C++20 everywhere).
# CXXSTD only reaches the qmake build of Natron (NATRON_BUILD_SYSTEM=qmake) and the Makefile
# builds of the plug-ins (PLUGINS_BUILD_SYSTEM=make), which compile with this CXX. The CMake
# builds (the default) drop the -std flag from CXX and set the standard themselves: Natron's
# CMakeLists.txt from NATRON_QT6 (CMAKE_CXX_STANDARD, with GNU extensions: -std=gnu++17/20),
# the plug-ins from OFX_CXX_STANDARD (build-plugins.sh, without GNU extensions).
if [ "${QT_VERSION_MAJOR:-5}" -ge 6 ]; then
    CXXSTD="c++20"
else
    CXXSTD="c++17"
fi

# https://stackoverflow.com/a/42232124
if [ "$PKGOS" = "Windows" ]; then
    CXXSTD="gnu${CXXSTD#c}"
fi

if [ "$PKGOS" = "OSX" ]; then
    # Mac compiler
    #
    #
    # 10 -> 10.6
    # 16 -> 10.12
    osxver=$(uname -r)

    # Never use a clang newer than MacPorts' unconditional default compiler for this OS
    # (ports with C++14+ requirements may get a newer one), so that older systems
    # (e.g. macOS 10.15) keep building with clang <= 17 exactly as before.
    darwin_major=${osxver%%.*}
    if [ "$darwin_major" -ge 25 ]; then
        clang_max=22
    elif [ "$darwin_major" -ge 24 ]; then
        clang_max=18
    else
        clang_max=17
    fi

    # use a MacPorts clang (with OpenMP) if one not newer than clang_max is available
    clang_mp_found=0
    for v in 22 21 20 19 18 17 16 15 14 13 12 11 10 9.0 8.0 7.0 6.0 5.0 4.0; do
        if [ "${v%%.*}" -le "$clang_max" ] && command -v "clang-mp-$v" >/dev/null 2>&1; then
            clang_mp_found=1
            break
        fi
    done
    if [ "$clang_mp_found" = 0 ] && compgen -c clang-mp- >/dev/null 2>&1; then
        echo "Warning: only MacPorts clang-mp-N newer than clang-mp-$clang_max found:" \
             "building with Apple clang, without OpenMP. Install clang-$clang_max (see INSTALL_MACOS.md)." >&2
    fi
    if [ "$clang_mp_found" = 1 ]; then
        COMPILER=clang-omp
        if grep -q "configure.optflags.*-Os" /opt/local/libexec/macports/lib/port1.0/portconfigure.tcl; then
            true
            #echo "Warning: clang-3.9.1 is known to generate wrong code with -Os on openexr, please edit /opt/local/libexec/macports/lib/port1.0/portconfigure.tcl and set configure.optflags to -O2"
            #exit 1
        fi
    fi
    # clang path on homebrew (should always be the latest version)
    if command -v /usr/local/opt/llvm@11/bin/clang >/dev/null 2>&1; then
        COMPILER=clang-omp
    fi
    #COMPILER=clang-omp
    COMPILER=${COMPILER:-clang}

    if [ "$COMPILER" != "gcc" -a "$COMPILER" != "clang" -a "$COMPILER" != "clang-omp" ]; then
        echo "Error: COMPILER must be gcc or clang or clang-omp"
        exit 1
    fi
    if [ "$COMPILER" = "clang" ]; then
        case "$macosx" in
            9|10|11|12)
                # GXX should be an openmp-capable compiler (to compile CImg.ofx)

                # older version, using clang-5.0
                CC=clang-mp-5.0
                CXX="clang++-mp-5.0 -std=${CXXSTD}"
                GXX=g++-mp-4.9
                OBJECTIVE_CC=$CC
                OBJECTIVE_CXX=$CXX
                ;;
            *)
                # newer OS X / macOS version link with libc++ and can use the system clang
                CC=clang
                CXX="clang++ -std=${CXXSTD}"
                OBJECTIVE_CC=$CC
                OBJECTIVE_CXX=$CXX
                ;;
        esac
    elif [ "$COMPILER" = "clang-omp" ]; then
        unset CC CXX  # chosen below; don't silently use a compiler from the environment
        # newer version (testing) using clang
        # if a recent clang-mp is available
        if command -v clang-mp-6.0 >/dev/null 2>&1; then
            CC=clang-mp-6.0
            CXX="clang++-mp-6.0 -stdlib=libc++ -std=${CXXSTD}"
        elif command -v clang-mp-5.0 >/dev/null 2>&1; then
            CC=clang-mp-5.0
            CXX="clang++-mp-5.0 -stdlib=libc++ -std=${CXXSTD}"
        elif command -v /usr/local/opt/llvm@11/bin/clang >/dev/null 2>&1; then
            CC=/usr/local/opt/llvm@11/bin/clang
            CXX="/usr/local/opt/llvm@11/bin/clang++ -std=${CXXSTD}"
        fi
        # clang > 7.0 sometimes chokes on building Universal CImg.ofx, probably because of #pragma omp atomic
        #Undefined symbols for architecture i386:
        #"___atomic_load", referenced from:
        #_.omp_outlined..468 in CImgExpression.o
        #_.omp_outlined..608 in CImgExpression.o
        #
        # The newest installed MacPorts clang up to the MacPorts default for the OS
        # is used: clang-22 on macOS 26+ (Darwin >= 25), clang-18 on macOS 15, clang-17 before.
        # Two former reasons to avoid clang >= 17 are resolved (verified on macOS 26):
        # - clang 17/18 with -fopenmp linked @rpath/libc++.1.dylib instead of
        #   /usr/lib/libc++.1.dylib; it now links /usr/lib/libc++.1.dylib.
        # - clang >= 19 emits __kmpc_dispatch_deinit, missing from libomp 11.1.0;
        #   libomp is no longer pinned to 11.1.0 (see tools/MacPorts/lang/libomp).
        # Sanity check on a new OS/compiler:
        #   printf 'int main(){}\n' > t.cpp; clang++-mp-N -stdlib=libc++ -fopenmp t.cpp -o t && otool -L t
        case "$osxver" in
            #9.*|10.*)
            #    true;;
            *)
                if [ "$clang_max" -ge 22 ] && command -v clang-mp-22 >/dev/null 2>&1; then
                    CC=clang-mp-22
                    CXX="clang++-mp-22 -stdlib=libc++ -std=${CXXSTD}"
                elif [ "$clang_max" -ge 21 ] && command -v clang-mp-21 >/dev/null 2>&1; then
                    CC=clang-mp-21
                    CXX="clang++-mp-21 -stdlib=libc++ -std=${CXXSTD}"
                elif [ "$clang_max" -ge 20 ] && command -v clang-mp-20 >/dev/null 2>&1; then
                    CC=clang-mp-20
                    CXX="clang++-mp-20 -stdlib=libc++ -std=${CXXSTD}"
                elif [ "$clang_max" -ge 19 ] && command -v clang-mp-19 >/dev/null 2>&1; then
                    CC=clang-mp-19
                    CXX="clang++-mp-19 -stdlib=libc++ -std=${CXXSTD}"
                elif [ "$clang_max" -ge 18 ] && command -v clang-mp-18 >/dev/null 2>&1; then
                    CC=clang-mp-18
                    CXX="clang++-mp-18 -stdlib=libc++ -std=${CXXSTD}"
                elif command -v clang-mp-17 >/dev/null 2>&1; then
                    CC=clang-mp-17
                    CXX="clang++-mp-17 -stdlib=libc++ -std=${CXXSTD}"
                elif command -v clang-mp-16 >/dev/null 2>&1; then
                    CC=clang-mp-16
                    CXX="clang++-mp-16 -stdlib=libc++ -std=${CXXSTD}"
                elif command -v clang-mp-15 >/dev/null 2>&1; then
                    CC=clang-mp-15
                    CXX="clang++-mp-15 -stdlib=libc++ -std=${CXXSTD}"
                elif command -v clang-mp-14 >/dev/null 2>&1; then
                    CC=clang-mp-14
                    CXX="clang++-mp-14 -stdlib=libc++ -std=${CXXSTD}"
                elif command -v clang-mp-13 >/dev/null 2>&1; then
                    CC=clang-mp-13
                    CXX="clang++-mp-13 -stdlib=libc++ -std=${CXXSTD}"
                elif command -v clang-mp-12 >/dev/null 2>&1; then
                    CC=clang-mp-12
                    CXX="clang++-mp-12 -stdlib=libc++ -std=${CXXSTD}"
                elif command -v clang-mp-11 >/dev/null 2>&1; then
                    CC=clang-mp-11
                    CXX="clang++-mp-11 -stdlib=libc++ -std=${CXXSTD}"
                elif command -v clang-mp-10 >/dev/null 2>&1; then
                    CC=clang-mp-10
                    CXX="clang++-mp-10 -stdlib=libc++ -std=${CXXSTD}"
                elif command -v clang-mp-9.0 >/dev/null 2>&1; then
                    CC=clang-mp-9.0
                    CXX="clang++-mp-9.0 -stdlib=libc++ -std=${CXXSTD}"
                fi
                ;;
        esac
        if [ -z "${CC:-}" ]; then
            echo "Error: no usable MacPorts clang-mp-N (N <= $clang_max) found" >&2
            exit 1
        fi
        case "$osxver" in
        2[123].*)
            # clang-mp can't compile QtMac.mm on Monterey (macOS 12 to 14): use Apple's clang.
            # Full paths: CMake can't find a bare "clang++" for CMAKE_OBJCXX_COMPILER
            OBJECTIVE_CC=/usr/bin/clang
            OBJECTIVE_CXX=/usr/bin/clang++
            ;;
        *)
            OBJECTIVE_CC=$CC
            OBJECTIVE_CXX=$CXX
            ;;
        esac
    else
        #GCC_VERSION=4.8
        GCC_VERSION=4.9
        #GCC_VERSION=5
        #GCC_VERSION=6
        CC=gcc-mp-${GCC_VERSION} 
        CXX="g++-mp-${GCC_VERSION} -std=${CXXSTD}"
        OBJECTIVE_CC=gcc-4.2
        OBJECTIVE_CXX=g++-4.2
    fi

    # SDK root was moved with Xcode 4.3
    MACOSX_SDK_ROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs
    case "$osxver" in
        10.*) MACOSX_SDK_ROOT=/Developer/SDKs;;
    esac
    # MACOSX_DEPLOYMENT_TARGET is normally set by common.sh (NATRON_MACOS_MIN, 14.0, for release
    # builds, the build Mac's macOS otherwise); the table below is only a fallback for scripts
    # that don't set it.
    case "${MACOSX_DEPLOYMENT_TARGET:+set}:$osxver" in
        set:*)
            ;;
        *:10.*)
            MACOSX_DEPLOYMENT_TARGET=10.6
            ;;
        *:13.*)
            MACOSX_DEPLOYMENT_TARGET=10.9
            ;;
        *:16.*)
            MACOSX_DEPLOYMENT_TARGET=10.12
            ;;
        *:17.*)
            MACOSX_DEPLOYMENT_TARGET=10.13
            ;;
        *:18.*)
            MACOSX_DEPLOYMENT_TARGET=10.14
            ;;
        *:19.*)
            MACOSX_DEPLOYMENT_TARGET=10.15
            ;;
        *:20.*)
            MACOSX_DEPLOYMENT_TARGET=11
            ;;
        *:21.*)
            MACOSX_DEPLOYMENT_TARGET=12
            ;;
        *:22.*)
            MACOSX_DEPLOYMENT_TARGET=13
            ;;
        *:23.*)
            MACOSX_DEPLOYMENT_TARGET=14
            ;;
        *:24.*)
            MACOSX_DEPLOYMENT_TARGET=15
            ;;
   esac
    export MACOSX_DEPLOYMENT_TARGET
    export MACOSX_SYSROOT="${MACOSX_SDK_ROOT}/MacOSX${MACOSX_DEPLOYMENT_TARGET}.sdk"


elif [ "$PKGOS" = "Linux" ]; then
    if [ "${COMPILE_TYPE:-}" = "debug" ]; then
        BF="-g"
    else
        BF="-O2"
    fi

    if [ "$BITS" = "32" ]; then
        BF="$BF -march=i686 -mtune=i686"
    elif [ "$BITS" = "64" ]; then
        BF="$BF -fPIC"
    fi
fi



if [ "$PKGOS" = "Windows" ] && [ "${MINGW_TOOLCHAIN:-gcc}" = "clang" ]; then
    # MSYS2 CLANG64 and CLANGARM64 have no gcc: clang, with LLVM's OpenMP (llvm-openmp), which
    # COMPILER=clang-omp enables for Natron and the plug-ins (common.sh sets MINGW_TOOLCHAIN)
    COMPILER=${COMPILER:-clang-omp}
    CC=${CC:-clang}
    CXX=${CXX:-clang++ -std=${CXXSTD}}
fi

COMPILER=${COMPILER:-gcc}
CC=${CC:-gcc}
CXX=${CXX:-g++ -std=${CXXSTD}}
OBJECTIVE_CC=${OBJECTIVE_CC:-${CC}}
OBJECTIVE_CXX=${OBJECTIVE_CXX:-${CXX}}

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
