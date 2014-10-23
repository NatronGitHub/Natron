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



#endif // GLOBALFUNCTIONSWRAPPER_H
