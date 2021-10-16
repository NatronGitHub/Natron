#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TARGET = glog
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

CONFIG += glog-flags gflags-flags

include(../../global.pri)
include(../../libs.pri)

CONFIG -= warn_on
# disable preprocessor warnings (-Wno-macro-redefined just appeared in recent compilers)
*g++* {
  QMAKE_CXXFLAGS += -Wp,-w
}
*clang* | *xcode* {
  QMAKE_CXXFLAGS += -Wp,-w
}

win32 {
SOURCES += \
	src/logging.cc \
	src/raw_logging.cc \
	src/utilities.cc \
        src/vlog_is_on.cc \
        src/windows/port.cc

HEADERS += \
	src/utilities.h \
	src/stacktrace_generic-inl.h \
	src/stacktrace.h \
	src/stacktrace_x86_64-inl.h \
	src/base/googleinit.h \
	src/base/mutex.h \
	src/base/commandlineflags.h \
	src/stacktrace_powerpc-inl.h \
	src/stacktrace_x86-inl.h \
	src/config.h \
	src/stacktrace_libunwind-inl.h \
	src/windows/glog/raw_logging.h \
	src/windows/glog/vlog_is_on.h \
	src/windows/glog/logging.h \
	src/windows/glog/log_severity.h \
        src/windows/port.h \
        src/windows/config.h

} else {
SOURCES += \
	src/demangle.cc \
	src/logging.cc \
	src/raw_logging.cc \
	src/signalhandler.cc \
	src/symbolize.cc \
	src/utilities.cc \
	src/vlog_is_on.cc


HEADERS += \
	src/base/commandlineflags.h \
	src/base/googleinit.h \
	src/base/mutex.h \
	src/config_freebsd.h \
	src/config.h \
	src/config_hurd.h \
	src/config_linux.h \
	src/config_mac.h \
	src/demangle.h \
	src/glog/logging.h \
	src/glog/log_severity.h \
	src/glog/raw_logging.h \
	src/glog/vlog_is_on.h \
	src/stacktrace_generic-inl.h \
	src/stacktrace.h \
	src/stacktrace_libunwind-inl.h \
	src/stacktrace_powerpc-inl.h \
	src/stacktrace_x86_64-inl.h \
	src/stacktrace_x86-inl.h \
	src/symbolize.h \
	src/utilities.h
}
