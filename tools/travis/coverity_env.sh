# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2015 INRIA and Alexandre Gauthier
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

if [[ ${COVERITY_SCAN_BRANCH} == 1 ]]; then
    if [[ ${TRAVIS_OS_NAME} == "linux" &&
          ${CC} == "gcc"
       ]]; then
        echo "Coverity: build enabled"
    else
        export COVERITY_BUILD_DISABLED=1
        echo "Coverity: build disabled"
    fi
else
    echo "No Coverity"
fi

