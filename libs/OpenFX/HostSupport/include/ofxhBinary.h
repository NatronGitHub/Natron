#ifndef OFX_BINARY_H
#define OFX_BINARY_H

/*
Software License :

Copyright (c) 2007-2009, The Open Effects Association Ltd. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
* Neither the name The Open Effects Association Ltd, nor the names of its 
contributors may be used to endorse or promote products derived from this
software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <string>
#include <iostream>

#if defined(_WIN32) || defined(_WIN64)
#ifndef WINDOWS
#define WINDOWS
#endif
#elif defined(__linux__) || defined(__FreeBSD__) || defined( __APPLE__) || defined(unix) || defined(__unix) || defined(_XOPEN_SOURCE) || defined(_POSIX_SOURCE)
#define UNIX
#else
#error cannot detect operating system
#endif

#if defined(UNIX)
#include <dlfcn.h>
#elif defined (WINDOWS)
#include "windows.h"
#include <assert.h>
#endif

#include <sys/stat.h>

namespace OFX 
{

  /// class representing a DLL/Shared Object/etc
  class Binary {
    /// destruction will close the library and invalidate
    /// any function pointers returned by lookupSymbol()
  protected :
    std::string _binaryPath;
    bool _invalid;
#if defined(UNIX)
    void *_dlHandle;
#elif defined (WINDOWS)
    HINSTANCE _dlHandle;
#endif
    time_t _time;
    off_t _size;
    int _users;
  public :

    /// create object representing the binary.  will stat() it, 
    /// and this fails, will set binary to be invalid.
    Binary(const std::string &binaryPath);

    ~Binary() { unload(); }

    // calls stat, returns true if successfull, false otherwise
    static bool getFileModTimeAndSize(const std::string &binaryPath, time_t& modificationTime, off_t& fileSize);

    bool isLoaded() const { return _dlHandle != 0; }

    /// is this binary invalid? (did the a stat() or load() on the file fail,
    /// or are we missing a some of the symbols?
    bool isInvalid() const { return _invalid; }

    /// set invalid status (e.g. called by user if a mandatory symbol was missing)
    void setInvalid(bool invalid) { _invalid = invalid; }

    /// Last modification time of the file.
    time_t getTime() const { return _time; }

    /// Current size of the file.
    off_t getSize() const { return _size; }

    /// Path to the file.
    const std::string &getBinaryPath() const { return _binaryPath; }

    void ref();
    void unref();

    /// open the binary.
    void load();

    /// close the binary
    void unload();

    /// look up a symbol in the binary file and return it as a pointer.
    /// returns null pointer if not found, or if the library is not loaded.
    void *findSymbol(const std::string &symbol);
  };
}

#endif
