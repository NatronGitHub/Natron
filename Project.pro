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

TEMPLATE = subdirs

# build things in the order we give
CONFIG += ordered

gbreakpad: SUBDIRS += BreakpadClient CrashReporter CrashReporterCLI

SUBDIRS += \
    HostSupport \
    Engine \
    Renderer \
    Gui \
    Tests \
    App

OTHER_FILES += \
    Global/Enums.h \
    Global/GLIncludes.h \
    Global/GlobalDefines.h \
    Global/KeySymbols.h \
    Global/Macros.h \
    Global/MemoryInfo.h \
    Global/QtCompat.h \
    global.pri \
    config.pri

include(global.pri)
include(config.pri)

*-xcode {
  # Qt 4.8.5's XCode generator has a bug and places moc_*.cpp files next to the sources instead of inside the build dir
  # However, setting the MOC_DIR doesn't fix that (Xcode build fails)
  # Simple rtule: don't use Xcode
  #MOC_DIR = $$OUT_PWD
  warning("Xcode generator wrongly places the moc files in the source directory. You thus cannot compile with different Qt versions using Xcode.")
}

CONFIG(debug, debug|release){
    message("Compiling in DEBUG mode.")
} else {
    message("Compiling in RELEASE mode.")
}
