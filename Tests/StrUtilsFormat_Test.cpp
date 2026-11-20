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

// StrUtils::vformat(): printf-style formatting of a va_list, as Log::print(const char*, ...) uses it. The
// old formatter of Engine/Log.cpp took "..." and was called with the va_list as one argument, so "%s"
// printed the bytes of the va_list.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cstdarg>
#include <string>

#include <gtest/gtest.h>

#include "Global/StrUtils.h"

NATRON_NAMESPACE_USING

namespace {
// a variadic function that forwards its arguments as a va_list, like Log::print(const char*, ...)
std::string
format(const char* fmt,
       ...)
{
    va_list args;

    va_start(args, fmt);
    std::string str = StrUtils::vformat(fmt, args);
    va_end(args);

    return str;
}
}

TEST(StrUtilsFormat, Arguments)
{
    EXPECT_EQ( std::string("plain"), format("plain") );
    EXPECT_EQ( std::string("a string, 42, 1.50"), format("%s, %d, %.2f", "a string", 42, 1.5) );
    EXPECT_EQ( std::string(""), format("%s", "") );
    EXPECT_EQ( std::string(), format(NULL) );
}

TEST(StrUtilsFormat, LongResult)
{
    // longer than any initial buffer guess
    std::string longArg(10000, 'x');

    EXPECT_EQ( std::string("<") + longArg + ">", format( "<%s>", longArg.c_str() ) );
}
