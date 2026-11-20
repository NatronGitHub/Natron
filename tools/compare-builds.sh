#!/usr/bin/env bash
# Compare two Natron builds, e.g. the one built with qmake and the one built with CMake
# (#1181), on macOS, Linux or Windows (MSYS2).
#
#   tools/compare-builds.sh [-k] [-r] [-w WRAPPER] [-o OUTDIR] [-x REGEX]... TREE_A TREE_B
#
# A TREE is a directory: a Natron.app bundle, a directory containing Natron.app (e.g. a mounted
# DMG), a portable Linux or Windows tree (bin/, lib/, Resources/, Plugins/), or the staging
# directory filled by tools/jenkins/build-natron.sh ($TMP_BINARIES_PATH).
#
# Compares:
#   - the file list (relative paths, types and link targets);
#   - the sha256 of every file that is not an executable or a library (resources, scripts,
#     OpenColorIO configs, PyPlugs...), and the diff of the text files that differ;
#   - Info.plist (macOS);
#   - for every executable or library present in both trees: the linked libraries
#     (macOS: otool -L; Linux: NEEDED; Windows: the imported DLLs), the rpaths (LC_RPATH,
#     RPATH/RUNPATH), and on macOS the architectures and the minimum macOS version;
# and checks each tree on its own for library references it can't resolve without
# DYLD_*/LD_LIBRARY_PATH (macOS: absolute paths outside the tree and the system; Linux:
# ldd "not found").
# With -r, also runs the programs of each tree: "NatronRenderer -v", "Natron -v",
# NatronRenderer with a Python script that imports NatronEngine and PySide, natron-python,
# and compares their output. Running a GUI-less NatronRenderer needs no display; on Linux
# QT_QPA_PLATFORM=offscreen is set.
#
# Exit status: 0 if the builds are equivalent, 1 if they differ or a tree has unresolved
# references, 2 on a usage error, a missing tool or a failed check (the result can't be trusted).
#   -k          keep going when a tool is missing (the checks that need it are SKIPPED, exit 2)
#   -r          also run the programs (see above)
#   -w WRAPPER  run each program through WRAPPER (e.g. a script that runs it under a debugger)
#   -o DIR      where to write each check's raw output (default: a new temporary directory)
#   -x REGEX    ignore the files whose relative path matches REGEX (extended regex, repeatable),
#               e.g. -x '^Contents/Resources/docs/' to skip the bundled documentation
# Always ignored: __pycache__ and .pyc files, Sphinx's .doctrees and .buildinfo (they change at
# every documentation build), and the code signatures (_CodeSignature, which change at every
# signing). Each tree's own path is replaced
# by <TREE> in the outputs, so that only real differences show.

set -u
usage() { sed -n '2,38p' "$0" | sed -e 's/^# \{0,1\}//'; exit 2; }
keep=0
run=0
wrapper=""
outdir=""
excludes=('(^|/)__pycache__(/|$)' '\.pyc$' '(^|/)\.doctrees(/|$)' '(^|/)\.buildinfo$' '(^|/)_CodeSignature(/|$)')
while getopts "krw:o:x:h" opt; do
    case $opt in
    k) keep=1 ;;
    r) run=1 ;;
    w) wrapper=$OPTARG ;;
    o) outdir=$OPTARG ;;
    x) excludes+=("$OPTARG") ;;
    *) usage ;;
    esac
done
shift $((OPTIND - 1))
[ $# -eq 2 ] && [ -d "$1" ] && [ -d "$2" ] || usage
A=$(cd "$1" && pwd -P)
B=$(cd "$2" && pwd -P)
[ -n "$outdir" ] || outdir=$(mktemp -d "${TMPDIR:-/tmp}/compare-builds.XXXXXX")
mkdir -p "$outdir"
case "$(uname -s)" in
    Darwin) os=macos ;;
    MINGW*|MSYS*|CYGWIN*|UCRT64*|CLANG*) os=windows ;;
    *) os=linux ;;
esac
differ=0   # the builds differ, or a tree has unresolved references
broken=0   # a tool is missing or a check failed
exclude_re=$(IFS='|'; echo "${excludes[*]}")

# need TOOL...: check that the tools exist; with -k, return 1 (the caller skips its checks)
need() {
    local t
    for t in "$@"; do
        command -v "$t" >/dev/null 2>&1 && continue
        echo "  missing tool: $t"
        broken=1
        [ $keep = 1 ] || exit 2
        return 1
    done
    return 0
}
for t in find sort awk sed diff grep file comm; do command -v $t >/dev/null 2>&1 || { echo "error: missing tool: $t"; exit 2; }; done
if command -v sha256sum >/dev/null 2>&1; then sha256() { sha256sum "$@"; }
elif command -v shasum >/dev/null 2>&1; then sha256() { shasum -a 256 "$@"; }
else echo "error: no sha256sum or shasum"; exit 2; fi
timeout_cmd=()
if command -v timeout >/dev/null 2>&1; then timeout_cmd=(timeout -k 10 300)
elif command -v gtimeout >/dev/null 2>&1; then timeout_cmd=(gtimeout -k 10 300); fi

