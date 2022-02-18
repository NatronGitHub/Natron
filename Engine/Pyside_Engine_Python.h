/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

//Defined to avoid including some headers when running shiboken which may crash shiboken (particularly boost headers)
#define SBK_RUN

#ifndef PYSIDE_ENGINE_PYTHON_H
#define PYSIDE_ENGINE_PYTHON_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

/**
 * @brief This is the global header used by Shiboken to generate bindings for the Engine library.
 * Do not include it when compiling Natron.
 **/

#ifdef SBK2_RUN
#include <pyside2_global.h>
#else
#include <pyside_global.h>
#endif
#include <string>
//Global
#include <GlobalDefines.h>
#include <Enums.h>

//Engine
#include "PyExprUtils.h"
#include "MergingEnum.h"
#include "PyGlobalFunctions.h"
#include "PyNodeGroup.h"
#include "PyAppInstance.h"
#include "PyRoto.h"
#include "PyTracker.h"
#include "PyNode.h"
#include "PyParameter.h"
#include "RectI.h"
#include "RectD.h"

#endif // PYSIDE_ENGINE_PYTHON_H
