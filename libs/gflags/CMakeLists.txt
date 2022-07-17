# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2022 The Natron developers
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

set(gflags_HEADERS
    src/config.h
    src/gflags/gflags_completions.h
    src/gflags/gflags_declare.h
    src/gflags/gflags.h
    src/mutex.h
    src/util.h
)
set(gflags_SOURCES
    src/gflags.cc
    src/gflags_completions.cc
    src/gflags_reporting.cc
)
if(WINDOWS)
    set(gflags_HEADERS ${gflags_HEADERS} src/windows_port.h)
    set(gflags_SOURCES ${gflags_SOURCES} src/windows_port.cc)
endif()
add_library(gflags STATIC ${gflags_HEADERS} ${gflags_SOURCES})
target_include_directories(gflags PUBLIC . src src/gflags)
set_target_properties(gflags PROPERTIES POSITION_INDEPENDENT_CODE ON)
