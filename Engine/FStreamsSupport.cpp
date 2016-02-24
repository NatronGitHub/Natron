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
// ***** END PYTHON BLOCK ****

#include "FStreamsSupport.h"


NATRON_NAMESPACE_ENTER;


void
FStreamsSupport::open (FStreamsSupport::ifstream *stream, const std::string& path,
                  std::ios_base::openmode mode)
{
#ifdef __NATRON_WIN32__
    // Windows std::ifstream accepts non-standard wchar_t*
    // On MingW, we use our own FStreamsSupport::ifstream
    std::wstring wpath = Global::utf8_to_utf16(path);
    stream->open (wpath.c_str(), mode);
    stream->seekg (0, std::ios_base::beg); // force seek, otherwise broken
#else
    stream->open (path.c_str(), mode);
#endif
}



void
FStreamsSupport::open (FStreamsSupport::ofstream *stream, const std::string& path,
                  std::ios_base::openmode mode)
{
#ifdef __NATRON_WIN32__
    // Windows std::ofstream accepts non-standard wchar_t*
    // On MingW, we use our own FStreamsSupport::ofstream
    std::wstring wpath = Global::utf8_to_utf16(path);
    stream->open (wpath.c_str(), mode);
#else
    stream->open (path.c_str(), mode);
#endif
}


    

NATRON_NAMESPACE_EXIT;