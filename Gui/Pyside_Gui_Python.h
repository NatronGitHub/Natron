#ifndef PYSIDE_GUI_PYTHON_H
#define PYSIDE_GUI_PYTHON_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

//Defined to avoid including some headers when running shiboken which may crash shiboken (particularily boost headers)
#define SBK_RUN

#include <pyside_global.h>

//Engine
#include "AppInstanceWrapper.h"
#include "NodeGroupWrapper.h"
#include "GlobalFunctionsWrapper.h"
#include "NodeWrapper.h"
#include "ParameterWrapper.h"

//Gui
#include "GlobalGuiWrapper.h"
#include "GuiAppWrapper.h"
#include "PythonPanels.h"

#endif // PYSIDE_GUI_PYTHON_H
