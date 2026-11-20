#!/bin/bash

# List the DLLs that the Windows installer bundles, and write them to dllVersions.sh, which
# build-Windows-installer.sh sources. Also writes dllVersions.txt, a readable report (one line per
# DLL: where it goes and where it comes from), for comparing two installers.
#
# The DLLs are not listed by hand: MSYS2 is a rolling distribution, and every library update
# renames some DLLs (libcrypto-3 -> -4, libpoppler-117 -> -165...), so a hand-written list was
# always stale (#1133). They are the dependency closure of the binaries that the installer ships,
# read from their import tables with ntldd:
# - Natron's executables (Natron, NatronRenderer, natron-python, Tests...) and the OFX plug-ins,
#   from $TMP_BINARIES_PATH (built by build-natron.sh and build-plugins.sh);
# - the command-line tools that the installer ships (NATRON_TOOLS below: OIIO, OpenEXR, libtiff
#   and FFmpeg tools);
# - the modules that are loaded at run time, so not in any import table: the Qt plug-ins, the Qt
#   DLLs (all of them are shipped, for PySide), the Python extension modules and PySide/Shiboken.
# A DLL is bundled when a directory of DLL_SEARCH_DIRS has it: anything else (Windows' own DLLs) is
# left to the system. All bundled DLLs end up in one directory (bin/), so there is one DLL per
# name: the one of the first directory of DLL_SEARCH_DIRS that has it, which is also the one that
# Windows loads at run time (FFmpeg and LibRaw of the licence first, see common.sh).
#
# The runtime DLLs of the toolchain come from the import tables too (UCRT64, gcc: libstdc++,
# libgcc_s, libgomp, libwinpthread; CLANG64/CLANGARM64: libc++, libunwind, libomp,
# libwinpthread), and are checked at the end.
#
# Most DLLs go to bin/, next to Natron.exe, which Windows always searches. The ones that only the
# Arena or G'MIC plug-in uses (*_PRIVATE_DLLS) go into the plug-in bundle, as before, so that the
# optional installer component of the plug-in carries them: build-Windows-installer.sh embeds a
# manifest that lists them into the plug-in.
#
# Environment:
#   MINGW_PREFIX       MSYS2 prefix (/ucrt64, /clangarm64...); SDK_HOME is accepted too
#   TMP_BINARIES_PATH  Natron's and the plug-ins' binaries (bin/*.exe, OFX/Plugins/*.ofx.bundle)
#   QT_VERSION_MAJOR, PYVER, NATRON_LICENSE
#   DLL_SEARCH_DIRS    the directories of the DLLs to bundle, in priority order, separated by ':'
#                      (default: FFmpeg's and LibRaw's of the licence, COMMERCIAL: libheif-lgpl's
#                      and libtiff-lgpl's, then $MINGW_PREFIX/bin)
#   FFMPEG_PATH        prefix of the FFmpeg to bundle (default: from NATRON_LICENSE), for ffmpeg.exe
#   NTLDD              the ntldd program (default: ntldd, from ${MINGW_PACKAGE_PREFIX}-ntldd)

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

OUTPUT_FILE_NAME="dllVersions.sh"
REPORT_FILE_NAME="dllVersions.txt"

SDK_HOME="${MINGW_PREFIX:-${SDK_HOME:-}}"
if [ -z "$SDK_HOME" ] || [ ! -d "$SDK_HOME/bin" ]; then
    echo "Error: MINGW_PREFIX must be the MSYS2 prefix (e.g. /ucrt64): run this in an MSYS2 UCRT64, CLANG64 or CLANGARM64 shell"
    exit 1
fi
if [ -z "${TMP_BINARIES_PATH:-}" ] || [ ! -d "$TMP_BINARIES_PATH/bin" ]; then
    echo "Error: TMP_BINARIES_PATH must contain the Natron binaries (bin/Natron.exe...)"
    exit 1
fi
if [ -z "${PYVER:-}" ]; then
    echo "Error: PYVER must be set to the Python version (e.g. 3.14)"
    exit 1
fi
if [ -z "${QT_VERSION_MAJOR:-}" ]; then
    echo "Error: QT_VERSION_MAJOR must be set (5 or 6)"
    exit 1
