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

TARGET = Serialization
TEMPLATE = lib
CONFIG += staticlib
QT += core network gui opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += concurrent

CONFIG += moc
CONFIG += boost qt python

include(../global.pri)
include(../libs.pri)
include(../config.pri)

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
    CacheSerialization.h \
    CurveSerialization.h \
    FormatSerialization.h \
    FrameKeySerialization.h \
    FrameParamsSerialization.h \
    ImageKeySerialization.h \
    ImageParamsSerialization.h \
    KnobSerialization.h \
    NodeSerialization.h \
    NodeBackdropSerialization.h \
    NodeClipBoard.h \
    NodeGroupSerialization.h \
    NodeGuiSerialization.h \
    NodeSerialization.h \
    NonKeyParamsSerialization.h \
    ProjectGuiSerialization.h \
    ProjectSerialization.h \
    RectDSerialization.h \
    RectISerialization.h \
    RotoContextSerialization.h \
    RotoDrawableItemSerialization.h \
    RotoItemSerialization.h \
    RotoLayerSerialization.h \
    RotoStrokeItemSerialization.h \
    SerializationBase.h \
    SerializationFwd.h \
    SerializationIO.h \
    TextureRectSerialization.h \
    TrackerSerialization.h \
    WorkspaceSerialization.h


SOURCES += \
    KnobSerialization.cpp \
    ProjectGuiSerialization.cpp \
    BezierCPSerialization.cpp \
    BezierSerialization.cpp \
    CurveSerialization.cpp \
    FormatSerialization.cpp \
    FrameKeySerialization.cpp \
    FrameParamsSerialization.cpp \
    ImageKeySerialization.cpp \
    ImageParamsSerialization.cpp \
    NodeSerialization.cpp \
    NodeClipBoard.cpp \
    NonKeyParamsSerialization.cpp \
    ProjectSerialization.cpp \
    RectDSerialization.cpp \
    RectISerialization.cpp \
    RotoContextSerialization.cpp \
    RotoDrawableItemSerialization.cpp \
    RotoItemSerialization.cpp \
    RotoLayerSerialization.cpp \
    RotoStrokeItemSerialization.cpp \
    SerializationIO.cpp \
    TextureRectSerialization.cpp \
    TrackerSerialization.cpp \
    WorkspaceSerialization.cpp
