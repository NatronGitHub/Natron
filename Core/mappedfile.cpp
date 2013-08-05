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

 

 



#include "Superviser/powiterFn.h"
#include "Core/mappedfile.h"

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
        file_handle_ = ::CreateFile((LPCWSTR)pathname, GENERIC_READ,
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
            throw str.c_str();
            return;
        }
        struct stat sbuf;
        if (::fstat(file_handle_, &sbuf) == -1){
            string str("MemoryFile EXC : Failed to get file infos: ");
            str.append(pathname);
            throw str.c_str();
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
            throw str.c_str();
            
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
                case Powiter_Enums::if_exists_fail_if_not_exists_create:
                    windows_open_mode = CREATE_NEW;
                    break;
                case Powiter_Enums::if_exists_keep_if_dont_exists_fail:
                    windows_open_mode = OPEN_EXISTING;
                    break;
                case Powiter_Enums::if_exists_keep_if_dont_exists_create:
                    windows_open_mode = OPEN_ALWAYS;
                    break;
                case Powiter_Enums::if_exists_truncate_if_not_exists_fail:
                    windows_open_mode = TRUNCATE_EXISTING;
                    break;
                case Powiter_Enums::if_exists_truncate_if_not_exists_create:
                    windows_open_mode = CREATE_ALWAYS;
                    break;
                default:
                    string str("MemoryFile EXC : Invalid open mode. ");
                    str.append(pathname);
                    throw str.c_str();
                    return;
            }
            const size_t min_file_size = 4096;
            file_handle_ = ::CreateFile((LPCWSTR)pathname, GENERIC_READ | GENERIC_WRITE,
                                        0, 0, windows_open_mode, FILE_ATTRIBUTE_NORMAL, 0);
            if (file_handle_ == INVALID_HANDLE_VALUE){
                string str("MemoryFile EXC : Failed to open file ");
                str.append(pathname);
                throw str.c_str();
                return;
            }
            size_t initial_file_size = ::GetFileSize(file_handle_, 0);
            size_t adjusted_file_size = initial_file_size == 0 ? min_file_size : initial_file_size;
            file_mapping_handle_ = ::CreateFileMapping(
                                                       file_handle_, 0, PAGE_READWRITE, 0, adjusted_file_size, 0);
            if (file_mapping_handle_ == INVALID_HANDLE_VALUE){
                string str("MemoryFile EXC : Failed to create mapping :  ");
                str.append(pathname);
                throw str.c_str();
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
        
        //MMAPfile::MMAPfile( const std::string path,Powiter_Enums::MMAPfile_mode mode,size_t length,
        //                   int offset,size_t newFileSize ,char* hint):_handle(0),_data(0),_path(path),_mode(mode),_size(length),_error(false)
        //{
        //#ifdef __POWITER_WIN32__
        //	_mapped_handle = NULL;
        //#endif
        //	open(path, mode,length, offset,newFileSize,hint);
        //}
        //
        //
        //MMAPfile::MMAPfile():_handle(0),_data(0),_size(0),_path(0){}
        //
        //void MMAPfile::open(const std::string path,Powiter_Enums::MMAPfile_mode mode, size_t length,int offset,size_t newFileSize,char* hint){
        //    _mode = mode;
        //
        //#ifdef __POWITER_UNIX__
        //    if(is_open()){
        //        cout << "Mapped file already opened. " << _path << endl;
        //        return;
        //    }
        //    //--------------Open underlying file--------------------------------------//
        //    bool readonly = (_mode == Powiter_Enums::ReadOnly);
        //    int flags = (readonly ? O_RDONLY : O_RDWR);
        //    if(newFileSize!=0 && !readonly){
        //       flags |= (O_CREAT | O_TRUNC);
        //    }
        //    _handle= ::open(path.c_str(),flags,S_IRUSR | S_IWUSR);
        //    if(_handle==-1){
        //        cleanup("Failed opening file to map");
        //        cout << strerror(errno) << endl;
        //        return;
        //    }
        //    //--------------Set file size---------------------------------------------//
        //
        //    if (newFileSize != 0 && !readonly)
        //        if (ftruncate(_handle, newFileSize) == -1){
        //            cleanup("failed setting file size");
        //            return;
        //        }
        //    //--------------Determine file size---------------------------------------//
        //    bool success = true;
        //    struct stat info;
        //    if (length != max_length)
        //        _size = length;
        //    else {
        //        success = ::fstat(_handle, &info) != -1;
        //        _size = info.st_size;
        //    }
        //    if (!success){
        //        cleanup("failed getting file size");
        //        return;
        //    }
        //    //--------------Create mapping--------------------------------------------//
        //    void* data = ::mmap( hint, _size,
        //                        readonly ? PROT_READ : (PROT_READ | PROT_WRITE),
        //                        readonly ? MAP_PRIVATE : MAP_SHARED,
        //                        _handle, offset );
        //    if(data == MAP_FAILED) {
        //        cleanup("failed mapping file");
        //		return;
        //    }
        //    _data = reinterpret_cast<char*>(data);
        //    return;
        //
        //    /*===========================================*/
        //
        //
        //#else // windows
        //
        //
        //    /*===========================================*/
        //    if (is_open()){
        //        cout << "file already open" << endl;
        //        return;
        //    }
        //    bool readonly = (_mode == Powiter_Enums::MMAPfile_mode::ReadOnly);
        //
        //    //--------------Open underlying file--------------------------------------//
        //	DWORD dwDesiredAccess =readonly ?GENERIC_READ : (GENERIC_READ | GENERIC_WRITE);
        //	DWORD dwCreationDisposition = (newFileSize != 0 && !readonly) ? CREATE_ALWAYS : OPEN_EXISTING;
        //	DWORD dwFlagsandAttributes = readonly ? FILE_ATTRIBUTE_READONLY :FILE_ATTRIBUTE_TEMPORARY;
        //	_handle =::CreateFileA(
        //		path.c_str(),
        //		dwDesiredAccess,
        //		FILE_SHARE_READ,
        //		NULL,
        //		dwCreationDisposition,
        //		dwFlagsandAttributes,
        //		NULL );
        //
        //    if (_handle == INVALID_HANDLE_VALUE) {
        //        cleanup("Failed to create file (CreateFileA returned INVALID_HANDLE_VALUE)");
        //        return;
        //    }
        //
        //    //--------------Set file size---------------------------------------------//
        //
        //    if (newFileSize != 0 && !readonly) {
        //        LONG sizehigh =0;// (newFileSize >> (sizeof(LONG) * 8));
        //        LONG sizelow = (newFileSize & 0xffffffff);
        //        DWORD result =::SetFilePointer(_handle, sizelow, &sizehigh, FILE_BEGIN);
        //		if(result == INVALID_SET_FILE_POINTER){
        //			cout << "invalid set file pointer" << endl;
        //		}
        //        if ( result == INVALID_SET_FILE_POINTER &&GetLastError() != NO_ERROR || !SetEndOfFile(_handle) )
        //        {
        //            cleanup("failed setting file size");
        //			return;
        //        }
        //    }
        //
        //	// Determine file size. Dynamically locate GetFileSizeEx for compatibility
        //	// with old Platform SDK (thanks to Pavel Vozenilik).
        //	typedef BOOL (WINAPI *func)(HANDLE, PLARGE_INTEGER);
        //	HMODULE hmod = ::GetModuleHandleA("kernel32.dll");
        //	func get_size =
        //		reinterpret_cast<func>(::GetProcAddress(hmod, "GetFileSizeEx"));
        //	if (get_size) {
        //		LARGE_INTEGER info;
        //		if (get_size(_handle, &info)) {
        //			boost::intmax_t size =
        //				( (static_cast<boost::intmax_t>(info.HighPart) << 32) |
        //				info.LowPart );
        //			_size =
        //				static_cast<std::size_t>(
        //				length != max_length ?
        //				std::min<boost::intmax_t>(length, size) :
        //			size
        //				);
        //		} else {
        //			cleanup("failed querying file size");
        //			return;
        //		}
        //	} else {
        //		DWORD hi;
        //		DWORD low;
        //		if ( (low = ::GetFileSize(_handle, &hi))
        //			!=
        //			INVALID_FILE_SIZE )
        //		{
        //			boost::intmax_t size =
        //				(static_cast<boost::intmax_t>(hi) << 32) | low;
        //			_size =
        //				static_cast<std::size_t>(
        //				length != max_length ?
        //				std::min<boost::intmax_t>(length, size) :
        //			size
        //				);
        //		} else {
        //			cleanup("failed querying file size");
        //			return;
        //		}
        //	}
        //
        //    //--------------Create mapping--------------------------------------------//
        //	bool priv = _mode == Powiter_Enums::MMAPfile_mode::Priv;
        //	DWORD protect = priv ? PAGE_WRITECOPY : readonly ? PAGE_READONLY : PAGE_READWRITE;
        //    _mapped_handle=::CreateFileMappingA( _handle, NULL,readonly ? PAGE_READONLY : PAGE_READWRITE,0, 0, NULL );
        //    if (_mapped_handle == NULL) {
        //        cleanup("couldn't create mapping");
        //        return;
        //    }
        //
        //    //--------------Access data-----------------------------------------------//
        //
        //	DWORD access = priv ? FILE_MAP_COPY : readonly ? FILE_MAP_READ : FILE_MAP_WRITE;
        //    void* data =::MapViewOfFileEx( _mapped_handle,
        //                      access,
        //                      (DWORD) (offset >> 32),
        //                      (DWORD) (offset & 0xffffffff),
        //                      _size != max_length ? _size : 0, (LPVOID)hint );
        //	if(!data){
        //		cleanup("failed mapping view");
        //		return;
        //	}
        //
        //    _data = reinterpret_cast<char*>(data);
        //#endif
        //}
        //
        //int alignment()
        //{
        //#ifdef __POWITER_WIN32__
        //    SYSTEM_INFO info;
        //    ::GetSystemInfo(&info);
        //    return static_cast<int>(info.dwAllocationGranularity);
        //#else
        //    return static_cast<int>(sysconf(_SC_PAGESIZE));
        //#endif
        //}
        //
        //
        //
        //void MMAPfile::cleanup(const std::string& msg){
        //#ifdef __POWITER_WIN32__
        //	DWORD error = GetLastError();
        //	if (_mapped_handle != NULL)
        //		::CloseHandle(_mapped_handle);
        //	if (_handle != INVALID_HANDLE_VALUE)
        //		::CloseHandle(_handle);
        //	SetLastError(error);
        //	cout << msg.c_str() << endl;
        //#else
        //    if(_handle!=0){
        //		::close(_handle);
        //    }
        //    cout << msg << endl;
        //#endif
        //}
        //void MMAPfile::clear(bool error){
        //
        //	_data = 0;
        //	_size = 0;
        //	_path.clear();
        //#ifdef __POWITER_WIN32__
        //	_handle = INVALID_HANDLE_VALUE;
        //	_mapped_handle = NULL;
        //#else
        //	_handle = 0;
        //#endif
        //	_error = error;
        //}
        //
        //void MMAPfile::close(){
        //    bool error = false;
        //#ifdef __POWITER_WIN32__
        //    if (_handle == INVALID_HANDLE_VALUE)
        //        return;
        //    error = !::UnmapViewOfFile(_data) || error;
        //    error = !::CloseHandle(_mapped_handle) || error;
        //    error = !::CloseHandle(_handle) || error;
        //    _handle = INVALID_HANDLE_VALUE;
        //    _mapped_handle = NULL;
        //#else
        //    if (!_handle)
        //        return;
        //    error = ::munmap(reinterpret_cast<void*>(_data), _size) != 0 || error;
        //    error = ::close(_handle) != 0 || error;
        //    _handle = 0;
        //#endif
        //	clear(error);
        //    if (error) {
        //        cout << "error closing mapped file" << endl;;
        //    }
        //
        //}
