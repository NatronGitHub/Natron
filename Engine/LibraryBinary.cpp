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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "LibraryBinary.h"

#include <cassert>
#include <iostream>

#include "Global/GlobalDefines.h"
#ifdef __NATRON_UNIX__
#include <dlfcn.h>
#endif

NATRON_NAMESPACE_ENTER;

LibraryBinary::LibraryBinary(LibraryBinary::LibraryTypeEnum type)
    : _type(type)
      , _library(0)
      , _valid(false)
{
}

LibraryBinary::LibraryBinary(const std::map<std::string, void(*)()> &functions)
    : _type(LibraryBinary::eLibraryTypeBuiltin)
      , _library(0)
      , _valid(false)
{
#ifdef __NATRON_UNIX__
    _functions = functions;
#else
    for (std::map<std::string, void(*)()>::const_iterator it = functions.begin();
         it != functions.end();
         ++it) {
        _functions.insert( std::make_pair(it->first,(HINSTANCE)it->second) );
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
#ifdef UNICODE
    std::wstring ws = Global::s2ws(binaryPath);
    _library = LoadLibrary(ws.c_str());
#else
    _library = LoadLibrary( binaryPath.c_str() );
#endif
    
#elif defined(__NATRON_UNIX__)
    _library = dlopen(binaryPath.c_str(), RTLD_LAZY|RTLD_LOCAL);
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

#if defined(__NATRON_UNIX__)
// dlsym() is declared as returning void*, but it really returns void(*)()
// and cast between pointer-to-object and pointer-to-function is only conditionally-supported.
// Ref:
// ISO/IEC 14882:2011
// 5.2.10 Reinterpret cast [expr.reinterpret.cast]
// 8 Converting a function pointer to an object pointer type or vice versa is conditionally-supported. The meaning of such a conversion is implementation-defined, except that if an implementation supports conversions in both directions, converting a prvalue of one type to the other type and back, possibly with different cvqualification, shall yield the original pointer value.
namespace {
typedef void(*value_type)();
typedef value_type (*value_dlsym_t)(void *, const char *);
}
#endif

bool
LibraryBinary::loadFunctions(const std::vector<std::string> & funcNames)
{
    bool ret = true;

    assert(_valid);
    for (U32 i = 0; i < funcNames.size(); ++i) {
#ifdef __NATRON_WIN32__
        value_type v = (value_type)GetProcAddress( _library,funcNames[i].c_str() );
#elif defined(__NATRON_UNIX__)
        value_type v = ((value_dlsym_t)(dlsym))( _library, funcNames[i].c_str() );
#endif
        if (!v) {
            std::cout << "Couldn't find function " << funcNames[i] << " in binary " << _binaryPath  << std::endl;
            ret = false;
            continue;
        }
        _functions.insert( make_pair(funcNames[i],v) );
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

NATRON_NAMESPACE_EXIT;

