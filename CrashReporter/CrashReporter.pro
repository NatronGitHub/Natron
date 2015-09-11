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

TARGET = NatronCrashReporter
QT       += core network gui

CONFIG += console
CONFIG -= app_bundle
CONFIG += moc
CONFIG += qt

TEMPLATE = app


*g++* | *clang* {
#See https://bugreports.qt.io/browse/QTBUG-35776 we cannot use
# QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
# QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO
# QMAKE_OBJECTIVE_CFLAGS_RELEASE_WITH_DEBUGINFO
# QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO

    CONFIG(relwithdebinfo) {
        CONFIG += release
        DEFINES *= NDEBUG
        QMAKE_CXXFLAGS += -O2 -g
        QMAKE_CXXFLAGS -= -O3
    }
}

win32-msvc* {
    CONFIG(relwithdebinfo) {
        CONFIG += release
        DEFINES *= NDEBUG
        QMAKE_CXXFLAGS_RELEASE += -Zi
        QMAKE_LFLAGS_RELEASE += /DEBUG /OPT:REF
    }
}

CONFIG(debug, debug|release){
    DEFINES *= DEBUG
} else {
    DEFINES *= NDEBUG
}

macx {
  # Set the pbuilder version to 46, which corresponds to Xcode >= 3.x
  # (else qmake generates an old pbproj on Snow Leopard)
  QMAKE_PBUILDER_VERSION = 46

  QMAKE_MACOSX_DEPLOYMENT_VERSION = $$split(QMAKE_MACOSX_DEPLOYMENT_TARGET, ".")
  QMAKE_MACOSX_DEPLOYMENT_MAJOR_VERSION = $$first(QMAKE_MACOSX_DEPLOYMENT_VERSION)
  QMAKE_MACOSX_DEPLOYMENT_MINOR_VERSION = $$last(QMAKE_MACOSX_DEPLOYMENT_VERSION)
  universal {
    message("Compiling for universal OSX $${QMAKE_MACOSX_DEPLOYMENT_MAJOR_VERSION}.$$QMAKE_MACOSX_DEPLOYMENT_MINOR_VERSION")
    contains(QMAKE_MACOSX_DEPLOYMENT_MAJOR_VERSION, 10) {
      contains(QMAKE_MACOSX_DEPLOYMENT_TARGET, 4)|contains(QMAKE_MACOSX_DEPLOYMENT_MINOR_VERSION, 5) {
        # OSX 10.4 (Tiger) and 10.5 (Leopard) are x86/ppc
        message("Compiling for universal ppc/i386")
        CONFIG += x86 ppc
      }
      contains(QMAKE_MACOSX_DEPLOYMENT_MINOR_VERSION, 6) {
        message("Compiling for universal i386/x86_64")
        # OSX 10.6 (Snow Leopard) may run on Intel 32 or 64 bits architectures
        CONFIG += x86 x86_64
      }
      # later OSX instances only run on x86_64, universal builds are useless
      # (unless a later OSX supports ARM)
    }
  }

  #link against the CoreFoundation framework for the StandardPaths functionnality
  LIBS += -framework CoreServices
}


unix:!mac {
    DEFINES += N_UNDF=0
}

win32:Debug: DEFINES *= _DEBUG 

SOURCES += \
    CrashDialog.cpp \
    main.cpp \
    CallbacksManager.cpp

HEADERS += \
    CrashDialog.h \
    CallbacksManager.h

BREAKPAD_PATH = ../google-breakpad/src
INCLUDEPATH += $$BREAKPAD_PATH

SOURCES += \
        $$BREAKPAD_PATH/common/md5.cc \
        $$BREAKPAD_PATH/common/string_conversion.cc \
        $$BREAKPAD_PATH/common/convert_UTF.c \

