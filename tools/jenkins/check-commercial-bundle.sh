#!/bin/bash
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2026 The Natron developers
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

# Fail if a COMMERCIAL (non-GPL) Natron build contains or links GPL code (#1302, #1177).
#
#   check-commercial-bundle.sh <directory or file>...
#
# Checks every executable or shared library (Mach-O, ELF or PE, found by its magic number, so
# that .ofx, .pyd and extension-less executables are included) under the arguments:
# 1. its file name, and the names of the libraries it links (otool -L, or objdump -p), must not
#    be a GPL, AGPL or nonfree library: x264, x265, Xvid, libpostproc, vid.stab, Rubber Band,
#    FFTW, poppler, SoX, libmad, GSL, readline, gdbm, frei0r, libcdio, libdvdread, libdvdnav,
#    libdvdcss, libmpeg2, liba52, libfaad, libdca, xavs, xavs2, davs2, libsmbclient, libzvbi,
#    JBIG-KIT (libjbig, linked by MSYS2's libtiff), libao and libid3tag (linked by SoX) (GPL-2+),
#    Ghostscript (libgs, AGPL), fdk-aac (nonfree), and the Qt modules that are GPL-3 only (Qt
#    Charts, Data Visualization, Network Authorization, Designer, Virtual Keyboard, Quick 3D,
#    Lottie/Bodymovin) and their PySide modules, e.g. QtCharts, Qt5Charts.dll,
#    libQt5Charts.so.5, QtCharts.cpython-311-darwin.so or QtCharts.pyd. A library that is
#    linked but not bundled is reported too: that is how a COMMERCIAL IO.ofx failed to load on
#    Windows (libheif linked x264, which only GPL builds bundle, #1177);
# 2. the FFmpeg libraries it contains must be LGPL: the "libavcodec license: ..." strings, which
#    avcodec_license() and the other *_license() functions (and "ffmpeg -L") return, must all
#    be "LGPL version 2.1 or later" (as the natron_ffmpeg-lgpl package and the ffmpeg-lgpl
#    subport require: --enable-version3 would make it LGPL 3), and FFmpeg's configuration string
#    must have neither --enable-gpl nor --enable-nonfree;
# 3. LibRaw must be built without the GPL demosaic packs: none of the pack's functions
#    (exported class members, so they survive strip) may be present;
# 4. the file must not contain the entry points of x264, x265, Xvid, vid.stab or FFTW, which
#    catch a static or renamed copy of these libraries.
# The file contents are inspected, nothing is executed, so the check also works on a bundle for
# another architecture.
#
# Exit status: 0 if no GPL component was found, 1 otherwise, 2 on a usage error.

set -u

if [ $# -eq 0 ]; then
    echo "Usage: $0 <directory or file>..." >&2
    exit 2
fi

OBJDUMP="${OBJDUMP:-objdump}"
for tool in "$OBJDUMP" od grep find; do
    if ! command -v "$tool" > /dev/null; then
        echo "Error: $tool not found" >&2
        exit 2
    fi
done
if [ "$(uname -s)" = "Darwin" ] && ! command -v otool > /dev/null; then
    echo "Error: otool not found (install the Xcode command line tools)" >&2
    exit 2
fi
nfiles=0
nerrors=0

error() {
    echo "Error: GPL component in a COMMERCIAL build: $*"
    nerrors=$((nerrors + 1))
}

# is_gpl_lib <file name>: GPL-only libraries (not LGPL, not GPL with a linking exception). The
# names are matched case-insensitively (Windows file names, e.g. LIBX264-165.DLL or qt5charts.dll).
is_gpl_lib() {
    local status=1
    if shopt -q nocasematch; then
        is_gpl_lib_nocase "$1" && status=0
    else
        shopt -s nocasematch
        is_gpl_lib_nocase "$1" && status=0
        shopt -u nocasematch
    fi
    return $status
}

# (bash 3.2, /bin/bash on macOS, has no ${1,,}: use with nocasematch set)
is_gpl_lib_nocase() {
    case "$1" in
        libx264[-_.]*|libx264|x264-*|libx265[-_.]*|libx265|xvidcore*|libxvidcore*) return 0;;
        libpostproc[-_.]*|postproc-*|libvidstab[-_.]*|librubberband[-_.]*) return 0;;
        libfftw3*|fftw3*|libpoppler*|libsox[-_.]*|libsox|libmad[-_.]*|libmad) return 0;;
        libgsl[-_.]*|libgslcblas*|libreadline*|libgdbm*|libfrei0r*|libcdio*|libdvdread*|libdvdnav*) return 0;;
        libdvdcss*|libmpeg2[-_.]*|libmpeg2|libmpeg2convert*|liba52[-_.]*|liba52|libfaad*) return 0;;
        libdca[-_.]*|libdca|libxavs*|libdavs2*|libsmbclient*|libzvbi*) return 0;;
        libgs[-_.]*|libgs|gsdll*|libfdk-aac*) return 0;;
        libjbig*|libao[-_.]*|libao|libid3tag*) return 0;;
    esac
    # Qt modules that are GPL-3 only, without a linking exception ("QT_BEGIN_LICENSE:GPL" or
    # "GPL-EXCEPT" in their headers), and their PySide modules: QtCharts (macOS framework, PySide),
    # Qt5Charts.dll, libQt5Charts.so.5, ...
    # (parameter expansion ignores nocasematch: strip the prefixes by length)
    local qtmod="$1"
    case "$qtmod" in
        lib*) qtmod="${qtmod:3}";;
    esac
    case "$qtmod" in
        Qt[0-9]*) qtmod="${qtmod:3}";;
        Qt*) qtmod="${qtmod:2}";;
        *) return 1;;
    esac
    # TODO(Qt6): Qt6 has more GPL-3-only modules, e.g. Graphs and HttpServer (check ShaderTools,
    # QuickTimeline, Quick3DPhysics and the others) before the installers support Qt6
    case "$qtmod" in
        Charts*|DataVisualization*|NetworkAuth*|Designer*|VirtualKeyboard*|Quick3D*|Bodymovin*) return 0;;
    esac
    return 1
}

