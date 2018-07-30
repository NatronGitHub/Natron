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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/StrUtils.h"
#include "Global/ProcInfo.h"
#include "Global/PythonUtils.h"

#include <vector>




#ifdef __NATRON_WIN32__
// g++ knows nothing about wmain
// https://sourceforge.net/p/mingw-w64/wiki2/Unicode%20apps/
extern "C" {
    int wmain(int argc, wchar_t** argv)
#else
    int main(int argc, char *argv[])
#endif
{


    std::vector<std::string> commandLineArgsUtf8;
    NATRON_NAMESPACE::ProcInfo::ensureCommandLineArgsUtf8(argc, argv, &commandLineArgsUtf8);
    if (commandLineArgsUtf8.empty() || (int)commandLineArgsUtf8.size() != argc) {
        return 1;
    }

    NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::setupPythonEnv(commandLineArgsUtf8[0].c_str());


#if PY_MAJOR_VERSION >= 3
    // Python 3
    std::vector<wchar_t*> wideArgs(argc);
    for (int i = 0; i < argc; ++i) {
        std::wstring utf16 = NATRON_NAMESPACE::StrUtils::utf8_to_utf16(commandLineArgsUtf8[i]);
        wideArgs[i] = wcsdup(utf16.c_str());
    }
    int ret = Py_Main(commandLineArgsUtf8.size(), &wideArgs[0]);
    return ret;
#else
    std::vector<char*> utf8Args(argc);
    for (int i = 0; i < argc; ++i) {
        utf8Args[i] = strdup(commandLineArgsUtf8[i].c_str());
    }
    int ret = Py_Main(commandLineArgsUtf8.size(), &utf8Args[0]);
    return ret;
#endif
} //main
#ifdef __NATRON_WIN32__
} // extern "C"
#endif
