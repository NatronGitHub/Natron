/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

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
    
    for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ((*it)->isActivated() && (*it)->getParentMultiInstanceName().empty()) {
            ret.push_back(new Effect(*it));
        }
    }
    return ret;
}