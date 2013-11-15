//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef NATRON_ENGINE_MEMORYFILE_H_
#define NATRON_ENGINE_MEMORYFILE_H_

// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// (C) Copyright Craig Henderson 2002 'boost/memmap.hpp' from sandbox
// (C) Copyright Jonathan Graehl 2004.

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

#include <string>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"
#include "Global/Enums.h"

/*
 Read/write memory-mapped file wrapper.
 A memory-mapped file is a disk file that is mapped into the process
 virtual address space (i.e: RAM). Closing the mapped-file is equivalen
 It handles only files that can be wholly loaded
 into the address space of the process.
 The constructor attemps to create the file if the file
 doesn't exist alredy  and opens the mapping of the (created) file into the RAM.
 The destructor closes the mapping, effectively removing the RAM portion but not the file.
 The "data" function returns a pointer to the beginning of the file,
 if the file has been successfully opened, otherwise it returns 0.
 The "size" function returns the initial length of the file in bytes,
 if the file has been successfully opened, otherwise it returns 0.
 Afterwards it returns the size of the physical file if the mapping is closed.
 The "resize" function changes the number of bytes of the significant
 part of the file. The resulting size can be retrieved
 using the "size" function.
 The "reserve" grows the phisical file to the specified number of bytes.
 The size of the resulting file can be retrieved using "capacity".
 The capacity  cannot be shrinked with reserve;
 a value smaller than the current capacity is ignored.
 The "capacity()" function return the size the physical file has at this time.
 The "flush" function ensure that the disk is updated
 with the data written in memory.
 */
class MemoryFile {
public:
    MemoryFile(const std::string& pathname, Natron::MMAPfile_mode open_mode);
    ~MemoryFile();
    char* data() const { return data_; }
    void resize(size_t new_size);
    void reserve(size_t new_capacity);
    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }
    bool flush();
    std::string path() const {return _path;}
private:
    std::string _path;
    char* data_;
    size_t size_;
    size_t capacity_;
#if defined(__NATRON_UNIX__)
    int file_handle_;
#elif defined(__NATRON_WIN32__)
    HANDLE file_handle_;
    HANDLE file_mapping_handle_;
#else
#error Only Posix or Windows systems can use memory-mapped files.
#endif
};


/*
 Read-only memory-mapped file wrapper.
 It handles only files that can be wholly loaded
 into the address space of the process.
 The constructor opens the file, the destructor closes it.
 The "data" function returns a pointer to the beginning of the file,
 if the file has been successfully opened, otherwise it returns 0.
 The "size" function returns the length of the file in bytes,
 if the file has been successfully opened, otherwise it returns 0.
 */
class InputMemoryFile {
public:
    InputMemoryFile(const char *pathname);
    ~InputMemoryFile();
    const char* data() const { return data_; }
    size_t size() const { return size_; }
private:
    const char* data_;
    size_t size_;
#if defined(__NATRON_UNIX__)
    int file_handle_;
#elif defined(_WIN32)
    HANDLE file_handle_;
    HANDLE file_mapping_handle_;
#else
#error Only Posix or Windows systems can use memory-mapped files.
#endif
};



#endif /* defined(NATRON_ENGINE_MEMORYFILE_H_) */
