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

# Usage: download_baymax.sh <Natron-Tests checkout>
# Puts the Robot image sequence of the BayMax example in <Natron-Tests>/BayMax/Robot, as Natron-Tests' runTests.sh
# does before it renders BayMax (the render soak test, Tests/gui/run_render_soak_test.py --graphs baymax, needs it
# without running runTests.sh). The zip comes from SourceForge, whose mirrors sometimes fail: each is tried in turn.
set -euo pipefail

tests="${1:?usage: $0 <Natron-Tests checkout>}"
[ -d "$tests/BayMax" ] || { echo "Error: $tests/BayMax not found"; exit 1; }
[ -d "$tests/BayMax/Robot" ] && exit 0

zip="Natron_2.3.12_BayMax.zip"
url="https://downloads.sourceforge.net/project/natron/Examples/$zip"
mirrors="phoenixnap.dl.sourceforge.net cfhcable.dl.sourceforge.net master.dl.sourceforge.net netix.dl.sourceforge.net"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
location=$(curl -sI --max-time 60 "$url" | awk 'tolower($1)=="location:"{print $2}' | tr -d '\r' | tail -1)
[ -n "$location" ] || location="$url"
ok=0
for host in __auto__ $mirrors; do
    try="$location"
    [ "$host" = __auto__ ] || try=$(printf '%s' "$location" | sed -E "s#https?://[^/]+/#https://${host}/#")
    echo "downloading $zip (mirror: $host)"
    if curl -fL --retry 2 --retry-delay 3 --max-time 1800 -o "$work/$zip" "$try" && unzip -tqq "$work/$zip" >/dev/null 2>&1; then
        ok=1
        break
    fi
done
[ "$ok" = 1 ] || { echo "Error: could not download $zip from any SourceForge mirror"; exit 1; }
(cd "$work" && unzip -qq "$zip")
mv "$work/Natron_2.3.12_BayMax/Robot" "$tests/BayMax/"
echo "BayMax/Robot: $(ls "$tests/BayMax/Robot" | wc -l) files"
