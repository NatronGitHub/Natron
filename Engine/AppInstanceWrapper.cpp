//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */


#include "AppInstanceWrapper.h"

#include "Engine/AppInstance.h"

App::App(AppInstance* instance)
: _instance(instance)
{
    
}

int
App::getAppID() const
{
    return _instance->getAppID();
}

Effect
App::createEffect(const std::string& pluginID,int majorVersion, int minorVersion) const
{
    CreateNodeArgs args(pluginID.c_str(),
                        "",
                        majorVersion,
                        minorVersion,
                        -1,
                        false,
                        INT_MIN,
                        INT_MIN,
                        false,
                        true,
                        QString(),
                        CreateNodeArgs::DefaultValuesList());
    return Effect(_instance->createNode(args));
}