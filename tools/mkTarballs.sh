#!/bin/sh
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# Copyright (C) 2018-2022 The Natron developers
# Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

# Creates source tarballs from release tags for Natron and plugins

# release tag
RELEASE=${1:-}
if [ -z "$RELEASE" ]; then
  echo "Please specify a release (x.y.z)"
  exit 1
fi

echo "Creating tarballs for release ${RELEASE} ..."

# tmp dir
TMP_DIR=/tmp/Natron-sources-${RELEASE}
rm -rf "$TMP_DIR" || true
if [ ! -d "$TMP_DIR" ]; then
  mkdir -p "$TMP_DIR" || exit 1
fi

# repos
REPOS="
https://github.com/NatronGitHub/Natron
https://github.com/NatronGitHub/openfx-io
https://github.com/NatronGitHub/openfx-misc
https://github.com/NatronGitHub/openfx-gmic
https://github.com/NatronGitHub/openfx-arena"

cd "$TMP_DIR" || exit 1

# clone repos
for repo in $REPOS; do
  git clone $repo || exit 1
done

# checkout and make tarballs
for folder in *; do
  if [ -d "$folder" ]; then
    mv $folder ${folder}-${RELEASE} || exit 1
    cd ${folder}-${RELEASE} || exit 1
    if [ "$folder" = "Natron" ]; then
      git checkout $RELEASE || exit 1
    else
      git checkout Natron-${RELEASE} || exit 1
    fi
    git submodule update -i --recursive || exit 1
    find . -name .git -print0 | xargs -0 rm -rf || exit 1
    cd "$TMP_DIR" || exit 1 
    tar cvvJf ${folder}-${RELEASE}.tar.xz ${folder}-${RELEASE} || exit 1
    rm -rf ${folder}-${RELEASE} || exit 1
  fi
done

# result
echo "Result in ${TMP_DIR}:"
ls -lah "$TMP_DIR"