# untree TREE FILE: print FILE with every occurrence of TREE (literal text) replaced by <TREE>
untree() {
    TREE_PATH=$1 awk '{
        p = ENVIRON["TREE_PATH"]; out = ""
        while ((i = index($0, p)) > 0) { out = out substr($0, 1, i - 1) "<TREE>"; $0 = substr($0, i + length(p)) }
        print out $0
    }' "$2"
}

# the file list of a tree, without the excluded files: "type path [-> target]"
list_files() {
    (cd "$1" && find . -print | sed -e 's|^\./||' -e '/^\.$/d' | grep -Ev -- "$exclude_re" | LC_ALL=C sort | { st=0; while IFS= read -r f; do
        if [ -L "$f" ]; then l=$(readlink "$f") || st=1; echo "link $f -> $l"; elif [ -d "$f" ]; then echo "dir $f"; else echo "file $f"; fi
    done; exit "$st"; })
}

# classify the regular files of a tree: "bin path" (Mach-O, ELF, PE) or "data path"
classify() {
    (cd "$1" && find . -type f -print | sed -e 's|^\./||' | grep -Ev -- "$exclude_re" | LC_ALL=C sort | { st=0; while IFS= read -r f; do
        t=$(file -b "$f") || { st=1; continue; }
        case "$t" in
            *Mach-O*|*ELF*|*PE32*) echo "bin $f" ;;
            *) echo "data $f" ;;
        esac
    done; exit "$st"; })
}

echo "A: $A"
echo "B: $B"
echo "report files: $outdir"

echo
echo "== files"
for side in A B; do
    if [ $side = A ]; then t=$A; else t=$B; fi
    if ! (set -o pipefail; list_files "$t") > "$outdir/files.$side.txt" 2>&1; then
        echo "  ERROR: listing the files of $side failed"; broken=1
    fi
    if ! (set -o pipefail; classify "$t") > "$outdir/classes.$side.txt" 2>&1; then
        echo "  ERROR: classifying the files of $side failed"; broken=1
    fi
done
if diff -u "$outdir/files.A.txt" "$outdir/files.B.txt" > "$outdir/files.diff"; then
    printf '  %-22s same (%s entries)\n' "file list" "$(wc -l < "$outdir/files.A.txt" | tr -d ' ')"
else
    printf '  %-22s DIFFERENT (%s)\n' "file list" "$outdir/files.diff"
    grep -E '^[-+][^-+]' "$outdir/files.diff" | sed -e 's/^/      /' | head -60
    n=$(grep -cE '^[-+][^-+]' "$outdir/files.diff")
    [ "$n" -le 60 ] || echo "      ... ($n lines)"
    differ=1
fi

# files present in both trees, by class
LC_ALL=C comm -12 "$outdir/classes.A.txt" "$outdir/classes.B.txt" > "$outdir/common.txt"
sed -n 's/^data //p' "$outdir/common.txt" > "$outdir/common-data.txt"
sed -n 's/^bin //p' "$outdir/common.txt" > "$outdir/common-bin.txt"

echo
echo "== contents of the data files present in both ($(wc -l < "$outdir/common-data.txt" | tr -d ' ') files)"
: > "$outdir/data.diff"
nd=0
while IFS= read -r f; do
    ha=$(sha256 "$A/$f" | awk '{print $1}')
    hb=$(sha256 "$B/$f" | awk '{print $1}')
    [ "$ha" = "$hb" ] && continue
    nd=$((nd + 1))
    echo "  differs: $f"
    case "$(file -b "$A/$f")" in
        *text*|*XML*|*JSON*) diff -u "$A/$f" "$B/$f" | sed -e "s|$A|<TREE>|g;s|$B|<TREE>|g" >> "$outdir/data.diff" ;;
        *) echo "binary file $f differs" >> "$outdir/data.diff" ;;
    esac
done < "$outdir/common-data.txt"
if [ $nd = 0 ]; then
    echo "  same"
else
    echo "  $nd file(s) differ (diffs: $outdir/data.diff)"
    head -40 "$outdir/data.diff" | sed -e 's/^/      /'
    differ=1
fi

