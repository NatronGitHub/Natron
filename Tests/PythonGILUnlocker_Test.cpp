/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
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

// PythonGILUnlocker (#1610) releases the GIL with PyEval_SaveThread() and takes it again with
// PyEval_RestoreThread(), only if this thread holds it. It used to call PyGILState_Release(PyGILState_UNLOCKED)
// without a matching PyGILState_Ensure(): in a thread whose only PyGILState_Ensure() was that of a
// PythonGILLocker, this destroyed the Python thread state, and a thread without the GIL could not use it.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <thread>

#include <gtest/gtest.h>

#include "Engine/AppManager.h"

NATRON_NAMESPACE_USING

// In a thread that is not a Python thread (a render thread), the thread state of the PythonGILLocker must
// survive a PythonGILUnlocker, and the GIL must be released while it exists.
TEST(PythonGILUnlocker, KeepsThreadState)
{
    ASSERT_TRUE( Py_IsInitialized() );
    bool releasedInside = false;
    bool sameState = false;
    bool keptDict = false;
    std::thread t([&] {
        PythonGILLocker pgl;
        PyThreadState* before = PyThreadState_Get();
        PyObject* dict = PyThreadState_GetDict(); // borrowed
        PyObject* one = PyLong_FromLong(1);
        PyDict_SetItemString(dict, "natronTest1610", one);
        Py_DECREF(one);
        {
            PythonGILUnlocker unlocker;
            releasedInside = !PyGILState_Check();
        }
        sameState = (PyThreadState_Get() == before);
        PyObject* d = PyThreadState_GetDict();
        keptDict = d && PyDict_GetItemString(d, "natronTest1610") != NULL;
        if (keptDict) {
            PyDict_DelItemString(d, "natronTest1610");
        }
    });
    t.join(); // the main thread does not hold the GIL after initPython()
    EXPECT_TRUE(releasedInside);
    EXPECT_TRUE(sameState);
    EXPECT_TRUE(keptDict);
}

// Without the GIL (a thread that never ran Python), a PythonGILUnlocker does nothing.
TEST(PythonGILUnlocker, WithoutGILDoesNothing)
{
    ASSERT_TRUE( Py_IsInitialized() );
    bool hadGIL = true;
    bool hasGIL = true;
    std::thread t([&] {
        hadGIL = PyGILState_Check();
        {
            PythonGILUnlocker unlocker;
        }
        hasGIL = PyGILState_Check();
    });
    t.join();
    EXPECT_FALSE(hadGIL);
    EXPECT_FALSE(hasGIL);
}
