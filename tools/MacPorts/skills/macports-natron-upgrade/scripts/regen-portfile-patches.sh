#!/bin/bash
# Regenerate every Portfile.patch as the unified diff between Portfile.orig and Portfile.
#
# Safe for pinned ports: this only (re)writes Portfile.patch and never
# modifies Portfile or Portfile.orig.
#
# Usage: regen-portfile-patches.sh [MACPORTS_ROOT]
#   MACPORTS_ROOT defaults to the current directory (should be tools/MacPorts).
set -u

root="${1:-.}"
if [ ! -d "$root" ]; then
    echo "error: not a directory: $root" >&2
    exit 1
fi

count=0
# Iterate every Portfile.orig; regenerate its sibling Portfile.patch.
while IFS= read -r orig; do
    dir=$(dirname "$orig")
    [ -f "$dir/Portfile" ] || continue
    # diff exits 1 when files differ (the normal case); only >1 is a real error.
    ( cd "$dir" && diff -u Portfile.orig Portfile > Portfile.patch )
    rc=$?
    if [ "$rc" -gt 1 ]; then
        echo "error: diff failed in $dir (exit $rc)" >&2
        exit "$rc"
    fi
    echo "regenerated $dir/Portfile.patch"
    count=$((count + 1))
done < <(find "$root" -name Portfile.orig)

echo "done: $count Portfile.patch file(s) regenerated"
