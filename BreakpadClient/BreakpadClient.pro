# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

TARGET = BreakpadClient
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

include(../global.pri)
include(../config.pri)


win32-msvc* {
	CONFIG(64bit) {
		QMAKE_LFLAGS += /MACHINE:X64
	} else {
		QMAKE_LFLAGS += /MACHINE:X86
	}
}

win32: DEFINES *= UNICODE

BREAKPAD_PATH = ../google-breakpad/src
INCLUDEPATH += $$BREAKPAD_PATH

# every *nix
unix {
        SOURCES += $$BREAKPAD_PATH/client/minidump_file_writer.cc \
                $$BREAKPAD_PATH/common/string_conversion.cc \
                $$BREAKPAD_PATH/common/convert_UTF.c \
                $$BREAKPAD_PATH/common/md5.cc
}

# mac os x
mac {
        # hack to make minidump_generator.cc compile as it uses
        # esp instead of __esp
        # DEFINES += __DARWIN_UNIX03=0 -- looks like we do not need it anymore

        SOURCES += $$BREAKPAD_PATH/client/mac/handler/exception_handler.cc \
                $$BREAKPAD_PATH/client/mac/handler/minidump_generator.cc \
                $$BREAKPAD_PATH/client/mac/handler/dynamic_images.cc \
                $$BREAKPAD_PATH/client/mac/crash_generation/crash_generation_client.cc \
                $$BREAKPAD_PATH/common/mac/string_utilities.cc \
                $$BREAKPAD_PATH/common/mac/file_id.cc \
                $$BREAKPAD_PATH/common/mac/macho_id.cc \
                $$BREAKPAD_PATH/common/mac/macho_utilities.cc \
                $$BREAKPAD_PATH/common/mac/macho_walker.cc
        OBJECTIVE_SOURCES += \
                $$BREAKPAD_PATH/common/mac/MachIPC.mm
}

# linux
linux {
        SOURCES += $$BREAKPAD_PATH/client/linux/handler/exception_handler.cc \
                $$BREAKPAD_PATH/client/linux/handler/minidump_descriptor.cc \
                $$BREAKPAD_PATH/client/linux/crash_generation/crash_generation_client.cc \
                $$BREAKPAD_PATH/common/linux/guid_creator.cc \
                $$BREAKPAD_PATH/common/linux/file_id.cc
}

win32 {
        SOURCES += $$BREAKPAD_PATH/client/windows/handler/exception_handler.cc \
                $$BREAKPAD_PATH/client/windows/crash_generation/crash_generation_client.cc \
                $$BREAKPAD_PATH/client/windows/crash_generation/client_info.cc \
                $$BREAKPAD_PATH/client/windows/crash_generation/minidump_generator.cc \
                $$BREAKPAD_PATH/common/windows/guid_string.cc
}
