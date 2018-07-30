# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://natrongithub.github.io/>,
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

TARGET = NatronRenderer
CONFIG += console
CONFIG -= app_bundle
# Cairo is still the default renderer for Roto
!enable-osmesa {
   CONFIG += enable-cairo
}
CONFIG += moc
CONFIG += boost qt python shiboken pyside osmesa fontconfig
enable-cairo: CONFIG += cairo
CONFIG += static-yaml-cpp static-engine static-host-support static-serialization static-breakpadclient static-libmv static-openmvg static-ceres static-libtess

!noexpat: CONFIG += expat

TEMPLATE = app

win32 {
	RC_FILE += ../Natron.rc
}

include(../global.pri)

SOURCES += \
    NatronRenderer_main.cpp

INSTALLS += target

