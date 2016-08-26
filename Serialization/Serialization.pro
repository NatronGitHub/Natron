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
include(../../libs.pri)
include(../../config.pri)

INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD/../Global

HEADERS += \
    BezierSerialization.h \
    BezierCPSerialization.h \
    CacheSerialization.h \
    CurveSerialization.h \
    FormatSerialization.h \
    NodeSerialization.h \
    FrameEntrySerialization.h \
    FrameParamsSerialization.h \
    ImageKeySerialization.h \
    ImageParamsSerialization.h \
    KnobSerialization.h \
    NodeBackdropSerialization.h \
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
    TextureRectSerialization.h \
    TrackerSerialization.h


SOURCES += \
    KnobSerialization.cpp \
    ProjectGuiSerialization.cpp
