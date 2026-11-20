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
#    libdvdcss, libmpeg2, liba52, libfaad, libdca, xavs, xavs2, davs2, libsmbclient, libzvbi
#    (labelled GPL-2+ by MacPorts), Ghostscript (libgs, AGPL), fdk-aac (nonfree). A library that is
#    linked but not bundled is reported too: that is how a COMMERCIAL IO.ofx failed to load on
#    Windows (libheif linked x264, which only GPL builds bundle, #1177);
# 2. the FFmpeg libraries it contains must be LGPL: the "libavcodec license: ..." strings, which
#    avcodec_license() and the other *_license() functions (and "ffmpeg -L") return, must all
#    be "LGPL version ...", and FFmpeg's configuration string must have neither --enable-gpl
#    nor --enable-nonfree;
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

# is_gpl_lib <file name>: GPL-only libraries (not LGPL, not GPL with a linking exception).
is_gpl_lib() {
    case "$1" in
        libx264[-_.]*|libx264|x264-*|libx265[-_.]*|libx265|xvidcore*|libxvidcore*) return 0;;
        libpostproc[-_.]*|postproc-*|libvidstab[-_.]*|librubberband[-_.]*) return 0;;
        libfftw3*|fftw3*|libpoppler*|libsox[-_.]*|libsox|libmad[-_.]*|libmad) return 0;;
        libgsl[-_.]*|libgslcblas*|libreadline*|libgdbm*|libfrei0r*|libcdio*|libdvdread*|libdvdnav*) return 0;;
        libdvdcss*|libmpeg2[-_.]*|libmpeg2|libmpeg2convert*|liba52[-_.]*|liba52|libfaad*) return 0;;
        libdca[-_.]*|libdca|libxavs*|libdavs2*|libsmbclient*|libzvbi*) return 0;;
        libgs[-_.]*|libgs|gsdll*|libfdk-aac*) return 0;;
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

# linked_libs <file> <type>: the names of the libraries the file links
linked_libs() {
    case "$2" in
        macho)
            # first line: the file name (and, for a library, its own install name follows)
            otool -L "$1" 2>/dev/null | sed -n -e '2,$s/^[[:space:]]*\([^ ]*\) (.*/\1/p' | sed -e 's@.*/@@'
            ;;
        elf)
            "$OBJDUMP" -p "$1" 2>/dev/null | awk '$1 == "NEEDED" {print $2}'
            ;;
        pe)
            "$OBJDUMP" -p "$1" 2>/dev/null | sed -n -e 's/^[[:space:]]*DLL Name: *//p' | tr -d '\r'
            ;;
    esac
}

check_file() {
    local f="$1" t base lib s
    t="$(file_type "$f")"
    [ -n "$t" ] || return 0
    nfiles=$((nfiles + 1))
    base="$(basename "$f")"
    if is_gpl_lib "$base"; then
        error "$f is a GPL library"
    fi
    while read -r lib; do
        # (otool -L also lists a library's own install name)
        [ -n "$lib" ] && [ "$lib" != "$base" ] || continue
        if is_gpl_lib "$lib"; then
            error "$f links $lib"
        fi
    done < <(linked_libs "$f" "$t" | sort -u)
    # FFmpeg: e.g. "libavcodec license: GPL version 2 or later" (LICENSE_PREFIX FFMPEG_LICENSE)
    while read -r s; do
        [ -n "$s" ] || continue
        case "$s" in
            *" license: LGPL version "*) echo "Info: $base: $s";;
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
