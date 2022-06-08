#!/bin/bash
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

if [ "$#" -ne 2 ]; then
    echo "usage: <generated-dir> <module-wrapper>" 2>&1
    exit 1
fi

wrapper_dir="$1"
wrapper_name="$2"
wrapper_source=${wrapper_name}_module_wrapper.cpp
wrapper_header=${wrapper_name}_python.h

SED="sed"
if [ "$(uname)" = "Darwin" ]; then
    # this script uses the sed cammand "T;", which is a GNU extension
    SED=gsed
    path_to_sed="$(which $SED)"
    if [ ! -x "$path_to_sed" ] ; then
        echo "$SED is required but not available; please install it:"
        echo "- 'brew install gnu-sed' on Homebrew,"
        echo "- 'sudo port install gsed' on MacPorts."
        exit 1
    fi
fi

# To be run after shiboken to fix errors

if [ "$wrapper_name" = "natrongui" ]; then
    $SED -e "/SbkPySide2_QtCoreTypes;/d" -i'.bak' "$wrapper_dir/$wrapper_source"
    $SED -e "/SbkPySide2_QtCoreTypeConverters;/d" -i'.bak' "$wrapper_dir/$wrapper_source"
    $SED -e '/SbkNatronEngineTypes;/d' -i'.bak' "$wrapper_dir/$wrapper_source"
    $SED -e '/SbkNatronEngineTypeConverters;/d' -i'.bak' "$wrapper_dir/$wrapper_source"
    $SED -e 's/cleanTypesAttributes/cleanGuiTypesAttributes/g' -i'.bak' "$wrapper_dir/$wrapper_source"
fi

# fix warnings
$SED -e 's@^#include <shiboken.h>$@#include "Global/Macros.h"\
CLANG_DIAG_OFF(mismatched-tags)\
GCC_DIAG_OFF(unused-parameter)\
GCC_DIAG_OFF(missing-field-initializers)\
GCC_DIAG_OFF(missing-declarations)\
GCC_DIAG_OFF(uninitialized)\
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF\
#include <pysidesignal.h>\
#include <shiboken.h> // produces many warnings@' -i'.bak' "$wrapper_dir"/*.cpp

$SED -e 's@// inner classes@// inner classes\
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING@' -i'.bak' "$wrapper_dir"/*.cpp
$SED -e 's@// Extra includes@// Extra includes\
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING@' -i'.bak' "$wrapper_dir/$wrapper_source"

# replace NATRON_NAMESPACE with Natron for enums with flags (e.g. StandardButtonEnum)
$SED -e 's@"1:NatronEngine\.NATRON_NAMESPACE@"1:NatronEngine.Natron@g' -e 's@, NatronEngine\.NATRON_NAMESPACE@, NatronEngine.Natron@g' -e 's@"1:NatronGui\.NATRON_NAMESPACE@"NatronGui.Natron@g' -e 's@"NATRON_NAMESPACE@"Natron@g' -i'.bak' "$wrapper_dir"/*_wrapper.cpp

# re-add the Natron namespace
#sed -e 's@" ::\([^s][^t][^d]\)@ NATRON_NAMESPACE::\1@g' -i'.bak' Engine/Qt5/NatronEngine/*.cpp Engine/Qt5/NatronEngine/*.h Gui/Qt5/NatronGui/*.cpp Gui/Qt5/NatronGui/*.h

$SED -e 's@SbkType< ::@SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::@g' -e 's@SbkType<NATRON_NAMESPACE.*::QFlags<@SbkType< ::QFlags<::@g' -e's@NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Rect@NATRON_NAMESPACE::Rect@g' -i'.bak' "$wrapper_dir/$wrapper_header"
$SED -e 's@^class @NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER\
class @g;T;{:y;/^};/! {n;by}};s@^};@};\
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT@g' -i'.bak' "$wrapper_dir"/*.h

# replace NATRON_NAMESPACE::NATRON_NAMESPACE with NATRON_NAMESPACE in the enums wrappers
$SED -e 's@NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::NATRON_NAMESPACE@NATRON_NAMESPACE@g' -i'.bak' "$wrapper_dir/$wrapper_header"
$SED -e 's@NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::NATRON_ENUM@NATRON_ENUM@g' -i'.bak' "$wrapper_dir/$wrapper_header"

$SED -e 's@^#include <pysidemetafunction.h>$@CLANG_DIAG_OFF(header-guard)\
#include <pysidemetafunction.h> // has wrong header guards in pyside 1.2.2@' -i'.bak' "$wrapper_dir"/*.cpp

$SED -e 's@^#include <pyside2_qtcore_python.h>$@CLANG_DIAG_OFF(deprecated)\
CLANG_DIAG_OFF(uninitialized)\
CLANG_DIAG_OFF(keyword-macro)\
#include <pyside2_qtcore_python.h> // produces warnings\
CLANG_DIAG_ON(deprecated)\
CLANG_DIAG_ON(uninitialized)\
CLANG_DIAG_ON(keyword-macro)@' -i'.bak' "$wrapper_dir"/*.cpp "$wrapper_dir"/*.h

$SED -e 's@^#include <pyside2_qtgui_python.h>$@CLANG_DIAG_OFF(deprecated)\
CLANG_DIAG_OFF(uninitialized)\
CLANG_DIAG_OFF(keyword-macro)\
#include <pyside2_qtgui_python.h> // produces warnings\
CLANG_DIAG_ON(deprecated)\
CLANG_DIAG_ON(uninitialized)\
CLANG_DIAG_ON(keyword-macro)@' -i'.bak' "$wrapper_dir"/*.cpp "$wrapper_dir"/*.h

# clean up
rm "$wrapper_dir"/*.bak