fi
if [ "${NATRON_LICENSE:-}" != "GPL" ] && [ "${NATRON_LICENSE:-}" != "COMMERCIAL" ]; then
    echo "Error: NATRON_LICENSE must be set to GPL or COMMERCIAL"
    exit 1
fi
NTLDD="${NTLDD:-ntldd}"
if ! command -v "$NTLDD" > /dev/null; then
    echo "Error: $NTLDD not found: install ${MINGW_PACKAGE_PREFIX:-mingw-w64-<env>}-ntldd"
    exit 1
fi

if [ -z "${FFMPEG_PATH:-}" ]; then
    if [ "$NATRON_LICENSE" = "GPL" ]; then
        FFMPEG_PATH="$SDK_HOME/ffmpeg-gpl2"
    else
        FFMPEG_PATH="$SDK_HOME/ffmpeg-lgpl"
    fi
    echo "FFMPEG_PATH not set, assuming $FFMPEG_PATH"
fi
if [ -z "${DLL_SEARCH_DIRS:-}" ]; then
    if [ "$NATRON_LICENSE" = "GPL" ]; then
        DLL_SEARCH_DIRS="$FFMPEG_PATH/bin:$SDK_HOME/libraw-gpl2/bin:$SDK_HOME/bin"
    else
        DLL_SEARCH_DIRS="$FFMPEG_PATH/bin:$SDK_HOME/libraw-lgpl/bin:$SDK_HOME/libheif-lgpl/bin:$SDK_HOME/libtiff-lgpl/bin:$SDK_HOME/bin"
    fi
    echo "DLL_SEARCH_DIRS not set, assuming $DLL_SEARCH_DIRS"
fi
# Split DLL_SEARCH_DIRS on ':', but keep Windows drive letters ("C:/msys64/ucrt64/bin"). Empty
# entries (e.g. from an empty LIBHEIF_PATH) are skipped: an empty prefix must never turn into /bin,
# which is MSYS's /usr/bin. A missing directory is an error: the licence-specific ones (FFmpeg,
# LibRaw, libheif, libtiff) must be there, or the DLL of the wrong licence would be bundled from
# $MINGW_PREFIX/bin (e.g. MSYS2's LibRaw, without the GPL demosaic packs, in a GPL build).
SEARCH_DIRS=()
IFS=: read -r -a _dirs <<< "$DLL_SEARCH_DIRS"
_i=0
while [ "$_i" -lt "${#_dirs[@]}" ]; do
    d="${_dirs[$_i]}"
    _i=$((_i + 1))
    if [[ "$d" =~ ^[A-Za-z]$ ]] && [ "$_i" -lt "${#_dirs[@]}" ] && [[ "${_dirs[$_i]}" == [/\\]* ]]; then
        d="$d:${_dirs[$_i]}"
        _i=$((_i + 1))
    fi
    if [ -z "$d" ]; then
        continue
    fi
    case "$d" in
        /bin|/bin/|/usr/bin|/usr/bin/)
            echo "Error: DLL_SEARCH_DIRS contains $d, the MSYS runtime (an empty prefix?): $DLL_SEARCH_DIRS"
            exit 1
            ;;
    esac
    if [ ! -d "$d" ]; then
        echo "Error: the DLL search directory $d does not exist (DLL_SEARCH_DIRS=$DLL_SEARCH_DIRS): install the package that provides it"
        exit 1
    fi
    SEARCH_DIRS+=("$d")
done
unset _dirs _i
if [ "${#SEARCH_DIRS[@]}" = 0 ]; then
    echo "Error: DLL_SEARCH_DIRS is empty"
    exit 1
fi

