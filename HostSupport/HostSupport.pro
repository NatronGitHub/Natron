# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2021 The Natron developers
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

TARGET = HostSupport
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

include(../global.pri)

!noexpat: CONFIG += expat
 
contains(CONFIG,trace_ofx_actions) {
    DEFINES += OFX_DEBUG_ACTIONS
}

contains(CONFIG,trace_ofx_params) {
    DEFINES += OFX_DEBUG_PARAMETERS
}

contains(CONFIG,trace_ofx_properties) {
    DEFINES += OFX_DEBUG_PROPERTIES
}

precompile_header {
  #message("Using precompiled header")
  # Use Precompiled headers (PCH)
  # we specify PRECOMPILED_DIR, or qmake places precompiled headers in Natron/c++.pch, thus blocking the creation of the Unix executable
  PRECOMPILED_DIR = pch
  PRECOMPILED_HEADER = pch.h
}

#OpenFX C api includes and OpenFX c++ layer includes that are located in the submodule under /libs/OpenFX
INCLUDEPATH += $$PWD/../libs/OpenFX/include
INCLUDEPATH += $$PWD/../libs/OpenFX_extensions
INCLUDEPATH += $$PWD/../libs/OpenFX/HostSupport/include
INCLUDEPATH += $$PWD/../libs/OpenFX/include/nuke
INCLUDEPATH += $$PWD/../libs/OpenFX/include/tuttle

win32-msvc* {
	CONFIG(64bit) {
		QMAKE_LFLAGS += /MACHINE:X64
	} else {
		QMAKE_LFLAGS += /MACHINE:X86
	}
}

win32 {
	DEFINES *= WIN32
	CONFIG(64bit){
		DEFINES *= WIN64
	}
}

noexpat {
    SOURCES += \
    ../libs/OpenFX/HostSupport/expat-2.1.0/lib/xmlparse.c \
    ../libs/OpenFX/HostSupport/expat-2.1.0/lib/xmltok.c \
    ../libs/OpenFX/HostSupport/expat-2.1.0/lib/xmltok_impl.c \
    
    HEADERS += \
        ../libs/OpenFX/HostSupport/expat-2.1.0/lib/expat.h \
        ../libs/OpenFX/HostSupport/expat-2.1.0/lib/expat_external.h \
        ../libs/OpenFX/HostSupport/expat-2.1.0/lib/ascii.h \
        ../libs/OpenFX/HostSupport/expat-2.1.0/lib/xmltok.h \
        ../libs/OpenFX/HostSupport/expat-2.1.0/lib/xmltok_impl.h \
        ../libs/OpenFX/HostSupport/expat-2.1.0/lib/asciitab.h \
        ../libs/OpenFX/HostSupport/expat-2.1.0/expat_config.h \

    DEFINES += HAVE_EXPAT_CONFIG_H

    INCLUDEPATH += $$PWD/../libs/OpenFX/HostSupport/expat-2.1.0/lib
}

SOURCES += \
    ../libs/OpenFX/HostSupport/src/ofxhBinary.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhClip.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhHost.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhImageEffect.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhImageEffectAPI.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhInteract.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhMemory.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhParam.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhPluginAPICache.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhPluginCache.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhPropertySuite.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhUtilities.cpp \
    ../libs/OpenFX_extensions/ofxhParametricParam.cpp

HEADERS += \
    ../libs/OpenFX/HostSupport/include/ofxhBinary.h \
    ../libs/OpenFX/HostSupport/include/ofxhClip.h \
    ../libs/OpenFX/HostSupport/include/ofxhHost.h \
    ../libs/OpenFX/HostSupport/include/ofxhImageEffect.h \
    ../libs/OpenFX/HostSupport/include/ofxhImageEffectAPI.h \
    ../libs/OpenFX/HostSupport/include/ofxhInteract.h \
    ../libs/OpenFX/HostSupport/include/ofxhMemory.h \
    ../libs/OpenFX/HostSupport/include/ofxhParam.h \
    ../libs/OpenFX/HostSupport/include/ofxhPluginAPICache.h \
    ../libs/OpenFX/HostSupport/include/ofxhPluginCache.h \
    ../libs/OpenFX/HostSupport/include/ofxhProgress.h \
    ../libs/OpenFX/HostSupport/include/ofxhPropertySuite.h \
    ../libs/OpenFX/HostSupport/include/ofxhTimeLine.h \
    ../libs/OpenFX/HostSupport/include/ofxhUtilities.h \
    ../libs/OpenFX/HostSupport/include/ofxhXml.h \
    ../libs/OpenFX/include/ofxCore.h \
    ../libs/OpenFX/include/ofxDialog.h \
    ../libs/OpenFX/include/ofxImageEffect.h \
    ../libs/OpenFX/include/ofxInteract.h \
    ../libs/OpenFX/include/ofxKeySyms.h \
    ../libs/OpenFX/include/ofxMemory.h \
    ../libs/OpenFX/include/ofxMessage.h \
    ../libs/OpenFX/include/ofxMultiThread.h \
    ../libs/OpenFX/include/ofxNatron.h \
    ../libs/OpenFX/include/ofxOpenGLRender.h \
    ../libs/OpenFX/include/ofxParam.h \
    ../libs/OpenFX/include/ofxParametricParam.h \
    ../libs/OpenFX/include/ofxPixels.h \
    ../libs/OpenFX/include/ofxProgress.h \
    ../libs/OpenFX/include/ofxProperty.h \
    ../libs/OpenFX/include/ofxSonyVegas.h \
    ../libs/OpenFX/include/ofxTimeLine.h \
    ../libs/OpenFX/include/nuke/camera.h \
    ../libs/OpenFX/include/nuke/fnOfxExtensions.h \
    ../libs/OpenFX/include/nuke/fnPublicOfxExtensions.h \
    ../libs/OpenFX/include/tuttle/ofxReadWrite.h \
    ../libs/OpenFX_extensions/ofxhParametricParam.h
