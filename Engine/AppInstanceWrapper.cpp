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

#include <QDebug>

#include "Engine/AppInstance.h"
#include "Engine/Project.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/EffectInstance.h"
App::App(AppInstance* instance)
: Group(instance->getProject())
, _instance(instance)
{
    
}

int
App::getAppID() const
{
    return _instance->getAppID();
}

Effect*
App::createNode(const std::string& pluginID,
                int majorVersion,
                Effect* group) const
{
    boost::shared_ptr<NodeCollection> collection;
    if (group) {
        boost::shared_ptr<Natron::Node> node = group->getInternalNode();
        assert(node);
        boost::shared_ptr<NodeGroup> isGrp = boost::dynamic_pointer_cast<NodeGroup>(node->getLiveInstance()->shared_from_this());
        if (!isGrp) {
            qDebug() << "The group passed to createNode() is not a group, defaulting to the project root.";
        } else {
            collection = boost::dynamic_pointer_cast<NodeCollection>(isGrp);
            assert(collection);
        }
    }
    
    if (!collection) {
        collection = boost::dynamic_pointer_cast<NodeCollection>(_instance->getProject());
    }
    
    assert(collection);
    
    CreateNodeArgs args(pluginID.c_str(),
                        "",
                        majorVersion,
                        -1,
                        -1,
                        false,
                        INT_MIN,
                        INT_MIN,
                        false,
                        true,
                        QString(),
                        CreateNodeArgs::DefaultValuesList(),
                        collection);
    boost::shared_ptr<Natron::Node> node = _instance->createNode(args);
    if (node) {
        return new Effect(node);
    } else {
        return NULL;
    }
}

