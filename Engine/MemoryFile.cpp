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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/MemoryFile.h"


#ifdef __NATRON_WIN32__
# include <windows.h>
#else // unix
      //# include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>      // mmap, munmap.
#include <sys/stat.h>
#include <sys/types.h>     // struct stat.
#include <unistd.h>        // sysconf.
#include <cstring>
#include <cerrno>
#include <cstdio>
#endif
#include <iostream>
#include <stdexcept>

#include "Global/Macros.h"

#define MIN_FILE_SIZE 4096

struct MemoryFilePrivate
{
    std::string path; //< filepath of the backing file
    char* data; //< pointer to the begining of the mapped file
    size_t size; //< the effective size of the file
#if defined(__NATRON_UNIX__)
    int file_handle; //< unix file handle
#elif defined(__NATRON_WIN32__)
    HANDLE file_handle; //< windows file handle
    HANDLE file_mapping_handle; //< windows memory mapped handle
#else
#error Only Unix or Windows systems can use memory-mapped files.
#endif

    MemoryFilePrivate(const std::string & filepath)
        : path(filepath)
          , data(0)
          , size(0)
#if defined(__NATRON_UNIX__)
          , file_handle(-1)
#elif defined(__NATRON_WIN32__)
          , file_handle(INVALID_HANDLE_VALUE)
          , file_mapping_handle(INVALID_HANDLE_VALUE)
#endif
    {
    }

    void openInternal(MemoryFile::FileOpenModeEnum open_mode);

    void closeMapping();
};

MemoryFile::MemoryFile()
    : _imp( new MemoryFilePrivate( std::string() ) )
{
}

MemoryFile::MemoryFile(const std::string & filepath,
                       FileOpenModeEnum open_mode)
    : _imp( new MemoryFilePrivate(filepath) )
{
    _imp->openInternal(open_mode);
}

MemoryFile::MemoryFile(const std::string & filepath,
                       size_t size,
                       FileOpenModeEnum open_mode)
    : _imp( new MemoryFilePrivate(filepath) )
{
    _imp->openInternal(open_mode);
    resize(size);
}

void
MemoryFile::open(const std::string & filepath,
                 FileOpenModeEnum open_mode)
{
    if (!_imp->path.empty() || _imp->data) {
        return;
    }
    _imp->path = filepath;
    _imp->openInternal(open_mode);
}

void
MemoryFilePrivate::openInternal(MemoryFile::FileOpenModeEnum open_mode)
{
#if defined(__NATRON_UNIX__)
    /*********************************************************
    ********************************************************

       CHOOSING FILE OPEN MODE
    ********************************************************
    *********************************************************/
    int posix_open_mode = O_RDWR;
    switch (open_mode) {
    case MemoryFile::eFileOpenModeEnumIfExistsFailElseCreate:
        posix_open_mode |= O_EXCL | O_CREAT;
        break;
    case MemoryFile::eFileOpenModeEnumIfExistsKeepElseFail:
        break;
    case MemoryFile::eFileOpenModeEnumIfExistsKeepElseCreate:
        posix_open_mode |= O_CREAT;
        break;
    case MemoryFile::eFileOpenModeEnumIfExistsTruncateElseFail:
        posix_open_mode |= O_TRUNC;
        break;
    case MemoryFile::eFileOpenModeEnumIfExistsTruncateElseCreate:
        posix_open_mode |= O_TRUNC | O_CREAT;
        break;
    default:

        return;
    }

    /*********************************************************
    ********************************************************

       OPENING THE FILE WITH RIGHT PERMISSIONS:
       - R/W user
       - R Group
       - R Other
    ********************************************************
    *********************************************************/
    file_handle = ::open(path.c_str(), posix_open_mode, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (file_handle == -1) {
        std::string str("MemoryFile EXC : Failed to open ");
        str.append(path);
        throw std::runtime_error(str);
    }

    /*********************************************************
    ********************************************************

       GET FILE SIZE
    ********************************************************
    *********************************************************/
    struct stat sbuf;
    if (::fstat(file_handle, &sbuf) == -1) {
        std::string str("MemoryFile EXC : Failed to get file info: ");
        str.append(path);
        throw std::runtime_error(str);
    }

    /*********************************************************
    ********************************************************

       IF FILE IS NOT EMPTY,  MMAP IT
    ********************************************************
    *********************************************************/
    if (sbuf.st_size > 0) {
        data = static_cast<char*>( ::mmap(
                                       0, sbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, file_handle, 0) );
        if (data == MAP_FAILED) {
            data = 0;
            std::string str("MemoryFile EXC : Failed to create mapping: ");
            str.append(path);
            throw std::runtime_error(str);
        } else {
            size = sbuf.st_size;
        }
    }
#elif defined(__NATRON_WIN32__)

    /*********************************************************
    ********************************************************

       CHOOSING FILE OPEN MODE
    ********************************************************
    *********************************************************/
    int windows_open_mode;
    switch (open_mode) {
    case MemoryFile::eFileOpenModeEnumIfExistsFailElseCreate:
        windows_open_mode = CREATE_NEW;
        break;
    case MemoryFile::eFileOpenModeEnumIfExistsKeepElseFail:
        windows_open_mode = OPEN_EXISTING;
        break;
    case MemoryFile::eFileOpenModeEnumIfExistsKeepElseCreate:
        windows_open_mode = OPEN_ALWAYS;
        break;
    case MemoryFile::eFileOpenModeEnumIfExistsTruncateElseFail:
        windows_open_mode = TRUNCATE_EXISTING;
        break;
    case MemoryFile::eFileOpenModeEnumIfExistsTruncateElseCreate:
        windows_open_mode = CREATE_ALWAYS;
        break;
    default:
        std::string str("MemoryFile EXC : Invalid open mode. ");
        str.append(path);
        throw std::runtime_error(str);

        return;
    }

    /*********************************************************
    ********************************************************

       OPENING THE FILE WITH RIGHT PERMISSIONS:
       - R/W
    ********************************************************
    *********************************************************/
    file_handle = ::CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                               0, 0, windows_open_mode, FILE_ATTRIBUTE_NORMAL, 0);
    if (file_handle == INVALID_HANDLE_VALUE) {
        std::string str("MemoryFile EXC : Failed to open file ");
        str.append(path);
        throw std::runtime_error(str);
    }

    /*********************************************************
    ********************************************************

       GET FILE SIZE
    ********************************************************
    *********************************************************/
    size_t fileSize = ::GetFileSize(file_handle, 0);

    /*********************************************************
    ********************************************************

       IF FILE IS NOT EMPTY,  MMAP IT
    ********************************************************
    *********************************************************/
    if (fileSize > 0) {
        file_mapping_handle = ::CreateFileMapping(file_handle, 0, PAGE_READWRITE, 0, 0, 0);
        data = static_cast<char*>( ::MapViewOfFile(file_mapping_handle, FILE_MAP_WRITE, 0, 0, 0) );
        if (data) {
            size = fileSize;
        } else {
            throw std::runtime_error("MemoryFile EXC : Failed to create mapping.");
        }
    }

#endif // if defined(__NATRON_UNIX__)
} // openInternal

