#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TEMPLATE = subdirs

# build things in the order we give
CONFIG += ordered

SUBDIRS += \
    HostSupport \
    Engine \
    Gui \
    Renderer \
    Tests \
    App \
    CrashReporter

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
