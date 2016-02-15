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

/**
* @brief Used to wrap all global functions that are in the Natron namespace so shiboken
* doesn't generate the Natron namespace
**/

#ifndef GLOBALFUNCTIONSWRAPPER_H
#define GLOBALFUNCTIONSWRAPPER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"
#include "Engine/AppManager.h"
#include "Engine/PyAppInstance.h"
#include "Global/MemoryInfo.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

class PyCoreApplication
{
    
public:
    
    PyCoreApplication() {}
    
    virtual ~PyCoreApplication() {}
    
    inline std::list<std::string>
    getPluginIDs() const
    {
        return appPTR->getPluginIDs();
    }
    
    inline std::list<std::string>
    getPluginIDs(const std::string& filter) const
    {
        return appPTR->getPluginIDs(filter);
    }
    
    inline int
    getNumInstances() const
    {
        return appPTR->getNumInstances();
    }
    
    inline std::list<std::string>
    getNatronPath() const
    {
        return appPTR->getNatronPath();
    }
    
    inline void
    appendToNatronPath(const std::string& path)
    {
        appPTR->appendToNatronPath(path);
    }
    
    inline bool isLinux() const
    {
#ifdef __NATRON_LINUX__
        return true;
#else
        return false;
#endif
    }
    
    inline bool isWindows() const
    {
#ifdef __NATRON_WIN32__
        return true;
#else
        return false;
#endif
    }
    
    inline bool isMacOSX() const
    {
#ifdef __NATRON_OSX__
        return true;
#else
        return false;
#endif
    }
    
    inline bool isUnix() const
    {
#ifdef __NATRON_UNIX__
        return true;
#else
        return false;
#endif
    }
    
    inline std::string getNatronVersionString() const
    {
        return NATRON_VERSION_STRING;
    }
    
    inline int getNatronVersionMajor() const
    {
        return NATRON_VERSION_MAJOR;
    }
    
    inline int getNatronVersionMinor() const
    {
        return NATRON_VERSION_MINOR;
    }
    
    inline int getNatronVersionRevision() const
    {
        return NATRON_VERSION_REVISION;
    }
    
    inline int getNatronVersionEncoded() const
    {
        return NATRON_VERSION_ENCODED;
    }
    
    inline std::string getNatronDevelopmentStatus() const
    {
        return std::string(NATRON_DEVELOPMENT_STATUS);
    }
    
    inline int getBuildNumber() const
    {
        return NATRON_BUILD_NUMBER;
    }
    
    inline bool is64Bit() const
    {
        return !isApplication32Bits();
    }
    
    bool isBackground() const
    {
        return appPTR->isBackground();
    }
    
    inline int getNumCpus() const
    {
        return appPTR->getHardwareIdealThreadCount();
    }
    
    inline App*
    getInstance(int idx) const
    {
        AppInstance* app = appPTR->getAppInstance(idx);
        if (!app) {
            return 0;
        }
        return new App(app);
    }
    
    
    inline AppSettings* getSettings() const
    {
        return new AppSettings(appPTR->getCurrentSettings());
    }
    
    inline void setOnProjectCreatedCallback(const std::string& pythonFunctionName)
    {
        appPTR->setOnProjectCreatedCallback(pythonFunctionName);
    }
    
    inline void setOnProjectLoadedCallback(const std::string& pythonFunctionName)
    {
        appPTR->setOnProjectLoadedCallback(pythonFunctionName);
    }
    
    

};

NATRON_NAMESPACE_EXIT;

#if defined(PYSIDE_H) && defined(PYSIDE_OLD)
namespace PySide {
PYSIDE_API PyObject* getWrapperForQObject(QObject* cppSelf, SbkObjectType* sbk_type);
}
#endif



#endif // GLOBALFUNCTIONSWRAPPER_H
