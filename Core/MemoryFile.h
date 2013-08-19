//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 




// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// (C) Copyright Craig Henderson 2002 'boost/memmap.hpp' from sandbox
// (C) Copyright Jonathan Graehl 2004.

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

#ifndef __PowiterOsX__mappedfile__
#define __PowiterOsX__mappedfile__

#include <iostream>
#include "Superviser/powiterFn.h"

/*re-writing of the portable mapped-file implementation, as the boost::iostreams::mapped_file does not have the behaviour
// desired in this case.*/
//
//class MMAPfile{
//    static const size_t max_length = (size_t)-1;
//    
//    char* _data;
//    size_t _size;
//    size_t _capacity;
//    Powiter_Enums::MMAPfile_mode _mode;
//    bool _error;
//#ifdef __POWITER_WIN32__
//    HANDLE               _handle;
//    HANDLE               _mapped_handle;
//#else
//    int _handle;
//#endif
//    std::string _path;
//    
//public:
//    /*Constructs an empty MMAPfile object. No mapping has been done, you must call the 
//     open function to actually create the mapping.*/
//    MMAPfile();
//    
//    /*Constructs a MMAPfile object and create a mapping between a disk file and the virtual
//     address space of the process. 
//     @param : path : the file to be mapped (or created), depending on the mode
//     @param: mode : the mode used to create the mapping (mostly regarding what to do
//     if the file already exists).
//    */
//    MMAPfile(const std::string path,Powiter_Enums::MMAPfile_mode mode,size_t length,
//              int offset=0,size_t newFileSize = 0,char* hint = 0 );
//    
//    void open(const std::string path,Powiter_Enums::MMAPfile_mode mode,size_t length,
//              int offset=0,size_t newFileSize = 0,char* hint = 0);
//
//    char* data(){return _data;}
//    
//    void close();
//    
//    size_t size() const {return _size;}
//    
//    void resize(size_t new_size);
//    
//    void reserve(size_t new_capacity);
//    
//    size_t capacity() const { return _capacity; }
//    
//    bool flush();
//    
//    bool is_open() const { return _handle != 0; }
//    
//    static int alignment();
//
//    
//private:
//
//
//    void cleanup(const std::string& msg);
//	void clear(bool);
//};



/*
 Read/write memory-mapped file wrapper.
 It handles only files that can be wholly loaded
 into the address space of the process.
 The constructor opens the file, the destructor closes it.
 The "data" function returns a pointer to the beginning of the file,
 if the file has been successfully opened, otherwise it returns 0.
 The "size" function returns the initial length of the file in bytes,
 if the file has been successfully opened, otherwise it returns 0.
 Afterwards it returns the size the physical file will get if it is closed now.
 The "resize" function changes the number of bytes of the significant
 part of the file. The resulting size can be retrieved
 using the "size" function.
 The "reserve" grows the phisical file to the specified number of bytes.
 The size of the resulting file can be retrieved using "capacity".
 Memory mapped files cannot be shrinked;
 a value smaller than the current capacity is ignored.
 The "capacity()" function return the size the physical file has at this time.
 The "flush" function ensure that the disk is updated
 with the data written in memory.
 */
class MemoryFile {
public:
    MemoryFile(const char *pathname, Powiter::MMAPfile_mode open_mode);
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
#if defined(__POWITER_UNIX__)
    int file_handle_;
#elif defined(__POWITER_WIN32__)
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
#if defined(__POWITER_UNIX__)
    int file_handle_;
#elif defined(_WIN32)
    HANDLE file_handle_;
    HANDLE file_mapping_handle_;
#else
#error Only Posix or Windows systems can use memory-mapped files.
#endif
};



#endif /* defined(__PowiterOsX__mappedfile__) */
