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

#ifndef FSTREAMSSUPPORT_H
#define FSTREAMSSUPPORT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****
#include "Global/GlobalDefines.h"

#include <fstream>
#if defined(__NATRON_WIN32__) && defined(__GLIBCXX__)
#define FILESYSTEM_USE_STDIO_FILEBUF 1
#include <ext/stdio_filebuf.h> // __gnu_cxx::stdio_filebuf
#endif

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

NATRON_NAMESPACE_ENTER;

namespace FStreamsSupport {
    
#ifdef FILESYSTEM_USE_STDIO_FILEBUF
    typedef __gnu_cxx::stdio_filebuf<char> stdio_filebuf;
#else
    typedef std::basic_filebuf<char> stdio_filebuf;
#endif
    
/// To avoid memory leaks, the open functions below
/// return this wrapper which ensure memory freeing in a RAII style.
template <typename STREAM>
class IOStreamWrapperTemplated
{
    
public:
    
    IOStreamWrapperTemplated()
    : _stream(0)
    , _buffer(0)
    {
        
    }
    
    IOStreamWrapperTemplated(STREAM* stream,
                             stdio_filebuf* buffer = 0)
    : _stream(stream)
    , _buffer(buffer)
    {
        
    }
    
    operator bool() const
    {
        return _stream ? (bool)(*_stream) : false;
    }
    
    STREAM& operator*() const
    {
        assert(_stream);
        return *_stream;
    }
    
    //For convenience add this operator to access the stream
    STREAM* operator->() const
    {
        assert(_stream);
        return _stream;
    }
    
    void setBuffers(STREAM* stream, stdio_filebuf* buffer = 0)
    {
        _stream = stream;
        _buffer = buffer;
    }
    
    ~IOStreamWrapperTemplated()
    {
        reset();
    }
    
    void reset()
    {
        delete _stream;
        
        //According to istream/ostream documentation, the destructor does not
        //delete the internal buffer. If we want the file to be closed, we need
        //to delete it ourselves
        delete _buffer;
    }
    
private:
    
    STREAM* _stream;
    stdio_filebuf* _buffer;
    
};
    
typedef IOStreamWrapperTemplated<std::istream> IStreamWrapper;
typedef IOStreamWrapperTemplated<std::ostream> OStreamWrapper;

void open(IStreamWrapper* stream,const std::string& filename, std::ios_base::openmode mode = std::ios_base::in);

void open(OStreamWrapper* stream,const std::string& filename, std::ios_base::openmode mode = std::ios_base::out);

}

NATRON_NAMESPACE_EXIT;

#endif // FSTREAMSSUPPORT_H
