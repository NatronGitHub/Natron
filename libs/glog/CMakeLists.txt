# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2023 The Natron developers
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

set(glog_HEADERS
    src/base/commandlineflags.h
    src/base/googleinit.h
    src/base/mutex.h
    src/config.h
    src/stacktrace.h
    src/stacktrace_generic-inl.h
    src/stacktrace_libunwind-inl.h
    src/stacktrace_powerpc-inl.h
    src/stacktrace_x86-inl.h
    src/stacktrace_x86_64-inl.h
    src/utilities.h
)
set(glog_SOURCES
    src/logging.cc
    src/raw_logging.cc
    src/utilities.cc
    src/vlog_is_on.cc
)
if(WINDOWS)
    set(glog_HEADERS
        ${glog_HEADERS}
        src/windows/config.h
        src/windows/glog/log_severity.h
        src/windows/glog/logging.h
        src/windows/glog/raw_logging.h
        src/windows/glog/vlog_is_on.h
        src/windows/port.h
    )
    set(glog_SOURCES
        ${glog_SOURCES}
        src/windows/port.cc
    )
else()
    set(glog_HEADERS
        ${glog_HEADERS}
        src/config_freebsd.h
        src/config_hurd.h
        src/config_linux.h
        src/config_mac.h
        src/demangle.h
        src/glog/logging.h
        src/glog/log_severity.h
        src/glog/raw_logging.h
        src/glog/vlog_is_on.h
        src/symbolize.h
    )
    set(glog_SOURCES
        ${glog_SOURCES}
        src/demangle.cc
        src/signalhandler.cc
        src/symbolize.cc
    )
endif()
add_library(glog STATIC ${glog_HEADERS} ${glog_SOURCES})
target_include_directories(glog PRIVATE src)
target_compile_definitions(glog PRIVATE GOOGLE_GLOG_DLL_DECL=)
if(WINDOWS)
    target_include_directories(APPEND glog src/windows)
    target_compile_definitions(APPEND glog GLOG_NO_ABBREVIATED_SEVERITIES)
endif()
target_link_libraries(glog PUBLIC gflags)
set_target_properties(glog PROPERTIES POSITION_INDEPENDENT_CODE ON)
add_library(glog::glog ALIAS glog)