# Info.plist files (macOS bundles), printed in a canonical form
if [ "$os" = macos ]; then
    echo
    echo "== Info.plist"
    grep -E '(^|/)Contents/Info\.plist$' "$outdir/common-data.txt" > "$outdir/plists.txt" || true
    if [ ! -s "$outdir/plists.txt" ]; then
        echo "  (no Info.plist in both trees)"
    elif need plutil; then
        while IFS= read -r f; do
            plutil -p "$A/$f" > "$outdir/plist.A.txt" 2>&1 || broken=1
            plutil -p "$B/$f" > "$outdir/plist.B.txt" 2>&1 || broken=1
            if diff -u "$outdir/plist.A.txt" "$outdir/plist.B.txt" > "$outdir/plist.diff"; then
                printf '  %-40s same\n' "$f"
            else
                printf '  %-40s DIFFERENT\n' "$f"
                sed -e 's/^/      /' "$outdir/plist.diff" | head -30
                differ=1
            fi
        done < "$outdir/plists.txt"
    fi
fi

# per-binary properties, one line per property; the tree path is replaced by <TREE>
case $os in
macos)
    need otool lipo || true
    # otool prints a header line ending in ':' per file or per architecture
    # shellcheck disable=SC2016 # awk code
    archs='/:$/ { arch = ""; if (match($0, /\(architecture [^)]*\)/)) arch = substr($0, RSTART + 14, RLENGTH - 15) " "; next }'
    binprops() {
        local id
        id=$(otool -D "$1" | sed -e '/:$/d' | sed -n 1p)
        echo "archs $(lipo -archs "$1" 2>/dev/null || echo '?')"
        [ -z "$id" ] || echo "id $id"
        otool -L "$1" | awk "$archs"' { print "lib " arch $1 }' | grep -vxF "lib ${id:-/}" | LC_ALL=C sort -u
        otool -l "$1" | awk "$archs"' /LC_RPATH/ { getline; getline; print "rpath " arch $2 }'
        otool -l "$1" | awk "$archs"' /LC_BUILD_VERSION/ { getline; getline; getline; print "minos " arch $2 } /LC_VERSION_MIN_MACOSX/ { getline; getline; print "minos " arch $2 }' | LC_ALL=C sort -u
    }
    # references that can't be resolved without DYLD_*: absolute paths outside the tree and the system
    outside() {
        local f id
        while IFS= read -r f; do
            id=$(otool -D "$1/$f" | sed -e '/:$/d' | sed -n 1p)
            otool -L "$1/$f" | sed -e '/:$/d' | awk '{print $1}' | grep -vxF -- "${id:-/}" | grep -E '^/' |
                grep -vE '^/(usr/lib|System)/' | grep -vF -- "$1/" | sed -e "s|^|$f: library |"
            otool -l "$1/$f" | awk '/LC_RPATH/{getline; getline; print $2}' | grep -E '^/' |
                grep -vE '^/(usr/lib|System)/' | grep -vF -- "$1/" | sed -e "s|^|$f: rpath |"
        done
        return 0
    }
    ;;
linux)
    need readelf ldd || true
    binprops() {
        readelf -d "$1" | sed -n -E 's/.*\(NEEDED\).*\[(.*)\]$/lib \1/p' | LC_ALL=C sort
        readelf -d "$1" | sed -n -E 's/.*\((RPATH|RUNPATH)\).*\[(.*)\]$/rpath \1 \2/p'
        readelf -d "$1" | sed -n -E 's/.*\(SONAME\).*\[(.*)\]$/soname \1/p'
    }
    outside() {
        local f
        while IFS= read -r f; do
            env -u LD_LIBRARY_PATH ldd "$1/$f" 2>/dev/null | grep -E 'not found' | awk -v f="$f" '{print f ": library " $1 " not found"}'
        done
        return 0
    }
    ;;
windows)
    need objdump || true
    binprops() {
        objdump -p "$1" | sed -n -E 's/^[[:space:]]*DLL Name: (.*)$/lib \1/p' | LC_ALL=C sort -u
    }
    outside() { return 0; } # DLLs are found through PATH: not checked
    ;;
esac

echo
echo "== executables and libraries present in both ($(wc -l < "$outdir/common-bin.txt" | tr -d ' ') files)"
: > "$outdir/binaries.diff"
nb=0
while IFS= read -r f; do
    (set -o pipefail; binprops "$A/$f") > "$outdir/bin.A.raw" 2>&1 || { echo "  ERROR: $f (A)"; broken=1; }
    (set -o pipefail; binprops "$B/$f") > "$outdir/bin.B.raw" 2>&1 || { echo "  ERROR: $f (B)"; broken=1; }
    untree "$A" "$outdir/bin.A.raw" > "$outdir/bin.A.txt"
    untree "$B" "$outdir/bin.B.raw" > "$outdir/bin.B.txt"
    if ! diff -u --label "A/$f" --label "B/$f" "$outdir/bin.A.txt" "$outdir/bin.B.txt" >> "$outdir/binaries.diff"; then
        nb=$((nb + 1))
    fi
