#!/bin/bash
# Check that every Mach-O file (executables, dylibs, bundles, static archives) runs on a given
# macOS version: the minimum macOS of each architecture (LC_BUILD_VERSION minos, or the older
# LC_VERSION_MIN_MACOSX) must not be newer than the target.
#
#   check-minos.sh [-t TARGET] [-v] PATH...   PATH: a directory, a file, or a .dmg (mounted read-only)
#   check-minos.sh [-t TARGET] [-v] -l        read the paths to check from stdin, one per line
#
#   -t TARGET   oldest macOS that must be supported (default: $MACOSX_DEPLOYMENT_TARGET, else 14.0)
#   -v          list every Mach-O file with its minos, not only the ones that are too new
#
# Output: one line "minos<TAB>file" per file needing a newer macOS than TARGET, then a summary
# (number of files per minos). Exit status: 0 if all files are OK, 1 if some need a newer macOS,
# 2 on usage errors. Files without any minimum-version load command are counted as "none".
# Used by natron-build.sh (on the DMG) and provision.sh (to find ports built for a newer macOS).
set -euo pipefail

TARGET=${MACOSX_DEPLOYMENT_TARGET:-14.0}
VERBOSE=0
STDIN=0
while getopts "t:vlh" opt; do
    case $opt in
        t) TARGET=$OPTARG ;;
        v) VERBOSE=1 ;;
        l) STDIN=1 ;;
        *) sed -n 2,15p "$0"; exit 2 ;;
    esac
done
shift $((OPTIND - 1))
[ "$STDIN" = 1 ] || [ $# -gt 0 ] || { sed -n 2,15p "$0"; exit 2; }

tmp=$(mktemp -d "${TMPDIR:-/tmp}/check-minos.XXXXXX")
mounts=()
cleanup() {
    local m
    for m in ${mounts[@]+"${mounts[@]}"}; do hdiutil detach -quiet "$m" || hdiutil detach -quiet -force "$m" || true; done
    rm -rf "$tmp"
}
trap cleanup EXIT

# all regular files to look at, NUL-separated
: > "$tmp/files"
add_path() {
    local p=$1 m
    if [ -f "$p" ] && [ "${p%.dmg}" != "$p" ]; then
        # mounted at $tmp/<name>.dmg: the files are reported as <name>.dmg/...
        m=$tmp/$(basename "$p"); mkdir "$m"
        hdiutil attach -quiet -readonly -nobrowse -noautoopen -mountpoint "$m" "$p"
        mounts+=("$m")
        find "$m" -type f -print0 >> "$tmp/files"
    elif [ -d "$p" ]; then
        find "$p" -type f -print0 >> "$tmp/files"
    elif [ -f "$p" ]; then
        printf '%s\0' "$p" >> "$tmp/files"
    elif [ -e "$p" ] || [ -L "$p" ]; then
        :   # symlinks (their targets are listed too) and special files
    else
        echo "check-minos.sh: $p: no such file or directory" >&2
        exit 2
    fi
}
if [ "$STDIN" = 1 ]; then
    while IFS= read -r p; do [ -n "$p" ] && add_path "$p"; done
fi
for p in "$@"; do add_path "$p"; done

# keep the Mach-O files and the static archives (file(1) reads them in batches)
if [ -s "$tmp/files" ]; then
    xargs -0 file -N -F $'\t' < "$tmp/files" |
        awk -F '\t' '$2 ~ /Mach-O|ar archive/ && $1 !~ / \(for architecture [^)]*\):?$/ {print $1}' > "$tmp/macho"
else
    : > "$tmp/macho"
fi

# the newest minimum macOS of all architectures (and archive members) of each file
status=0
while IFS= read -r f; do
    v=$({ otool -arch all -l "$f" 2>/dev/null || true; } | awk '
        function newer(a, b,   x, y, i, n) { # 1 if version a > b
            n = split(a, x, "."); split(b, y, ".")
            for (i = 1; i <= 3; i++) { if ((x[i] + 0) != (y[i] + 0)) return (x[i] + 0) > (y[i] + 0) }
            return 0
        }
        $1 == "cmd" { cmd = $2; plat = "" }
        cmd == "LC_BUILD_VERSION" && $1 == "platform" { plat = $2 }
        cmd == "LC_BUILD_VERSION" && $1 == "minos" && (plat == "1" || plat == "MACOS" || plat == "macos") { v = $2 }
        cmd == "LC_VERSION_MIN_MACOSX" && $1 == "version" { v = $2 }
        v != "" { if (best == "" || newer(v, best)) best = v; v = "" }
        END { print (best == "" ? "none" : best) }')
    printf '%s\t%s\n' "$v" "$f" >> "$tmp/minos"
done < "$tmp/macho"
[ -f "$tmp/minos" ] || : > "$tmp/minos"

awk -F '\t' -v target="$TARGET" -v verbose="$VERBOSE" -v tmp="$tmp/" '
    function newer(a, b,   x, y, i, n) {
        n = split(a, x, "."); split(b, y, ".")
        for (i = 1; i <= 3; i++) { if ((x[i] + 0) != (y[i] + 0)) return (x[i] + 0) > (y[i] + 0) }
        return 0
    }
    {
        if (index($2, tmp) == 1) $2 = substr($2, length(tmp) + 1)
        count[$1]++; total++
        bad = ($1 != "none" && newer($1, target))
        if (bad) { nbad++; print $1 "\t" $2 } else if (verbose) print $1 "\t" $2
    }
    END {
        printf "check-minos: %d Mach-O files, target macOS %s:", total, target > "/dev/stderr"
        for (v in count) printf " %s=%d", v, count[v] > "/dev/stderr"
        printf "\n" > "/dev/stderr"
        if (nbad) { printf "check-minos: %d files need a macOS newer than %s\n", nbad, target > "/dev/stderr"; exit 1 }
    }' "$tmp/minos" || status=$?
exit "$status"