char*
MemoryFile::data() const
{
    return _imp->data;
}

size_t
MemoryFile::size() const
{
    return _imp->size;
}

std::string
MemoryFile::path() const
{
    return _imp->path;
}

void
MemoryFile::resize(size_t new_size)
{
#if defined(__NATRON_UNIX__)
    if (_imp->data) {
        if (::munmap(_imp->data, _imp->size) < 0) {
            std::string str("MemoryFile EXC : Failed to unmap the mapped file: ");
            str.append( std::strerror(errno) );
            throw std::runtime_error(str);
        }
    }
    if (::ftruncate(_imp->file_handle, new_size) < 0) {
        std::string str("MemoryFile EXC : Failed to resize the mapped file: ");
        str.append( std::strerror(errno) );
        throw std::runtime_error(str);
    }
    _imp->data = static_cast<char*>( ::mmap(
                                         0, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, _imp->file_handle, 0) );
    if (_imp->data == MAP_FAILED) {
        _imp->data = 0;
        std::string str("MemoryFile EXC : Failed to create mapping: ");
        str.append(_imp->path);
        throw std::runtime_error(str);
    }

#elif defined(__NATRON_WIN32__)
    ::UnmapViewOfFile(_imp->data);
    ::CloseHandle(_imp->file_mapping_handle);
    _imp->file_mapping_handle = ::CreateFileMapping(
        _imp->file_handle, 0, PAGE_READWRITE, 0, new_size, 0);
    _imp->data = static_cast<char*>( ::MapViewOfFile(
                                         _imp->file_mapping_handle, FILE_MAP_WRITE, 0, 0, 0) );
#endif

    if (!_imp->data) {
        throw std::bad_alloc();
    }
    _imp->size = new_size;
}

void
MemoryFilePrivate::closeMapping()
{
#if defined(__NATRON_UNIX__)
    if (::munmap(data,size) != 0) {
        std::string str("MemoryFile EXC : Failed to unmap the mapped file: ");
        str.append( std::strerror(errno) );
        throw std::runtime_error(str);
    }
    ::close(file_handle);
#elif defined(__NATRON_WIN32__)
    if (::UnmapViewOfFile(data) == 0) {
        throw std::runtime_error("Failed to unmap the mapped file");
    }
    ::CloseHandle(file_mapping_handle);
    ::CloseHandle(file_handle);
#endif
}

bool
MemoryFile::flush()
{
#if defined(__NATRON_UNIX__)

    return ::msync(_imp->data, _imp->size, MS_SYNC) == 0;
#elif defined(__NATRON_WIN32__)

    return ::FlushViewOfFile(_imp->data, _imp->size) != 0;
#endif
}

MemoryFile::~MemoryFile()
{
    if (_imp->data) {
        try {
            _imp->closeMapping();
        } catch (const std::exception & e) {
            std::cerr << e.what() << std::endl;
        }
    }
    delete _imp;
}

void
MemoryFile::remove()
{
    if ( !_imp->path.empty() ) {
        if (_imp->data) {
            _imp->closeMapping();
        }
        if (::remove( _imp->path.c_str() ) != 0) {
            std::cerr << "Attempt to remove an unexisting file." << std::endl;
        }
        _imp->path.clear();
        _imp->data = 0;
    }
}

