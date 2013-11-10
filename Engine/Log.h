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

#ifndef LOG_H
#define LOG_H

#ifdef NATRON_LOG

#include "Engine/Singleton.h"
namespace Natron{

class LogPrivate;
class Log : public Singleton<Natron::Log>
{
    LogPrivate* _imp;
public:

    Log();

    virtual ~Log();

    /**
    * @brief Opens a new file with the name 'fileName' which
    * will serve as a log sink for next calls.
    **/
    static void open(const std::string& fileName);

    /**
    * @brief Begins a new function in the log. It will print a new delimiter
    * and a START tag. This is used to bracket a call to print.
    **/
    static void beginFunction(const std::string& callerName,const std::string& function);

    /**
    * @brief Prints the content of 'log' into the log's file. If beginFunction was
    * not called then it will be called with functionName. Do not insert line-breaks
    * the log will do it for you and format the content to 80 columns.
    **/
    static void print(const std::string& log);

    /**
     * @brief Same as print but using printf-like formating. Do not insert line-breaks
     * the log will do it for you and format the content to 80 columns.
     **/
    static void print(const char *format, ...);

    /**
    * @brief Ends a function in the log. It will print a new delimiter
    * and a STOP tag. This is used to bracket a call to print.
    **/
    static void endFunction(const std::string& callerName,const std::string& function);

};

}

#endif


#endif // LOG_H