# COMMERCIAL builds: MSYS2's libheif links the GPL x264 and x265 encoders, and MSYS2's libtiff
# links JBIG-KIT (GPL), so the DLLs of natron_libheif-lgpl and natron_libtiff-lgpl (#1449, #1177),
# whose bin/ are in DLL_SEARCH_DIRS before $MINGW_PREFIX/bin, replace them: same DLL names
# (libheif.dll, libtiff-6.dll), without the GPL parts. natron_openimageio and the other DLLs are
# built against MSYS2's rolling libheif and libtiff, while the -lgpl packages are pinned: the DLL
# that replaces MSYS2's must be the same version (same ABI). (The GPL check below catches a
# libheif.dll or libtiff-6.dll that still links x265 or libjbig, not an ABI mismatch.)
if [ "$NATRON_LICENSE" = "COMMERCIAL" ] && command -v pacman > /dev/null; then
    if [ -z "${MINGW_PACKAGE_PREFIX:-}" ]; then
        echo "Error: MINGW_PACKAGE_PREFIX is not set (run this in an MSYS2 UCRT64, CLANG64 or CLANGARM64 shell)"
        exit 1
    fi
    for lib in libheif libtiff; do
        v_msys2="$(pacman -Q "${MINGW_PACKAGE_PREFIX}-${lib}" 2>/dev/null | awk '{print $2}')"
        v_lgpl="$(pacman -Q "${MINGW_PACKAGE_PREFIX}-natron_${lib}-lgpl" 2>/dev/null | awk '{print $2}')"
        # (the versions without the pkgrel)
        if [ -z "$v_lgpl" ] || [ "${v_msys2%-*}" != "${v_lgpl%-*}" ]; then
            echo "Error: ${MINGW_PACKAGE_PREFIX}-natron_${lib}-lgpl (${v_lgpl:-not installed}) must have the version of MSYS2's ${MINGW_PACKAGE_PREFIX}-${lib} (${v_msys2:-not installed}), which natron_openimageio links: update tools/MINGW-packages/mingw-w64-${lib}-lgpl"
            exit 1
        fi
    done
    unset lib v_msys2 v_lgpl
fi

# Windows' own DLLs are the ones that ntldd finds under the Windows directory (cygpath -W), and the
# API sets (api-ms-win-*, ext-ms-*). A DLL that ntldd finds anywhere else, but not in the search
# directories, would be missing on the users' machines: that is an error.
if [ -n "${WINDOWS_DIR:-}" ]; then
    : # (set by the caller, e.g. for a test outside Windows)
elif command -v cygpath > /dev/null; then
    WINDOWS_DIR="$(cygpath -m -W)"
else
    WINDOWS_DIR="C:/Windows"
fi
WINDOWS_DIR="${WINDOWS_DIR//\\//}"
WINDOWS_DIR="${WINDOWS_DIR%/}"

# Never bundled, even if an MSYS2 package installed them in a search directory: Mesa's
# opengl32.dll would replace the OpenGL driver of the system (the software renderer is an optional
# installer component, see build-Windows-installer.sh), and the Vulkan loader and the Direct3D
# shader compiler are always the system's.
NEVER_BUNDLED="opengl32.dll glu32.dll libgallium_wgl.dll vulkan-1.dll d3dcompiler_47.dll"

# The DLLs that only the Arena or the G'MIC plug-in uses, and that go into its bundle (shell
# patterns, lower case), as in the hand-written lists of Natron 2.5. Whatever else a plug-in needs
# goes to bin/, as do these if anything in bin/ needs them too.
# shellcheck disable=SC2034 # (used through a nameref, plugin_dlls)
ARENA_PRIVATE_DLLS=('libcdr-*.dll' 'libpoppler-*.dll' 'librevenge-*.dll' 'libzip*.dll' 'libmad-*.dll' 'libsox-*.dll')
# shellcheck disable=SC2034 # (used through a nameref, plugin_dlls)
GMIC_PRIVATE_DLLS=('libcurl-*.dll')

# The command-line tools that the installer ships in bin/ (build-Windows-installer.sh copies
# NATRON_TOOLS).
NATRON_TOOLS=()
for t in "$SDK_HOME"/bin/{iconvert,idiff,igrep,iinfo,exrheader,tiffinfo}.exe "$FFMPEG_PATH"/bin/{ffmpeg,ffprobe}.exe; do
    if [ ! -f "$t" ]; then
        echo "Error: $t not found"
        exit 1
    fi
    NATRON_TOOLS+=("$t")
done

if [ "$QT_VERSION_MAJOR" = 5 ]; then
    # PySide2 goes with Qt5, PySide6 with Qt6
    PYSIDE_MAJOR=2
