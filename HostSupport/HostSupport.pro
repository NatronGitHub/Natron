#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TARGET = HostSupport
TEMPLATE = lib
CONFIG += staticlib
CONFIG += expat
CONFIG -= qt

include(../global.pri)
include(../config.pri)


#OpenFX C api includes and OpenFX c++ layer includes that are located in the submodule under /libs/OpenFX
INCLUDEPATH += $$PWD/../libs/OpenFX/include
INCLUDEPATH += $$PWD/../libs/OpenFX_extensions
INCLUDEPATH += $$PWD/../libs/OpenFX/HostSupport/include

win32-msvc* {
	CONFIG(64bit) {
		QMAKE_LFLAGS += /MACHINE:X64
	} else {
		QMAKE_LFLAGS += /MACHINE:X86
	}
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
    ../libs/OpenFX/include/ofxImageEffect.h \
    ../libs/OpenFX/include/ofxInteract.h \
    ../libs/OpenFX/include/ofxKeySyms.h \
    ../libs/OpenFX/include/ofxMemory.h \
    ../libs/OpenFX/include/ofxMessage.h \
    ../libs/OpenFX/include/ofxMultiThread.h \
    ../libs/OpenFX/include/ofxOpenGLRender.h \
    ../libs/OpenFX/include/ofxParam.h \
    ../libs/OpenFX/include/ofxParametricParam.h \
    ../libs/OpenFX/include/ofxPixels.h \
    ../libs/OpenFX/include/ofxProgress.h \
    ../libs/OpenFX/include/ofxProperty.h \
    ../libs/OpenFX/include/ofxTimeLine.h \
    ../libs/OpenFX_extensions/tuttle/ofxGraphAPI.h \
    ../libs/OpenFX_extensions/tuttle/ofxMetadata.h \
    ../libs/OpenFX_extensions/tuttle/ofxParam.h \
    ../libs/OpenFX_extensions/tuttle/ofxParamAPI.h \
    ../libs/OpenFX_extensions/tuttle/ofxReadWrite.h \
    ../libs/OpenFX_extensions/ofxhParametricParam.h \
    ../libs/OpenFX/include/natron/IOExtensions.h
