/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include "PyGlobalGui.h"


#include <sstream>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
#include "natrongui_python.h"
#include <shiboken.h> // produces many warnings
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
CLANG_DIAG_ON(mismatched-tags)
GCC_DIAG_ON(unused-parameter)

#include "Gui/GuiApplicationManager.h"

NATRON_NAMESPACE_ENTER
NATRON_PYTHON_NAMESPACE_ENTER

QPixmap
PyGuiApplication::getIcon(PixmapEnum val) const
{
    QPixmap ret;

    appPTR->getIcon(val, &ret);

    return ret;
}

GuiApp*
PyGuiApplication::getGuiInstance(int idx) const
{
    AppInstancePtr app = appPTR->getAppInstance(idx);

    if (!app) {
        return 0;
    }
    GuiAppInstancePtr guiApp = toGuiAppInstance(app);
    if (!guiApp) {
        return 0;
    }


    // First, try to re-use an existing Effect object that was created for this node.
    // If not found, create one.
    std::stringstream ss;
    ss << kPythonTmpCheckerVariable << " = " << app->getAppIDString() ;
    std::string script = ss.str();
    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, 0, 0);

    // Clear errors if our call to interpretPythonScript failed, we don't want the
    // calling function to fail aswell.
    NATRON_PYTHON_NAMESPACE::clearPythonStdErr();
    if (ok) {
        PyObject* pyApp = 0;
        PyObject* mainModule = NATRON_PYTHON_NAMESPACE::getMainModule();
        if ( PyObject_HasAttrString(mainModule, kPythonTmpCheckerVariable) ) {
            pyApp = PyObject_GetAttrString(mainModule, kPythonTmpCheckerVariable);
            if (pyApp == Py_None) {
                pyApp = 0;
            }
        }
        GuiApp* cppApp = 0;
        if (pyApp && Shiboken::Object::isValid(pyApp)) {
            cppApp = (GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)pyApp);
        }
        NATRON_PYTHON_NAMESPACE::interpretPythonScript("del " kPythonTmpCheckerVariable, 0, 0);
        NATRON_PYTHON_NAMESPACE::clearPythonStdErr();
        if (cppApp) {
            return cppApp;
        }
    }


    return new GuiApp(guiApp);
}

NATRON_PYTHON_NAMESPACE_EXIT
NATRON_NAMESPACE_EXIT
