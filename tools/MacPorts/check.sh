#!/bin/bash
# Check Natron's MacPorts overlay (this directory) against upstream MacPorts.
#
# usage: ./check.sh [-a|--all]
#
# For each overlay port, by default only the ports that are installed (use
# -a/--all to check every overlay port), report:
# - "is obsolete": Portfile.orig differs from the upstream Portfile (the
#   overlay must be reintegrated, see the printed command), or, for a port
#   without Portfile.orig, the Portfile differs from upstream;
# - "is missing" / "differs from upstream": files/ entries of the upstream
#   port that the overlay lacks or has in another version;
# - "is unused": files/ entries that the port's Portfile doesn't reference
#   and that upstream doesn't ship either.
# For the whole overlay, also report port directories without a Portfile
# (MacPorts ignores them; a directory with a README.md is assumed to be kept
# on purpose), and whether /opt/MacPorts-Natron, the copy that MacPorts reads,
# differs from this tree (run ./sync.sh from an up-to-date checkout).
#
# All comparisons are by content, not by date, so the result doesn't depend on
# when the files were checked out. Nothing is modified.
# Exit status: 0 if nothing needs action, 1 otherwise, 2 on usage errors.

set -u # Treat unset variables as an error when substituting.
cd "$(dirname "$0")" || exit 2

all=no
for arg in "$@"; do
    case "$arg" in
        -a|--all) all=yes ;;
        -h|--help) sed -n '2,23p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "usage: $0 [-a|--all]" >&2; exit 2 ;;
    esac
done

# upstream ports tree: the one of the [default] source in sources.conf
# (e.g. rsync://rsync.macports.org/macports/release/tarballs/ports.tar.gz or
# https://distfiles.macports.org/ports.tar.gz), else the first one found
ports=
default_src=$(awk '!/^[[:space:]]*#/ && /\[default/ {print $1; exit}' /opt/local/etc/macports/sources.conf 2>/dev/null || true)
if [ -n "$default_src" ]; then
    d="/opt/local/var/macports/sources/$(echo "$default_src" | sed -e 's@^[a-z]*://@@' -e 's@\.tar\.gz$@@')"
    [ -d "$d" ] && ports="$d"
fi
if [ -z "$ports" ]; then
    for d in /opt/local/var/macports/sources/rsync.macports.org/macports/release/tarballs/ports /opt/local/var/macports/sources/rsync.macports.org/release/tarballs/ports /opt/local/var/macports/sources/distfiles.macports.org/ports; do
        if [ -d "$d" ]; then ports="$d"; break; fi
    done
fi
if [ -z "$ports" ]; then
    echo "Error: upstream MacPorts ports tree not found (run 'sudo port selfupdate' or 'sudo port sync')" >&2
    exit 2
fi

status=0
need_action() {
    echo "$*"
    status=1
}

# names of the installed ports (checked only unless --all)
installed=
if [ "$all" = no ]; then
    if ! command -v port > /dev/null; then
        echo "Error: 'port' not found in PATH (or use --all)" >&2
        exit 2
    fi
    installed=$(port -q installed 2> /dev/null | awk '{print $1}' | sort -u)
fi

# is_installed NAME: the port, or one of its python subports (py-foo -> py310-foo)
is_installed() {
    case "$1" in
        py-*) echo "$installed" | grep -q -E "^py[0-9]+-${1#py-}\$" && return 0 ;;
    esac
    echo "$installed" | grep -q -x -F -- "$1"
}

checked=0
skipped=0
for p in $(find . -name Portfile -not -path './skills/*' | sed 's@^\./@@' | sort); do
    d=$(dirname "$p")
    name=$(awk '$1 == "name" {print $2; exit}' "$p")
    case "$name" in
        ''|*'$'*|*'['*) name=$(basename "$d") ;;
    esac
    if [ "$all" = no ] && ! is_installed "$name"; then
        skipped=$((skipped + 1))
        continue
    fi
    checked=$((checked + 1))

    # the Portfile
    if [ -f "${p}.orig" ]; then
        if [ ! -f "$ports/$p" ]; then
            need_action "$p is obsolete: upstream has no $p any more (the port was moved or removed)"
        elif ! cmp -s "$ports/$p" "${p}.orig"; then
            need_action "$p is obsolete: upstream changed since Portfile.orig"
            if [ -f "${p}.patch" ]; then
                echo "\$ cp $ports/$p ${p}.orig; cp $ports/$p ${p}; (cd $d && patch < Portfile.patch)"
            fi
        fi
    elif [ -f "$ports/$p" ] && ! cmp -s "$ports/$p" "$p"; then
        need_action "$p is obsolete: it differs from upstream and has no Portfile.orig (add one, or copy upstream)"
        echo "\$ diff -u $ports/$p $p"
    fi

    # files/ entries of the upstream port
    if [ -d "$ports/$d/files" ]; then
        for q in $(cd "$ports/$d" && find files -type f | sort); do
            if [ ! -f "$d/$q" ]; then
                need_action "$d/$q is missing (upstream has it)"
                echo "\$ mkdir -p $(dirname "$d/$q"); cp $ports/$d/$q $d/$q"
            elif ! cmp -s "$ports/$d/$q" "$d/$q"; then
                need_action "$d/$q differs from upstream"
                echo "\$ cp $ports/$d/$q $d/$q"
            fi
        done
    fi

    # files/ entries that nothing uses
    if [ -d "$d/files" ]; then
        for q in $(cd "$d" && find files -type f | sort); do
            grep -q -F -- "$(basename "$q")" "$p" && continue
            [ -f "$ports/$d/$q" ] && continue
            need_action "$d/$q is unused: $p doesn't reference it and upstream doesn't ship it"
            echo "\$ git rm $d/$q"
        done
    fi
done

# port directories without a Portfile (MacPorts ignores them)
for d in $(find . -mindepth 2 -maxdepth 2 -type d -not -path './skills*' | sed 's@^\./@@' | sort); do
    [ -f "$d/Portfile" ] && continue
    if [ -f "$d/README.md" ]; then
        echo "note: $d has no Portfile (kept on purpose, see $d/README.md)"
    else
        need_action "$d has no Portfile: MacPorts ignores it; remove it or restore its Portfile"
    fi
done

# the copy that MacPorts reads
mp=/opt/MacPorts-Natron
if [ -d "$mp" ]; then
    stale=$(diff -rq -x 'PortIndex*' -x '.*' . "$mp" 2> /dev/null)
    if [ -n "$stale" ]; then
        need_action "$mp differs from this tree: MacPorts would use other Portfiles. Run ./sync.sh from an up-to-date checkout (differences: $(echo "$stale" | wc -l | tr -d ' '))"
        echo "$stale" | head -10
    fi
else
    need_action "$mp doesn't exist: run ./sync.sh (see INSTALL_MACOS.md)"
fi

if [ "$all" = no ]; then
    echo "checked $checked installed overlay ports ($skipped not installed; use --all to check them too)"
else
    echo "checked $checked overlay ports"
fi
exit $status
