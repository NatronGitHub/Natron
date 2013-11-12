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
#include "Log.h"
#ifdef NATRON_LOG


#include <cassert>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <string>

#include <QFile>
#include <QTextStream>
#include <QMutex>

#include "Global/GlobalDefines.h"


namespace Natron{

class LogPrivate{
    
public:
    
    QMutex _lock;
    QFile* _file;
    QTextStream* _stream;
    int _beginsCount;

    LogPrivate():
    _lock()
    , _file(NULL)
    , _stream(NULL)
    , _beginsCount(0)
    {}

    ~LogPrivate(){
        if(_stream){
            delete _stream;
        }
        if(_file){
            _file->close();
            delete _file;
        }
        
    }

    void open(const std::string& fileName){
        QMutexLocker locker(&_lock);
        if(_file)
            return;
        _file = new QFile(fileName.c_str());
        _file->open(QIODevice::WriteOnly | QIODevice::Truncate);
        _stream = new QTextStream(_file);
    }

    void beginFunction(const std::string& callerName,const std::string& function){
        if(!_file){
            QString filename(NATRON_APPLICATION_NAME "_log.txt");
            open(filename.toStdString());
        }
        QMutexLocker locker(&_lock);
        _stream->operator <<("********************************************************************************\n");
        for(int i = 0 ; i < _beginsCount;++i){
            _stream->operator <<("    ");
        }
        _stream->operator <<("START ");
        _stream->operator <<(callerName.c_str());
        _stream->operator <<("    ");
        _stream->operator <<(function.c_str());
        _stream->operator <<('\n');
        _stream->flush();
        ++_beginsCount;
    }

    void print(const std::string& log){
        QMutexLocker locker(&_lock);
        assert(_file);
        for(int i = 0 ; i < _beginsCount;++i){
            _stream->operator <<("    ");
        }
        QString str(log.c_str());
        for (int i = 0; i < str.size();++i) {
            _stream->operator <<(str.at(i).toAscii());
            if (i%80 == 0 && i!=0) { // format to 80 columns
                /*Find closest word end and insert a new line*/
                ++i;
                while(i < str.size() && str.at(i)!=QChar(' ')){
                    _stream->operator <<(str.at(i).toAscii());
                    ++i;
                }
                _stream->operator <<('\n');
                for(int i = 0 ; i < _beginsCount;++i){
                    _stream->operator <<("    ");
                }
            }
        }
        _stream->operator <<(log.c_str());
        _stream->operator <<('\n');
        _stream->flush();
    }

    void endFunction(const std::string& callerName,const std::string& function){
        QMutexLocker locker(&_lock);
        --_beginsCount;
        for(int i = 0 ; i < _beginsCount;++i){
            _stream->operator <<("    ");
        }
        _stream->operator <<("STOP ");
        _stream->operator <<(callerName.c_str());
        _stream->operator <<("    ");
        _stream->operator <<(function.c_str());
        _stream->operator <<('\n');
        _stream->flush();
        
    }
};

Log::Log(): Singleton<Natron::Log>(),_imp(new LogPrivate()){}

Log::~Log(){ delete _imp; }

void Log::open(const std::string& fileName){
    Log::instance()->_imp->open(fileName);
}

void Log::beginFunction(const std::string& callerName,const std::string& function){
    Log::instance()->_imp->beginFunction(callerName,function);
}

void Log::print(const std::string& log){
    Log::instance()->_imp->print(log);
}

void Log::print(const char *format, ...){
    va_list args;
    va_start(args, format);
    char buf[10000];
    sprintf(buf, format, args);
    va_end(args);
    std::string str(buf);
    Log::print(str);
}


void Log::endFunction(const std::string& callerName,const std::string& function){
    Log::instance()->_imp->endFunction(callerName,function);
}


}//namespace Natron
#endif
