# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2023 The Natron developers
# (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

set(hoedown_HEADERS
    src/autolink.h
    src/buffer.h
    src/document.h
    src/escape.h
    src/html.h
    src/stack.h
    src/version.h
)
set(hoedown_SOURCES
    src/autolink.c
    src/buffer.c
    src/document.c
    src/escape.c
    src/html.c
    src/html_blocks.c
    src/html_smartypants.c
    src/stack.c
    src/version.c
)
add_library(hoedown STATIC ${hoedown_SOURCES} ${hoedown_HEADERS})
target_include_directories(hoedown PUBLIC src)
set_target_properties(hoedown PROPERTIES POSITION_INDEPENDENT_CODE ON)
