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

TARGET = Serialization
TEMPLATE = lib
CONFIG += staticlib
QT += core network gui opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += concurrent

CONFIG += moc
CONFIG += boost qt python

CONFIG += yaml-cpp-flags

include(../global.pri)

INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD/../Global

INCLUDEPATH += $$PWD/../libs/OpenFX/include
DEPENDPATH  += $$PWD/../libs/OpenFX/include
INCLUDEPATH += $$PWD/../libs/OpenFX_extensions
DEPENDPATH  += $$PWD/../libs/OpenFX_extensions
INCLUDEPATH += $$PWD/../libs/OpenFX/HostSupport/include
DEPENDPATH  += $$PWD/../libs/OpenFX/HostSupport/include
INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD/../Global
INCLUDEPATH += $$PWD/../libs/SequenceParsing


HEADERS += \
    BezierSerialization.h \
    BezierCPSerialization.h \
    CurveSerialization.h \
    FormatSerialization.h \
    KnobSerialization.h \
    KnobTableItemSerialization.h \
    NodeSerialization.h \
    NodeBackdropSerialization.h \
    NodeClipBoard.h \
    NodeGuiSerialization.h \
    NodeSerialization.h \
    ProjectGuiSerialization.h \
    ProjectSerialization.h \
    RectDSerialization.h \
    RectISerialization.h \
    RotoStrokeItemSerialization.h \
    SettingsSerialization.h \
    SerializationBase.h \
    SerializationFwd.h \
    SerializationIO.h \
    SerializationCompat.h \
    WorkspaceSerialization.h


SOURCES += \
    BezierCPSerialization.cpp \
    BezierSerialization.cpp \
    CurveSerialization.cpp \
    FormatSerialization.cpp \
    KnobSerialization.cpp \
    KnobTableItemSerialization.cpp \
    NodeSerialization.cpp \
    NodeClipBoard.cpp \
    ProjectSerialization.cpp \
    RectDSerialization.cpp \
    RectISerialization.cpp \
    RotoStrokeItemSerialization.cpp \
    SettingsSerialization.cpp \
    WorkspaceSerialization.cpp