else
    PYSIDE_MAJOR="$QT_VERSION_MAJOR"
fi
PY_LIBDIR="$SDK_HOME/lib/python${PYVER}"
QT_PLUGINS_DIR="$SDK_HOME/share/qt${QT_VERSION_MAJOR}/plugins"

### Dependency graph

declare -A DLL_PATH=()   # lower-case DLL name -> its file in SEARCH_DIRS (the first directory wins)
declare -A DEPS=()       # file -> its bundled direct dependencies (files), one per line
declare -A MISSING=()    # file -> the DLLs it needs that nothing provides (names, one per line)
declare -A FOREIGN=()    # file -> the DLLs it needs that ntldd found outside the search directories

# Index the DLLs of the search directories by lower-case name (Windows file names are
# case-insensitive, and import tables don't always use the case of the file), keeping the case of
# the file, which is the name it gets in the installer.
shopt -s nullglob nocaseglob
for d in "${SEARCH_DIRS[@]}"; do
    for f in "$d"/*.dll; do
        name="$(basename "$f")"
        name="${name,,}"
        case " $NEVER_BUNDLED " in
            *" $name "*) continue ;;
        esac
        if [ -z "${DLL_PATH[$name]+x}" ]; then
            DLL_PATH[$name]="$f"
        fi
    done
done
shopt -u nullglob nocaseglob

# COMMERCIAL builds: libheif.dll and libtiff-6.dll must be the ones of natron_libheif-lgpl and
# natron_libtiff-lgpl (checked above), not MSYS2's.
if [ "$NATRON_LICENSE" = "COMMERCIAL" ]; then
    for name in libheif.dll libtiff-6.dll; do
        lib="${name%%[-.]*}" # libheif, libtiff
        f="${DLL_PATH[$name]:-}"
        if [ -n "$f" ] && [[ "${f,,}" != */"$lib"-lgpl/bin/* ]]; then
            echo "Error: COMMERCIAL build, but $name would come from $f, not from natron_${lib}-lgpl: put $SDK_HOME/${lib}-lgpl/bin before $SDK_HOME/bin in DLL_SEARCH_DIRS"
            exit 1
        fi
    done
    unset name lib f
fi

# Set REPLY to the file of the DLL named $1 (lower case) in the search directories, or "" (a
# system DLL, or a missing one).
dll_path() {
    REPLY="${DLL_PATH[$1]:-}"
}

TMPDIRS=()
cleanup() {
    if [ "${#TMPDIRS[@]}" -gt 0 ]; then
        rm -rf "${TMPDIRS[@]}"
    fi
}
trap cleanup EXIT

# Fill DEPS[$1], MISSING[$1] and FOREIGN[$1] for the PE file $1.
# ntldd prints one line per imported DLL:
#   "	libfoo-1.dll => C:\msys64\ucrt64\bin\libfoo-1.dll (0x0000000012340000)"
#   "	api-ms-win-crt-runtime-l1-1-0.dll => not found"
# (Not verified yet: whether ntldd lists the delay-load imports too. If it does, an optional DLL
# that is absent and delay-loaded would become an Error; compare with objdump -p before a tag.)
# A DLL whose name is in the search directories is bundled, wherever ntldd found it (bin/ will
# have all the bundled ones). Otherwise it must be a Windows DLL (WINDOWS_DIR, or an API set), or a
# DLL next to the file in a directory that the installer copies whole and where Windows also finds
# it at run time (shipped_whole): only the Python extension directories, since Python (3.8 and
# later) loads extension modules with LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR. Not the Qt plug-in
# directories: Qt loads plug-ins with plain LoadLibrary, which looks for their dependencies in bin/,
# not next to the plug-in. Not $TMP_BINARIES_PATH/bin: only its .exe files are copied. (The .ofx
# files are scanned from a temporary copy, see scan_deps.)
shipped_whole() { # shipped_whole <directory>
    case "$1" in
        "$PY_LIBDIR"/lib-dynload|"$PY_LIBDIR"/site-packages/PySide*|"$PY_LIBDIR"/site-packages/shiboken*) return 0 ;;
    esac
    return 1
}

scan_deps() {
    local f="$1" target="$1" tmpdir="" out line name path dir deps="" missing="" foreign=""
    if [ -n "${DEPS[$f]+x}" ]; then
        return 0
    fi
    case "${f,,}" in
        *.dll|*.exe|*.pyd) ;;
        *)
            # e.g. a .ofx: give ntldd a name it recognizes as a module. ntldd then searches the
            # temporary directory, not the plug-in bundle: a DLL shipped inside the bundle at build
            # time (none is today: the private DLLs are added by build-Windows-installer.sh) would
            # be reported "not found", which is an Error for plug-ins (check_missing). To ship one,
            # copy the bundle's DLLs next to the copy, or accept MISSING names that the bundle has.
            tmpdir="$(mktemp -d)"
            TMPDIRS+=("$tmpdir")
            target="$tmpdir/$(basename "$f").dll"
            cp "$f" "$target"
            ;;
    esac
    dir="$(dirname "$f")"
    if ! shipped_whole "$dir"; then
        dir=""
    fi
    if command -v cygpath > /dev/null; then
        target="$(cygpath -m "$target")"
        if [ -n "$dir" ]; then
            dir="$(cygpath -m "$dir")"
        fi
    fi
    if ! out="$("$NTLDD" "$target" 2>&1)"; then
        echo "Error: $NTLDD failed on $f:"
        echo "$out"
        exit 1
    fi
    if [ -n "$tmpdir" ]; then
        rm -rf "$tmpdir"
    fi
    while IFS= read -r line; do
        line="${line%$'\r'}"
        case "$line" in
            *" => "*) ;;
            *) continue ;;
        esac
        name="${line%% => *}"
        name="${name#"${name%%[![:space:]]*}"}" # strip the leading blanks
        name="${name,,}"
        dll_path "$name"
        if [ -n "$REPLY" ]; then
            deps="$deps$REPLY"$'\n'
            continue
        fi
        case "$name" in
            api-ms-win-*|ext-ms-*) continue ;; # API sets, resolved by Windows
        esac
        path="${line#* => }"
        if [ "$path" = "not found" ]; then
            missing="$missing$name"$'\n'
            continue
        fi
        path="${path% (0x*}"
        path="${path//\\//}"
        case "${path,,}" in
            "${WINDOWS_DIR,,}"/*) ;; # a Windows DLL
            *)
                # next to the file, in a directory that is copied whole
                if [ -z "$dir" ] || [[ "${path,,}" != "${dir,,}"/* ]]; then
                    foreign="$foreign$name ($path)"$'\n'
                fi
                ;;
        esac
    done <<< "$out"
    DEPS[$f]="$deps"
    MISSING[$f]="$missing"
    FOREIGN[$f]="$foreign"
}

# CLOSURE[file]=1 for every bundled DLL that the files given as arguments need, directly or not
# (not the arguments themselves). Accumulates over calls, until reset with CLOSURE=(). VISITED
# lists the files of the last call (the arguments and their dependencies), for check_missing.
declare -A CLOSURE=()
VISITED=()
closure() {
    local -a queue=("$@")
    local i=0 f d
    while [ "$i" -lt "${#queue[@]}" ]; do
        f="${queue[$i]}"
        i=$((i + 1))
        scan_deps "$f"
        while IFS= read -r d; do
            if [ -n "$d" ] && [ -z "${CLOSURE[$d]+x}" ]; then
                CLOSURE[$d]=1
                queue+=("$d")
            fi
        done <<< "${DEPS[$f]}"
    done
    VISITED=("${queue[@]}")
}

# Report the DLLs that the files of the last closure need but that would not be bundled: the ones
# that ntldd found outside the search directories and the Windows directory are always an error.
# The ones that nothing provides are an error for programs and plug-ins, and a warning for the
# modules loaded at run time.
check_missing() {
    local level="$1" f errors=0
    for f in "${VISITED[@]}"; do
        if [ -n "${FOREIGN[$f]}" ]; then
            echo "Error: $f needs DLLs that are neither in $DLL_SEARCH_DIRS nor in $WINDOWS_DIR (add their directory to DLL_SEARCH_DIRS):"
            printf '%s' "${FOREIGN[$f]}" | sed -e 's/^/    /'
            errors=1
        fi
        if [ -n "${MISSING[$f]}" ]; then
            if [ "$level" = "Error" ]; then
                echo "Error: $f needs DLLs that are nowhere (not found by ntldd): $(printf '%s' "${MISSING[$f]}" | tr '\n' ' ')"
                errors=1
            else
                # (these modules won't load, as before this script)
                echo "Warning: $(basename "$f") needs DLLs that are nowhere: $(printf '%s' "${MISSING[$f]}" | tr '\n' ' ')"
            fi
        fi
    done
    if [ "$errors" = 1 ]; then
        exit 1
    fi
    return 0
}

### Roots

shopt -s nullglob nocaseglob

# Natron's executables and the plug-ins that use only bin/ (all but Arena and G'MIC)
if [ ! -f "$TMP_BINARIES_PATH/bin/Natron.exe" ] || [ ! -f "$TMP_BINARIES_PATH/bin/NatronRenderer.exe" ]; then
    echo "Error: Natron.exe or NatronRenderer.exe is missing from $TMP_BINARIES_PATH/bin"
    exit 1
fi
NATRON_ROOTS=("$TMP_BINARIES_PATH"/bin/*.exe "${NATRON_TOOLS[@]}")
ARENA_OFX=""
GMIC_OFX=""
for ofx in "$TMP_BINARIES_PATH"/OFX/Plugins/*.ofx.bundle/Contents/*/*.ofx; do
    case "$(basename "$ofx")" in
        Arena.ofx) ARENA_OFX="$ofx" ;;
        GMIC.ofx) GMIC_OFX="$ofx" ;;
        *) NATRON_ROOTS+=("$ofx") ;;
    esac
