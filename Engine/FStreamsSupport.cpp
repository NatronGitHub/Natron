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

namespace {

#if defined(__NATRON_WIN32__) &&  defined(__GLIBCXX__)
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
    
// MingW
static std::istream* open_ifstream_impl(const std::string &filename, std::ios_base::openmode mode)
{
    std::wstring wfilename = Global::s2ws(filename);
    int oflag = ios_open_mode_to_oflag(mode);
    int fd;
    errno_t errcode = _wsopen_s(&fd, wfilename.c_str(), oflag, _SH_DENYNO, _S_IREAD | _S_IWRITE);
    if (errcode != 0 || fd == -1) {
        return 0;
    }
    __gnu_cxx::stdio_filebuf<char>* buffer = new __gnu_cxx::stdio_filebuf<char>(fd, mode, 1);
    
    
    if (!buffer) {
        return 0;
    }
    return new std::istream(buffer);
}

static std::ostream* open_ofstream_impl(const std::string &filename, std::ios_base::openmode mode)
{
    std::wstring wfilename = Global::s2ws(filename);
    int oflag = ios_open_mode_to_oflag(mode);
    int fd;
    errno_t errcode = _wsopen_s(&fd, wfilename.c_str(), oflag, _SH_DENYNO, _S_IREAD | _S_IWRITE);
    if (errcode != 0 || fd == -1) {
        return 0;
    }
    __gnu_cxx::stdio_filebuf<char>* buffer = new __gnu_cxx::stdio_filebuf<char>(fd, mode, 1);
    if (!buffer) {
        return 0;
    }
    return new std::ostream(buffer);
}
#else
// Msvc or unix
static std::ifstream* open_ifstream_impl(const std::string &filename, std::ios_base::openmode mode)
{
#if defined(__NATRON_WIN32__)
#ifndef _MSC_VER_
#error "Windows builds only support GCC or MSVC"
#endif
    std::wstring wfilename = Global::s2ws(filename);
#endif
    std::ifstream *ret = new std::ifstream();
    if (!ret) {
        return 0;
    }
    try {
        ret->open(
#ifdef __NATRON_WIN32__
                  wfilename.c_str(),
#else
                  filename.c_str(),
#endif
                  mode);
    } catch (const std::exception & e) {
        delete ret;
        return 0;
    }
    
    if (!*ret) {
        delete ret;
        return 0;
    }
    
    return ret;
} // open_ifstream_impl

static std::ofstream* open_ofstream_impl(const std::string &filename, std::ios_base::openmode mode)
{
#if defined(__NATRON_WIN32__)
    std::wstring wfilename = Global::s2ws(filename);
#endif
    std::ofstream *ret = new std::ofstream();
    if (!ret) {
        return 0;
    }
    try {
        ret->open(
#ifdef __NATRON_WIN32__
                  wfilename.c_str(),
#else
                  filename.c_str(),
#endif
                  mode);
    } catch (const std::exception & e) {
        delete ret;
        return 0;
    }
    
    if (!*ret) {
        delete ret;
        return 0;
    }
    
    return ret;
}
#endif //  defined(__NATRON_WIN32__) &&  defined(__GLIBCXX__)

} // anon


boost::shared_ptr<std::istream>
FStreamsSupport::open_ifstream(const std::string& filename, std::ios_base::openmode mode)
{
    std::istream* ret = open_ifstream_impl(filename, mode | std::ios_base::in);
    if (ret) {
        boost::shared_ptr<std::istream> stream(ret);
        if (mode & std::ios_base::ate) {
            stream->seekg (0, std::ios_base::end);
        } else {
            stream->seekg (0, std::ios_base::beg); // force seek, otherwise broken
        }
        if (stream->fail()) {
            stream.reset();
        }
        return stream;
    } else {
        return boost::shared_ptr<std::istream>();
    }
}


boost::shared_ptr<std::ostream>
FStreamsSupport::open_ofstream(const std::string& filename, std::ios_base::openmode mode)
{
    std::ostream* ret = open_ofstream_impl(filename, mode | std::ios_base::out);
    if (ret) {
        boost::shared_ptr<std::ostream> stream(ret);
        if ((mode & std::ios_base::app) == 0) {
            stream->seekp (0, std::ios_base::beg);
        }
        if (stream->fail()) {
            stream.reset();
        }
        return stream;
    } else {
        return boost::shared_ptr<std::ostream>();
    }
}
    

NATRON_NAMESPACE_EXIT;