done < "$outdir/common-bin.txt"
if [ $nb = 0 ]; then
    echo "  same linked libraries and rpaths"
else
    echo "  $nb file(s) differ (diffs: $outdir/binaries.diff)"
    sed -e 's/^/      /' "$outdir/binaries.diff" | head -80
    differ=1
fi

echo
echo "== unresolved references (each tree on its own)"
for side in A B; do
    if [ $side = A ]; then t=$A; else t=$B; fi
    sed -n 's/^bin //p' "$outdir/classes.$side.txt" | outside "$t" > "$outdir/outside.$side.txt" 2>&1
    if [ -s "$outdir/outside.$side.txt" ]; then
        echo "  $side: PROBLEM ($(wc -l < "$outdir/outside.$side.txt" | tr -d ' ') references, see $outdir/outside.$side.txt)"
        sed -e 's/^/      /' "$outdir/outside.$side.txt" | head -20
        differ=1
    else
        echo "  $side: none"
    fi
done

if [ $run = 1 ]; then
    echo
    echo "== programs"
    # find PROGRAM in a tree: Contents/MacOS (macOS), bin/ (portable), or the tree itself
    find_prog() {
        local d
        for d in Natron.app/Contents/MacOS Contents/MacOS bin .; do
            for e in "" .exe -bin -bin.exe; do
                if [ -f "$1/$d/$2$e" ] && [ -x "$1/$d/$2$e" ]; then echo "$1/$d/$2$e"; return 0; fi
            done
        done
        return 1
    }
    cat > "$outdir/import.py" <<'EOF'
import sys
import NatronEngine
print("Python", sys.version.split()[0])
print("NatronEngine", NatronEngine.natron.getNatronVersionString(),
      NatronEngine.natron.getNatronDevelopmentStatus(), NatronEngine.natron.getBuildNumber())
try:
    import PySide2.QtCore as QtCore
    print("PySide2", QtCore.qVersion())
except ImportError:
    import PySide6.QtCore as QtCore
    print("PySide6", QtCore.qVersion())
EOF
    [ "$os" = linux ] && export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"
    run_prog() { # run_prog TREE PROGRAM ARGS...: output with the tree path replaced
        local p
        p=$(find_prog "$1" "$2") || { echo "(no $2)"; return 0; }
        shift 2
        # shellcheck disable=SC2086 # the wrapper may have arguments
        "${timeout_cmd[@]}" $wrapper "$p" "$@" < /dev/null 2>&1
        echo "exit status: $?"
    }
    runcheck() { # runcheck NAME PROGRAM ARGS...
        local name=$1 side t
        shift
        for side in A B; do
            if [ $side = A ]; then t=$A; else t=$B; fi
            run_prog "$t" "$@" > "$outdir/run-$name.$side.raw"
            # keep the program's own output: drop the lines of a debugger wrapper and empty lines
            untree "$t" "$outdir/run-$name.$side.raw" | grep -vE '^(\(lldb\)|Process [0-9]+|Current executable|target\.|settings |$)' > "$outdir/run-$name.$side.txt"
        done
        if diff -u "$outdir/run-$name.A.txt" "$outdir/run-$name.B.txt" > "$outdir/run-$name.diff"; then
            printf '  %-22s same\n' "$name"
        else
            printf '  %-22s DIFFERENT (%s)\n' "$name" "$outdir/run-$name.diff"
            sed -e 's/^/      /' "$outdir/run-$name.diff" | head -30
            differ=1
        fi
        for side in A B; do
            grep -E 'version|NatronEngine|PySide|Python [0-9]|exit status' "$outdir/run-$name.$side.txt" | head -5 | sed -e "s/^/      $side: /"
        done
    }
    runcheck renderer-version NatronRenderer -v
    runcheck natron-version Natron -v
    runcheck python-import NatronRenderer -t "$outdir/import.py"
    runcheck natron-python natron-python -c 'import sys; print(sys.version.split()[0]); import encodings, ssl, sqlite3; print("stdlib ok")'
fi

echo
if [ $broken = 1 ]; then
    echo "RESULT: INCOMPLETE: a tool is missing or a check failed (see above); the comparison can't be trusted"
    exit 2
elif [ $differ = 0 ]; then
    echo "RESULT: equivalent"
    exit 0
else
    echo "RESULT: different (details above and in $outdir)"
    exit 1
fi
