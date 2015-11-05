/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_GLOBAL_LIBRARYBINARY_H
#define NATRON_GLOBAL_LIBRARYBINARY_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <string>
#include <map>
#include <vector>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"
#include "Engine/EngineFwd.h"


namespace Natron {
#ifdef __NATRON_OSX__
    #define NATRON_LIBRARY_EXT "dylib"
#elif defined(__NATRON_LINUX__)
    #define NATRON_LIBRARY_EXT "so"
#elif defined(__NATRON_WIN32__)
    #define NATRON_LIBRARY_EXT "dll"
#else
    #error "Operating system not supported by the library loader."
#endif

class LibraryBinary
{
#ifdef __NATRON_WIN32__
    typedef HINSTANCE value_type;
#elif defined(__NATRON_UNIX__)
    typedef void(*value_type)();
#endif

public:

    enum LibraryTypeEnum
    {
        eLibraryTypeExternal,
        eLibraryTypeBuiltin
    };

    LibraryBinary(LibraryBinary::LibraryTypeEnum type);

    LibraryBinary(const std::map<std::string,void(*)()> & functions);

    LibraryBinary(const std::string & binaryPath);

    LibraryBinary(const std::string & binaryPath,
                  const std::vector<std::string> & funcNames);

    ~LibraryBinary();

    bool loadBinary(const std::string & binaryPath);

    bool loadFunctions(const std::vector<std::string> & funcNames);

    /*Call this after the constructor to find out if the binary has loaded successfully.
       If not, you should delete this object.*/
    bool isValid() const
    {
        return _valid;
    }

    /*Returns a pointer to the function with name functionName. The return value
       is a pair whose first member indicates whether it could find the function or not.s*/
    template <typename T>
    std::pair<bool,T> findFunction(const std::string & functionName) const
    {
        std::map<std::string,value_type>::const_iterator it = _functions.find(functionName);

        if ( it == _functions.end() ) {
            return std::make_pair(false,(T)0);
        } else {
            return std::make_pair(true,(T)it->second);
        }
    }

    const std::map<std::string,value_type> & getAllFunctions() const
    {
        return _functions;
    }

private:

    LibraryTypeEnum _type;
#ifdef __NATRON_WIN32__
    HINSTANCE _library;
#elif defined(__NATRON_UNIX__)
    void* _library;
#endif
    std::string _binaryPath;
    bool _valid;
    std::map<std::string,value_type> _functions; // <function name, pointer>
};
} // namespace Natron
#endif // ifndef NATRON_GLOBAL_LIBRARYBINARY_H
