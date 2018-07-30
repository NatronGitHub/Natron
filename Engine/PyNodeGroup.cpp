/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "PyNodeGroup.h"

#include <cassert>
#include <stdexcept>

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/PyNode.h"
#include "Engine/ReadNode.h"
NATRON_NAMESPACE_ENTER
NATRON_PYTHON_NAMESPACE_ENTER

Group::Group()
    : _collection()
{
}

void
Group::init(const NodeCollectionPtr& collection)
{
    _collection = collection;
}

Group::~Group()
{
}

Effect*
Group::getNode(const QString& fullySpecifiedName) const
{
    if ( !_collection.lock() ) {
        return 0;
    }
    NodePtr node = _collection.lock()->getNodeByFullySpecifiedName( fullySpecifiedName.toStdString() );
    if ( node && node->isActivated() ) {
        return new Effect(node);
    } else {
        return NULL;
    }
}

std::list<Effect*>
Group::getChildren() const
{
    std::list<Effect*> ret;

    if ( !_collection.lock() ) {
        return ret;
    }

    NodesList nodes = _collection.lock()->getNodes();

    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( (*it)->isActivated() && (*it)->getParentMultiInstanceName().empty() ) {


#if NATRON_VERSION_ENCODED < NATRON_VERSION_ENCODE(3,0,0)
            // In Natron 2, the meta Read node is NOT a Group, hence the internal decoder node is not a child of the Read node.
            // As a result of it, if the user deleted the meta Read node, the internal decoder is node destroyed
            if ((*it)->getIOContainer()) {
                continue;
            }
#endif

            ret.push_back( new Effect(*it) );
        }
    }

    return ret;
}

NATRON_PYTHON_NAMESPACE_EXIT
NATRON_NAMESPACE_EXIT
