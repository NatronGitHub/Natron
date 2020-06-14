# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2020 The Natron Developers
# (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

TARGET = NatronProjectConverter
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
# Cairo is still the default renderer for Roto
!enable-osmesa {
   CONFIG += enable-cairo
}
CONFIG += moc rcc
CONFIG += boost-serialization-lib boost opengl qt python shiboken pyside osmesa fontconfig
enable-cairo: CONFIG += cairo
CONFIG += static-yaml-cpp static-gui static-engine static-serialization static-host-support static-breakpadclient static-libmv static-openmvg static-ceres static-libtess
QT += gui core opengl network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets concurrent

CONFIG += openmvg-flags glad-flags yaml-cpp-flags

!noexpat: CONFIG += expat

DEFINES += NATRON_BOOST_SERIALIZATION_COMPAT

INCLUDEPATH += google-test/include
INCLUDEPATH += google-test
INCLUDEPATH += google-mock/include
INCLUDEPATH += google-mock


include(../global.pri)

SOURCES += \
    ProjectConverter_main.cpp \
    ../Serialization/SerializationCompat.cpp
#HEADERS += \
