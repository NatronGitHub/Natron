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
// ***** END PYTHON BLOCK *****

#include "Log.h"

#ifdef NATRON_LOG


#include <cassert>
#include <cstdio>
#include <cstdarg>
#include <memory>
#include <cstdlib>
#include <string>

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QMutex>

#include "Global/GlobalDefines.h"

NATRON_NAMESPACE_USING

// see second answer of http://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
static
std::string
string_format(const std::string fmt, ...)
{
    int size = ((int)fmt.size()) * 2 + 50;   // Use a rubric appropriate for your code
    std::string str;
    va_list ap;
    while (1) {     // Maximum two passes on a POSIX system...
        str.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf((char *)str.data(), size, fmt.c_str(), ap);
        va_end(ap);
        if (n > -1 && n < size) {  // Everything worked
            str.resize(n);
            return str;
        }
        if (n > -1)  // Needed size returned
            size = n + 1;   // For null char
        else
            size *= 2;      // Guess at a larger size (OS specific)
    }
    return str;
}


class NATRON_NAMESPACE::LogPrivate
{
public:

    QMutex _lock;
    QFile* _file;
    QTextStream* _stream;
    int _beginsCount;

    LogPrivate()
        : _lock()
          , _file(NULL)
          , _stream(NULL)
          , _beginsCount(0)
    {
    }

    ~LogPrivate()
    {
        delete _stream;
        if (_file) {
            _file->close();
            delete _file;
        }
    }

    void open(const std::string & fileName)
    {
        QMutexLocker locker(&_lock);

        if (_file) {
            return;
        }
        _file = new QFile( fileName.c_str() );
        _file->open(QIODevice::WriteOnly | QIODevice::Truncate);
        _stream = new QTextStream(_file);
    }

    void beginFunction(const std::string & callerName,
                       const std::string & function)
    {
        if (!_file) {
            QString filename(NATRON_APPLICATION_NAME + QString("_log") + QString::number( QCoreApplication::instance()->applicationPid() ) + ".txt");
            open( filename.toStdString() );
        }
        QMutexLocker locker(&_lock);
        _stream->operator <<("********************************************************************************\n");
        for (int i = 0; i < _beginsCount; ++i) {
            _stream->operator <<("    ");
        }
        _stream->operator <<("START ");
        _stream->operator <<( callerName.c_str() );
        _stream->operator <<("    ");
        _stream->operator <<( function.c_str() );
        _stream->operator <<('\n');
        _stream->flush();
        ++_beginsCount;
    }

    void print(const std::string & log)
    {
        QMutexLocker locker(&_lock);

        assert(_file);
        for (int i = 0; i < _beginsCount; ++i) {
            _stream->operator <<("    ");
        }
        QString str( log.c_str() );
        for (int i = 0; i < str.size(); ++i) {
            _stream->operator <<( str.at(i).toAscii() );
            if ( (i % 80 == 0) && (i != 0) ) { // format to 80 columns
                /*Find closest word end and insert a new line*/
                ++i;
                while ( i < str.size() && str.at(i) != QChar(' ') ) {
                    _stream->operator <<( str.at(i).toAscii() );
                    ++i;
                }
                _stream->operator <<('\n');
                for (int i = 0; i < _beginsCount; ++i) {
                    _stream->operator <<("    ");
                }
            }
        }
        _stream->operator <<( log.c_str() );
        _stream->operator <<('\n');
        _stream->flush();
    }

    void endFunction(const std::string & callerName,
                     const std::string & function)
    {
        QMutexLocker locker(&_lock);

        --_beginsCount;
        for (int i = 0; i < _beginsCount; ++i) {
            _stream->operator <<("    ");
        }
        _stream->operator <<("STOP ");
        _stream->operator <<( callerName.c_str() );
        _stream->operator <<("    ");
        _stream->operator <<( function.c_str() );
        _stream->operator <<('\n');
        _stream->flush();
    }
};

Log::Log()
    : Singleton<Log>(),_imp( new LogPrivate() )
{
}

Log::~Log()
{
    delete _imp;
}

void
Log::open(const std::string & fileName)
{
    Log::instance()->_imp->open(fileName);
}

void
Log::beginFunction(const std::string & callerName,
                   const std::string & function)
{
    Log::instance()->_imp->beginFunction(callerName,function);
}

void
Log::print(const std::string & log)
{
    Log::instance()->_imp->print(log);
}

void
Log::print(const char *format,
           ...)
{
    va_list args;

    va_start(args, format);
    std::string str = string_format(format, args);
    va_end(args);
    Log::print(str);
}

void
Log::endFunction(const std::string & callerName,
                 const std::string & function)
{
    Log::instance()->_imp->endFunction(callerName,function);
}

NATRON_NAMESPACE_EXIT;

#endif // ifdef NATRON_LOG
