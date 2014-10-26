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
#include "Engine/Project.h"
#include "Engine/Node.h"

App::App(AppInstance* instance)
: _instance(instance)
{
    
}

int
App::getAppID() const
{
    return _instance->getAppID();
}

Effect*
App::createNode(const std::string& pluginID,int majorVersion, int minorVersion) const
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
    boost::shared_ptr<Natron::Node> node = _instance->createNode(args);
    if (node) {
        return new Effect(node);
    } else {
        return NULL;
    }
}

Effect*
App::getNode(const std::string& name) const
{
    boost::shared_ptr<Natron::Node> node = _instance->getProject()->getNodeByName(name);
    if (node) {
        return new Effect(node);
    } else {
        return NULL;
    }
}

std::list<Effect*>
App::getNodes() const
{
    std::list<Effect*> ret;
    std::vector<boost::shared_ptr<Natron::Node> > nodes = _instance->getProject()->getCurrentNodes();
    
    for (std::vector<boost::shared_ptr<Natron::Node> >::const_iterator it = nodes.begin(); it!=nodes.end(); ++it) {
        if ((*it)->isActivated() && (*it)->getParentMultiInstanceName().empty()) {
            ret.push_back(new Effect(*it));
        }
    }
    return ret;
}