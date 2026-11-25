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
# - pinned ports (a PINNED file in the port directory, see below) keep their
#   Portfile: only Portfile.orig and Portfile.patch follow upstream. The
#   versions are reported, and "upstream caught up: unpin" when a port pinned
#   ahead of upstream has been reached or overtaken by upstream;
# - "is missing" / "differs from upstream": files/ entries of the upstream
#   port that the overlay lacks or has in another version;
# - "is unused": files/ entries whose name doesn't appear in the port's
#   Portfile and that upstream doesn't ship either. This is a name search: a
#   file referenced only through a variable (patch-${version}.diff, a foreach
#   over a list) is reported too, so read the Portfile before removing it.
# A port counts as installed if its name or one of its subports is installed.
# For aqua/qt5, whose subports other than qtbase come from MacPorts' binary archives, report a
# Natron patchfile that is neither for qtbase nor listed as build-only (see the Portfile).
# For the whole overlay, also report port directories without a Portfile
# (MacPorts ignores them; a directory with a README.md is assumed to be kept
# on purpose), and whether /opt/MacPorts-Natron (or $MACPORTS_NATRON), the
# copy that MacPorts reads, differs from this tree.
#
# PINNED files: the first line is the kind of pin, the second line why.
# - "ahead": our Portfile has a newer version than upstream; unpin it (delete
#   PINNED, take upstream's Portfile, keep Natron's changes in Portfile.patch,
#   drop patches that upstream integrated) as soon as upstream catches up.
# - "older": our Portfile is held at an older version until the reason is gone.
#
# All comparisons are by content, not by date, so the result doesn't depend on
# when the files were checked out. Nothing is modified. The printed commands
# are relative to this directory (tools/MacPorts).
# Exit status: 0 if nothing needs action, 1 if something does, 2 on usage or
# environment errors (no upstream ports tree, `port` missing or failing).

set -u # Treat unset variables as an error when substituting.
cd "$(dirname "$0")" || exit 2

all=no
for arg in "$@"; do
    case "$arg" in
        -a|--all) all=yes ;;
        -h|--help) sed -n '2,37p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
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
    if [ -n "$ports" ]; then
        echo "warning: the ports tree of the [default] source (${default_src:-none}) was not found, using $ports, which may be out of date: run 'sudo port selfupdate'" >&2
    fi
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
    if ! installed=$(port -q installed 2>&1); then
        echo "Error: 'port -q installed' failed (or use --all):" >&2
        echo "$installed" >&2
        exit 2
    fi
    installed=$(echo "$installed" | awk '{print $1}' | sort -u)
fi

# is_installed PORTFILE NAME: the port, or one of the subports declared in its
# Portfile (literal names, or ${name}-... patterns such as the qt5 modules)
is_installed() {
    echo "$installed" | grep -q -x -F -- "$2" && return 0
    local s
    for s in $(awk '$1 == "subport" {print $2}' "$1" | tr -d '{}' | sort -u); do
        case "$s" in
            '$name'-*) echo "$installed" | grep -q -F -- "$2-" && return 0 ;;
            *'$'*) ;;
            *) echo "$installed" | grep -q -x -F -- "$s" && return 0 ;;
        esac
    done
    return 1
}

# port_version DIR: "version revision" of the port in DIR (empty if unknown)
port_version() {
    command -v port > /dev/null || return 0
    port -q -D "$1" info --line --version --revision 2> /dev/null | awk '{print $1, ($2 == "" ? 0 : $2)}'
}

# vr "V R": V_R, as MacPorts prints it (? if unknown)
vr() {
    if [ -n "$1" ]; then echo "${1% *}_${1##* }"; else echo "?"; fi
}

