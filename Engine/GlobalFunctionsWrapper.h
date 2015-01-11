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

#endif // GLOBALFUNCTIONSWRAPPER_H
