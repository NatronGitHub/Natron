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

TARGET = Natron

# the list of currently maintained versions (those that have to be merged into the master branch)
VERSION_21 = 2.1.10
VERSION_22 = 2.2.10
VERSION_23 = 2.3.13
VERSION_30 = 3.0.0

# The version for this branch
VERSION = $$VERSION_23

TEMPLATE = app
win32 {
	CONFIG += console
	RC_FILE += ../Natron.rc
} else {
	CONFIG += app
}
CONFIG += moc
CONFIG += boost boost-serialization-lib opengl qt cairo python shiboken pyside 
CONFIG += static-gui static-engine static-host-support static-breakpadclient static-libmv static-openmvg static-ceres static-qhttpserver static-libtess

QT += gui core opengl network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets concurrent

!noexpat: CONFIG += expat

macx {
  ### custom variables for the Info.plist file
  # use a custom Info.plist template
  QMAKE_INFO_PLIST = NatronInfo.plist
  # Set the application icon
  ICON = ../Gui/Resources/Images/natronIcon.icns
  # replace com.yourcompany with something more meaningful
  QMAKE_TARGET_BUNDLE_PREFIX = fr.inria
  QMAKE_PKGINFO_TYPEINFO = Ntrn
}



win32-msvc* {
	CONFIG(64bit) {
		QMAKE_LFLAGS += /MACHINE:X64
	} else {
		QMAKE_LFLAGS += /MACHINE:X86
	}
        QMAKE_LFLAGS += /ENTRY:"mainCRTStartup"
}


include(../OpenColorIO-Configs.pri)
include(../global.pri)

macx {
    Resources.files += $$PWD/../Gui/Resources/Images/natronProjectIcon_osx.icns
    Resources.path = Contents/Resources
    QMAKE_BUNDLE_DATA += Resources
    Fontconfig.files = $$PWD/../Gui/Resources/etc/fonts
    Fontconfig.path = Contents/Resources/etc
    QMAKE_BUNDLE_DATA += Fontconfig
}
!macx {
    Resources.path = $$OUT_PWD
    INSTALLS += Resources
}



win32-g++ {
#Gcc is very picky here, if we include these libraries before the includes commands above, it will yield tons of unresolved externals
	LIBS += -lmpr
	#MingW needs to link against fontconfig explicitly since in Msys2 Qt does not link against fontconfig
        LIBS +=  -lopengl32 -lfontconfig
}

SOURCES += \
    NatronApp_main.cpp

INSTALLS += target



