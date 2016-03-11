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

# This script has to be run after uncrustify, to fix various glitches.
# Usage:
# - run uncrustify first
#  uncrustify -c tools/uncrustify.cfg --replace */*.cpp */*.h
# - fix the output of uncrustify using this script
#  sed -f tools/uncrustify-post.sed -i .bak */*.cpp */*.h

# arguments to CLANG_DIAG_ON, CLANG_DIAG_OFF, GCC_DIAG_ON, GCC_DIAG_OFF should have no spaces around "-"
/CLANG_DIAG_ON/ s/ - /-/g
/CLANG_DIAG_OFF/ s/ - /-/g
/GCC_DIAG_ON/ s/ - /-/g
/GCC_DIAG_OFF/ s/ - /-/g
