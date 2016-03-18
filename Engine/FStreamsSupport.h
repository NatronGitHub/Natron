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

#ifndef FSTREAMSSUPPORT_H
#define FSTREAMSSUPPORT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****
#include "Global/GlobalDefines.h"

#include <string>
#include <fstream>
#if defined(__NATRON_WIN32__) && defined(__GLIBCXX__)
#define FSTREAM_USE_STDIO_FILEBUF 1
#include "fstream_mingw.h"
#endif


NATRON_NAMESPACE_ENTER;


namespace FStreamsSupport {
    
#if FSTREAM_USE_STDIO_FILEBUF
    // MingW uses GCC to build, but does not support having a wchar_t* passed as argument
    // of ifstream::open or ofstream::open. To properly support UTF-8 encoding on MingW we must
    // use the __gnu_cxx::stdio_filebuf GNU extension that can be used with _wfsopen and returned
    // into a istream which share the same API as ifsteam. The same reasoning holds for ofstream.
    typedef basic_ifstream<char> ifstream;
    typedef basic_ofstream<char> ofstream;
#else
    typedef std::ifstream ifstream;
    typedef std::ofstream ofstream;
#endif
    


void open(ifstream* stream,const std::string& filename, std::ios_base::openmode mode = std::ios_base::in);

void open(ofstream* stream,const std::string& filename, std::ios_base::openmode mode = std::ios_base::out);

} //FStreamsSupport

NATRON_NAMESPACE_EXIT;

#endif // FSTREAMSSUPPORT_H
