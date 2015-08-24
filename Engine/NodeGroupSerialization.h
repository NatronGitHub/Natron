//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */


#ifndef NODEGROUPSERIALIZATION_H
#define NODEGROUPSERIALIZATION_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#include <boost/serialization/list.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
#endif

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeSerialization.h"
#include "Engine/KnobSerialization.h"

#define NODE_COLLECTION_SERIALIZATION_VERSION 1

#define NODE_GROUP_SERIALIZATION_VERSION 1

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
                                         std::map<std::string,bool>* moduleUpdatesProcessed,
                                         bool* hasProjectAWriter);
    
private:
                                         
    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        int nodesCount = (int)_serializedNodes.size();
        ar & boost::serialization::make_nvp("NodesCount",nodesCount);
        
        for (std::list< boost::shared_ptr<NodeSerialization> >::const_iterator it = _serializedNodes.begin();
             it != _serializedNodes.end();
             ++it) {
            ar & boost::serialization::make_nvp("item",**it);
        }
    }
    
    template<class Archive>
    void load(Archive & ar,
              const unsigned int /*version*/)
    {
        int nodesCount ;
        ar & boost::serialization::make_nvp("NodesCount",nodesCount);
        
        for (int i = 0; i < nodesCount; ++i) {
            boost::shared_ptr<NodeSerialization> s(new NodeSerialization);
            ar & boost::serialization::make_nvp("item",*s);
            _serializedNodes.push_back(s);
        }

    }
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

BOOST_CLASS_VERSION(NodeCollectionSerialization,NODE_COLLECTION_SERIALIZATION_VERSION)



#endif // NODEGROUPSERIALIZATION_H
