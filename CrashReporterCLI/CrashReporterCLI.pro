# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# Copyright (C) 2018-2023 The Natron developers
# Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

# The binary name needs to be NatronRenderer as this is what the user lauches
# It is renamed during deployment
TARGET = NatronRendererCrashReporter
QT       += core network
QT       -= gui

# - on Linux and OSX, make a symbolic link to the NatronRenderer binary
#  (which is inside Natron.app on OSX) next to the NatronRendererCrashReporter binary
#  example on OSX:
#  $ cd build/Debug
#  $ ln -s ../../Renderer/build/Debug/NatronRenderer NatronRenderer-bin
win32 {
        RC_FILE += ../Natron.rc
}

CONFIG += console
CONFIG -= app_bundle
CONFIG += moc
CONFIG += qt
CONFIG += static-breakpadclient
TEMPLATE = app

include(../global.pri)

DEFINES *= REPORTER_CLI_ONLY

#used by breakpad internals
unix:!mac {
    DEFINES += N_UNDF=0
}

#google-breakpad use this to determine if build is debug
win32:Debug: DEFINES *= _DEBUG 


SOURCES += \
        ../CrashReporter/main.cpp \
        ../CrashReporter/CallbacksManager.cpp \
        ../Global/ProcInfo.cpp \
        ../Global/StrUtils.cpp

HEADERS += ../CrashReporter/CallbacksManager.h \
           ../Global/ProcInfo.h \
           ../Global/Macros.h \
           ../Global/StrUtils.h

INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD/../Global
DEPENDPATH += $$PWD/../Global

INSTALLS += target
