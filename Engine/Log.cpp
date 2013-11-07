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
#include "Log.h"
#ifdef POWITER_LOG


#include <cassert>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <string>

#include <QFile>
#include <QTextStream>

#include "Global/GlobalDefines.h"


namespace Powiter{

class LogPrivate{
    
public:
    
    QFile* _file;
    QTextStream* _stream;
    bool _isBetweenBeginAndEnd;

    LogPrivate():
        _file(NULL)
      , _stream(NULL)
      , _isBetweenBeginAndEnd(false)
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
        if(_file)
            return;
        _file = new QFile(fileName.c_str());
        _file->open(QIODevice::WriteOnly | QIODevice::Truncate);
        _stream = new QTextStream(_file);
    }

    void beginFunction(const std::string& callerName,const std::string& function){
        if(!_file){
            QString filename(POWITER_APPLICATION_NAME "_log.txt");
            open(filename.toStdString());
        }
        _isBetweenBeginAndEnd = true;
        _stream->operator <<("************************************************************************\n");
        _stream->operator <<("START ");
        _stream->operator <<(callerName.c_str());
        _stream->operator <<("    ");
        _stream->operator <<(function.c_str());
        _stream->operator <<('\n');
        _stream->flush();
    }

    void print(const std::string& callerName,const std::string& functionName,const std::string& log){
        bool wasBeginCalled = true;
        if(!_isBetweenBeginAndEnd){
            beginFunction(callerName,functionName);
            wasBeginCalled = false;
        }
        _stream->operator <<("    ");
        _stream->operator <<(log.c_str());
        _stream->operator <<('\n');
        _stream->flush();
        if(!wasBeginCalled){
            endFunction(callerName,functionName);
        }

    }

    void endFunction(const std::string& callerName,const std::string& function){
        _isBetweenBeginAndEnd = false;
        _stream->operator <<("STOP ");
        _stream->operator <<(callerName.c_str());
        _stream->operator <<("    ");
        _stream->operator <<(function.c_str());
        _stream->operator <<('\n');
        _stream->flush();
    }
};

Log::Log(): Singleton<Powiter::Log>(),_imp(new LogPrivate()){}

Log::~Log(){ delete _imp; }

void Log::open(const std::string& fileName){
    Log::instance()->_imp->open(fileName);
}

void Log::beginFunction(const std::string& callerName,const std::string& function){
    Log::instance()->_imp->beginFunction(callerName,function);
}

void Log::print(const std::string& callerName,const std::string& functionName,const std::string& log){
    Log::instance()->_imp->print(callerName,functionName,log);
}

void Log::print(const std::string& callerName,const std::string& functionName,const char *format, ...){
    va_list args;
    va_start(args, format);
    char buf[10000];
    sprintf(buf, format, args);
    va_end(args);
    std::string str(buf);
    Log::print(callerName,functionName,str);
}


void Log::endFunction(const std::string& callerName,const std::string& function){
    Log::instance()->_imp->endFunction(callerName,function);
}

#endif

}//namespace Powiter
