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

# Where the OFX test plug-in bundle is assembled, in the build tree's Tests/OfxPlugins (shared by
# OfxTestPlugin.pro and Tests.pro; the CMake build does the same in Tests/CMakeLists.txt). The bundle
# architecture folder follows the OFX packaging spec, as HostSupport looks for it.
macx {
    NATRON_TEST_OFX_ARCHDIR = MacOS
} else:win32 {
    contains(QT_ARCH, x86_64) {
        NATRON_TEST_OFX_ARCHDIR = Win64
    } else {
        NATRON_TEST_OFX_ARCHDIR = Win32
    }
} else {
    freebsd: NATRON_TEST_OFX_OS = FreeBSD
    else: NATRON_TEST_OFX_OS = Linux
    contains(QT_ARCH, x86_64) {
        NATRON_TEST_OFX_ARCHDIR = $${NATRON_TEST_OFX_OS}-x86-64
    } else:contains(QT_ARCH, arm64) {
        NATRON_TEST_OFX_ARCHDIR = $${NATRON_TEST_OFX_OS}-aarch64
    } else {
        NATRON_TEST_OFX_ARCHDIR = $${NATRON_TEST_OFX_OS}-$${QT_ARCH}
    }
}
# OUT_PWD is the build folder of the project that includes this file: Tests or Tests/OfxTestPlugin
equals(TARGET, NatronTestPlugin) {
    NATRON_TEST_OFX_PLUGIN_DIR = $${OUT_PWD}/../OfxPlugins
} else {
    NATRON_TEST_OFX_PLUGIN_DIR = $${OUT_PWD}/OfxPlugins
}
NATRON_TEST_OFX_BINARY_DIR = $${NATRON_TEST_OFX_PLUGIN_DIR}/NatronTestPlugin.ofx.bundle/Contents/$${NATRON_TEST_OFX_ARCHDIR}
