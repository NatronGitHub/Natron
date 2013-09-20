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

#include "Engine/MemoryFile.h"

#ifdef __POWITER_WIN32__
# include <windows.h>
#else // unix
//# include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>      // mmap, munmap.
#include <sys/stat.h>
#include <sys/types.h>     // struct stat.
#include <unistd.h>        // sysconf.
#include <cstring>
#include <cstdio>
#endif
#include <iostream>
#include <stdexcept>

#include "Global/Macros.h"


using namespace std;

InputMemoryFile::InputMemoryFile(const char *pathname):
data_(0),
size_(0),
#if defined(__POWITER_UNIX__)
file_handle_(-1)
{
    file_handle_ = ::open(pathname, O_RDONLY);
    if (file_handle_ == -1) return;
    struct stat sbuf;
    if (::fstat(file_handle_, &sbuf) == -1) return;
    data_ = static_cast<const char*>(::mmap(
                                            0, sbuf.st_size, PROT_READ, MAP_SHARED, file_handle_, 0));
    if (data_ == MAP_FAILED) data_ = 0;
    else size_ = sbuf.st_size;
#elif defined(__POWITER_WIN32__)
    file_handle_(INVALID_HANDLE_VALUE),
    file_mapping_handle_(INVALID_HANDLE_VALUE)
    {
        file_handle_ = ::CreateFile(pathname, GENERIC_READ,
                                    FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (file_handle_ == INVALID_HANDLE_VALUE) return;
        file_mapping_handle_ = ::CreateFileMapping(
                                                   file_handle_, 0, PAGE_READONLY, 0, 0, 0);
        if (file_mapping_handle_ == INVALID_HANDLE_VALUE) return;
        data_ = static_cast<char*>(::MapViewOfFile(
                                                   file_mapping_handle_, FILE_MAP_READ, 0, 0, 0));
        if (data_) size_ = ::GetFileSize(file_handle_, 0);
#endif
    }
    
    InputMemoryFile::~InputMemoryFile() {
#if defined(__POWITER_UNIX__)
        ::munmap(const_cast<char*>(data_), size_);
        ::close(file_handle_);
#elif defined(__POWITER_WIN32__)
        ::UnmapViewOfFile(data_);
        ::CloseHandle(file_mapping_handle_);
        ::CloseHandle(file_handle_);
#endif
    }
    
    MemoryFile::MemoryFile(const char *pathname, Powiter::MMAPfile_mode open_mode):
    _path(pathname),
    data_(0),
    size_(0),
    capacity_(0),
#if defined(__POWITER_UNIX__)
    file_handle_(-1)
    {
        int posix_open_mode = O_RDWR;
        switch (open_mode)
        {
            case Powiter::if_exists_fail_if_not_exists_create:
                posix_open_mode |= O_EXCL | O_CREAT;
                break;
            case Powiter::if_exists_keep_if_dont_exists_fail:
                break;
            case Powiter::if_exists_keep_if_dont_exists_create:
                posix_open_mode |= O_CREAT;
                break;
            case Powiter::if_exists_truncate_if_not_exists_fail:
                posix_open_mode |= O_TRUNC;
                break;
            case Powiter::if_exists_truncate_if_not_exists_create:
                posix_open_mode |= O_TRUNC | O_CREAT;
                break;
            default: return;
        }
        const size_t min_file_size = 4096;
        file_handle_ = ::open(pathname, posix_open_mode, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (file_handle_ == -1){
            string str("MemoryFile EXC : Failed to open ");
            str.append(pathname);
            throw std::runtime_error(str);

            return;
        }
        struct stat sbuf;
        if (::fstat(file_handle_, &sbuf) == -1){
            string str("MemoryFile EXC : Failed to get file infos: ");
            str.append(pathname);
            throw std::runtime_error(str);

            return;
        }
        size_t initial_file_size = sbuf.st_size;
        size_t adjusted_file_size = initial_file_size == 0 ? min_file_size : initial_file_size;
        ::ftruncate(file_handle_, adjusted_file_size);
        data_ = static_cast<char*>(::mmap(
                                          0, adjusted_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, file_handle_, 0));
        if (data_ == MAP_FAILED){
            data_ = 0;
            string str("MemoryFile EXC : Failed to create mapping: ");
            str.append(pathname);
            throw std::runtime_error(str);

            
        }
        else {
            size_ = initial_file_size;
            capacity_ = adjusted_file_size;
        }
#elif defined(__POWITER_WIN32__)
        file_handle_(INVALID_HANDLE_VALUE),
        file_mapping_handle_(INVALID_HANDLE_VALUE)
        {
            int windows_open_mode;
            switch (open_mode)
            {
                case Powiter::if_exists_fail_if_not_exists_create:
                    windows_open_mode = CREATE_NEW;
                    break;
                case Powiter::if_exists_keep_if_dont_exists_fail:
                    windows_open_mode = OPEN_EXISTING;
                    break;
                case Powiter::if_exists_keep_if_dont_exists_create:
                    windows_open_mode = OPEN_ALWAYS;
                    break;
                case Powiter::if_exists_truncate_if_not_exists_fail:
                    windows_open_mode = TRUNCATE_EXISTING;
                    break;
                case Powiter::if_exists_truncate_if_not_exists_create:
                    windows_open_mode = CREATE_ALWAYS;
                    break;
                default:
                    string str("MemoryFile EXC : Invalid open mode. ");
                    str.append(pathname);
                    throw std::runtime_error(str);

                    return;
            }
            const size_t min_file_size = 4096;
            file_handle_ = ::CreateFile(pathname, GENERIC_READ | GENERIC_WRITE,
                                        0, 0, windows_open_mode, FILE_ATTRIBUTE_NORMAL, 0);
            if (file_handle_ == INVALID_HANDLE_VALUE){
                string str("MemoryFile EXC : Failed to open file ");
                str.append(pathname);
                throw std::runtime_error(str);

                return;
            }
            size_t initial_file_size = ::GetFileSize(file_handle_, 0);
            size_t adjusted_file_size = initial_file_size == 0 ? min_file_size : initial_file_size;
            file_mapping_handle_ = ::CreateFileMapping(
                                                       file_handle_, 0, PAGE_READWRITE, 0, adjusted_file_size, 0);
            if (file_mapping_handle_ == INVALID_HANDLE_VALUE){
                string str("MemoryFile EXC : Failed to create mapping :  ");
                str.append(pathname);
                throw std::runtime_error(str);

                return;
            }
            data_ = static_cast<char*>(::MapViewOfFile(
                                                       file_mapping_handle_, FILE_MAP_WRITE, 0, 0, 0));
            if (data_) {
                size_ = initial_file_size;
                capacity_ = adjusted_file_size;
            }
#endif
        }
        
        void MemoryFile::resize(size_t new_size) {
            if (new_size > capacity_) reserve(new_size);
            size_ = new_size;
        }
        
        void MemoryFile::reserve(size_t new_capacity) {
            if (new_capacity <= capacity_) return;
#if defined(__POWITER_UNIX__)
            ::munmap(data_, size_);
            ::ftruncate(file_handle_, new_capacity);
            data_ = static_cast<char*>(::mmap(
                                              0, new_capacity, PROT_READ | PROT_WRITE, MAP_SHARED, file_handle_, 0));
            if (data_ == MAP_FAILED) data_ = 0;
            capacity_ = new_capacity;
#elif defined(__POWITER_WIN32__)
            ::UnmapViewOfFile(data_);
            ::CloseHandle(file_mapping_handle_);
            file_mapping_handle_ = ::CreateFileMapping(
                                                       file_handle_, 0, PAGE_READWRITE, 0, new_capacity, 0);
            capacity_ = new_capacity;
            data_ = static_cast<char*>(::MapViewOfFile(
                                                       file_mapping_handle_, FILE_MAP_WRITE, 0, 0, 0));
#endif
        }
        
        
        MemoryFile::~MemoryFile() {
#if defined(__POWITER_UNIX__)
            ::munmap(data_, size_);
            if (size_ != capacity_)
            {
                ::ftruncate(file_handle_, size_);
            }
            ::close(file_handle_);
#elif defined(__POWITER_WIN32__)
            ::UnmapViewOfFile(data_);
            ::CloseHandle(file_mapping_handle_);
            if (size_ != capacity_)
            {
                ::SetFilePointer(file_handle_, size_, 0, FILE_BEGIN);
                ::SetEndOfFile(file_handle_);
            }
            ::CloseHandle(file_handle_);
#endif
        }
        
        bool MemoryFile::flush() {
#if defined(__POWITER_UNIX__)
            return ::msync(data_, size_, MS_SYNC) == 0;
#elif defined(__POWITER_WIN32__)
            return ::FlushViewOfFile(data_, size_) != 0;
#endif
        }
        