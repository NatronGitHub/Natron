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
TARGET = NatronTestPlugin

# the compiler, deployment target and architecture flags of the rest of the build
include(../../global.pri)
CONFIG -= qt app_bundle lib_bundle

include(OfxTestPlugin.pri)

INCLUDEPATH += $$PWD/../../libs/OpenFX/include
DESTDIR = $${NATRON_TEST_OFX_BINARY_DIR}
QMAKE_EXTENSION_SHLIB = ofx
win32: TARGET_EXT = .ofx

# every source file of this folder is part of the bundle (see NatronTestPluginRegistry.h)
SOURCES += $$files($$PWD/*.cpp)
HEADERS += $$files($$PWD/*.h)

# Copies of the bundle that the host must skip (Tests/OfxPluginScan_Test.cpp, #1380): "Any bundle or
# sub-directory name starting with the character '@' is to be ignored" (ofxPackaging.rst), and a
# directory whose name only contains ".ofx.bundle" is not a bundle. Same as Tests/CMakeLists.txt.
NATRON_TEST_OFX_SKIPPED = \
    @NatronTestPluginDisabled.ofx.bundle/Contents/$${NATRON_TEST_OFX_ARCHDIR}/@NatronTestPluginDisabled.ofx \
    @Disabled/NatronTestPluginInDisabledDir.ofx.bundle/Contents/$${NATRON_TEST_OFX_ARCHDIR}/NatronTestPluginInDisabledDir.ofx \
    NatronTestPluginOld.ofx.bundle.old/Contents/$${NATRON_TEST_OFX_ARCHDIR}/NatronTestPluginOld.ofx
for(skipped, NATRON_TEST_OFX_SKIPPED) {
    skippedPath = $${NATRON_TEST_OFX_PLUGIN_DIR}/$${skipped}
    QMAKE_POST_LINK += $$sprintf($$QMAKE_MKDIR_CMD, $$shell_quote($$shell_path($$dirname(skippedPath)))) $$escape_expand(\\n\\t)
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_quote($$shell_path($${DESTDIR}/$${TARGET}.ofx)) $$shell_quote($$shell_path($${skippedPath})) $$escape_expand(\\n\\t)
}
