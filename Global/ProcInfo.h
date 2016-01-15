/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "../Global/Macros.h"

#include <QProcess>
#include <QString>

#ifdef __NATRON_OSX__
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <libproc.h>
#include <sys/sysctl.h>
#elif defined(__NATRON_WIN32__)
#include <windows.h>
#endif

#ifdef __NATRON_UNIX__
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace Natron
{

/**
* @brief Returns the application's executable absolute file path.
* @param argv0Param As a last resort, if system functions fail to return the
* executable's file path, use the string passed to the command-line.
**/
QString applicationFilePath(const char* argv0Param);


/**
* @brief Same as applicationFilePath(const char*) execept that it strips the
* basename of the executable from its path and return the latest.
**/
QString applicationDirPath(const char* argv0Param);

/**
* @brief Returns true if the process with the given pid and given executable absolute
* file path exists and is still running.
**/
bool checkIfProcessIsRunning(const char* processAbsoluteFilePath, Q_PID pid);

} //Natron
#endif // NATRON_GLOBAL_PROCINFO_H
