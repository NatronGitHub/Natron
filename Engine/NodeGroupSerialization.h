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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#endif

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeSerialization.h"
#include "Engine/KnobSerialization.h"
#include "Engine/EngineFwd.h"


#define NODE_COLLECTION_SERIALIZATION_VERSION 1

#define NODE_GROUP_SERIALIZATION_VERSION 1

NATRON_NAMESPACE_ENTER;

class NodeCollectionSerialization
{
    std::list< boost::shared_ptr<NodeSerialization> > _serializedNodes;
    
public:
    
    NodeCollectionSerialization()
    {
    }
    
    virtual ~NodeCollectionSerialization()
    {
        _serializedNodes.clear();
    }
    
    void initialize(const NodeCollection& group);
    
    const std::list< boost::shared_ptr<NodeSerialization> > & getNodesSerialization() const
    {
        return _serializedNodes;
    }
    
    void addNodeSerialization(const boost::shared_ptr<NodeSerialization>& s)
    {
        _serializedNodes.push_back(s);
    }
    
    static bool restoreFromSerialization(const std::list< boost::shared_ptr<NodeSerialization> > & serializedNodes,
                                         const boost::shared_ptr<NodeCollection>& group,
                                         bool createNodes,
                                         std::map<std::string,bool>* moduleUpdatesProcessed);
    
private:
                                         
    friend class ::boost::serialization::access;
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        int nodesCount = (int)_serializedNodes.size();
        ar & ::boost::serialization::make_nvp("NodesCount",nodesCount);
        
        for (std::list< boost::shared_ptr<NodeSerialization> >::const_iterator it = _serializedNodes.begin();
             it != _serializedNodes.end();
             ++it) {
            ar & ::boost::serialization::make_nvp("item",**it);
        }
    }
    
    template<class Archive>
    void load(Archive & ar,
              const unsigned int /*version*/)
    {
        int nodesCount ;
        ar & ::boost::serialization::make_nvp("NodesCount",nodesCount);
        
        for (int i = 0; i < nodesCount; ++i) {
            boost::shared_ptr<NodeSerialization> s(new NodeSerialization);
            ar & ::boost::serialization::make_nvp("item",*s);
            _serializedNodes.push_back(s);
        }

    }
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

NATRON_NAMESPACE_EXIT;

BOOST_CLASS_VERSION(NATRON_NAMESPACE::NodeCollectionSerialization,NODE_COLLECTION_SERIALIZATION_VERSION)



#endif // NODEGROUPSERIALIZATION_H