# version_ge "V1 R1" "V2 R2": true if version V1 (revision R1) >= V2 (R2)
version_ge() {
    local v1 r1 v2 r2
    v1=${1% *}; r1=${1##* }; v2=${2% *}; r2=${2##* }
    if [ "$v1" = "$v2" ]; then
        [ "$r1" -ge "$r2" ]
    else
        [ "$(printf '%s\n%s\n' "$v1" "$v2" | sort -V | tail -1)" = "$v1" ]
    fi
}

checked=0
skipped=0
for p in $(find . -name Portfile -not -path './skills/*' | sed 's@^\./@@' | sort); do
    d=$(dirname "$p")
    name=$(awk '$1 == "name" {print $2; exit}' "$p")
    case "$name" in
        ''|*'$'*|*'['*) name=$(basename "$d") ;;
    esac
    if [ "$all" = no ] && ! is_installed "$p" "$name"; then
        skipped=$((skipped + 1))
        continue
    fi
    checked=$((checked + 1))

    # pinned ports: how far from upstream
    if [ -f "$d/PINNED" ]; then
        kind=$(sed -n 1p "$d/PINNED")
        why=$(sed -n 2p "$d/PINNED")
        ours=$(port_version "$d")
        theirs=
        [ -f "$ports/$p" ] && theirs=$(port_version "$ports/$d")
        case "$kind" in
            ahead)
                if [ -n "$ours" ] && [ -n "$theirs" ] && version_ge "$theirs" "$ours"; then
                    need_action "$d: upstream caught up: unpin (upstream $(vr "$theirs"), ours $(vr "$ours")): delete $d/PINNED, take upstream's Portfile, keep Natron's changes in Portfile.patch, drop patches that upstream integrated"
                else
                    echo "note: $d is pinned ahead of upstream (ours $(vr "$ours"), upstream $(vr "$theirs")): $why"
                fi
                ;;
            older)
                echo "note: $d is held at an older version (ours $(vr "$ours"), upstream $(vr "$theirs")): $why"
                ;;
            *)
                need_action "$d/PINNED: the first line must be 'ahead' or 'older' (found '$kind')"
                ;;
        esac
    fi

    # the Portfile
    if [ -f "${p}.orig" ]; then
        if [ ! -f "$ports/$p" ]; then
            need_action "$p is obsolete: upstream has no $p any more (the port was moved or removed)"
        elif ! cmp -s "$ports/$p" "${p}.orig"; then
            if [ -f "$d/PINNED" ]; then
                need_action "$p is obsolete: upstream changed since Portfile.orig (pinned: keep Portfile, update Portfile.orig and Portfile.patch only)"
                echo "\$ cp $ports/$p ${p}.orig; (cd $d && diff -u Portfile.orig Portfile > Portfile.patch)"
            else
                need_action "$p is obsolete: upstream changed since Portfile.orig"
                if [ -f "${p}.patch" ]; then
                    echo "\$ cp $ports/$p ${p}.orig; cp $ports/$p ${p}; (cd $d && patch < Portfile.patch)"
                else
                    echo "\$ cp $ports/$p ${p}.orig   # no Portfile.patch: merge the upstream changes into $p by hand"
                fi
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

    # files/ entries that nothing uses (name search, see the header)
    if [ -d "$d/files" ]; then
        for q in $(cd "$d" && find files -type f | sort); do
            grep -q -F -- "$(basename "$q")" "$p" && continue
            [ -f "$ports/$d/$q" ] && continue
            need_action "$d/$q is unused: its name doesn't appear in $p and upstream doesn't ship it (check the Portfile before removing it)"
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

# aqua/qt5 installs its subports other than qtbase from MacPorts' binary archives
# (natron_archive_subports in its Portfile): a Natron patch on one of them would be lost. Every
# patchfile that Portfile.patch adds must be for qtbase (patch-qtbase-*, always built here) or
# listed in natron_archive_build_only_patches (a patch that only changes the build, not the result).
# Not detected (the same list as in the Portfile comment): configure arguments, variants,
# revisions, build settings, and patch files whose names are built from variables.
qt5=aqua/qt5
if [ -f "$qt5/Portfile.patch" ]; then
    build_only=$(sed -n 's/^set natron_archive_build_only_patches {\(.*\)}$/\1/p' "$qt5/Portfile")
    # every patch file named on a line that Portfile.patch adds: a patchfiles-append line, its
    # continuation lines, or a line added to an upstream patchfiles list
    for q in $(grep '^+[^+]' "$qt5/Portfile.patch" | grep -oE '[A-Za-z0-9_.+-]+\.(diff|patch)' | grep -v '^Portfile\.patch$' | sort -u); do
        case "$q" in patch-qtbase-*) continue ;; esac
        case " $build_only " in *" $q "*) continue ;; esac
        need_action "$qt5: Natron patch $q may apply to a subport installed from MacPorts' archives (natron_archive_subports): bump that subport's revision, remove it from natron_archive_subports, or add $q to natron_archive_build_only_patches if it only affects the build"
    done
fi

# the copy that MacPorts reads
mp=${MACPORTS_NATRON:-/opt/MacPorts-Natron}
if [ -d "$mp" ]; then
    stale=$(diff -rq -x 'PortIndex*' -x '.*' . "$mp" 2> /dev/null)
    if [ -n "$stale" ]; then
        # the scripts and docs at the top of the overlay don't change what MacPorts builds
        ports_stale=$(echo "$stale" | grep -v -E "^(Files|Only in) (\./|\.: |$mp/|$mp: )(README[^/ ]*|[^/ ]*\.sh|skills)( |$|/|:)")
        if [ -n "$ports_stale" ]; then
            need_action "$mp differs from this tree: MacPorts would use other Portfiles or files. Run ./sync.sh from an up-to-date checkout (differences: $(echo "$ports_stale" | wc -l | tr -d ' '))"
            echo "$ports_stale" | head -10
        else
            echo "note: only the overlay's scripts or docs differ in $mp (the ports are the same); ./sync.sh would update them"
        fi
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
