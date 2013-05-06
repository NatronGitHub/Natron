//
//  mappedfile.h
//  PowiterOsX
//
//  Created by Alexandre on 3/26/13.
//  Copyright (c) 2013 Alexandre. All rights reserved.
//
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
 desired in this case.*/

class MMAPfile{
    static const size_t max_length = (size_t)-1;
    
    char* _data;
    size_t _size;
    Powiter_Enums::MMAPfile_mode _mode;
    bool _error;
#ifdef __POWITER_WIN32__
    HANDLE               _handle;
    HANDLE               _mapped_handle;
#else
    int _handle;
#endif
    std::string _path;
    
public:
    
    MMAPfile();
    
    MMAPfile( const std::string path,Powiter_Enums::MMAPfile_mode mode,size_t length,
              int offset=0,size_t newFileSize = 0,char* hint = 0 );
    
    void open(const std::string path,Powiter_Enums::MMAPfile_mode mode,size_t length,
              int offset=0,size_t newFileSize = 0,char* hint = 0);

    char* data(){return _data;}
    
    void close();
    
    size_t size(){return _size;}
    
    bool is_open() const{ return _handle != 0; }
    
    static int alignment();

    
private:


    void cleanup(std::string msg);
	void clear(bool);
};



#endif /* defined(__PowiterOsX__mappedfile__) */