done

# Modules loaded at run time (LoadLibrary, Python imports): missing dependencies only warn, since
# a module that can't load (e.g. the PySide module of a Qt module that MSYS2 doesn't install) is
# no worse than before.
# COMMERCIAL builds leave out the Qt modules that are GPL-3 only, and the Qt plug-ins that link
# them (build-Windows-installer.sh removes these plug-ins and the PySide modules). GPL builds keep
# them all.
qt_gpl_root() {
    [ "$NATRON_LICENSE" = "COMMERCIAL" ] || return 1
    local mod
    case "${1#"$QT_PLUGINS_DIR"/}" in
        designer/*|virtualkeyboard/*|assetimporters/*|platforminputcontexts/*virtualkeyboard*) return 0 ;;
    esac
    for mod in charts datavisualization networkauth designer virtualkeyboard quick3d bodymovin; do
        case "$(basename "${1,,}")" in
            qt"${QT_VERSION_MAJOR}${mod}"*.dll|qt"${mod}".*pyd) return 0 ;; # (Qt5Charts.dll, QtCharts.pyd)
        esac
    done
    return 1
}
RUNTIME_ROOTS=()
for m in "$SDK_HOME"/bin/Qt"${QT_VERSION_MAJOR}"*.dll; do
    qt_gpl_root "$m" || RUNTIME_ROOTS+=("$m")
done
for m in "$QT_PLUGINS_DIR"/*/*.dll; do
    case "${m,,}" in
        *mysql*|*psql*) ;; # removed by build-Windows-installer.sh
        *) qt_gpl_root "$m" || RUNTIME_ROOTS+=("$m") ;;
    esac
