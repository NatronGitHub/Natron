/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ExprtkFunctions.h"

#include "Engine/Noise.h"
#include "Engine/PyExprUtils.h"
#include "Engine/KnobPrivate.h"

#if defined(__CYGWIN__) || defined(__MINGW32__)
// exprtk requires -Wa,-mbig-obj on mingw, but there is a bug that prevents linking if not using -O3
// see:
// - https://sourceforge.net/p/mingw-w64/discussion/723797/thread/c6b70624/
// - https://github.com/assimp/assimp/issues/177#issuecomment-217605051
// - http://discourse.chaiscript.com/t/compiled-in-std-lib-with-mingw/66/2
#pragma GCC optimize ("-O3")
#endif

#pragma GCC visibility push(hidden)
//#include "exprtk.hpp"
#pragma GCC visibility pop

NATRON_NAMESPACE_ENTER

// everything was moved to KnobExpression.cpp for object size optimization purposes

NATRON_NAMESPACE_EXIT
