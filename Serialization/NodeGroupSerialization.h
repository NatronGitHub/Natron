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

#ifndef NODEGROUPSERIALIZATION_H
#define NODEGROUPSERIALIZATION_H

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Serialization/NodeSerialization.h"
#include "Serialization/KnobSerialization.h"


#define NODE_COLLECTION_SERIALIZATION_VERSION 1

#define NODE_GROUP_SERIALIZATION_VERSION 1

SERIALIZATION_NAMESPACE_ENTER

/**
 * @brief Deprecated: just used for backward compat
 **/
class NodeCollectionSerialization
{

private:

    // The list of all nodes in the collection
    std::list< NodeSerializationPtr > _serializedNodes;

public:

    NodeCollectionSerialization()
    {
    }

    virtual ~NodeCollectionSerialization()
    {
        _serializedNodes.clear();
    }

    const std::list< NodeSerializationPtr > & getNodesSerialization() const
    {
        return _serializedNodes;
    }

    void addNodeSerialization(const NodeSerializationPtr& s)
    {
        _serializedNodes.push_back(s);
    }


private:

    friend class ::boost::serialization::access;
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        int nodesCount = (int)_serializedNodes.size();
        ar & ::boost::serialization::make_nvp("NodesCount", nodesCount);

        for (std::list< NodeSerializationPtr >::const_iterator it = _serializedNodes.begin();
             it != _serializedNodes.end();
             ++it) {
            ar & ::boost::serialization::make_nvp("item", **it);
        }
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int /*version*/)
    {
        int nodesCount;
        ar & ::boost::serialization::make_nvp("NodesCount", nodesCount);

        for (int i = 0; i < nodesCount; ++i) {
            NodeSerializationPtr s(new NodeSerialization);
            ar & ::boost::serialization::make_nvp("item", *s);
            _serializedNodes.push_back(s);
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

SERIALIZATION_NAMESPACE_EXIT;

BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::NodeCollectionSerialization, NODE_COLLECTION_SERIALIZATION_VERSION)

#endif // #ifdef NATRON_BOOST_SERIALIZATION_COMPAT
#endif // NODEGROUPSERIALIZATION_H
