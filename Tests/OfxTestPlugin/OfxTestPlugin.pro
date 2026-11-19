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

# A minimal OFX plug-in bundle for the tests (see NatronTestPlugin.cpp and Tests/OfxTestPlugin.h).
# It is assembled in the build tree and never installed.

TEMPLATE = lib
CONFIG += plugin no_plugin_name_prefix
CONFIG -= qt app_bundle lib_bundle
CONFIG += c++17
TARGET = NatronTestPlugin

include(OfxTestPlugin.pri)

INCLUDEPATH += ../../libs/OpenFX/include
DESTDIR = $${NATRON_TEST_OFX_BINARY_DIR}
QMAKE_EXTENSION_SHLIB = ofx
win32: TARGET_EXT = .ofx

SOURCES += NatronTestPlugin.cpp
