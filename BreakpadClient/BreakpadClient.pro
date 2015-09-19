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

gbreakpad {

# disable warnings, since this is an external library
QMAKE_CFLAGS_WARN_ON=-Wno-deprecated
QMAKE_CXXFLAGS_WARN_ON=-Wno-deprecated

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
DEPENDPATH += $$BREAKPAD_PATH

# every *nix
unix {
        SOURCES += \
                $$BREAKPAD_PATH/client/minidump_file_writer.cc \
                $$BREAKPAD_PATH/common/convert_UTF.c \
                $$BREAKPAD_PATH/common/dwarf_cfi_to_module.cc \
                $$BREAKPAD_PATH/common/dwarf_cu_to_module.cc \
                $$BREAKPAD_PATH/common/dwarf_line_to_module.cc \
                $$BREAKPAD_PATH/common/language.cc \
                $$BREAKPAD_PATH/common/md5.cc \
                $$BREAKPAD_PATH/common/module.cc \
                $$BREAKPAD_PATH/common/simple_string_dictionary.cc \
                $$BREAKPAD_PATH/common/stabs_to_module.cc \
                $$BREAKPAD_PATH/common/string_conversion.cc

#                $$BREAKPAD_PATH/common/stabs_reader.cc \

}

# mac os x
mac {
        # hack to make minidump_generator.cc compile as it uses
        # esp instead of __esp
        # DEFINES += __DARWIN_UNIX03=0 -- looks like we do not need it anymore

        SOURCES += \
                $$BREAKPAD_PATH/client/mac/crash_generation/crash_generation_client.cc \
                $$BREAKPAD_PATH/client/mac/crash_generation/crash_generation_server.cc \
                $$BREAKPAD_PATH/client/mac/handler/breakpad_nlist_64.cc \
                $$BREAKPAD_PATH/client/mac/handler/dynamic_images.cc \
                $$BREAKPAD_PATH/client/mac/handler/exception_handler.cc \
                $$BREAKPAD_PATH/client/mac/handler/minidump_generator.cc \
                $$BREAKPAD_PATH/client/mac/handler/protected_memory_allocator.cc \
                $$BREAKPAD_PATH/common/mac/arch_utilities.cc \
                $$BREAKPAD_PATH/common/mac/bootstrap_compat.cc \
                $$BREAKPAD_PATH/common/mac/file_id.cc \
                $$BREAKPAD_PATH/common/mac/launch_reporter.cc \
                $$BREAKPAD_PATH/common/mac/macho_id.cc \
                $$BREAKPAD_PATH/common/mac/macho_reader.cc \
                $$BREAKPAD_PATH/common/mac/macho_utilities.cc \
                $$BREAKPAD_PATH/common/mac/macho_walker.cc \
                $$BREAKPAD_PATH/common/mac/string_utilities.cc
        OBJECTIVE_SOURCES += \
                $$BREAKPAD_PATH/common/mac/MachIPC.mm
}

# linux
linux {
        SOURCES += \
                $$BREAKPAD_PATH/client/linux/crash_generation/crash_generation_client.cc \
                $$BREAKPAD_PATH/client/linux/crash_generation/crash_generation_server.cc \
                $$BREAKPAD_PATH/client/linux/dump_writer_common/thread_info.cc \
                $$BREAKPAD_PATH/client/linux/dump_writer_common/ucontext_reader.cc \
                $$BREAKPAD_PATH/client/linux/handler/exception_handler.cc \
                $$BREAKPAD_PATH/client/linux/handler/minidump_descriptor.cc \
                $$BREAKPAD_PATH/client/linux/log/log.cc \
                $$BREAKPAD_PATH/client/linux/microdump_writer/microdump_writer.cc \
                $$BREAKPAD_PATH/client/linux/minidump_writer/linux_core_dumper.cc \
                $$BREAKPAD_PATH/client/linux/minidump_writer/linux_dumper.cc \
                $$BREAKPAD_PATH/client/linux/minidump_writer/linux_ptrace_dumper.cc \
                $$BREAKPAD_PATH/client/linux/minidump_writer/minidump_writer.cc \
                $$BREAKPAD_PATH/client/linux/sender/google_crash_report_sender.cc \
                $$BREAKPAD_PATH/common/linux/crc32.cc \
                $$BREAKPAD_PATH/common/linux/dump_symbols.cc \
                $$BREAKPAD_PATH/common/linux/elf_core_dump.cc \
                $$BREAKPAD_PATH/common/linux/elf_symbols_to_module.cc \
                $$BREAKPAD_PATH/common/linux/elfutils.cc \
                $$BREAKPAD_PATH/common/linux/file_id.cc \
                $$BREAKPAD_PATH/common/linux/google_crashdump_uploader.cc \
                $$BREAKPAD_PATH/common/linux/guid_creator.cc \
                $$BREAKPAD_PATH/common/linux/http_upload.cc \
                $$BREAKPAD_PATH/common/linux/libcurl_wrapper.cc \
                $$BREAKPAD_PATH/common/linux/linux_libc_support.cc \
                $$BREAKPAD_PATH/common/linux/memory_mapped_file.cc \
                $$BREAKPAD_PATH/common/linux/safe_readlink.cc \
                $$BREAKPAD_PATH/common/linux/synth_elf.cc
}

win32 {
        SOURCES += \
                $$BREAKPAD_PATH/client/windows/crash_generation/client_info.cc \
                $$BREAKPAD_PATH/client/windows/crash_generation/crash_generation_client.cc \
                $$BREAKPAD_PATH/client/windows/crash_generation/crash_generation_server.cc \
                $$BREAKPAD_PATH/client/windows/crash_generation/minidump_generator.cc \
                $$BREAKPAD_PATH/client/windows/handler/exception_handler.cc \
                $$BREAKPAD_PATH/client/windows/sender/crash_report_sender.cc \
                $$BREAKPAD_PATH/common/windows/dia_util.cc \
                $$BREAKPAD_PATH/common/windows/guid_string.cc \
                $$BREAKPAD_PATH/common/windows/http_upload.cc \
                $$BREAKPAD_PATH/common/windows/omap.cc \
                $$BREAKPAD_PATH/common/windows/pdb_source_line_writer.cc \
                $$BREAKPAD_PATH/common/windows/string_utils.cc
}

}
