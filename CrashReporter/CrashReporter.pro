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

win32 {
    CONFIG += console
    RC_FILE += ../Natron.rc
} else {
    CONFIG += app
}
#CONFIG += console
#CONFIG -= app_bundle
#CONFIG -= app


CONFIG += moc
CONFIG += qt
QT       += core network gui opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += static-breakpadclient

include(../global.pri)

#used by breakpad internals on Linux
unix:!mac: DEFINES += N_UNDF=0

#Windows: google-breakpad use this to determine if build is debug
win32:Debug: DEFINES *= _DEBUG 

INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD/../Global
DEPENDPATH += $$PWD/../Global


SOURCES += \
    CrashDialog.cpp \
    main.cpp \
    CallbacksManager.cpp \
    ../Global/ProcInfo.cpp \
    ../Global/StrUtils.cpp

HEADERS += \
    CrashDialog.h \
    CallbacksManager.h \
    ../Global/Macros.h \
    ../Global/ProcInfo.h \
    ../Global/StrUtils.h



RESOURCES += \
    ../Gui/GuiResources.qrc

INSTALLS += target
