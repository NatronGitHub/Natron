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

#ifndef NATRON_GLOBAL_STRUTILS_H
#define NATRON_GLOBAL_STRUTILS_H

#include <string>
#include <QtCore/QString>

#include "Global/Macros.h"


NATRON_NAMESPACE_ENTER

namespace StrUtils {

// Should be used in asserts to ensure strings are utf8
bool is_utf8(const char * string);

/*Converts a std::string to wide string*/
std::wstring utf8_to_utf16(const std::string & s);

std::string utf16_to_utf8 (const std::wstring& str);

#ifdef __NATRON_WIN32__
//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::string GetLastErrorAsString();
#endif // __NATRON_WIN32__

// Ensure that path ends with a '/' character
void ensureLastPathSeparator(QString& path);

} // namespace StrUtils

NATRON_NAMESPACE_EXIT

#endif // ifndef NATRON_GLOBAL_STRUTILS_H
