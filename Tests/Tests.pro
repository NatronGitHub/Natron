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

TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG += moc rcc
CONFIG += boost glew opengl qt cairo python shiboken pyside
QT += gui core opengl network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets concurrent

!noexpat: CONFIG += expat

INCLUDEPATH += google-test/include
INCLUDEPATH += google-test
INCLUDEPATH += google-mock/include
INCLUDEPATH += google-mock


QMAKE_CLEAN += ofxTestLog.txt test_dot_generator0.jpg

#OpenFX C api includes and OpenFX c++ layer includes that are located in the submodule under /libs/OpenFX
INCLUDEPATH += $$PWD/../libs/OpenFX/include
INCLUDEPATH += $$PWD/../libs/OpenFX_extensions
INCLUDEPATH += $$PWD/../libs/OpenFX/HostSupport/include
INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD/../libs/SequenceParsing

################
# Gui


win32-msvc*{
	CONFIG(64bit) {
		CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Gui/x64/release/ -lGui
		CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Gui/x64/debug/ -lGui
	} else {
		CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Gui/win32/release/ -lGui
		CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Gui/win32/debug/ -lGui
	}
} else {
	win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Gui/release/ -lGui
	else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Gui/debug/ -lGui
	else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Gui/build/Release/ -lGui
	else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Gui/build/Debug/ -lGui
        else:unix {
            LIBS += -L$$OUT_PWD/../Gui/ -lGui
            # GLU is required by ViewerGL, but some versions of glew don't link it (e.g. Ubuntu 12.04)
            !macx: LIBS += -lGLU
        }
}
INCLUDEPATH += $$PWD/../Gui
DEPENDPATH += $$PWD/../Gui

win32-msvc*{
	CONFIG(64bit) {
		CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Gui/x64/release/libGui.lib
		CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Gui/x64/debug/libGui.lib
	} else {
		CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Gui/win32/release/libGui.lib
		CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Gui/win32/debug/libGui.lib
	}
} else {
	win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Gui/release/libGui.a
	else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Gui/debug/libGui.a
	else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Gui/release/Gui.lib
	else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Gui/debug/Gui.lib
	else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Gui/build/Release/libGui.a
	else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Gui/build/Debug/libGui.a
	else:unix: PRE_TARGETDEPS += $$OUT_PWD/../Gui/libGui.a
}
################
# Engine

win32-msvc*{
	CONFIG(64bit) {
		CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Engine/x64/release/ -lEngine
		CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Engine/x64/debug/ -lEngine
	} else {
		CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Engine/win32/release/ -lEngine
		CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Engine/win32/debug/ -lEngine
	}
} else {
	win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Engine/release/ -lEngine
	else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Engine/debug/ -lEngine
	else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Engine/build/Release/ -lEngine
	else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Engine/build/Debug/ -lEngine
	else:unix: LIBS += -L$$OUT_PWD/../Engine/ -lEngine
}

INCLUDEPATH += $$PWD/../Engine
DEPENDPATH += $$PWD/../Engine

win32-msvc*{
	CONFIG(64bit) {
		CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/x64/release/libEngine.lib
		CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/x64/debug/libEngine.lib
	} else {
		CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/win32/release/libEngine.lib
		CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/win32/debug/libEngine.lib
	}
} else {
	win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/release/libEngine.a
	else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/debug/libEngine.a
	else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/release/Engine.lib
	else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/debug/Engine.lib
	else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/build/Release/libEngine.a
	else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/build/Debug/libEngine.a
	else:unix: PRE_TARGETDEPS += $$OUT_PWD/../Engine/libEngine.a
}

################
# HostSupport

win32-msvc*{
	CONFIG(64bit) {
		CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../HostSupport/x64/release/ -lHostSupport
		CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../HostSupport/x64/debug/ -lHostSupport
	} else {
		CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../HostSupport/win32/release/ -lHostSupport
		CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../HostSupport/win32/debug/ -lHostSupport
	}
} else {
	win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../HostSupport/release/ -lHostSupport
	else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../HostSupport/debug/ -lHostSupport
	else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../HostSupport/build/Release/ -lHostSupport
	else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../HostSupport/build/Debug/ -lHostSupport
	else:unix: LIBS += -L$$OUT_PWD/../HostSupport/ -lHostSupport
}

INCLUDEPATH += $$PWD/../HostSupport
DEPENDPATH += $$PWD/../HostSupport

win32-msvc*{
	CONFIG(64bit) {
		CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/x64/release/libHostSupport.lib
		CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/x64/debug/libHostSupport.lib
	} else {
		CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/win32/release/libHostSupport.lib
		CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/win32/debug/libHostSupport.lib
	}
} else {
	win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/release/libHostSupport.a
	else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/debug/libHostSupport.a
	else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/release/HostSupport.lib
	else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/debug/HostSupport.lib
	else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/build/Release/libHostSupport.a
	else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/build/Debug/libHostSupport.a
	else:unix: PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/libHostSupport.a
}

################
# BreakpadClient

!disable-breakpad {

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/x64/release/ -lBreakpadClient
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/x64/debug/ -lBreakpadClient
        } else {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/win32/release/ -lBreakpadClient
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/win32/debug/ -lBreakpadClient
        }
} else {
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/release/ -lBreakpadClient
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/debug/ -lBreakpadClient
        else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/build/Release/ -lBreakpadClient
        else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/build/Debug/ -lBreakpadClient
        else:unix: LIBS += -L$$OUT_PWD/../BreakpadClient/ -lBreakpadClient
}

BREAKPAD_PATH = $$PWD/../google-breakpad/src
INCLUDEPATH += $$BREAKPAD_PATH
DEPENDPATH += $$BREAKPAD_PATH

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/x64/release/libBreakpadClient.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/x64/debug/libBreakpadClient.lib
        } else {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/win32/release/libBreakpadClient.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/win32/debug/libBreakpadClient.lib
        }
} else {
        win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/release/libBreakpadClient.a
        else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/debug/libBreakpadClient.a
        else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/release/BreakpadClient.lib
        else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/debug/BreakpadClient.lib
        else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/build/Release/libBreakpadClient.a
        else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/build/Debug/libBreakpadClient.a
        else:unix: PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/libBreakpadClient.a
}

} # !disable-breakpad

include(../global.pri)
include(../config.pri)

SOURCES += \
    google-test/src/gtest-all.cc \
    google-test/src/gtest_main.cc \
    google-mock/src/gmock-all.cc \
    BaseTest.cpp \
    Hash64_Test.cpp \
    Image_Test.cpp \
    Lut_Test.cpp \
    KnobFile_Test.cpp \
    Curve_Test.cpp

HEADERS += \
    BaseTest.h
