/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_ENGINE_LOG_H
#define NATRON_ENGINE_LOG_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#ifdef NATRON_LOG

#include "Global/Macros.h"
#include "Engine/Singleton.h"
#include "Engine/EngineFwd.h"


namespace Natron {
class LogPrivate;
class Log
    : public Singleton<Natron::Log>
{
    LogPrivate* _imp;

public:

    Log();

    virtual ~Log() FINAL;

    /**
     * @brief Opens a new file with the name 'fileName' which
     * will serve as a log sink for next calls.
     **/
    static void open(const std::string & fileName);

    /**
     * @brief Begins a new function in the log. It will print a new delimiter
     * and a START tag. This is used to bracket a call to print.
     **/
    static void beginFunction(const std::string & callerName,const std::string & function);

    /**
     * @brief Prints the content of 'log' into the log's file. If beginFunction was
     * not called then it will be called with functionName. Do not insert line-breaks
     * the log will do it for you and format the content to 80 columns.
     **/
    static void print(const std::string & log);

    /**
     * @brief Same as print but using printf-like formating. Do not insert line-breaks
     * the log will do it for you and format the content to 80 columns.
     **/
    static void print(const char *format, ...);

    /**
     * @brief Ends a function in the log. It will print a new delimiter
     * and a STOP tag. This is used to bracket a call to print.
     **/
    static void endFunction(const std::string & callerName,const std::string & function);
    static bool enabled()
    {
        return true;
    }
};
}
#else
#include <string>

namespace Natron {
// no-op class
class Log
{
    Log()
    {
    }

    ~Log()
    {
    };

public:
    static void instance()
    {
    }

    static void open(const std::string & )
    {
    }

    static void beginFunction(const std::string &,
                              const std::string & )
    {
    }

    static void print(const std::string & )
    {
    }

    static void print(const char *,
                      ...)
    {
    }

    static void endFunction(const std::string &,
                            const std::string & )
    {
    }

    static bool enabled()
    {
        return false;
    }
};
}

#endif // ifdef NATRON_LOG


#endif // NATRON_ENGINE_LOG_H
