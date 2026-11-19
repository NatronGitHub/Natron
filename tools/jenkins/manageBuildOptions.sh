#!/bin/bash
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2016 INRIA and Alexandre Gauthier
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

if [ -z "${TMP_PATH:-}" ]; then
    echo "TMP_PATH unset"
    exit 1
fi

BUILD_OPTIONS_BASENAME="buildOptions.sh"
BUILD_OPTIONS_FILE="$TMP_PATH/$BUILD_OPTIONS_BASENAME"

function ensureBuildOptionFileExists() {
    bash createBuildOptionsFile.sh "$BUILD_OPTIONS_FILE"
}

function updateBuildOptions() {
    if [ ! -f "$BUILD_OPTIONS_FILE" ]; then
        echo "WARNING: $BUILD_OPTIONS_FILE is required, please create it with createBuildOptionsFile.sh"
        exit 1
    fi
    source "$BUILD_OPTIONS_FILE"
}

# Set a build option in the build option file so it can be retrieved at different places.
# The file is sourced, so the value is shell-quoted (it may contain spaces, e.g.
# NATRON_EXTRA_CMAKE_FLAGS), then escaped for the sed replacement (\, & and /).
function setBuildOption() {
    local value
    value=$(printf '%q' "$2" | sed -e 's/[\/&\\]/\\&/g')
    $GSED -e "s/^ *${1//\//\\/}=.*/${1//\//\\/}=${value}/" --in-place "$BUILD_OPTIONS_FILE"
}

function dumpBuildOptions() {
    if [ -f "$BUILD_OPTIONS_FILE" ]; then
        cat "$BUILD_OPTIONS_FILE"
    fi
}

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
