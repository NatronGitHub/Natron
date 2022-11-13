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
wrapper_name=$2
wrapper_source=${wrapper_name}_module_wrapper.cpp
wrapper_header=${wrapper_name}_python.h

# To be run after shiboken to fix errors

sed -e '/<destroylistener.h>/d' -i'.bak' "$wrapper_dir"/*.cpp
sed -e '/<destroylistener.h>/d' -i'.bak' "$wrapper_dir"/*.cpp
sed -e '/Py_BEGIN_ALLOW_THREADS/d' -i'.bak' "$wrapper_dir"/*.cpp
sed -e '/Py_BEGIN_ALLOW_THREADS/d' -i'.bak' "$wrapper_dir"/*.cpp
sed -e '/Py_END_ALLOW_THREADS/d' -i'.bak' "$wrapper_dir"/*.cpp
sed -e '/Py_END_ALLOW_THREADS/d' -i'.bak' "$wrapper_dir"/*.cpp
if [ "$wrapper_name" = "natrongui" ]; then
    $SED -e "/SbkPySide_QtCoreTypes;/d" -i'.bak' "$wrapper_dir/$wrapper_source"
    $SED -e "/SbkPySide_QtCoreTypeConverters;/d" -i'.bak' "$wrapper_dir/$wrapper_source"
    $SED -e '/SbkNatronEngineTypes;/d' -i'.bak' "$wrapper_dir/$wrapper_source"
    $SED -e '/SbkNatronEngineTypeConverters;/d' -i'.bak' "$wrapper_dir/$wrapper_source"
    $SED -e 's/cleanTypesAttributes/cleanGuiTypesAttributes/g' -i'.bak' "$wrapper_dir/$wrapper_source"
fi
 
# fix warnings
sed -e 's@^#include <shiboken.h>$@#include "Global/Macros.h"\
CLANG_DIAG_OFF(mismatched-tags)\
GCC_DIAG_OFF(unused-parameter)\
GCC_DIAG_OFF(missing-field-initializers)\
GCC_DIAG_OFF(missing-declarations)\
GCC_DIAG_OFF(uninitialized)\
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF\
#include <shiboken.h> // produces many warnings@' -i'.bak' "$wrapper_dir"/*.cpp

sed -e 's@// Extra includes@// Extra includes\
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING@' -i'.bak' "$wrapper_dir"/*.cpp

# replace NATRON_NAMESPACE with Natron for enums with flags (e.g. StandardButtonEnum)
sed -e 's@"NatronEngine\.NATRON_NAMESPACE@"NatronEngine.Natron@g' -e 's@, NatronEngine\.NATRON_NAMESPACE@, NatronEngine.Natron@g' -e 's@"NatronGui\.NATRON_NAMESPACE@"NatronGui.Natron@g' -e 's@"NATRON_NAMESPACE@"Natron@g' -i'.bak' "$wrapper_dir"/*_wrapper.cpp

# re-add the Natron namespace
#sed -e 's@" ::\([^s][^t][^d]\)@ NATRON_NAMESPACE::\1@g' -i'.bak' Engine/NatronEngine/*.cpp Engine/NatronEngine/*.h Gui/NatronGui/*.cpp Gui/NatronGui/*.h

sed -e 's@SbkType< ::@SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::@g' -e 's@SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::QFlags<@SbkType< ::QFlags<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::@g' -e's@NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Rect@NATRON_NAMESPACE::Rect@g' -i'.bak' "$wrapper_dir/$wrapper_header"
sed -e 's@^class @NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER\
class @g' -e 's@^};@};\
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT@g'  -i'.bak' "$wrapper_dir"/*.h

# replace NATRON_NAMESPACE::NATRON_NAMESPACE with NATRON_NAMESPACE in the enums wrappers
sed -e 's@NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::NATRON_NAMESPACE@NATRON_NAMESPACE@g' -i'.bak' "$wrapper_dir/$wrapper_header"
sed -e 's@NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::NATRON_ENUM@NATRON_ENUM@g' -i'.bak' "$wrapper_dir/$wrapper_header"
sed -e 's@"NATRON_ENUM@"Natron@g' -e 's@\.NATRON_ENUM@.Natron@g' -e 's@NATRON_ENUM\.@Natron.@g' -i'.bak' "$wrapper_dir"/*_wrapper.cpp

perl -pe 'BEGIN{undef $/;} s/    \{\n.*SnakeOil(.*\n)*.*SnakeOil.*\n    }//g;' -i'.bak' "$wrapper_dir/$wrapper_source"
sed -e '/SnakeOil/d' -i'.bak' "$wrapper_dir/$wrapper_source"
sed -e '/snakeoil_python/d' -i'.bak' "$wrapper_dir/$wrapper_header"

sed -e 's@^#include <pysidemetafunction.h>$@CLANG_DIAG_OFF(header-guard)\
#include <pysidemetafunction.h> // has wrong header guards in pyside 1.2.2@' -i'.bak' "$wrapper_dir"/*.cpp

sed -e 's@^#include <pyside_qtcore_python.h>$@CLANG_DIAG_OFF(deprecated)\
CLANG_DIAG_OFF(uninitialized)\
CLANG_DIAG_OFF(keyword-macro)\
#include <pyside_qtcore_python.h> // produces warnings\
CLANG_DIAG_ON(deprecated)\
CLANG_DIAG_ON(uninitialized)\
CLANG_DIAG_ON(keyword-macro)@' -i'.bak' "$wrapper_dir"/*.cpp "$wrapper_dir"/*.h

sed -e 's@^#include <pyside_qtgui_python.h>$@CLANG_DIAG_OFF(deprecated)\
CLANG_DIAG_OFF(uninitialized)\
CLANG_DIAG_OFF(keyword-macro)\
#include <pyside_qtgui_python.h> // produces warnings\
CLANG_DIAG_ON(deprecated)\
CLANG_DIAG_ON(uninitialized)\
CLANG_DIAG_ON(keyword-macro)@' -i'.bak' "$wrapper_dir"/*.cpp "$wrapper_dir"/*.h

# clean up
rm "$wrapper_dir"/*.bak
