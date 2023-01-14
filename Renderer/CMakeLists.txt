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

set(NatronRenderer_SOURCES NatronRenderer_main.cpp)
if(WINDOWS)
    list(APPEND NatronRenderer_SOURCES ../Natron.rc)
endif()
add_executable(NatronRenderer ${NatronRenderer_SOURCES})
target_link_libraries(NatronRenderer
    PRIVATE
        NatronEngine
        Qt5::Core
        Qt5::Concurrent
        Qt5::Network
        Python3::Python
)
target_include_directories(NatronRenderer
    PRIVATE
        ..
        ../Engine
        ../Global
)

install(TARGETS NatronRenderer DESTINATION "${CMAKE_INSTALL_BINDIR}")
