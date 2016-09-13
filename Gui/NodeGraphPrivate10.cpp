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

#include "NodeGraphPrivate.h"
#include "NodeGraph.h"

#include <stdexcept>

#include <QDebug>

#include "Engine/CreateNodeArgs.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"
#include "Engine/RotoLayer.h"

#include "Gui/DotGui.h"
#include "Gui/Edge.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h" // appPTR
#include "Gui/NodeGui.h"


#include "Serialization/NodeClipBoard.h"
#include "Serialization/NodeSerialization.h"

NATRON_NAMESPACE_ENTER;


void
NodeGraphPrivate::pasteNodesInternal(const SERIALIZATION_NAMESPACE::NodeSerializationList & clipboard,
                                     const QPointF& scenePos,
                                     bool useUndoCommand,
                                     std::list<std::pair<std::string, NodeGuiPtr > > *newNodes)
{
    if (clipboard.empty()) {
        return;
    }
    double xmax = INT_MIN;
    double xmin = INT_MAX;
    double ymin = INT_MAX;
    double ymax = INT_MIN;
    
    for (SERIALIZATION_NAMESPACE::NodeSerializationList::const_iterator it = clipboard.begin();
         it != clipboard.end(); ++it) {
        
        if ( ((*it)->_nodePositionCoords[0] + (*it)->_nodeSize[0]) > xmax ) {
            xmax = (*it)->_nodePositionCoords[0];
        }
        if ((*it)->_nodePositionCoords[0] < xmin) {
            xmin = (*it)->_nodePositionCoords[0];
        }
        if ( ((*it)->_nodePositionCoords[1] + (*it)->_nodeSize[1]) > ymax ) {
            ymax = (*it)->_nodePositionCoords[1];
        }
        if ((*it)->_nodePositionCoords[1] < ymin) {
            ymin = (*it)->_nodePositionCoords[1];
        }
    }
    
    
    QPointF offset((xmin + xmax) / 2., (ymin + ymax) / 2.);
    
    NodesGuiList newNodesList;
    std::list<std::pair<SERIALIZATION_NAMESPACE::NodeSerializationPtr, NodePtr > > newNodesMap;
    
    ///The script-name of the copy node is different than the one of the original one
    ///We store the mapping so we can restore node links correctly
    std::map<std::string, std::string> oldNewScriptNamesMap;
    {
        CreatingNodeTreeFlag_RAII createNodeTree( _publicInterface->getGui()->getApp() );
        for (SERIALIZATION_NAMESPACE::NodeSerializationList::const_iterator it = clipboard.begin();
             it != clipboard.end(); ++it) {
            const std::string& oldScriptName = (*it)->_nodeScriptName;
            NodeGuiPtr node = NodeGraphPrivate::pasteNode(*it, offset, scenePos, group.lock(), std::string(), NodePtr(), &oldNewScriptNamesMap);
            
            if (!node) {
                continue;
            }
            newNodes->push_back( std::make_pair(oldScriptName, node) );
            newNodesList.push_back(node);
            newNodesMap.push_back( std::make_pair( *it, node->getNode() ) );
            
            const std::string& newScriptName = node->getNode()->getScriptName();
            oldNewScriptNamesMap[oldScriptName] = newScriptName;
        }
        assert( clipboard.size() == newNodes->size() );
        
        ///Now that all nodes have been duplicated, try to restore nodes connections
        NodeGraphPrivate::restoreConnections(clipboard, *newNodes, oldNewScriptNamesMap);
        
        NodesList allNodes;
        _publicInterface->getGui()->getApp()->getProject()->getActiveNodes(&allNodes);
        
        
        //Restore links once all children are created for alias knobs/expressions
        for (std::list<std::pair<SERIALIZATION_NAMESPACE::NodeSerializationPtr, NodePtr > > ::iterator it = newNodesMap.begin(); it != newNodesMap.end(); ++it) {
            it->second->restoreKnobsLinks(*(it->first), allNodes, oldNewScriptNamesMap);
        }
    }
    
    
    if (newNodesList.size() > 1) {
        ///Only compute datas if we're pasting more than 1 node
        _publicInterface->getGui()->getApp()->getProject()->forceComputeInputDependentDataOnAllTrees();
    } else if (newNodesList.size() == 1) {
        newNodesList.front()->getNode()->getEffectInstance()->refreshMetaDatas_public(true);
    }
    
    if (useUndoCommand) {
        _publicInterface->pushUndoCommand( new AddMultipleNodesCommand(_publicInterface, newNodesList) );
    }
} // pasteNodesInternal

