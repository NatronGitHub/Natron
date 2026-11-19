/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "LibraryBinary.h"

#include <iostream>
#include <cassert>
#include <cstring> // memcpy
#include <stdexcept>

#include "Global/GlobalDefines.h"
#include "Global/StrUtils.h"

#ifdef __NATRON_UNIX__
#include <dlfcn.h>
#endif

NATRON_NAMESPACE_ENTER

LibraryBinary::LibraryBinary(LibraryBinary::LibraryTypeEnum type)
    : _type(type)
    , _library(0)
    , _valid(false)
{
}

LibraryBinary::LibraryBinary(const std::map<std::string, void(*) ()> &functions)
    : _type(LibraryBinary::eLibraryTypeBuiltin)
    , _library(0)
    , _valid(false)
{
#ifdef __NATRON_UNIX__
    _functions = functions;
#else
    for (std::map<std::string, void (*)()>::const_iterator it = functions.begin();
         it != functions.end();
         ++it) {
        _functions.insert( std::make_pair(it->first, (HINSTANCE)it->second) );
    }
#endif
}

LibraryBinary::LibraryBinary(const std::string & binaryPath)
    : _type(LibraryBinary::eLibraryTypeExternal)
    , _library(0)
    , _valid(false)
{
    loadBinary(binaryPath);
}

LibraryBinary::LibraryBinary(const std::string & binaryPath,
                             const std::vector<std::string> & funcNames)
    : _type(LibraryBinary::eLibraryTypeExternal)
    , _library(0)
    , _valid(false)
{
    if ( !loadBinary(binaryPath) ) {
        return;
    }
    loadFunctions(funcNames);
}

bool
LibraryBinary::loadBinary(const std::string & binaryPath)
{
    assert(!_valid);
    if (_type != eLibraryTypeExternal) {
        std::cout << "Trying to load a binary but the library is a built-in library." << std::endl;

        return false;
    }

    _binaryPath = binaryPath;
#ifdef __NATRON_WIN32__
    std::wstring ws = StrUtils::utf8_to_utf16(binaryPath);
    _library = LoadLibraryW( ws.c_str() );

#elif defined(__NATRON_UNIX__)
    _library = dlopen(binaryPath.c_str(), RTLD_LAZY | RTLD_LOCAL);
#endif
    if (!_library) {
#ifdef __NATRON_UNIX__
        std::cout << "Couldn't open library " << binaryPath  << ": " << dlerror() << std::endl;
#else
        std::cout << "Couldn't open library " << binaryPath  << std::endl;
#endif
        _valid = false;

        return false;
    }
    _valid = true;

    return true;
}

bool
LibraryBinary::loadFunctions(const std::vector<std::string> & funcNames)
{
    bool ret = true;

    assert(_valid);
    for (U32 i = 0; i < funcNames.size(); ++i) {
#ifdef __NATRON_WIN32__
        value_type v = (value_type)GetProcAddress( _library, funcNames[i].c_str() );
#elif defined(__NATRON_UNIX__)
        // dlsym() returns a void*, and a cast between pointer-to-object and pointer-to-function
        // is only conditionally-supported in C++ (ISO/IEC 14882:2011 5.2.10/8), but POSIX
        // requires the conversion to work: copy the pointer value.
        void* symbol = dlsym( _library, funcNames[i].c_str() );
        value_type v;
        static_assert(sizeof(v) == sizeof(symbol), "function and object pointers must have the same size");
        std::memcpy( &v, &symbol, sizeof(v) );
#endif
        if (!v) {
            std::cout << "Couldn't find function " << funcNames[i] << " in binary " << _binaryPath  << std::endl;
            ret = false;
            continue;
        }
        _functions.insert( make_pair(funcNames[i], v) );
    }

    return ret;
}

LibraryBinary::~LibraryBinary()
{
    if (!_valid) {
        return;
    }
    assert(_library);
#ifdef __NATRON_WIN32__
    FreeLibrary(_library);
#elif defined(__NATRON_UNIX__)
    dlclose(_library);
#endif
}

NATRON_NAMESPACE_EXIT

