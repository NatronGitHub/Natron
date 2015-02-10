//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "NodeGroupWrapper.h"

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeWrapper.h"

Group::Group()
: _collection()
{
    
}

void
Group::init(const boost::shared_ptr<NodeCollection>& collection)
{
    _collection = collection;
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