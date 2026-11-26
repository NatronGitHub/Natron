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

    # The system (Xcode or Command Line Tools) clang builds Natron and the plug-ins when it
    # supports $CXXSTD and links with MacPorts' libomp (#1134, decided 2026-11-23): the CMake
    # builds pass it -Xpreprocessor -fopenmp and -lomp (Natron's CMakeLists.txt, OFX_LIBOMP_ROOT
    # in build-plugins.sh). The qmake build and the plug-ins' Makefiles pass -fopenmp, which Apple
    # clang's driver rejects: they keep a MacPorts clang-mp-N, as does NATRON_MACOS_CLANG=macports.
    # No Apple clang bug is known to require a MacPorts clang; add an explicit version check
    # here, with the reason, if one is found.
    use_system_clang=0
    case "${NATRON_MACOS_CLANG:-system}" in
        system)
            if [ "${NATRON_BUILD_SYSTEM:-cmake}" = cmake ] && [ "${PLUGINS_BUILD_SYSTEM:-cmake}" = cmake ]; then
                # probed once per C++ standard: common.sh, which sources this file, is sourced by
                # every build script
                if [ "${NATRON_OMP_PROBE_STD:-}" != "$CXXSTD" ]; then
                    case "$CXXSTD" in
                        c++20) cplusplus=202002L;;
                        *) cplusplus=201703L;;
                    esac
                    system_clang_test=$(mktemp -d)
                    NATRON_OMP_PROBE_OK=0
                    if /usr/bin/clang++ -std="$CXXSTD" -DREQUIRED_CPLUSPLUS="$cplusplus" -Xpreprocessor -fopenmp \
                           -I/opt/local/include/libomp "$(dirname "${BASH_SOURCE[0]}")/omp-probe.cpp" \
                           -o "$system_clang_test/t" -L/opt/local/lib/libomp -lomp >/dev/null 2>&1 &&
                           "$system_clang_test/t"; then
                        NATRON_OMP_PROBE_OK=1
                    else
                        echo "Warning: /usr/bin/clang++ can't build $CXXSTD with OpenMP (MacPorts libomp):" \
                             "using a MacPorts clang-mp-N if there is one" >&2
                    fi
                    rm -rf "$system_clang_test"
                    export NATRON_OMP_PROBE_STD=$CXXSTD NATRON_OMP_PROBE_OK
                fi
                # (the result is kept for the whole build: rerun from a new shell after
                # changing Xcode or libomp)
                use_system_clang=${NATRON_OMP_PROBE_OK:-0}
            fi
            ;;
        macports) ;;
        *)
            echo "Error: NATRON_MACOS_CLANG must be system or macports, not \"$NATRON_MACOS_CLANG\"" >&2
            exit 1
            ;;
    esac

    # use the newest MacPorts clang (with OpenMP) not newer than clang_max, if there is one
    clang_mp_found=0
    clang_mp_version=
    for v in 22 21 20 19 18 17 16 15 14 13 12 11 10 9.0; do
        if [ "${v%%.*}" -le "$clang_max" ] && command -v "clang-mp-$v" >/dev/null 2>&1; then
            clang_mp_found=1
            clang_mp_version=$v
            break
        fi
    done
    if [ "${NATRON_MACOS_CLANG:-system}" = macports ] && [ "$clang_mp_found" = 0 ]; then
        echo "Error: NATRON_MACOS_CLANG=macports, but no MacPorts clang-mp-N (N <= $clang_max) is installed" >&2
        exit 1
    fi
    if [ "$use_system_clang" = 0 ] && [ "$clang_mp_found" = 0 ] && compgen -c clang-mp- >/dev/null 2>&1; then
        echo "Warning: no usable MacPorts clang-mp-N (9 <= N <= $clang_max) found:" \
             "building with Apple clang, without OpenMP. Install clang-$clang_max (see INSTALL_MACOS.md)." >&2
    fi
    if [ "$clang_mp_found" = 1 ]; then
        COMPILER=clang-omp
    fi
    COMPILER=${COMPILER:-clang}

    # No GCC on macOS: Qt and the MacPorts libraries use libc++, and the macOS installer
    # doesn't bundle the GCC runtime (build-OSX-installer.sh)
    if [ "$COMPILER" != "clang" -a "$COMPILER" != "clang-omp" ]; then
        echo "Error: COMPILER must be clang or clang-omp on macOS"
        exit 1
    fi
    if [ "$use_system_clang" = 1 ]; then
        # COMPILER=clang-omp: built with OpenMP, and build-OSX-installer.sh bundles libomp.
        # Full paths: CMake can't find a bare "clang++" for CMAKE_OBJCXX_COMPILER
        COMPILER=clang-omp
        CC=/usr/bin/clang
        CXX="/usr/bin/clang++ -std=${CXXSTD}"
        OBJECTIVE_CC=$CC
        OBJECTIVE_CXX=$CXX
    elif [ "$COMPILER" = "clang" ]; then
        # the system clang, without OpenMP
        CC=clang
        CXX="clang++ -std=${CXXSTD}"
        OBJECTIVE_CC=$CC
        OBJECTIVE_CXX=$CXX
    elif [ "$COMPILER" = "clang-omp" ]; then
        # The newest installed MacPorts clang up to the MacPorts default for the OS
        # (clang_max, found above): clang-22 on macOS 26+ (Darwin >= 25), clang-18 on macOS 15,
        # clang-17 before.
        # Two former reasons to avoid clang >= 17 are resolved (verified on macOS 26):
        # - clang 17/18 with -fopenmp linked @rpath/libc++.1.dylib instead of
        #   /usr/lib/libc++.1.dylib; it now links /usr/lib/libc++.1.dylib.
        # - clang >= 19 emits __kmpc_dispatch_deinit, missing from libomp 11.1.0;
        #   libomp is no longer pinned to 11.1.0 (see tools/MacPorts/lang/libomp).
        # Sanity check on a new OS/compiler:
        #   printf 'int main(){}\n' > t.cpp; clang++-mp-N -stdlib=libc++ -fopenmp t.cpp -o t && otool -L t
        unset CC CXX  # don't silently use a compiler from the environment
        if [ -n "$clang_mp_version" ]; then
            CC=clang-mp-$clang_mp_version
            CXX="clang++-mp-$clang_mp_version -stdlib=libc++ -std=${CXXSTD}"
        fi
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
    fi

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
