#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TARGET = gflags
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

CONFIG += gflags-flags

include(../../global.pri)
include(../../libs.pri)
include(../../config.pri)

CONFIG -= warn_on
# disable preprocessor warnings (-Wno-macro-redefined just appeared in recent compilers)
*g++* {
  QMAKE_CXXFLAGS += -Wp,-w
}
*clang* | *xcode* {
  QMAKE_CXXFLAGS += -Wp,-w
}

SOURCES += \
	src/gflags.cc \
	src/gflags_completions.cc \
        src/gflags_reporting.cc \


HEADERS += \
	src/config.h \
	src/gflags/gflags_completions.h \
	src/gflags/gflags_declare.h \
	src/gflags/gflags.h \
	src/mutex.h \
	src/util.h

win32 {
SOURCES += \
	src/windows_port.cc

HEADERS += \
	src/windows_port.h

}
        