NodeGuiPtr
NodeGraphPrivate::pasteNode(const SERIALIZATION_NAMESPACE::NodeSerializationPtr & internalSerialization,
                            const QPointF& averageNodesPosition,
                            const QPointF& position,
                            const NodeCollectionPtr& groupContainer,
                            const std::string& parentName,
                            const NodePtr& cloneMaster,
                            std::map<std::string, std::string>* oldNewScriptNameMapping)
{
    assert(groupContainer && internalSerialization);

    // Create the duplicate node
    NodePtr duplicateNode;
    {
        CreateNodeArgs args(internalSerialization->_pluginID, groupContainer);
        args.setProperty<SERIALIZATION_NAMESPACE::NodeSerializationPtr >(kCreateNodeArgsPropNodeSerialization, internalSerialization);
        if (!parentName.empty()) {
            args.setProperty<std::string>(kCreateNodeArgsPropMultiInstanceParentName, parentName);
        }
        args.setProperty<int>(kCreateNodeArgsPropPluginVersion, internalSerialization->_pluginMajorVersion, 0);
        args.setProperty<int>(kCreateNodeArgsPropPluginVersion, internalSerialization->_pluginMinorVersion, 1);
        args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
        args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);

        duplicateNode = groupContainer->getApplication()->createNode(args);

        if (!duplicateNode) {
            return NodeGuiPtr();
        }
    }

    // This is the node gui of the duplicate node
    NodeGuiPtr duplicateNodeUI = boost::dynamic_pointer_cast<NodeGui>(duplicateNode->getNodeGui());
    assert(duplicateNodeUI);

    // Check if the node originates from the same group or not
    NodeGroupPtr isContainerGroup = toNodeGroup(groupContainer);
    std::string targetGroupFullScriptName;
    if (isContainerGroup) {
        targetGroupFullScriptName = isContainerGroup->getNode()->getFullyQualifiedName();
    }

    const std::string& originalGroupFullScriptName = internalSerialization->_groupFullyQualifiedScriptName;

    // Set the new label of the node
    // If we paste the node in a different graph, it can keep its scriptname/label
    std::string name;
    if (targetGroupFullScriptName == originalGroupFullScriptName) {
        // We pasted the node in the same group, give it another label
        int no = 1;
        std::string label = internalSerialization->_nodeLabel;
        do {
            if (no > 1) {
                std::stringstream ss;
                ss << internalSerialization->_nodeLabel;
                ss << '_';
                ss << no;
                label = ss.str();
            }
            ++no;
        } while ( groupContainer->checkIfNodeLabelExists( label, duplicateNode ) );

        duplicateNode->setLabel(label);
    }

    // If the node was a clone, make it a clone too
    const std::string & masterNodeFullScriptName = internalSerialization->_masterNodeFullyQualifiedScriptName;
    if ( !masterNodeFullScriptName.empty() ) {
        NodePtr masterNode = groupContainer->getApplication()->getProject()->getNodeByName(masterNodeFullScriptName);

        // The node could not exist any longer if the user deleted it in the meantime
        if ( masterNode && masterNode->isActivated() ) {
            duplicateNode->getEffectInstance()->slaveAllKnobs( masterNode->getEffectInstance(), true );
        }
    }

    // All nodes that are reachable via expressions
    NodesList allNodes;
    groupContainer->getActiveNodes(&allNodes);

    // Add the node group itself
    if (isContainerGroup) {
        allNodes.push_back( isContainerGroup->getNode() );
    }

    // Position the node to the indicated position with the same offset of the selection center than the original node
    QPointF duplicatedNodeInitialPos = duplicateNodeUI->scenePos();
    QPointF offset = duplicatedNodeInitialPos - averageNodesPosition;
    QPointF newPos = position + offset;
    newPos = duplicateNodeUI->mapToParent(duplicateNodeUI->mapFromScene(newPos));
    duplicateNodeUI->setPosition( newPos.x(), newPos.y() );
    duplicateNodeUI->forceComputePreview( groupContainer->getApplication()->getProject()->currentFrame() );

    if (cloneMaster) {
        DotGui* isDot = dynamic_cast<DotGui*>( duplicateNodeUI.get() );
        // Dots cannot be cloned, just copy them
        if (!isDot) {
            duplicateNode->getEffectInstance()->slaveAllKnobs( cloneMaster->getEffectInstance(), false );
        }
    }

    // Recurse if this is a group or multi-instance
    NodeGroupPtr isGrp = toNodeGroup( duplicateNode->getEffectInstance()->shared_from_this() );

    const SERIALIZATION_NAMESPACE::NodeSerializationList& nodes = internalSerialization->_children;


    // If the script-name of the duplicate node has changed, register it into the oldNewScriptNameMapping
    // map so that connections and links can be restored.
    if ( internalSerialization->_nodeScriptName != duplicateNode->getScriptName() ) {
        (*oldNewScriptNameMapping)[internalSerialization->_nodeScriptName] = duplicateNode->getScriptName();
    }

    // For PyPlugs, don't recurse otherwise we would recreate all nodes on top of the ones created by the python script
    if ( !nodes.empty() && duplicateNode->getPluginPythonModule().empty()) {
        std::string parentName;
        NodeCollectionPtr collection;
        if (isGrp) {
            collection = isGrp;
        } else {
            assert( duplicateNode->isMultiInstance() );
            collection = duplicateNode->getGroup();
            parentName = duplicateNode->getScriptName_mt_safe();
        }
        std::list<std::pair<boost::shared_ptr<SERIALIZATION_NAMESPACE::NodeSerialization>, NodePtr > > newNodesMap;
        std::list<std::pair<std::string, NodeGuiPtr > > newNodes;
        for (SERIALIZATION_NAMESPACE::NodeSerializationList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
            NodeGuiPtr newChild = pasteNode(*it, QPointF(0, 0), QPointF(INT_MIN, INT_MIN), collection, parentName, NodePtr(), oldNewScriptNameMapping);
            if (newChild) {
                newNodes.push_back( std::make_pair( (*it)->_nodeScriptName, newChild ) );
                if ( (*it)->_nodeScriptName != newChild->getNode()->getScriptName() ) {
                    (*oldNewScriptNameMapping)[(*it)->_nodeScriptName] = newChild->getNode()->getScriptName();
                }
                allNodes.push_back( newChild->getNode() );
                newNodesMap.push_back(std::make_pair(*it,newChild->getNode()));
            }
        }
        NodeGraphPrivate::restoreConnections(nodes, newNodes, *oldNewScriptNameMapping);

        //Restore links once all children are created for alias knobs/expressions
        for (std::list<std::pair<boost::shared_ptr<SERIALIZATION_NAMESPACE::NodeSerialization>, NodePtr > > ::iterator it = newNodesMap.begin(); it != newNodesMap.end(); ++it) {
            it->second->restoreKnobsLinks(*(it->first), allNodes, *oldNewScriptNameMapping);
        }


    }


    return duplicateNodeUI;
} // NodeGraphPrivate::pasteNode

