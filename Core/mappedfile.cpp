//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include "Superviser/powiterFn.h"
#include "Core/mappedfile.h"

#ifdef __POWITER_WIN32__
# include <windows.h>
#else // unix
# include <errno.h>
# include <fcntl.h>
# include <sys/mman.h>      // mmap, munmap.
# include <sys/stat.h>
# include <sys/types.h>     // struct stat.
# include <unistd.h>        // sysconf.
#include <errno.h>
#include <cstring>
#include <cstdio>
#endif

using namespace std;

MMAPfile::MMAPfile( const std::string path,Powiter_Enums::MMAPfile_mode mode,size_t length,
                   int offset,size_t newFileSize ,char* hint):_handle(0),_data(0),_path(path),_mode(mode),_size(length),_error(false)
{
#ifdef __POWITER_WIN32__
	_mapped_handle = NULL;
#endif
	open(path, mode,length, offset,newFileSize,hint);
}


MMAPfile::MMAPfile():_handle(0),_data(0),_size(0),_path(0){}

void MMAPfile::open(const std::string path,Powiter_Enums::MMAPfile_mode mode, size_t length,int offset,size_t newFileSize,char* hint){
    _mode = mode;
    
#ifdef __POWITER_UNIX__
    if(is_open()){
        cout << "Mapped file already opened. " << _path << endl;
        return;
    }
    //--------------Open underlying file--------------------------------------//
    bool readonly = (_mode == Powiter_Enums::ReadOnly);
    int flags = (readonly ? O_RDONLY : O_RDWR);
    if(newFileSize!=0 && !readonly){
       flags |= (O_CREAT | O_TRUNC);
    }
    _handle= ::open(path.c_str(),flags,S_IRUSR | S_IWUSR);
    if(_handle==-1){
        cleanup("Failed opening file to map");
        cout << strerror(errno) << endl;
        return;
    }
    //--------------Set file size---------------------------------------------//

    if (newFileSize != 0 && !readonly)
        if (ftruncate(_handle, newFileSize) == -1){
            cleanup("failed setting file size");
            return;
        }
    //--------------Determine file size---------------------------------------//
    bool success = true;
    struct stat info;
    if (length != max_length)
        _size = length;
    else {
        success = ::fstat(_handle, &info) != -1;
        _size = info.st_size;
    }
    if (!success){
        cleanup("failed getting file size");
        return;
    }
    //--------------Create mapping--------------------------------------------//
    void* data = ::mmap( hint, _size,
                        readonly ? PROT_READ : (PROT_READ | PROT_WRITE),
                        readonly ? MAP_PRIVATE : MAP_SHARED,
                        _handle, offset );
    if(data == MAP_FAILED) {
        cleanup("failed mapping file");
		return;
    }
    _data = reinterpret_cast<char*>(data);
    return;
    
    /*===========================================*/
    
    
#else // windows
    
    
    /*===========================================*/
    if (is_open()){
        cout << "file already open" << endl;
        return;
    }
    bool readonly = (_mode == Powiter_Enums::MMAPfile_mode::ReadOnly);
    
    //--------------Open underlying file--------------------------------------//
	DWORD dwDesiredAccess =readonly ?GENERIC_READ : (GENERIC_READ | GENERIC_WRITE);
	DWORD dwCreationDisposition = (newFileSize != 0 && !readonly) ? CREATE_ALWAYS : OPEN_EXISTING;
	DWORD dwFlagsandAttributes = readonly ? FILE_ATTRIBUTE_READONLY :FILE_ATTRIBUTE_TEMPORARY;
	_handle =::CreateFileA( 
		path.c_str(),
		dwDesiredAccess,
		FILE_SHARE_READ,
		NULL,
		dwCreationDisposition,
		dwFlagsandAttributes,
		NULL );

    if (_handle == INVALID_HANDLE_VALUE) {
        cleanup("Failed to create file (CreateFileA returned INVALID_HANDLE_VALUE)");
        return;
    }
    
    //--------------Set file size---------------------------------------------//
    
    if (newFileSize != 0 && !readonly) {
        LONG sizehigh =0;// (newFileSize >> (sizeof(LONG) * 8));
        LONG sizelow = (newFileSize & 0xffffffff);
        DWORD result =::SetFilePointer(_handle, sizelow, &sizehigh, FILE_BEGIN);
		if(result == INVALID_SET_FILE_POINTER){
			cout << "invalid set file pointer" << endl;
		}	
        if ( result == INVALID_SET_FILE_POINTER &&GetLastError() != NO_ERROR || !SetEndOfFile(_handle) )
        {
            cleanup("failed setting file size");
			return;
        }
    }

	// Determine file size. Dynamically locate GetFileSizeEx for compatibility
	// with old Platform SDK (thanks to Pavel Vozenilik).
	typedef BOOL (WINAPI *func)(HANDLE, PLARGE_INTEGER);
	HMODULE hmod = ::GetModuleHandleA("kernel32.dll");
	func get_size =
		reinterpret_cast<func>(::GetProcAddress(hmod, "GetFileSizeEx"));
	if (get_size) {
		LARGE_INTEGER info;
		if (get_size(_handle, &info)) {
			boost::intmax_t size =
				( (static_cast<boost::intmax_t>(info.HighPart) << 32) |
				info.LowPart );
			_size =
				static_cast<std::size_t>(
				length != max_length ?
				std::min<boost::intmax_t>(length, size) :
			size
				);
		} else {
			cleanup("failed querying file size");
			return;
		}
	} else {
		DWORD hi;
		DWORD low;
		if ( (low = ::GetFileSize(_handle, &hi))
			!=
			INVALID_FILE_SIZE )
		{
			boost::intmax_t size =
				(static_cast<boost::intmax_t>(hi) << 32) | low;
			_size =
				static_cast<std::size_t>(
				length != max_length ?
				std::min<boost::intmax_t>(length, size) :
			size
				);
		} else {
			cleanup("failed querying file size");
			return;
		}
	}
    
    //--------------Create mapping--------------------------------------------//
	bool priv = _mode == Powiter_Enums::MMAPfile_mode::Priv;
	DWORD protect = priv ? PAGE_WRITECOPY : readonly ? PAGE_READONLY : PAGE_READWRITE;
    _mapped_handle=::CreateFileMappingA( _handle, NULL,readonly ? PAGE_READONLY : PAGE_READWRITE,0, 0, NULL );
    if (_mapped_handle == NULL) {
        cleanup("couldn't create mapping");
        return;
    }
    
    //--------------Access data-----------------------------------------------//
	
	DWORD access = priv ? FILE_MAP_COPY : readonly ? FILE_MAP_READ : FILE_MAP_WRITE;
    void* data =::MapViewOfFileEx( _mapped_handle,
                      access,
                      (DWORD) (offset >> 32),
                      (DWORD) (offset & 0xffffffff),
                      _size != max_length ? _size : 0, (LPVOID)hint );
	if(!data){
		cleanup("failed mapping view");
		return;
	}
    
    _data = reinterpret_cast<char*>(data);
#endif
}

