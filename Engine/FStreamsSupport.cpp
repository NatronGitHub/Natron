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

#include <fstream>
#include <string>
#if defined(__NATRON_WIN32__) &&  defined(__GLIBCXX__)
#include <ext/stdio_filebuf.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <Share.h>
#endif

NATRON_NAMESPACE_ENTER;

namespace  {
    
#if FILESYSTEM_USE_STDIO_FILEBUF
// MingW uses GCC to build, but does not support having a wchar_t* passed as argument
// of ifstream::open or ofstream::open. To properly support UTF-8 encoding on MingW we must
// use the __gnu_cxx::stdio_filebuf GNU extension that can be used with _wfsopen and returned
// into a istream which share the same API as ifsteam. The same reasoning holds for ofstream.

static int
ios_open_mode_to_oflag(std::ios_base::openmode mode)
{
    int f = 0;
    if (mode & std::ios_base::in) {
        f |= _O_RDONLY;
    }
    if (mode & std::ios_base::out) {
        f |= _O_WRONLY;
        f |= _O_CREAT;
        if (mode & std::ios_base::app) {
            f |= _O_APPEND;
        }
        if (mode & std::ios_base::trunc) {
            f |= _O_TRUNC;
        }
    }
    if (mode & std::ios_base::binary) {
        f |= _O_BINARY;
    } else {
        f |= _O_TEXT;
    }
    return f;
}

template <typename STREAM, typename FSTREAM>
bool
open_fstream_impl(const std::string& path,
                  std::ios_base::openmode mode,
                  STREAM** stream,
                  FStreamsSupport::stdio_filebuf** buffer)
{
    if (!stream || buffer) {
        return false;
    }
    std::wstring wpath = Global::s2ws(path);
    int fd;
    int oflag = ios_open_mode_to_oflag(mode);
    errno_t errcode = _wsopen_s(&fd, wpath.c_str(), oflag, _SH_DENYNO, _S_IREAD | _S_IWRITE);
    if (errcode != 0) {
        return 0;
    }
    *buffer = new FStreamsSupport::stdio_filebuf(fd, mode, 1);
    if (!*buffer) {
        return false;
    }
    *stream = new STREAM(*buffer);
    if (!*stream) {
        delete *buffer;
        *buffer = 0;
        return false;
    }
    return true;
}

#else // MSVC or Unix

#ifdef _WIN32
#ifndef _MSC_VER_
#error "open_ifstream_impl only supports GCC or MSVC"
#endif
#endif

template <typename STREAM, typename FSTREAM>
bool
open_fstream_impl(const std::string& path,
                  std::ios_base::openmode mode,
                  STREAM** stream,
                  FStreamsSupport::stdio_filebuf** buffer)
{
    if (!stream) {
        return false;
    }
    
    if (buffer) {
        *buffer = 0;
    }
    
#ifdef _WIN32
    std::wstring wpath = Global::s2ws(path);
#endif
    FSTREAM* ret = new FSTREAM();
    if (!ret) {
        return false;
    }
    try {
#ifdef _WIN32
        ret->open(wpath.c_str(),mode);
#else
        ret->open(path.c_str(),mode);
#endif
    } catch (const std::exception & e) {
        delete ret;
        return false;
    }
    
    if (!*ret) {
        delete ret;
        return false;
    }
    
    *stream = ret;
    
    return true;
} // open_fstream_impl

#endif //#if FILESYSTEM_USE_STDIO_FILEBUF

} // anon


void
FStreamsSupport::open (IStreamWrapper* stream,
                  const std::string& filename,
                  std::ios_base::openmode mode)
{
    if (!stream) {
        return;
    }
    
    std::istream* rawStream = 0;
    stdio_filebuf* buffer = 0;
    if (!open_fstream_impl<std::istream, std::ifstream>(filename, mode | std::ios_base::in, &rawStream, &buffer)) {
        //Should have been freed
        assert(!rawStream && !buffer);
        delete rawStream;
        delete buffer;
        return;
    }
    
    assert(rawStream);
    if (!rawStream) {
        return;
    }
    
    if (mode & std::ios_base::ate) {
        rawStream->seekg (0, std::ios_base::end);
    } else {
        rawStream->seekg (0, std::ios_base::beg); // force seek, otherwise broken
    }
    if (rawStream->fail()) {
        delete rawStream;
        delete buffer;
        return;
    }
    
    stream->setBuffers(rawStream, buffer);
    
}


void
FStreamsSupport::open(OStreamWrapper* stream,const std::string& filename, std::ios_base::openmode mode)
{
    if (!stream) {
        return;
    }
    
    std::ostream* rawStream = 0;
    stdio_filebuf* buffer = 0;
    if (!open_fstream_impl<std::ostream, std::ofstream>(filename, mode | std::ios_base::out, &rawStream, &buffer)) {
        //Should have been freed
        assert(!rawStream && !buffer);
        delete rawStream;
        delete buffer;
        return;
    }
    
    assert(rawStream);
    if (!rawStream) {
        return;
    }
    
    if ((mode & std::ios_base::app) == 0) {
        rawStream->seekp (0, std::ios_base::beg);
    }
    if (rawStream->fail()) {
        delete rawStream;
        delete buffer;
        return;
    }
    
    stream->setBuffers(rawStream, buffer);
}
    

NATRON_NAMESPACE_EXIT;