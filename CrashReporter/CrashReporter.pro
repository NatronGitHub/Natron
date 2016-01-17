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

# The binary name needs to be Natron as this is what the user lauches
# It is renamed during deployment
TARGET = NatronCrashReporter
VERSION = 2.0.0
TEMPLATE = app

# NatronCrashReporter is built as an app to make debugging easier:
# - on Linux and OSX, make a symbolic link to the Natron binary
#  (which is inside Natron.app on OSX) next to the NatronCrashReporter binary
#  example on OSX:
#  $ cd build/Debug/NatronCrashReporter.app/Contents/MacOS
#  $ ln -s ../../../../../../App/build/Debug/Natron.app/Contents/MacOS/Natron Natron-bin
macx {
CONFIG += app
} else {
CONFIG += console
CONFIG -= app_bundle
CONFIG -= app
}

CONFIG += moc
CONFIG += qt
QT       += core network gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

include(../global.pri)

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
