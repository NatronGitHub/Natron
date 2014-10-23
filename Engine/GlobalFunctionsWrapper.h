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
#include "Engine/AppInstance.h"

class AppInstance_PythonAPI
{
    AppInstance* _instance;
    
public:
    
    
    AppInstance_PythonAPI(AppInstance* instance)
    : _instance(instance)
    {
        
    }
    
    int getAppID() const { return _instance->getAppID(); }
};


inline std::list<std::string>
getPluginIDs()
{
    return Natron::getPluginIDs();
}

inline AppInstance_PythonAPI
getInstance(int idx)
{
    return AppInstance_PythonAPI(Natron::getInstance(idx));
}

inline int
getNumInstances()
{
    return Natron::getNumInstances();
}



#endif // GLOBALFUNCTIONSWRAPPER_H