void
NodeGraphPrivate::restoreConnections(const SERIALIZATION_NAMESPACE::NodeSerializationList & serializations,
                                     const std::list<std::pair<std::string, NodeGuiPtr > > & newNodes,
                                     const std::map<std::string, std::string> &oldNewScriptNamesMap)
{
    ///For all nodes restore its connections
    SERIALIZATION_NAMESPACE::NodeSerializationList::const_iterator itSer = serializations.begin();

    assert( serializations.size() == newNodes.size() );
    for (std::list<std::pair<std::string, NodeGuiPtr > >::const_iterator it = newNodes.begin();
         it != newNodes.end(); ++it, ++itSer) {
        const std::map<std::string, std::string> & inputNames = (*itSer)->_inputs;
        ///Restore each input
        for (std::map<std::string, std::string>::const_iterator it2 = inputNames.begin(); it2 != inputNames.end(); ++it2) {
            std::string inputScriptName = it2->second;
            std::map<std::string, std::string>::const_iterator foundMapping = oldNewScriptNamesMap.find(it2->second);
            if ( foundMapping != oldNewScriptNamesMap.end() ) {
                inputScriptName = foundMapping->second;
            }

            if ( inputScriptName.empty() ) {
                continue;
            }

            int index = it->second->getNode()->getInputNumberFromLabel(it2->first);
            if (index == -1) {
                qDebug() << "Could not find an input named " << it2->first.c_str();
                continue;
            }


            ///find a node  containing the same name. It should not match exactly because there's already
            /// the "-copy" that was added to its name
            for (std::list<std::pair<std::string, NodeGuiPtr > >::const_iterator it3 = newNodes.begin();
                 it3 != newNodes.end(); ++it3) {
                if (it3->second->getNode()->getScriptName() == inputScriptName) {
                    NodeCollection::connectNodes( index, it3->second->getNode(), it->second->getNode() );
                    break;
                }
            }
        }
    }
}

void
NodeGraphPrivate::toggleSelectedNodesEnabled()
{
    NodesGuiList toProcess;

    for (NodesGuiList::iterator it = _selection.begin(); it != _selection.end(); ++it) {
        KnobBoolPtr k = (*it)->getNode()->getDisabledKnob();
        if (!k) {
            continue;
        }
        if ( k->getValue() ) {
            toProcess.push_back(*it);
        }
    }
    ///if some nodes are disabled , enable them before

    if ( toProcess.size() == _selection.size() ) {
        _publicInterface->pushUndoCommand( new EnableNodesCommand(_selection) );
    } else if (toProcess.size() > 0) {
        _publicInterface->pushUndoCommand( new EnableNodesCommand(toProcess) );
    } else {
        _publicInterface->pushUndoCommand( new DisableNodesCommand(_selection) );
    }
}

NATRON_NAMESPACE_EXIT;
