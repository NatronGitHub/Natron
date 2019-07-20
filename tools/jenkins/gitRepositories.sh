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


# List here the repositories
GIT_URL_OPENFX_OPENCV_GITHUB=https://github.com/NatronGitHub/openfx-opencv.git
GIT_URL_NATRON_GITHUB=https://github.com/NatronGitHub/Natron.git
GIT_URL_NATRON_GFORGE=git+ssh://natron-ci@scm.gforge.inria.fr/gitroot/natron/natron.git
GIT_URL_NATRON_BITBUCKET=git@bitbucket.org:NatronGitHub/comper.git
GIT_URL_OPENFX_IO_GITHUB=https://github.com/NatronGitHub/openfx-io.git
GIT_URL_OPENFX_MISC_GITHUB=https://github.com/NatronGitHub/openfx-misc.git
GIT_URL_OPENFX_ARENA_GITHUB=https://github.com/NatronGitHub/openfx-arena.git
GIT_URL_OPENFX_GMIC_GITHUB=https://github.com/NatronGitHub/openfx-gmic.git
GIT_URL_NATRON_TESTS_GITHUB=https://github.com/NatronGitHub/Natron-Tests.git

# List of known Natron repositories
KNOWN_NATRON_REPOSITORIES="$GIT_URL_NATRON_GITHUB $GIT_URL_NATRON_GFORGE $GIT_URL_NATRON_BITBUCKET"

function setGitUrlsForNatronUrl() {
    GIT_URL_NATRON="$1"
    if [ "$GIT_URL_NATRON" = "$GIT_URL_NATRON_GITHUB" ]; then
        GIT_URL_OPENFX_IO=$GIT_URL_OPENFX_IO_GITHUB
        GIT_URL_OPENFX_ARENA=$GIT_URL_OPENFX_ARENA_GITHUB
        GIT_URL_OPENFX_GMIC=$GIT_URL_OPENFX_GMIC_GITHUB
        GIT_URL_OPENFX_MISC=$GIT_URL_OPENFX_MISC_GITHUB
        GIT_URL_OPENFX_OPENCV=$GIT_URL_OPENFX_OPENCV_GITHUB
        return 0
    elif [ "$GIT_URL_NATRON" = "$GIT_URL_NATRON_GFORGE" ]; then
        GIT_URL_OPENFX_IO=$GIT_URL_OPENFX_IO_GITHUB
        GIT_URL_OPENFX_ARENA=$GIT_URL_OPENFX_ARENA_GITHUB
        GIT_URL_OPENFX_GMIC=$GIT_URL_OPENFX_GMIC_GITHUB
        GIT_URL_OPENFX_MISC=$GIT_URL_OPENFX_MISC_GITHUB
        GIT_URL_OPENFX_OPENCV=$GIT_URL_OPENFX_OPENCV_GITHUB
        return 0
    elif [ "$GIT_URL_NATRON" = "$GIT_URL_NATRON_BITBUCKET" ]; then
        GIT_URL_OPENFX_IO=$GIT_URL_OPENFX_IO_GITHUB
        GIT_URL_OPENFX_ARENA=$GIT_URL_OPENFX_ARENA_GITHUB
        GIT_URL_OPENFX_GMIC=$GIT_URL_OPENFX_GMIC_GITHUB
        GIT_URL_OPENFX_MISC=$GIT_URL_OPENFX_MISC_GITHUB
        GIT_URL_OPENFX_OPENCV=$GIT_URL_OPENFX_OPENCV_GITHUB
        return 0
    fi
    return 1
}

function setGitUrlsForPluginCI() {
    GIT_URL_NATRON=$GIT_URL_NATRON_GITHUB
    GIT_URL_OPENFX_IO=$GIT_URL_OPENFX_IO_GITHUB
    GIT_URL_OPENFX_ARENA=$GIT_URL_OPENFX_ARENA_GITHUB
    GIT_URL_OPENFX_GMIC=$GIT_URL_OPENFX_GMIC_GITHUB
    GIT_URL_OPENFX_MISC=$GIT_URL_OPENFX_MISC_GITHUB
    GIT_URL_OPENFX_OPENCV=$GIT_URL_OPENFX_OPENCV_GITHUB
}

# Function to determine if a git url is a known Natron repository
# Usage:
# IS_GIT_URL_NATRON_REPO=0
# isNatronRepo "$GIT_URL" || IS_GIT_URL_NATRON_REPO=1
function isNatronRepo() {
    for i in $KNOWN_NATRON_REPOSITORIES;do
        if [ "$i" = "$1" ]; then
            return 1
        fi
    done
    return 0
}


# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