# mac os x
mac {
        # hack to make minidump_generator.cc compile as it uses
        # esp instead of __esp
        # DEFINES += __DARWIN_UNIX03=0 -- looks like we doesn't need it anymore

        SOURCES += \
				$$BREAKPAD_PATH/client/minidump_file_writer.cc \
                $$BREAKPAD_PATH/client/mac/handler/breakpad_nlist_64.cc \
                $$BREAKPAD_PATH/client/mac/handler/minidump_generator.cc \
                $$BREAKPAD_PATH/client/mac/handler/dynamic_images.cc \
                $$BREAKPAD_PATH/client/mac/crash_generation/crash_generation_server.cc \
                $$BREAKPAD_PATH/common/mac/string_utilities.cc \
                $$BREAKPAD_PATH/common/mac/file_id.cc \
                $$BREAKPAD_PATH/common/mac/macho_id.cc \
                $$BREAKPAD_PATH/common/mac/macho_utilities.cc \
                $$BREAKPAD_PATH/common/mac/macho_walker.cc \
                $$BREAKPAD_PATH/common/mac/bootstrap_compat.cc

        OBJECTIVE_SOURCES += \
                $$BREAKPAD_PATH/common/mac/MachIPC.mm
}

# other *nix
unix:!mac {
        SOURCES += \
				$$BREAKPAD_PATH/client/minidump_file_writer.cc \
                $$BREAKPAD_PATH/client/linux/handler/minidump_descriptor.cc \
                $$BREAKPAD_PATH/client/linux/crash_generation/crash_generation_server.cc \
                $$BREAKPAD_PATH/client/linux/minidump_writer/minidump_writer.cc \
                $$BREAKPAD_PATH/client/linux/minidump_writer/linux_dumper.cc \
                $$BREAKPAD_PATH/client/linux/minidump_writer/linux_core_dumper.cc \
                $$BREAKPAD_PATH/client/linux/minidump_writer/linux_ptrace_dumper.cc \
                $$BREAKPAD_PATH/client/linux/dump_writer_common/seccomp_unwinder.cc \
                $$BREAKPAD_PATH/client/linux/dump_writer_common/thread_info.cc \
                $$BREAKPAD_PATH/client/linux/dump_writer_common/ucontext_reader.cc \
                $$BREAKPAD_PATH/common/linux/guid_creator.cc \
                $$BREAKPAD_PATH/common/linux/file_id.cc \
                $$BREAKPAD_PATH/common/linux/linux_libc_support.cc \
                $$BREAKPAD_PATH/common/linux/memory_mapped_file.cc \
                $$BREAKPAD_PATH/common/linux/elfutils.cc \
                $$BREAKPAD_PATH/common/linux/elf_core_dump.cc \
                $$BREAKPAD_PATH/common/linux/dump_symbols.cc \
		$$BREAKPAD_PATH/common/linux/elf_symbols_to_module.cc \
		$$BREAKPAD_PATH/common/linux/safe_readlink.cc \
		$$BREAKPAD_PATH/common/linux/crc32.cc \
                $$BREAKPAD_PATH/common/module.cc \
                $$BREAKPAD_PATH/common/language.cc \
                $$BREAKPAD_PATH/common/stabs_reader.cc \
                $$BREAKPAD_PATH/common/stabs_to_module.cc \
		$$BREAKPAD_PATH/common/test_assembler.cc \
                $$BREAKPAD_PATH/common/dwarf_cu_to_module.cc \
                $$BREAKPAD_PATH/common/dwarf_cfi_to_module.cc \
                $$BREAKPAD_PATH/common/dwarf_line_to_module.cc \
		$$BREAKPAD_PATH/common/dwarf/dwarf2reader.cc \
 		$$BREAKPAD_PATH/common/dwarf/bytereader.cc \
		$$BREAKPAD_PATH/common/dwarf/cfi_assembler.cc \
		$$BREAKPAD_PATH/common/dwarf/functioninfo.cc \
		$$BREAKPAD_PATH/common/dwarf/dwarf2diehandler.cc 

		
}

win32 {
        SOURCES += \
                $$BREAKPAD_PATH/client/windows/crash_generation/client_info.cc \
                $$BREAKPAD_PATH/client/windows/crash_generation/crash_generation_server.cc \
                $$BREAKPAD_PATH/client/windows/crash_generation/minidump_generator.cc \
                $$BREAKPAD_PATH/common/windows/guid_string.cc
}

RESOURCES += \
    ../Gui/GuiResources.qrc

INSTALLS += target