int alignment()
{
#ifdef __POWITER_WIN32__
    SYSTEM_INFO info;
    ::GetSystemInfo(&info);
    return static_cast<int>(info.dwAllocationGranularity);
#else
    return static_cast<int>(sysconf(_SC_PAGESIZE));
#endif
}



void MMAPfile::cleanup(std::string msg){
#ifdef __POWITER_WIN32__
	DWORD error = GetLastError();
	if (_mapped_handle != NULL)
		::CloseHandle(_mapped_handle);
	if (_handle != INVALID_HANDLE_VALUE)
		::CloseHandle(_handle);
	SetLastError(error);
	cout << msg.c_str() << endl;
#else
    if(_handle!=0){
		::close(_handle);
    }
    cout << msg << endl;
#endif
}
void MMAPfile::clear(bool error){
	
	_data = 0;
	_size = 0;
	_path.clear();
#ifdef __POWITER_WIN32__
	_handle = INVALID_HANDLE_VALUE;
	_mapped_handle = NULL;
#else
	_handle = 0;
#endif
	_error = error;
}

void MMAPfile::close(){
    bool error = false;
#ifdef __POWITER_WIN32__
    if (_handle == INVALID_HANDLE_VALUE)
        return;
    error = !::UnmapViewOfFile(_data) || error;
    error = !::CloseHandle(_mapped_handle) || error;
    error = !::CloseHandle(_handle) || error;
    _handle = INVALID_HANDLE_VALUE;
    _mapped_handle = NULL;
#else
    if (!_handle)
        return;
    error = ::munmap(reinterpret_cast<void*>(_data), _size) != 0 || error;
    error = ::close(_handle) != 0 || error;
    _handle = 0;
#endif
	clear(error);
    if (error) {
        cout << "error closing mapped file" << endl;;
    }
   
}
