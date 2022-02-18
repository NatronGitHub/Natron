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

#ifndef NATRON_GLOBAL_STRUTILS_H
#define NATRON_GLOBAL_STRUTILS_H

#include <string>
#include <vector>

#include "../Global/Macros.h"


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

/*
 Removes all multiple directory separators "/" and resolves any
 "."s or ".."s found in the path, \a path.

 Symbolic links are kept. This function does not return the
 canonical path, but rather the simplest version of the input.
 For example, "./local" becomes "local", "local/../bin" becomes
 "bin" and "/local/usr/../bin" becomes "/local/bin".

*/
std::string cleanPath(const std::string &path);

/*

 Returns \a pathName with the '/' separators converted to
 separators that are appropriate for the underlying operating
 system.

 On Windows, toNativeSeparators("c:/winnt/system32") returns
 "c:\\winnt\\system32".

 The returned string may be the same as the argument on some
 operating systems, for example on Unix.

*/
std::string toNativeSeparators(const std::string &pathName);

/*
 Returns \a pathName using '/' as file separator. On Windows,
 for instance, fromNativeSeparators("\c{c:\\winnt\\system32}") returns
 "c:/winnt/system32".

 The returned string may be the same as the argument on some
 operating systems, for example on Unix.

 */
std::string fromNativeSeparators(const std::string &pathName);

std::vector<std::string> split(const std::string &text, char sep);

std::string join(const std::vector<std::string> &text, char sep);

} // namespace StrUtils

NATRON_NAMESPACE_EXIT

#endif // ifndef NATRON_GLOBAL_STRUTILS_H
