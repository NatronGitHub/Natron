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

set(Tests_HEADERS BaseTest.h)
set(Tests_SOURCES
    google-test/src/gtest-all.cc
    google-mock/src/gmock-all.cc
    BaseTest.cpp
    Hash64_Test.cpp
    Image_Test.cpp
    Lut_Test.cpp
    KnobFile_Test.cpp
    Curve_Test.cpp
    Tracker_Test.cpp
    wmain.cpp
)
add_executable(Tests ${Tests_HEADERS} ${Tests_SOURCES})
target_link_libraries(Tests
    PRIVATE
        Python3::Python
        NatronGui
        openMVG
)
target_include_directories(Tests
    PRIVATE
        ..
        ../libs/SequenceParsing
        google-test/include
        google-test
        google-mock/include
        google-mock
)
add_test(NAME Tests COMMAND Tests)
