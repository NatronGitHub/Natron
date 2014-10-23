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
* @brief Simple wrap for the AppInstance class that is the API we want to expose to the Python
* Engine module.
**/

#ifndef APPINSTANCEWRAPPER_H
#define APPINSTANCEWRAPPER_H

#include "Engine/NodeWrapper.h"

class AppInstance;

class App
{
    AppInstance* _instance;
    
public:
    
    
    App(AppInstance* instance);
    
    int getAppID() const;
    
    Effect createEffect(const std::string& pluginID,int majorVersion = -1, int minorVersion = -1) const;
};


#endif // APPINSTANCEWRAPPER_H
