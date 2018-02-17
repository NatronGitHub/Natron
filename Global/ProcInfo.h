/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#ifndef NATRON_GLOBAL_PROCINFO_H
#define NATRON_GLOBAL_PROCINFO_H

// Python.h not included here, because this is used by CrashReporter, which does not use python

#include "Global/Macros.h"

#if defined(__NATRON_WIN32__)
#include <windows.h>
#endif

#include <string>
#include <vector>



NATRON_NAMESPACE_ENTER

namespace ProcInfo {


/**
* @brief Returns the current process pid
**/
long long getCurrentProcessPID();

/**
 * @brief Returns the application's executable absolute file path.
 * @param argv0Param As a last resort, if system functions fail to return the
 * executable's file path, use the string passed to the command-line.
 **/
std::string applicationFilePath(const char* argv0Param);


/**
 * @brief Same as applicationFilePath(const char*) execept that it strips the
 * basename of the executable from its path and return the latest.
 **/
std::string applicationDirPath(const char* argv0Param);

/**
 * @brief Returns true if the process with the given pid and given executable absolute
 * file path exists and is still running.
 **/
bool checkIfProcessIsRunning(const char* processAbsoluteFilePath, long long pid);

/*
 This function sets the value of the environment variable named
 varName. It will create the variable if it does not exist. It
 returns 0 if the variable could not be set.

 putenv_wrapper() was introduced because putenv() from the standard
 C library was deprecated in VC2005 (and later versions). qputenv()
 uses the replacement function in VC, and calls the standard C
 library's implementation on all other platforms.

 */
bool putenv_wrapper(const char *varName, const std::string& value);

/*
 Returns the value of the environment variable with name
 varName.

 getenv_wrapper() was introduced because getenv() from the standard
 C library was deprecated in VC2005 (and later versions). qgetenv()
 uses the new replacement function in VC, and calls the standard C
 library's implementation on all other platforms.
 */
std::string getenv_wrapper(const char *varName);


void ensureCommandLineArgsUtf8(int argc, char **argv, std::vector<std::string>* utf8Args);
void ensureCommandLineArgsUtf8(int argc, wchar_t **argv, std::vector<std::string>* utf8Args);

} // namespace ProcInfo

NATRON_NAMESPACE_EXIT

#endif // NATRON_GLOBAL_PROCINFO_H