# file_type <file>: macho, elf, pe, or nothing
file_type() {
    local magic
    magic="$(od -An -tx1 -N4 "$1" 2>/dev/null | tr -d ' \n')"
    case "$magic" in
        feedface|cefaedfe|feedfacf|cffaedfe|cafebabe|bebafeca) echo macho;;
        7f454c46) echo elf;;
        4d5a*) echo pe;;
    esac
}

# linked_libs <file> <type>: the names of the libraries the file links. Fails if otool or objdump
# can't read a Mach-O or PE file (e.g. an objdump without support for its architecture), since
# its dependencies would go unchecked.
linked_libs() {
    local out
    case "$2" in
        macho)
            # first line: the file name (and, for a library, its own install name follows)
            out="$(otool -L "$1" 2>/dev/null)" || return 1
            # otool exits with 0 on an unreadable file, and prints e.g. "<file>: is a truncated
            # object file": every header line ("<file>:", "<file> (architecture arm64):") ends
            # with ":", the libraries are indented
            if [ -z "$out" ] || echo "$out" | grep -q -v -e '^[[:space:]]' -e ':$'; then
                return 1
            fi
            echo "$out" | sed -n -e '2,$s/^[[:space:]]*\([^ ]*\) (.*/\1/p' | sed -e 's@.*/@@'
            ;;
        elf)
            "$OBJDUMP" -p "$1" 2>/dev/null | awk '$1 == "NEEDED" {print $2}'
            ;;
        pe)
            out="$("$OBJDUMP" -p "$1" 2>/dev/null)" || return 1
            echo "$out" | sed -n -e 's/^[[:space:]]*DLL Name: *//p' | tr -d '\r'
            ;;
    esac
}

check_file() {
    local f="$1" t base lib libs s
    t="$(file_type "$f")"
    [ -n "$t" ] || return 0
    nfiles=$((nfiles + 1))
    base="$(basename "$f")"
    if is_gpl_lib "$base"; then
        error "$f is a GPL library"
    fi
    if ! libs="$(linked_libs "$f" "$t")"; then
        error "$f: cannot read the libraries it links (${t}): its dependencies are not checked"
        libs=""
    fi
    while read -r lib; do
        # (otool -L also lists a library's own install name)
        [ -n "$lib" ] && [ "$lib" != "$base" ] || continue
        if is_gpl_lib "$lib"; then
            error "$f links $lib"
        fi
    done < <(echo "$libs" | sort -u)
    # FFmpeg: e.g. "libavcodec license: GPL version 2 or later" (LICENSE_PREFIX FFMPEG_LICENSE)
    while read -r s; do
        [ -n "$s" ] || continue
        case "$s" in
            *" license: LGPL version 2.1 or later") echo "Info: $base: $s";;
            *" license: LGPL version "*) error "$f: $s (expected LGPL version 2.1 or later: FFmpeg configured with --enable-version3?)";;
            *) error "$f: $s";;
        esac
    done < <(LC_ALL=C grep -a -o -E 'lib(av(codec|device|filter|format|util)|sw(resample|scale)|postproc) license: [A-Za-z0-9 .+-]*' "$f" 2>/dev/null | sort -u)
    if LC_ALL=C grep -a -q -E -e '--enable-(gpl|nonfree)([^a-z0-9_-]|$)' "$f" 2>/dev/null \
       && LC_ALL=C grep -a -q -E 'lib(avcodec|avutil|avformat) license: ' "$f" 2>/dev/null; then
        error "$f: FFmpeg configured with --enable-gpl or --enable-nonfree"
    fi
    # LibRaw's GPL2 (AHD-Mod, AFD, VCD, Mixed, LMMSE) and GPL3 (AMaZE) demosaic packs
    s="$(LC_ALL=C grep -a -o -E 'ahd_interpolate_mod|afd_interpolate_pl|vcd_interpolate|lmmse_interpolate|amaze_demosaic' "$f" 2>/dev/null | sort -u | tr '\n' ' ')"
    if [ -n "$s" ]; then
        error "$f: LibRaw with a GPL demosaic pack (${s% })"
    fi
    # entry points of GPL libraries: a static or renamed copy, or a reference to one
    s="$(LC_ALL=C grep -a -o -E 'x264_encoder_open|x265_api_get|xvid_global|xvid_encore|vsMotionDetectInit|fftwf?l?_plan_dft' "$f" 2>/dev/null | sort -u | tr '\n' ' ')"
    if [ -n "$s" ]; then
        error "$f contains or references GPL code (${s% })"
    fi
}

for arg in "$@"; do
    if [ ! -e "$arg" ]; then
        echo "Error: $arg does not exist" >&2
        exit 2
    fi
    while IFS= read -r -d '' f; do
        check_file "$f"
    done < <(find "$arg" -type f -print0)
done

if [ "$nfiles" = 0 ]; then
    echo "Error: no executable or library found in $*" >&2
    exit 2
fi
if [ "$nerrors" != 0 ]; then
    echo "Error: $nerrors GPL component(s) found in $nfiles binaries: this is not a valid COMMERCIAL build."
    exit 1
fi
echo "Info: no GPL component found in $nfiles binaries."
exit 0
