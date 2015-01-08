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

class App : public Group
{
    AppInstance* _instance;
    
public:
    
    
    App(AppInstance* instance);
    
    int getAppID() const;
    
    /**
     * @brief Creates a new instance of the plugin identified by the given pluginID.
     * @param majorVersion If different than -1, it will try to load a specific major version
     * of the plug-in, otherwise it will default to the greatest major version found for that plug-in.
     * @param group If not NULL, this should be a pointer to a group node where-in to insert the newly created effect.
     * If NULL, the newly created node will be inserted in the project's root.
     **/
    Effect* createNode(const std::string& pluginID,
                       int majorVersion = -1,
                       Effect* group = 0) const;
    
    int timelineGetTime() const;
    
    int timelineGetLeftBound() const;
    
    int timelineGetRightBound() const;
};


#endif // APPINSTANCEWRAPPER_H
