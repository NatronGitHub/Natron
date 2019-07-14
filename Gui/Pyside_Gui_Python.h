/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef PYSIDE_GUI_PYTHON_H
#define PYSIDE_GUI_PYTHON_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

//Defined to avoid including some headers when running shiboken which may crash shiboken (particularly boost headers)
#define SBK_RUN

#include <pyside_global.h>
#include <QtGui/qpytextobject.h>

//Global
#include <GlobalDefines.h>
#include <Enums.h>

//Engine
#include "PyExprUtils.h"
#include "MergingEnum.h"
#include "PyGlobalFunctions.h"
#include "PyNodeGroup.h"
#include "PyAppInstance.h"
#include "PyItemsTable.h"
#include "PyOverlayInteract.h"
#include "PyRoto.h"
#include "PyTracker.h"
#include "PyNode.h"
#include "PyParameter.h"
#include "RectI.h"
#include "RectD.h"

//Gui
#include "PyGlobalGui.h"
#include "PyGuiApp.h"
#include "PythonPanels.h"

#endif // PYSIDE_GUI_PYTHON_H
