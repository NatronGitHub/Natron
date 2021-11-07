#!/bin/sh
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2016 INRIA and Alexandre Gauthier
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

# PySide version override, add 2 for Qt5/PySide2
PYV="${1:-2}"

# To be run after shiboken to fix errors

sed -e '/<destroylistener.h>/d' -i'.bak' Engine/NatronEngine5/*.cpp
sed -e '/<destroylistener.h>/d' -i'.bak' Gui/NatronGui5/*.cpp
sed -e "/SbkPySide${PYV}_QtCoreTypes;/d" -i'.bak' Gui/NatronGui5/natrongui_module_wrapper.cpp
sed -e "/SbkPySide${PYV}_QtCoreTypeConverters;/d" -i'.bak' Gui/NatronGui5/natrongui_module_wrapper.cpp
sed -e '/SbkNatronEngineTypes;/d' -i'.bak' Gui/NatronGui5/natrongui_module_wrapper.cpp
sed -e '/SbkNatronEngineTypeConverters;/d' -i'.bak' Gui/NatronGui5/natrongui_module_wrapper.cpp
sed -e 's/cleanTypesAttributes/cleanGuiTypesAttributes/g' -i'.bak' Gui/NatronGui5/natrongui_module_wrapper.cpp
sed -e '/Py_BEGIN_ALLOW_THREADS/d' -i'.bak' Engine/NatronEngine5/*.cpp
sed -e '/Py_BEGIN_ALLOW_THREADS/d' -i'.bak' Gui/NatronGui5/*.cpp
sed -e '/Py_END_ALLOW_THREADS/d' -i'.bak' Engine/NatronEngine5/*.cpp
sed -e '/Py_END_ALLOW_THREADS/d' -i'.bak' Gui/NatronGui5/*.cpp

# fix warnings
sed -e 's@^#include <shiboken.h>$@#include "Global/Macros.h"\
CLANG_DIAG_OFF(mismatched-tags)\
GCC_DIAG_OFF(unused-parameter)\
GCC_DIAG_OFF(missing-field-initializers)\
GCC_DIAG_OFF(missing-declarations)\
GCC_DIAG_OFF(uninitialized)\
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF\
#include <shiboken.h> // produces many warnings@' -i'.bak' Engine/NatronEngine5/*.cpp Gui/NatronGui5/*.cpp

sed -e 's@// inner classes@// inner classes\
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING@' -i'.bak' Engine/NatronEngine5/*.cpp Gui/NatronGui5/*.cpp

# replace NATRON_NAMESPACE with Natron for enums with flags (e.g. StandardButtonEnum)
sed -e 's@"1:NatronEngine\.NATRON_NAMESPACE@"1:NatronEngine.Natron@g' -e 's@, NatronEngine\.NATRON_NAMESPACE@, NatronEngine.Natron@g' -e 's@"1:NatronGui\.NATRON_NAMESPACE@"NatronGui.Natron@g' -e 's@"NATRON_NAMESPACE@"Natron@g' -i'.bak' Engine/NatronEngine5/*_wrapper.cpp

# re-add the Natron namespace
#sed -e 's@" ::\([^s][^t][^d]\)@ NATRON_NAMESPACE::\1@g' -i'.bak' Engine/NatronEngine5/*.cpp Engine/NatronEngine5/*.h Gui/NatronGui5/*.cpp Gui/NatronGui5/*.h

sed -e 's@SbkType< ::\(\(QFlags<\)\?Natron::.*\) >@SbkType< \1 >@g' -i Engine/NatronEngine5/natronengine_python.h
sed -e 's@SbkType< ::@SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::@g' -e 's@SbkType<Natron::QFlags<@SbkType< ::QFlags<NATRON_NAMESPACE::@g' -e's@NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Rect@NATRON_NAMESPACE::Rect@g' -i'.bak' Engine/NatronEngine5/natronengine_python.h Gui/NatronGui5/natrongui_python.h
sed -e 's@^class @NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER\
class @g;T;{:y;/^};/! {n;by}};s@^};@};\
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT@g' -i'.bak' Engine/NatronEngine5/*.h Gui/NatronGui5/*.h

# replace NATRON_NAMESPACE::NATRON_NAMESPACE with NATRON_NAMESPACE in the enums wrappers
sed -e 's@NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::NATRON_NAMESPACE@NATRON_NAMESPACE@g' -i'.bak' Engine/NatronEngine5/natronengine_python.h Gui/NatronGui5/natrongui_python.h

sed -e 's@^#include <pysidemetafunction.h>$@CLANG_DIAG_OFF(header-guard)\
#include <pysidemetafunction.h> // has wrong header guards in pyside 1.2.2@' -i'.bak' Engine/NatronEngine5/*.cpp Gui/NatronGui5/*.cpp

sed -e 's@^#include <pyside'${PYV}'_qtcore_python.h>$@CLANG_DIAG_OFF(deprecated)\
CLANG_DIAG_OFF(uninitialized)\
CLANG_DIAG_OFF(keyword-macro)\
#include <pyside'${PYV}'_qtcore_python.h> // produces warnings\
CLANG_DIAG_ON(deprecated)\
CLANG_DIAG_ON(uninitialized)\
CLANG_DIAG_ON(keyword-macro)@' -i'.bak' Engine/NatronEngine5/*.cpp Gui/NatronGui5/*.cpp Engine/NatronEngine5/*.h Gui/NatronGui5/*.h

sed -e 's@^#include <pyside'${PYV}'_qtgui_python.h>$@CLANG_DIAG_OFF(deprecated)\
CLANG_DIAG_OFF(uninitialized)\
CLANG_DIAG_OFF(keyword-macro)\
#include <pyside'${PYV}'_qtgui_python.h> // produces warnings\
CLANG_DIAG_ON(deprecated)\
CLANG_DIAG_ON(uninitialized)\
CLANG_DIAG_ON(keyword-macro)@' -i'.bak' Engine/NatronEngine5/*.cpp Gui/NatronGui5/*.cpp Engine/NatronEngine5/*.h Gui/NatronGui5/*.h

# clean up
rm Gui/NatronGui5/*.bak Engine/NatronEngine5/*.bak
