#!/bin/bash
# Check that every Mach-O file (executables, dylibs, bundles, static archives) runs on a given
# macOS version: the minimum macOS of each architecture (LC_BUILD_VERSION minos, or the older
# LC_VERSION_MIN_MACOSX) must not be newer than the target.
#
#   check-minos.sh [-t TARGET] [-v] PATH...   PATH: a directory, a file, or a .dmg (mounted read-only)
#   check-minos.sh [-t TARGET] [-v] -l        read the paths to check from stdin, one per line
#                                             (paths that can't be found are skipped)
#
#   -t TARGET   oldest macOS that must be supported (default: $MACOSX_DEPLOYMENT_TARGET, else
#               NATRON_MACOS_RELEASE_MIN from tools/jenkins/macos-release.sh)
#   -v          list every Mach-O file with its minos, not only the ones that are too new
#   -N          accept the files that have no minimum-version load command ("none"); by default
#               they fail the check, since their minimum macOS is unknown
#
# Output: one line "minos<TAB>file" per file needing a newer macOS than TARGET (or "none", or
# "error" when otool can't read it), then a summary (number of files per minos). Exit status: 0 if
# all files are OK, 1 if some need a newer macOS (or have none), 2 on usage errors, if otool is
# missing, or if otool fails on a file.
# Used by natron-build.sh (on the DMG) and provision.sh (to find ports built for a newer macOS).
set -euo pipefail

# shellcheck source=../jenkins/macos-release.sh
. "$(cd "$(dirname "$0")" && pwd)/../jenkins/macos-release.sh"
TARGET=${MACOSX_DEPLOYMENT_TARGET:-$NATRON_MACOS_RELEASE_MIN}
VERBOSE=0
STDIN=0
ALLOW_NONE=0
while getopts "t:vlNh" opt; do
    case $opt in
        t) TARGET=$OPTARG ;;
        v) VERBOSE=1 ;;
        l) STDIN=1 ;;
        N) ALLOW_NONE=1 ;;
        *) sed -n 2,19p "$0"; exit 2 ;;
    esac
done
shift $((OPTIND - 1))
[ "$STDIN" = 1 ] || [ $# -gt 0 ] || { sed -n 2,19p "$0"; exit 2; }
command -v otool > /dev/null || { echo "check-minos.sh: otool not found (Command Line Tools)" >&2; exit 2; }

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
    elif [ "$STDIN" = 1 ]; then
        :   # lists from "port contents" also have files in directories we can't read
            # (e.g. openldap's /opt/local/var/run/openldap-data), or that were deleted since
    else
        echo "check-minos.sh: $p: no such file or directory" >&2
        exit 2
    fi
}
if [ "$STDIN" = 1 ]; then
    # leading blanks are ignored (port contents indents its list)
    while IFS= read -r p; do p=${p#"${p%%[![:space:]]*}"}; [ -z "$p" ] || add_path "$p"; done
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
    if ! otool -arch all -l "$f" > "$tmp/otool" 2> /dev/null; then
        printf 'error\t%s\n' "$f" >> "$tmp/minos"
        continue
    fi
    v=$(awk '
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
        END { print (best == "" ? "none" : best) }' "$tmp/otool")
    printf '%s\t%s\n' "$v" "$f" >> "$tmp/minos"
done < "$tmp/macho"
[ -f "$tmp/minos" ] || : > "$tmp/minos"

awk -F '\t' -v target="$TARGET" -v verbose="$VERBOSE" -v allownone="$ALLOW_NONE" -v tmp="$tmp/" '
    function newer(a, b,   x, y, i, n) {
        n = split(a, x, "."); split(b, y, ".")
        for (i = 1; i <= 3; i++) { if ((x[i] + 0) != (y[i] + 0)) return (x[i] + 0) > (y[i] + 0) }
        return 0
    }
    {
        if (index($2, tmp) == 1) $2 = substr($2, length(tmp) + 1)
        count[$1]++; total++
        if ($1 == "error") { nerr++; print $1 "\t" $2; next }
        bad = ($1 == "none" ? !allownone : newer($1, target))
        if (bad) { nbad++; print $1 "\t" $2 } else if (verbose) print $1 "\t" $2
    }
    END {
        printf "check-minos: %d Mach-O files, target macOS %s:", total, target > "/dev/stderr"
        for (v in count) printf " %s=%d", v, count[v] > "/dev/stderr"
        printf "\n" > "/dev/stderr"
        if (nerr) { printf "check-minos: otool failed on %d files\n", nerr > "/dev/stderr"; exit 2 }
        if (nbad) { printf "check-minos: %d files need a macOS newer than %s, or have no minimum version\n", nbad, target > "/dev/stderr"; exit 1 }
    }' "$tmp/minos" || status=$?
exit "$status"
