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

// MingW
static std::istream* open_ifstream_impl(const std::string &filename)
{
    std::wstring wfilename = Global::s2ws(filename);
    int fd;
    errno_t errcode = _wsopen_s(&fd, wfilename.c_str(), _O_RDONLY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
    if (errcode != 0 || fd == -1) {
        return 0;
    }
    __gnu_cxx::stdio_filebuf<char>* buffer = new __gnu_cxx::stdio_filebuf<char>(fd, std::ios_base::in, 1);
    
    
    if (!buffer) {
        return 0;
    }
    return new std::istream(buffer);
}

static std::ostream* open_ofstream_impl(const std::string &filename)
{
    std::wstring wfilename = Global::s2ws(filename);
    int fd;
    errno_t errcode = _wsopen_s(&fd, wfilename.c_str(), _O_WRONLY | _O_TRUNC | _O_CREAT, _SH_DENYNO, _S_IREAD | _S_IWRITE);
    if (errcode != 0 || fd == -1) {
        return 0;
    }
    __gnu_cxx::stdio_filebuf<char>* buffer = new __gnu_cxx::stdio_filebuf<char>(fd, std::ios_base::out, 1);
    if (!buffer) {
        return 0;
    }
    return new std::ostream(buffer);
}
#else
// Msvc or unix
static std::ifstream* open_ifstream_impl(const std::string &filename)
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
                  std::ifstream::in);
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

static std::ofstream* open_ofstream_impl(const std::string &filename)
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
                  std::ofstream::out);
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
FStreamsSupport::open_ifstream(const std::string& filename)
{
    std::istream* ret = open_ifstream_impl(filename);
    if (ret) {
        return boost::shared_ptr<std::istream>(ret);
    } else {
        return boost::shared_ptr<std::istream>();
    }
}


boost::shared_ptr<std::ostream>
FStreamsSupport::open_ofstream(const std::string& filename)
{
    std::ostream* ret = open_ofstream_impl(filename);
    if (ret) {
        return boost::shared_ptr<std::ostream>(ret);
    } else {
        return boost::shared_ptr<std::ostream>();
    }
}
    

NATRON_NAMESPACE_EXIT;