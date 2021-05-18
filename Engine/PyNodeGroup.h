/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Engine_NodeGroupWrapper_h
#define Engine_NodeGroupWrapper_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;
NATRON_PYTHON_NAMESPACE_ENTER;

class Group
{
    NodeCollectionWPtr _collection;

public:

    Group();

    void init(const NodeCollectionPtr& collection);

    virtual ~Group();

    NodeCollectionPtr getInternalCollection() const
    {
        return _collection.lock();
    }

    /**
     * @brief Get a pointer to a node. The node is identified by its fully qualified name, e.g:
     * Group1.Blur1 if it belongs to the Group1, or just Blur1 if it belongs to the project's root.
     * This function is called recursively on subgroups until a match is found.
     * It is meant to be called only on the project's root.
     **/
    Effect* getNode(const QString& fullySpecifiedName) const;

    /**
     * @brief Get all nodes in the project's root.
     **/
    std::list<Effect*> getChildren() const;
};

NATRON_PYTHON_NAMESPACE_EXIT
NATRON_NAMESPACE_EXIT

#endif // Engine_NodeGroupWrapper_h
