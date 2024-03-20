/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef GLOBALFUNCTIONSWRAPPER_H
#define GLOBALFUNCTIONSWRAPPER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

/**
 * @brief Used to wrap all global functions that are in the Natron namespace so shiboken
 * doesn't generate the Natron namespace
 **/

#include "Engine/AppManager.h"
#include "Engine/MemoryInfo.h" // isApplication32Bits
#include "Engine/PyAppInstance.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;
NATRON_PYTHON_NAMESPACE_ENTER;

class PyCoreApplication
{
public:

    PyCoreApplication() {}

    virtual ~PyCoreApplication() {}

    inline QStringList getPluginIDs() const
    {
        QStringList ret;
        std::list<std::string> list =  appPTR->getPluginIDs();

        for (std::list<std::string>::iterator it = list.begin(); it != list.end(); ++it) {
            ret.push_back( QString::fromUtf8( it->c_str() ) );
        }

        return ret;
    }

    inline QStringList getPluginIDs(const QString& filter) const
    {
        QStringList ret;
        std::list<std::string> list =  appPTR->getPluginIDs( filter.toStdString() );

        for (std::list<std::string>::iterator it = list.begin(); it != list.end(); ++it) {
            ret.push_back( QString::fromUtf8( it->c_str() ) );
        }

        return ret;
    }

    inline int getNumInstances() const
    {
        return appPTR->getNumInstances();
    }

    inline QStringList getNatronPath() const
    {
        QStringList ret;
        std::list<std::string> list =  appPTR->getNatronPath();

        for (std::list<std::string>::iterator it = list.begin(); it != list.end(); ++it) {
            ret.push_back( QString::fromUtf8( it->c_str() ) );
        }

        return ret;
    }

    inline void appendToNatronPath(const QString& path)
    {
        appPTR->appendToNatronPath( path.toStdString() );
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

    inline QString getNatronVersionString() const
    {
        return QString::fromUtf8(NATRON_VERSION_STRING);
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

    inline QString getNatronDevelopmentStatus() const
    {
        return QString::fromUtf8(NATRON_DEVELOPMENT_STATUS);
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

    inline App* getInstance(int idx) const
    {
        AppInstancePtr app = appPTR->getAppInstance(idx);

        if (!app) {
            return 0;
        }

        return new App(app);
    }

    inline App* getActiveInstance() const
    {
        AppInstancePtr app = appPTR->getTopLevelInstance();

        if (!app) {
            return 0;
        }

        return new App(app);
    }

    inline AppSettings* getSettings() const
    {
        return new AppSettings( appPTR->getCurrentSettings() );
    }

    inline void setOnProjectCreatedCallback(const QString& pythonFunctionName)
    {
        appPTR->setOnProjectCreatedCallback( pythonFunctionName.toStdString() );
    }

    inline void setOnProjectLoadedCallback(const QString& pythonFunctionName)
    {
        appPTR->setOnProjectLoadedCallback( pythonFunctionName.toStdString() );
    }
};

NATRON_PYTHON_NAMESPACE_EXIT;
NATRON_NAMESPACE_EXIT;

#endif // GLOBALFUNCTIONSWRAPPER_H
