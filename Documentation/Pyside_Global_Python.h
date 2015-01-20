#ifndef PYSIDE_GLOBAL_PYTHON_H
#define PYSIDE_GLOBAL_PYTHON_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

//Defined to avoid including some headers when running shiboken which may crash shiboken (particularily boost headers)
#define SBK_RUN

#include <pyside_global.h>

#include "../Engine/Pyside_Engine_Python.h"
#include "../Gui/Pyside_Gui_Python.h"

#endif // PYSIDE_GLOBAL_PYTHON_H
