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

SED=sed
if [ `uname` = "Darwin" ]; then
    # this script uses the sed cammand "T;", which is a GNU extension
    SED=gsed
    path_to_sed=`which $SED`
    if [ ! -x "$path_to_sed" ] ; then
        echo "$SED is required but not available; please install it:"
        echo "- 'brew install gnu-sed' on Homebrew,"
        echo "- 'sudo port install gsed' on MacPorts."
        exit 1
    fi
fi

# To be run after shiboken to fix errors

$SED -e '/<destroylistener.h>/d' -i'.bak' Engine/Qt5/NatronEngine/*.cpp
$SED -e '/<destroylistener.h>/d' -i'.bak' Gui/Qt5/NatronGui/*.cpp
$SED -e "/SbkPySide${PYV}_QtCoreTypes;/d" -i'.bak' Gui/Qt5/NatronGui/natrongui_module_wrapper.cpp
$SED -e "/SbkPySide${PYV}_QtCoreTypeConverters;/d" -i'.bak' Gui/Qt5/NatronGui/natrongui_module_wrapper.cpp
$SED -e '/SbkNatronEngineTypes;/d' -i'.bak' Gui/Qt5/NatronGui/natrongui_module_wrapper.cpp
$SED -e '/SbkNatronEngineTypeConverters;/d' -i'.bak' Gui/Qt5/NatronGui/natrongui_module_wrapper.cpp
$SED -e 's/cleanTypesAttributes/cleanGuiTypesAttributes/g' -i'.bak' Gui/Qt5/NatronGui/natrongui_module_wrapper.cpp
$SED -e '/Py_BEGIN_ALLOW_THREADS/d' -i'.bak' Engine/Qt5/NatronEngine/*.cpp
$SED -e '/Py_BEGIN_ALLOW_THREADS/d' -i'.bak' Gui/Qt5/NatronGui/*.cpp
$SED -e '/Py_END_ALLOW_THREADS/d' -i'.bak' Engine/Qt5/NatronEngine/*.cpp
$SED -e '/Py_END_ALLOW_THREADS/d' -i'.bak' Gui/Qt5/NatronGui/*.cpp

# fix warnings
$SED -e 's@^#include <shiboken.h>$@#include "Global/Macros.h"\
CLANG_DIAG_OFF(mismatched-tags)\
GCC_DIAG_OFF(unused-parameter)\
GCC_DIAG_OFF(missing-field-initializers)\
GCC_DIAG_OFF(missing-declarations)\
GCC_DIAG_OFF(uninitialized)\
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF\
#include <pysidesignal.h>\
#include <shiboken.h> // produces many warnings@' -i'.bak' Engine/Qt5/NatronEngine/*.cpp Gui/Qt5/NatronGui/*.cpp

$SED -e 's@// inner classes@// inner classes\
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING@' -i'.bak' Engine/Qt5/NatronEngine/*.cpp Gui/Qt5/NatronGui/*.cpp
$SED -e 's@// Extra includes@// Extra includes\
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING@' -i'.bak' Engine/Qt5/NatronEngine/natronengine_module_wrapper.cpp Gui/Qt5/NatronGui/natrongui_module_wrapper.cpp

# replace NATRON_NAMESPACE with Natron for enums with flags (e.g. StandardButtonEnum)
$SED -e 's@"1:NatronEngine\.NATRON_NAMESPACE@"1:NatronEngine.Natron@g' -e 's@, NatronEngine\.NATRON_NAMESPACE@, NatronEngine.Natron@g' -e 's@"1:NatronGui\.NATRON_NAMESPACE@"NatronGui.Natron@g' -e 's@"NATRON_NAMESPACE@"Natron@g' -i'.bak' Engine/Qt5/NatronEngine/*_wrapper.cpp

# re-add the Natron namespace
#sed -e 's@" ::\([^s][^t][^d]\)@ NATRON_NAMESPACE::\1@g' -i'.bak' Engine/Qt5/NatronEngine/*.cpp Engine/Qt5/NatronEngine/*.h Gui/Qt5/NatronGui/*.cpp Gui/Qt5/NatronGui/*.h

# sed -e 's@SbkType< ::\(\(QFlags<\)\?Natron::.*\) >@SbkType< \1 >@g' -i Engine/Qt5/NatronEngine/natronengine_python.h
$SED -e 's@SbkType< ::@SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::@g' -e 's@SbkType<NATRON_NAMESPACE.*::QFlags<@SbkType< ::QFlags<::@g' -e's@NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Rect@NATRON_NAMESPACE::Rect@g' -i'.bak' Engine/Qt5/NatronEngine/natronengine_python.h Gui/Qt5/NatronGui/natrongui_python.h
$SED -e 's@^class @NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER\
class @g;T;{:y;/^};/! {n;by}};s@^};@};\
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT@g' -i'.bak' Engine/Qt5/NatronEngine/*.h Gui/Qt5/NatronGui/*.h

# replace NATRON_NAMESPACE::NATRON_NAMESPACE with NATRON_NAMESPACE in the enums wrappers
$SED -e 's@NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::NATRON_NAMESPACE@NATRON_NAMESPACE@g' -i'.bak' Engine/Qt5/NatronEngine/natronengine_python.h Gui/Qt5/NatronGui/natrongui_python.h
$SED -e 's@NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::NATRON_ENUM@NATRON_ENUM@g' -i'.bak' Engine/Qt5/NatronEngine/natronengine_python.h Gui/Qt5/NatronGui/natrongui_python.h

$SED -e 's@^#include <pysidemetafunction.h>$@CLANG_DIAG_OFF(header-guard)\
#include <pysidemetafunction.h> // has wrong header guards in pyside 1.2.2@' -i'.bak' Engine/Qt5/NatronEngine/*.cpp Gui/Qt5/NatronGui/*.cpp

$SED -e 's@^#include <pyside'${PYV}'_qtcore_python.h>$@CLANG_DIAG_OFF(deprecated)\
CLANG_DIAG_OFF(uninitialized)\
CLANG_DIAG_OFF(keyword-macro)\
#include <pyside'${PYV}'_qtcore_python.h> // produces warnings\
CLANG_DIAG_ON(deprecated)\
CLANG_DIAG_ON(uninitialized)\
CLANG_DIAG_ON(keyword-macro)@' -i'.bak' Engine/Qt5/NatronEngine/*.cpp Gui/Qt5/NatronGui/*.cpp Engine/Qt5/NatronEngine/*.h Gui/Qt5/NatronGui/*.h

$SED -e 's@^#include <pyside'${PYV}'_qtgui_python.h>$@CLANG_DIAG_OFF(deprecated)\
CLANG_DIAG_OFF(uninitialized)\
CLANG_DIAG_OFF(keyword-macro)\
#include <pyside'${PYV}'_qtgui_python.h> // produces warnings\
CLANG_DIAG_ON(deprecated)\
CLANG_DIAG_ON(uninitialized)\
CLANG_DIAG_ON(keyword-macro)@' -i'.bak' Engine/Qt5/NatronEngine/*.cpp Gui/Qt5/NatronGui/*.cpp Engine/Qt5/NatronEngine/*.h Gui/Qt5/NatronGui/*.h

# clean up
rm Gui/Qt5/NatronGui/*.bak Engine/Qt5/NatronEngine/*.bak