done
for m in "$PY_LIBDIR"/lib-dynload/*.pyd \
         "$PY_LIBDIR"/site-packages/PySide"${PYSIDE_MAJOR}"/*.pyd \
         "$PY_LIBDIR"/site-packages/shiboken"${PYSIDE_MAJOR}"/*.pyd; do
    case "$(basename "${m,,}")" in
        _tkinter.*) ;; # Tcl/Tk are not shipped
        *) qt_gpl_root "$m" || RUNTIME_ROOTS+=("$m") ;;
    esac
done

shopt -u nullglob nocaseglob

### Closures

echo "*** Scanning the DLL dependencies of ${#NATRON_ROOTS[@]} programs and plug-ins..."
closure "${NATRON_ROOTS[@]}"
check_missing Error
echo "*** Scanning the DLL dependencies of ${#RUNTIME_ROOTS[@]} modules loaded at run time..."
closure "${RUNTIME_ROOTS[@]}"
check_missing Warning
# the Qt DLLs themselves (all of them, for PySide)
for m in "${RUNTIME_ROOTS[@]}"; do
    case "$m" in
        "$SDK_HOME"/bin/*) CLOSURE[$m]=1 ;;
    esac
done
declare -A NATRON_SET=()
for d in "${!CLOSURE[@]}"; do
    NATRON_SET[$d]=1
done

# A plug-in's private DLLs: the ones matching its *_PRIVATE_DLLS patterns that bin/ doesn't have.
# The other DLLs it needs go to bin/.
declare -A ARENA_SET=() GMIC_SET=()
plugin_dlls() { # plugin_dlls <ofx> <name of the array of patterns> <name of the associative array to fill>
    local ofx="$1" d p name private
    local -n patterns_ref="$2" set_ref="$3" # (namerefs: read and fill the caller's arrays)
    if [ -z "$ofx" ]; then
        return 0
    fi
    CLOSURE=()
    closure "$ofx"
    check_missing Error
    for d in "${!CLOSURE[@]}"; do
        if [ -n "${NATRON_SET[$d]+x}" ]; then
            continue
        fi
        name="$(basename "${d,,}")"
        private=0
        for p in "${patterns_ref[@]}"; do
            # shellcheck disable=SC2254 # (p is a pattern)
            case "$name" in
                $p) private=1 ;;
            esac
        done
        if [ "$private" = 1 ]; then
            # shellcheck disable=SC2034 # (set_ref is a nameref)
            set_ref[$d]=1
        else
            NATRON_SET[$d]=1
        fi
    done
}
plugin_dlls "$ARENA_OFX" ARENA_PRIVATE_DLLS ARENA_SET
plugin_dlls "$GMIC_OFX" GMIC_PRIVATE_DLLS GMIC_SET

# A DLL in bin/ must find all its dependencies in bin/: move the ones it needs out of the plug-in
# bundles, as well as the private DLLs that both plug-ins need.
changed=1
while [ "$changed" = 1 ]; do
    changed=0
    for d in "${!ARENA_SET[@]}"; do
        if [ -n "${GMIC_SET[$d]+x}" ]; then
            NATRON_SET[$d]=1
        fi
    done
    for d in "${!NATRON_SET[@]}"; do
        scan_deps "$d"
        while IFS= read -r e; do
            if [ -n "$e" ] && [ -z "${NATRON_SET[$e]+x}" ]; then
                NATRON_SET[$e]=1
                changed=1
            fi
        done <<< "${DEPS[$d]}"
    done
    for d in "${!NATRON_SET[@]}"; do
        unset "ARENA_SET[$d]" "GMIC_SET[$d]"
    done
done
VISITED=("${!NATRON_SET[@]}")
check_missing Warning

### Checks

# The C++ and OpenMP runtimes of the toolchain must be there (they come from the import tables).
has_dll() {
    local name="$1" d
    for d in "${!NATRON_SET[@]}"; do
        if [ "$(basename "${d,,}")" = "$name" ]; then
            return 0
        fi
    done
    return 1
}
case "${MINGW_PACKAGE_PREFIX:-}" in
    *-clang-*)
        # CLANG64, CLANGARM64: LLVM's libc++, libunwind and OpenMP
        RUNTIME_DLLS="libc++.dll libunwind.dll libwinpthread-1.dll"
        OPENMP_DLL="libomp.dll"
        ;;
    *)
        # UCRT64: gcc's libstdc++, libgcc_s and libgomp
        RUNTIME_DLLS="libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll"
        OPENMP_DLL="libgomp-1.dll"
        ;;
esac
for name in $RUNTIME_DLLS; do
    if ! has_dll "$name"; then
        echo "Error: the toolchain runtime $name is not among the bundled DLLs"
        exit 1
    fi
done
if ! has_dll "$OPENMP_DLL"; then
    echo "Warning: the OpenMP runtime $OPENMP_DLL is not among the bundled DLLs (nothing links it)"
fi

# COMMERCIAL builds: no GPL-only library may be bundled (#1302, #1177). The DLL set is the
# closure of what the binaries link, so a GPL library here means that something links it (e.g. an
# FFmpeg or libheif built with x264): fail, and say what needs it. The list of GPL libraries is
# is_gpl_lib of check-commercial-bundle.sh (#1449), which build-Windows-installer.sh also runs on
# the finished bundle (it inspects the binaries' contents too).
if [ "$NATRON_LICENSE" = "COMMERCIAL" ]; then
    CHECK_COMMERCIAL="$(dirname "$0")/check-commercial-bundle.sh"
    if [ ! -f "$CHECK_COMMERCIAL" ] || ! grep -q '^is_gpl_lib() {' "$CHECK_COMMERCIAL"; then
        echo "Error: COMMERCIAL builds need is_gpl_lib from $CHECK_COMMERCIAL (#1449)"
        exit 1
    fi
    # (only the functions is_gpl_lib*, e.g. is_gpl_lib and the is_gpl_lib_nocase that it calls:
    # the script itself checks its arguments and exits)
    eval "$(sed -n -e '/^is_gpl_lib[A-Za-z0-9_]*() {/,/^}/p' "$CHECK_COMMERCIAL")"
    # Self-test: a function that is missing after the eval (e.g. a helper that is_gpl_lib calls)
    # would make every test fail silently inside the && lists below, and every DLL pass.
    if ! is_gpl_lib libx265-215.dll || ! is_gpl_lib qt5charts.dll || is_gpl_lib libpng16-16.dll; then
        echo "Error: is_gpl_lib from $CHECK_COMMERCIAL does not work here (libx265-215.dll and qt5charts.dll must be GPL, libpng16-16.dll not): update the extraction in genDllVersions.sh"
        exit 1
    fi
    gpl_found=0
    for d in "${!NATRON_SET[@]}" "${!ARENA_SET[@]}" "${!GMIC_SET[@]}"; do
        if is_gpl_lib "$(basename "${d,,}")"; then
            gpl_found=1
            echo "Error: COMMERCIAL build, but the GPL library $(basename "$d") would be bundled. It is needed by:"
            for f in "${!DEPS[@]}"; do
                case $'\n'"${DEPS[$f]}" in
                    *$'\n'"$d"$'\n'*) echo "    $f" ;;
                esac
            done
        fi
    done
    if [ "$gpl_found" = 1 ]; then
        exit 1
    fi
fi

### Output

emit_array() { # emit_array <variable name> <name of the associative array>
    local var="$1" d
    local -n out_ref="$2"
    printf '%s=(\n' "$var"
    for d in "${!out_ref[@]}"; do
        printf '    %q\n' "$d"
    done | sort -f
    printf ')\n'
}
{
    echo "#!/bin/bash"
    echo "# Generated by genDllVersions.sh: the DLLs bundled in bin/ (NATRON_DLL) and in the Arena"
    echo "# and G'MIC plug-in bundles (ARENA_DLL, GMIC_DLL), and the tools copied to bin/ (NATRON_TOOLS)."
    emit_array NATRON_DLL NATRON_SET
    emit_array ARENA_DLL ARENA_SET
    emit_array GMIC_DLL GMIC_SET
    printf 'NATRON_TOOLS=(\n'
    printf '    %q\n' "${NATRON_TOOLS[@]}"
    printf ')\n'
} > "$OUTPUT_FILE_NAME"

{
    for d in "${!NATRON_SET[@]}"; do printf 'bin\t%s\t%s\n' "$(basename "$d")" "$d"; done
    for d in "${!ARENA_SET[@]}"; do printf 'Arena.ofx.bundle\t%s\t%s\n' "$(basename "$d")" "$d"; done
    for d in "${!GMIC_SET[@]}"; do printf 'GMIC.ofx.bundle\t%s\t%s\n' "$(basename "$d")" "$d"; done
} | sort -f > "$REPORT_FILE_NAME"

echo "*** Bundled DLLs: ${#NATRON_SET[@]} in bin/, ${#ARENA_SET[@]} in Arena.ofx.bundle, ${#GMIC_SET[@]} in GMIC.ofx.bundle (list in $REPORT_FILE_NAME)"
cat "$REPORT_FILE_NAME"

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
