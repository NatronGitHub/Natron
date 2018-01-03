# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
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

QT       += core network
QT       -= gui
greaterThan(QT_MAJOR_VERSION, 4): QT += concurrent

TARGET = natron-python
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += python

TEMPLATE = app

include(../global.pri)


INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD/../Global

SOURCES += \
    python_main.cpp \
    ../Global/FStreamsSupport.cpp \
    ../Global/PythonUtils.cpp \
    ../Global/ProcInfo.cpp \
    ../Global/StrUtils.cpp 



HEADERS += \
    ../Global/FStreamsSupport.h \
    ../Global/fstream_mingw.h \
    ../Global/PythonUtils.h \
    ../Global/ProcInfo.h \
    ../Global/StrUtils.h


INSTALLS += target

