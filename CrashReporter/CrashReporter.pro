# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#The binary name needs to be Natron as this is what the user lauches
TARGET = Natron
VERSION = 2.0.0
TEMPLATE = app

QT       += core network gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += app
CONFIG += moc
CONFIG += qt


win32 {
    RC_FILE += ../Natron.rc
}

include(../global.pri)


macx {
  ### custom variables for the Info.plist file
  # use a custom Info.plist template
  QMAKE_INFO_PLIST = NatronInfo.plist
  # Set the application icon
  ICON = ../Gui/Resources/Images/natronIcon256_osx.icns
  # replace com.yourcompany with something more meaningful
  QMAKE_TARGET_BUNDLE_PREFIX = fr.inria
  QMAKE_PKGINFO_TYPEINFO = Ntrn
}



#used by breakpad internals on Linux
unix:!mac: DEFINES += N_UNDF=0

#Windows: google-breakpad use this to determine if build is debug
win32:Debug: DEFINES *= _DEBUG 

INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD/../Global
DEPENDPATH += $$PWD/../Global


BREAKPAD_PATH = $$PWD/../google-breakpad/src
INCLUDEPATH += $$BREAKPAD_PATH
DEPENDPATH += $$BREAKPAD_PATH

SOURCES += \
    CrashDialog.cpp \
    main.cpp \
    CallbacksManager.cpp \
    ../Global/ProcInfo.cpp

HEADERS += \
    CrashDialog.h \
    CallbacksManager.h \
    ../Global/Macros.h \
    ../Global/ProcInfo.h




win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/x64/release/ -lBreakpadClient
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/x64/debug/ -lBreakpadClient
        } else {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/win32/release/ -lBreakpadClient
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/win32/debug/ -lBreakpadClient
        }
} else {
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/release/ -lBreakpadClient
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/debug/ -lBreakpadClient
        else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/build/Release/ -lBreakpadClient
        else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/build/Debug/ -lBreakpadClient
        else:unix: LIBS += -L$$OUT_PWD/../BreakpadClient/ -lBreakpadClient
}



win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/x64/release/libBreakpadClient.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/x64/debug/libBreakpadClient.lib
        } else {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/win32/release/libBreakpadClient.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/win32/debug/libBreakpadClient.lib
        }
} else {
        win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/release/libBreakpadClient.a
        else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/debug/libBreakpadClient.a
        else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/release/BreakpadClient.lib
        else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/debug/BreakpadClient.lib
        else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/build/Release/libBreakpadClient.a
        else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/build/Debug/libBreakpadClient.a
        else:unix: PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/libBreakpadClient.a
}



RESOURCES += \
    ../Gui/GuiResources.qrc

INSTALLS += target


OCIO.files = \
$$PWD/../OpenColorIO-Configs/ChangeLog \
$$PWD/../OpenColorIO-Configs/README \
$$PWD/../OpenColorIO-Configs/aces_0.1.1 \
$$PWD/../OpenColorIO-Configs/aces_0.7.1 \
$$PWD/../OpenColorIO-Configs/blender \
$$PWD/../OpenColorIO-Configs/nuke-default \
$$PWD/../OpenColorIO-Configs/spi-anim \
$$PWD/../OpenColorIO-Configs/spi-vfx

# ACES 1.0.1 also has baked luts and python files which we don't want to bundle
OCIO_aces_101.files = \
$$PWD/../OpenColorIO-Configs/aces_1.0.1/config.ocio \
$$PWD/../OpenColorIO-Configs/aces_1.0.1/luts


macx {
    Resources.files += $$PWD/../Gui/Resources/Images/natronProjectIcon_osx.icns
    Resources.path = Contents/Resources
    QMAKE_BUNDLE_DATA += Resources
    Fontconfig.files = $$PWD/../Gui/Resources/etc/fonts
    Fontconfig.path = Contents/Resources/etc
    QMAKE_BUNDLE_DATA += Fontconfig
    OCIO.path = Contents/Resources/OpenColorIO-Configs
    QMAKE_BUNDLE_DATA += OCIO
    OCIO_aces_101.path = Contents/Resources/OpenColorIO-Configs/aces_1.0.1
    QMAKE_BUNDLE_DATA += OCIO_aces_101
}
!macx {
    Resources.path = $$OUT_PWD
    INSTALLS += Resources
    OCIO.path = $$OUT_PWD/OpenColorIO-Configs
    INSTALLS += OCIO
    OCIO_aces_101.path = $$OUT_PWD/OpenColorIO-Configs/aces_1.0.1
    INSTALLS += OCIO_aces_101
}
