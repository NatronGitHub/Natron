//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */
/**
* @brief Used to wrap all global functions that are in the Natron namespace so shiboken
* doesn't generate the Natron namespace
**/

#ifndef GLOBALFUNCTIONSWRAPPER_H
#define GLOBALFUNCTIONSWRAPPER_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Engine/AppManager.h"
#include "Engine/AppInstanceWrapper.h"
#include "Global/MemoryInfo.h"


inline std::list<std::string>
getPluginIDs()
{
    return Natron::getPluginIDs();
}

inline App
getInstance(int idx)
{
    return App(Natron::getInstance(idx));
}

inline int
getNumInstances()
{
    return Natron::getNumInstances();
}

inline std::list<std::string>
getNatronPath()
{
    return Natron::getNatronPath();
}

inline void
appendToNatronPath(const std::string& path)
{
    Natron::appendToNatronPath(path);
}

inline bool isLinux()
{
#ifdef __NATRON_LINUX__
    return true;
#else
    return false;
#endif
}

inline bool isWindows()
{
#ifdef __NATRON_WIN32__
    return true;
#else
    return false;
#endif
}

inline bool isMacOSX()
{
#ifdef __NATRON_OSX__
    return true;
#else
    return false;
#endif
}

inline bool isUnix()
{
#ifdef __NATRON_UNIX__
    return true;
#else
    return false;
#endif
}

inline std::string getNatronVersionString()
{
    return std::string(NATRON_VERSION_STRING);
}

inline int getNatronVersionMajor()
{
    return NATRON_VERSION_MAJOR;
}

inline int getNatronVersionMinor()
{
    return NATRON_VERSION_MINOR;
}

inline int getNatronVersionRevision()
{
    return NATRON_VERSION_REVISION;
}

inline int getNatronVersionEncoded()
{
    return NATRON_VERSION_ENCODED;
}

inline std::string getNatronDevelopmentStatus()
{
    return std::string(NATRON_DEVELOPMENT_STATUS);
}

inline int getBuildNumber()
{
    return NATRON_BUILD_NUMBER;
}

inline bool is64Bit()
{
    return !isApplication32Bits();
}

inline int getNumCpus()
{
    return appPTR->getHardwareIdealThreadCount();
}



#endif // GLOBALFUNCTIONSWRAPPER_H
