#!/bin/sh
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2015 INRIA and Alexandre Gauthier
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

# exit if a command returns an error status
set -e

# To be run after shiboken to fix errors

sed -e '/<destroylistener.h>/d' -i .bak Engine/NatronEngine/*.cpp
sed -e '/<destroylistener.h>/d' -i .bak Gui/NatronGui/*.cpp
sed -e '/SbkPySide_QtCoreTypes;/d' -i .bak Gui/NatronGui/natrongui_module_wrapper.cpp
sed -e '/SbkPySide_QtCoreTypeConverters;/d' -i .bak Gui/NatronGui/natrongui_module_wrapper.cpp
sed -e '/SbkNatronEngineTypes;/d' -i .bak Gui/NatronGui/natrongui_module_wrapper.cpp
sed -e '/SbkNatronEngineTypeConverters;/d' -i .bak Gui/NatronGui/natrongui_module_wrapper.cpp
sed -e 's/cleanTypesAttributes/cleanGuiTypesAttributes/g' -i .bak Gui/NatronGui/natrongui_module_wrapper.cpp
sed -e '/Py_BEGIN_ALLOW_THREADS/d' -i .bak Engine/NatronEngine/*.cpp
sed -e '/Py_BEGIN_ALLOW_THREADS/d' -i .bak Gui/NatronGui/*.cpp
sed -e '/Py_END_ALLOW_THREADS/d' -i .bak Engine/NatronEngine/*.cpp
sed -e '/Py_END_ALLOW_THREADS/d' -i .bak Gui/NatronGui/*.cpp

# fix warnings
sed -e 's@^#include <shiboken.h>$@#include "Global/Macros.h"\
CLANG_DIAG_OFF(mismatched-tags)\
GCC_DIAG_OFF(unused-parameter)\
GCC_DIAG_OFF(missing-field-initializers)\
GCC_DIAG_OFF(missing-declarations)\
#include <shiboken.h> // produces many warnings@' -i .bak Engine/NatronEngine/*.cpp Gui/NatronGui/*.cpp

sed -e 's@^#include <pysidemetafunction.h>$@CLANG_DIAG_OFF(header-guard)\
#include <pysidemetafunction.h> // has wrong header guards in pyside 1.2.2@' -i .bak Engine/NatronEngine/*.cpp Gui/NatronGui/*.cpp

sed -e 's@^#include <pyside_qtcore_python.h>$@CLANG_DIAG_OFF(deprecated)\
CLANG_DIAG_OFF(uninitialized)\
#include <pyside_qtcore_python.h> // produces warnings\
CLANG_DIAG_ON(deprecated)\
CLANG_DIAG_ON(uninitialized)@' -i .bak Engine/NatronEngine/*.cpp Gui/NatronGui/*.cpp Engine/NatronEngine/*.h Gui/NatronGui/*.h

sed -e 's@^#include <pyside_qtgui_python.h>$@CLANG_DIAG_OFF(deprecated)\
CLANG_DIAG_OFF(uninitialized)\
#include <pyside_qtgui_python.h> // produces warnings\
CLANG_DIAG_ON(deprecated)\
CLANG_DIAG_ON(uninitialized)@' -i .bak Engine/NatronEngine/*.cpp Gui/NatronGui/*.cpp Engine/NatronEngine/*.h Gui/NatronGui/*.h

# clean up
rm Gui/NatronGui/*.bak Engine/NatronEngine/*.bak
