# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2026 The Natron developers
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

# md4c, the CommonMark Markdown parser, and its HTML renderer (see README.Natron).
# libs/md4c/md4c.pro and the md4c-flags section of libs.pri are the qmake
# equivalents: keep them in sync, including the version below.
set(md4c_VERSION "0.6.0-46-gf9e5eb2")
set(md4c_HEADERS
    src/entity.h
    src/md4c.h
    src/md4c-html.h
)
set(md4c_SOURCES
    src/entity.c
    src/md4c.c
    src/md4c-html.c
)
add_library(md4c STATIC ${md4c_SOURCES} ${md4c_HEADERS})
target_include_directories(md4c SYSTEM PUBLIC src)
target_compile_definitions(md4c PUBLIC NATRON_MD4C_VERSION="${md4c_VERSION}")
target_compile_options(md4c PRIVATE ${NATRON_THIRD_PARTY_NO_WARNINGS})
set_target_properties(md4c PROPERTIES POSITION_INDEPENDENT_CODE ON)
