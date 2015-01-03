//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "NodeGroupWrapper.h"

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeWrapper.h"

Group::Group(const boost::shared_ptr<NodeCollection>& collection)
: _collection(collection)
{
    
}

Group::~Group()
{
    
}

Effect*
Group::getNode(const std::string& fullySpecifiedName) const
{
    if (!_collection.lock()) {
        return 0;
    }
    boost::shared_ptr<Natron::Node> node = _collection.lock()->getNodeByFullySpecifiedName(fullySpecifiedName);
    if (node && node->isActivated()) {
        return new Effect(node);
    } else {
        return NULL;
    }
}

std::list<Effect*>
Group::getChildren() const
{
    std::list<Effect*> ret;
    if (!_collection.lock()) {
        return ret;
    }

    NodeList nodes = _collection.lock()->getNodes();
    
    for (NodeList::iterator it = nodes.begin(); it!=nodes.end(); ++it) {
        if ((*it)->isActivated() && (*it)->getParentMultiInstanceName().empty()) {
            ret.push_back(new Effect(*it));
        }
    }
    return ret;
}