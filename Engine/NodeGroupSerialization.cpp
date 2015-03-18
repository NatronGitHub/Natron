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

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "NodeGroupSerialization.h"

#include "Engine/AppInstance.h"
#include "Engine/NodeGroup.h"
#include "Engine/ViewerInstance.h"

void
NodeCollectionSerialization::initialize(const NodeCollection& group)
{
    NodeList nodes;
    group.getActiveNodes(&nodes);
    
    _serializedNodes.clear();
    
    for (NodeList::iterator it = nodes.begin(); it != nodes.end() ; ++it) {
        if (!(*it)->getParentMultiInstance()) {
            boost::shared_ptr<NodeSerialization> state(new NodeSerialization(*it));
            _serializedNodes.push_back(state);
        }
    }
}

bool
NodeCollectionSerialization::restoreFromSerialization(const std::list< boost::shared_ptr<NodeSerialization> > & serializedNodes,
                                                      const boost::shared_ptr<NodeCollection>& group,
                                                      bool* hasProjectAWriter)
{
    

    bool mustShowErrorsLog = false;
    
    NodeGroup* isNodeGroup = dynamic_cast<NodeGroup*>(group.get());
    QString groupName;
    if (isNodeGroup) {
        groupName = isNodeGroup->getNode()->getLabel().c_str();
    } else {
        groupName = QObject::tr("top-level");
    }
    group->getApplication()->updateProjectLoadStatus(QObject::tr("Creating nodes in group: ") + groupName);
    
    ///If a parent of a multi-instance node doesn't exist anymore but the children do, we must recreate the parent.
    ///Problem: we have lost the nodes connections. To do so we restore them using the serialization of a child.
    ///This map contains all the parents that must be reconnected and an iterator to the child serialization
    std::map<boost::shared_ptr<Natron::Node>, std::list<boost::shared_ptr<NodeSerialization> >::const_iterator > parentsToReconnect;
    
    int nodesRestored = 0;
    for (std::list< boost::shared_ptr<NodeSerialization> >::const_iterator it = serializedNodes.begin(); it != serializedNodes.end(); ++it) {
       
        ++nodesRestored;
        
        std::string pluginID = (*it)->getPluginID();
        
        if ( appPTR->isBackground() && (pluginID == PLUGINID_NATRON_VIEWER || pluginID == "Viewer") ) {
            //if the node is a viewer, don't try to load it in background mode
            continue;
        }
        
        ///If the node is a multiinstance child find in all the serialized nodes if the parent exists.
        ///If not, create it
        
        if ( !(*it)->getMultiInstanceParentName().empty() ) {
            
            bool foundParent = false;
            for (std::list< boost::shared_ptr<NodeSerialization> >::const_iterator it2 = serializedNodes.begin();
                 it2 != serializedNodes.end(); ++it2) {
                
                if ( (*it2)->getNodeScriptName() == (*it)->getMultiInstanceParentName() ) {
                    foundParent = true;
                    break;
                }
                
            }
            if (!foundParent) {
                ///Maybe it was created so far by another child who created it so look into the nodes
                
                NodePtr parent = group->getNodeByName((*it)->getMultiInstanceParentName());
                if (parent) {
                    foundParent = true;
                }
                ///Create the parent
                if (!foundParent) {
                    boost::shared_ptr<Natron::Node> parent = group->getApplication()->createNode(CreateNodeArgs( pluginID.c_str(),
                                                                                                                "",
                                                                                                                (*it)->getPluginMajorVersion(),
                                                                                                                (*it)->getPluginMinorVersion(),
                                                                                                                true,
                                                                                                                INT_MIN,
                                                                                                                INT_MIN,
                                                                                                                false,
                                                                                                                true,
                                                                                                                false,
                                                                                                                QString(),
                                                                                                                CreateNodeArgs::DefaultValuesList(),
                                                                                                                group));
                    parent->setScriptName( (*it)->getMultiInstanceParentName().c_str() );
                    parentsToReconnect.insert( std::make_pair(parent, it) );
                }
            }
        }
        
        boost::shared_ptr<Natron::Node> n = group->getApplication()->loadNode( LoadNodeArgs(pluginID.c_str()
                                                                                               ,(*it)->getMultiInstanceParentName()
                                                                                               ,(*it)->getPluginMajorVersion()
                                                                                               ,(*it)->getPluginMinorVersion(),it->get(),false,group) );
        if (!n) {
            QString text( QObject::tr("The node ") );
            text.append( pluginID.c_str() );
            text.append( QObject::tr(" was found in the script but doesn't seem \n"
                                     "to exist in the currently loaded plug-ins.") );
            appPTR->writeToOfxLog_mt_safe(text);
            mustShowErrorsLog = true;
            continue;
        }
        if ( n->isOutputNode() ) {
            *hasProjectAWriter = true;
        }
        
        const std::list<boost::shared_ptr<NodeSerialization> >& children = (*it)->getNodesCollection();
        if (!children.empty()) {
            NodeGroup* isGrp = dynamic_cast<NodeGroup*>(n->getLiveInstance());
            if (isGrp) {
                boost::shared_ptr<Natron::EffectInstance> sharedEffect = isGrp->shared_from_this();
                boost::shared_ptr<NodeGroup> sharedGrp = boost::dynamic_pointer_cast<NodeGroup>(sharedEffect);
                NodeCollectionSerialization::restoreFromSerialization(children, sharedGrp , hasProjectAWriter);
            } else {
                assert(n->isMultiInstance());
                NodeCollectionSerialization::restoreFromSerialization(children, group, hasProjectAWriter);
            }
        }
    }
    
    
    group->getApplication()->updateProjectLoadStatus(QObject::tr("Restoring graph links in group: ") + groupName);

    NodeList nodes = group->getNodes();
    
    /// Connect the nodes together, and restore the slave/master links for all knobs.
    for (std::list< boost::shared_ptr<NodeSerialization> >::const_iterator it = serializedNodes.begin(); it != serializedNodes.end(); ++it) {
        if ( appPTR->isBackground() && ((*it)->getPluginID() == PLUGINID_NATRON_VIEWER) ) {
            //ignore viewers on background mode
            continue;
        }
        
        
        boost::shared_ptr<Natron::Node> thisNode = group->getNodeByName((*it)->getNodeScriptName());
        
        if (!thisNode) {
            continue;
        }
        
        ///for all nodes that are part of a multi-instance, fetch the main instance node pointer
        const std::string & parentName = (*it)->getMultiInstanceParentName();
        if ( !parentName.empty() ) {
            thisNode->fetchParentMultiInstancePointer();
        }
        
        ///restore slave/master link if any
        const std::string & masterNodeName = (*it)->getMasterNodeName();
        if ( !masterNodeName.empty() ) {
            ///find such a node
            boost::shared_ptr<Natron::Node> masterNode = thisNode->getApp()->getNodeByFullySpecifiedName(masterNodeName);
            
            if (!masterNode) {
                appPTR->writeToOfxLog_mt_safe(QString("Cannot restore the link between " + QString((*it)->getNodeScriptName().c_str()) + " and " + masterNodeName.c_str()));
                mustShowErrorsLog = true;
            } else {
                thisNode->getLiveInstance()->slaveAllKnobs( masterNode->getLiveInstance() );
            }
        } else {
            thisNode->restoreKnobsLinks(**it,nodes);
        }
        
        const std::vector<std::string> & oldInputs = (*it)->getOldInputs();
        if (!oldInputs.empty()) {
            
            /*
             * Prior to Natron v2 OpenFX effects had their inputs reversed internally
             */
            bool isOfxEffect = thisNode->isOpenFXNode();
            
            for (U32 j = 0; j < oldInputs.size(); ++j) {
                if ( !oldInputs[j].empty() && !group->connectNodes(isOfxEffect ? oldInputs.size() - 1 - j : j, oldInputs[j],thisNode.get()) ) {
                    std::string message = std::string("Failed to connect node ") + (*it)->getNodeScriptName() + " to " + oldInputs[j];
                    appPTR->writeToOfxLog_mt_safe(message.c_str());
                    mustShowErrorsLog =true;
                }
            }
        } else {
            const std::map<std::string,std::string>& inputs = (*it)->getInputs();
            for (std::map<std::string,std::string>::const_iterator it2 = inputs.begin(); it2 != inputs.end(); ++it2) {
                if (it2->second.empty()) {
                    continue;
                }
                int index = thisNode->getInputNumberFromLabel(it2->first);
                if (index == -1) {
                    appPTR->writeToOfxLog_mt_safe(QString("Could not find input named ") + it2->first.c_str());
                    continue;
                }
                if (!it2->second.empty() && !group->connectNodes(index, it2->second, thisNode.get())) {
                    std::string message = std::string("Failed to connect node ") + (*it)->getNodeScriptName() + " to " + it2->second;
                    appPTR->writeToOfxLog_mt_safe(message.c_str());
                    mustShowErrorsLog =true;
                }
            }
        }
    }
    
    ///Also reconnect parents of multiinstance nodes that were created on the fly
    for (std::map<boost::shared_ptr<Natron::Node>, std::list<boost::shared_ptr<NodeSerialization> >::const_iterator >::const_iterator
         it = parentsToReconnect.begin(); it != parentsToReconnect.end(); ++it) {
        const std::vector<std::string> & oldInputs = (*it->second)->getOldInputs();
        if (!oldInputs.empty()) {
            /*
             * Prior to Natron v2 OpenFX effects had their inputs reversed internally
             */
            bool isOfxEffect = it->first->isOpenFXNode();
            
            for (U32 j = 0; j < oldInputs.size(); ++j) {
                if ( !oldInputs[j].empty() && !group->connectNodes(isOfxEffect ? oldInputs.size() - 1 - j : j, oldInputs[j],it->first.get()) ) {
                    std::string message = std::string("Failed to connect node ") + it->first->getPluginLabel() + " to " + oldInputs[j];
                    appPTR->writeToOfxLog_mt_safe(message.c_str());
                    mustShowErrorsLog =true;
                    
                }
            }
        } else {
            const std::map<std::string,std::string>& inputs = (*it->second)->getInputs();
            for (std::map<std::string,std::string>::const_iterator it2 = inputs.begin(); it2 != inputs.end(); ++it2) {
                if (it2->second.empty()) {
                    continue;
                }
                int index = it->first->getInputNumberFromLabel(it2->first);
                if (index == -1) {
                    appPTR->writeToOfxLog_mt_safe(QString("Could not find input named ") + it2->first.c_str());
                    continue;
                }
                if (!it2->second.empty() && !group->connectNodes(index, it2->second, it->first.get())) {
                    std::string message = std::string("Failed to connect node ") + it->first->getPluginLabel() + " to " + it2->second;
                    appPTR->writeToOfxLog_mt_safe(message.c_str());
                    mustShowErrorsLog =true;
                }
            }
        }
    }
    return !mustShowErrorsLog;
}