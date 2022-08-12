/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "NodeGraphPrivate.h"
#include "NodeGraph.h"

#include <stdexcept>
#include <sstream> // stringstream

#include "Engine/CreateNodeArgs.h"
#include "Engine/NodeSerialization.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeSerialization.h"
#include "Engine/Project.h"
#include "Engine/RotoLayer.h"

#include "Gui/BackdropGui.h"
#include "Gui/DotGui.h"
#include "Gui/Edge.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h" // appPTR
#include "Gui/NodeClipBoard.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeGuiSerialization.h"


NATRON_NAMESPACE_ENTER


void
NodeGraphPrivate::pasteNodesInternal(const NodeClipBoard & clipboard,
                                     const QPointF& scenePos,
                                     bool useUndoCommand,
                                     std::list<std::pair<std::string, NodeGuiPtr> > *newNodes)
{
    if ( !clipboard.isEmpty() ) {
        double xmax = INT_MIN;
        double xmin = INT_MAX;
        double ymin = INT_MAX;
        double ymax = INT_MIN;

        for (std::list<NodeGuiSerializationPtr>::const_iterator it = clipboard.nodesUI.begin();
             it != clipboard.nodesUI.end(); ++it) {
            double x = (*it)->getX();
            double y = (*it)->getY();
            double w, h;
            (*it)->getSize(&w, &h);
            if ( (x + w) > xmax ) {
                xmax = x;
            }
            if (x < xmin) {
                xmin = x;
            }
            if ( (y + h) > ymax ) {
                ymax = y;
            }
            if (y < ymin) {
                ymin = y;
            }
        }


        QPointF offset( scenePos.x() - ( (xmin + xmax) / 2. ), scenePos.y() - ( (ymin + ymax) / 2. ) );

        assert( clipboard.nodes.size() == clipboard.nodesUI.size() );

        NodesGuiList newNodesList;
        std::list<std::pair<NodeSerializationPtr, NodePtr> > newNodesMap;

        ///The script-name of the copy node is different than the one of the original one
        ///We store the mapping so we can restore node links correctly
        std::map<std::string, std::string> oldNewScriptNamesMapping;
        {
            CreatingNodeTreeFlag_RAII createNodeTree( _publicInterface->getGui()->getApp() );
            const std::list<NodeSerializationPtr>& internalNodesClipBoard = clipboard.nodes;
            std::list<NodeSerializationPtr>::const_iterator itOther = internalNodesClipBoard.begin();
            for (std::list<NodeGuiSerializationPtr>::const_iterator it = clipboard.nodesUI.begin();
                 it != clipboard.nodesUI.end(); ++it, ++itOther) {
                const std::string& oldScriptName = (*itOther)->getNodeScriptName();
                NodeGuiPtr node = pasteNode( *itOther, *it, offset, group.lock(), std::string(), false, &oldNewScriptNamesMapping);

                if (!node) {
                    continue;
                }
                newNodes->push_back( std::make_pair(oldScriptName, node) );
                newNodesList.push_back(node);
                newNodesMap.push_back( std::make_pair( *itOther, node->getNode() ) );

                const std::string& newScriptName = node->getNode()->getScriptName();
                oldNewScriptNamesMapping[oldScriptName] = newScriptName;
            }
            assert( internalNodesClipBoard.size() == newNodes->size() );

            ///Now that all nodes have been duplicated, try to restore nodes connections
            restoreConnections(internalNodesClipBoard, *newNodes, oldNewScriptNamesMapping);

            NodesList allNodes;
            _publicInterface->getGui()->getApp()->getProject()->getActiveNodesExpandGroups(&allNodes);

            //Restore links once all children are created for alias knobs/expressions
            for (std::list<std::pair<NodeSerializationPtr, NodePtr> > ::iterator it = newNodesMap.begin(); it != newNodesMap.end(); ++it) {
                it->second->storeKnobsLinks(*(it->first), oldNewScriptNamesMapping);
                it->second->restoreKnobsLinks(allNodes, oldNewScriptNamesMapping, false); // may fail
            }
        }

        if (newNodesList.size() > 1) {
            ///Only compute datas if we're pasting more than 1 node
            _publicInterface->getGui()->getApp()->getProject()->forceComputeInputDependentDataOnAllTrees();
        } else if (newNodesList.size() == 1) {
            newNodesList.front()->getNode()->getEffectInstance()->refreshMetadata_public(true);
        }

        // Now that meta-datas are refreshed, compute preview
        for (NodesGuiList::const_iterator it = newNodesList.begin(); it!=newNodesList.end(); ++it) {
            (*it)->forceComputePreview(_publicInterface->getGui()->getApp()->getTimeLine()->currentFrame());
        }

        if (useUndoCommand) {
            _publicInterface->pushUndoCommand( new AddMultipleNodesCommand(_publicInterface, newNodesList) );
        }
    }
} // pasteNodesInternal

// Trim underscore followed by digits at the end of baseName (see #732).
static
void trimNumber(std::string &baseName)
{
    std::size_t found_underscore = baseName.rfind('_');
    if (found_underscore != std::string::npos && found_underscore != (baseName.size() - 1)) {
        std::size_t found_nondigit = baseName.find_last_not_of("0123456789");
        if (found_nondigit == found_underscore) {
            baseName.erase(found_underscore);
        }
    }
}

NodeGuiPtr
NodeGraphPrivate::pasteNode(const NodeSerializationPtr & internalSerialization,
                            const NodeGuiSerializationPtr & guiSerialization,
                            const QPointF & offset,
                            const NodeCollectionPtr& grp,
                            const std::string& parentName,
                            bool clone,
                            std::map<std::string, std::string>* oldNewScriptNamesMapping)
{
    CreateNodeArgs args(internalSerialization->getPluginID(), grp);
    args.setProperty<NodeSerializationPtr>(kCreateNodeArgsPropNodeSerialization, internalSerialization);
    if (!parentName.empty()) {
        args.setProperty<std::string>(kCreateNodeArgsPropMultiInstanceParentName, parentName);
    }
    args.setProperty<int>(kCreateNodeArgsPropPluginVersion, internalSerialization->getPluginMajorVersion(), 0);
    args.setProperty<int>(kCreateNodeArgsPropPluginVersion, internalSerialization->getPluginMinorVersion(), 1);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
    args.setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);
    
    NodePtr n = _publicInterface->getGui()->getApp()->createNode(args);

    if (!n) {
        return NodeGuiPtr();
    }
    NodeGuiIPtr gui_i = n->getNodeGui();
    NodeGuiPtr gui = std::dynamic_pointer_cast<NodeGui>(gui_i);
    assert(gui);

    if ( ( grp == group.lock() ) && ( !internalSerialization->getNode() || ( internalSerialization->getNode()->getGroup() == group.lock() ) ) ) {
        // We pasted the node in the same group, give it another label.
        // If the label is available, use it as is.
        // Else look for a similar name by adding a (different) number suffix.
        std::string name = internalSerialization->getNodeLabel();
        if ( grp->checkIfNodeLabelExists( name, n.get() ) ) {
            std::string baseName = name;
            trimNumber(baseName);
            int no = 1;
            do {
                name = baseName;
                if (no > 1) {
                    name += '_';
                    std::stringstream ss;
                    ss << no;
                    name += ss.str();
                }
                ++no;
            } while ( grp->checkIfNodeLabelExists( name, n.get() ) );
        }
        n->setLabel(name);
    } else {
        //If we paste the node in a different graph, it can keep its scriptname/label
    }


    const std::string & masterNodeName = internalSerialization->getMasterNodeName();
    if ( !masterNodeName.empty() ) {
        NodePtr masterNode = _publicInterface->getGui()->getApp()->getProject()->getNodeByName(masterNodeName);

        ///the node could not exist any longer if the user deleted it in the meantime
        if ( masterNode && masterNode->isActivated() ) {
            n->getEffectInstance()->slaveAllKnobs( masterNode->getEffectInstance().get(), true );
        }
    }

    //All nodes that are reachable via expressions
    NodesList allNodes;
    _publicInterface->getGui()->getApp()->getProject()->getActiveNodesExpandGroups(&allNodes);

    //We don't want the clone to have the same hash as the original
    n->incrementKnobsAge();
    if (guiSerialization) {
        gui->copyFrom(*guiSerialization);
    }
    QPointF newPos = gui->getPos_mt_safe() + offset;
    gui->setPosition( newPos.x(), newPos.y() );

    if (clone) {
        assert( internalSerialization->getNode() );
        DotGui* isDot = dynamic_cast<DotGui*>( gui.get() );
        ///Dots cannot be cloned, just copy them
        if (!isDot) {
            n->getEffectInstance()->slaveAllKnobs( internalSerialization->getNode()->getEffectInstance().get(), false );
        }
    }

    ///Recurse if this is a group or multi-instance
    NodeGroupPtr isGrp =
        std::dynamic_pointer_cast<NodeGroup>( n->getEffectInstance()->shared_from_this() );
    const std::list<NodeSerializationPtr>& nodes = internalSerialization->getNodesCollection();
    std::list<NodeGuiSerializationPtr>  nodesUi;
    if (guiSerialization) {
        nodesUi = guiSerialization->getChildren();
    }
    assert( nodes.size() == nodesUi.size() || nodesUi.empty() );

    if ( internalSerialization->getNodeScriptName() != n->getScriptName() ) {
        (*oldNewScriptNamesMapping)[internalSerialization->getNodeScriptName()] = n->getScriptName();
    }

    // For PyPlugs, don't recurse otherwise we would recreate all nodes on top of the ones created by the python script
    if ( !nodes.empty() && n->getPluginPythonModule().empty()) {
        std::string parentName;
        NodeCollectionPtr collection;
        if (isGrp) {
            collection = isGrp;
        } else {
            assert( n->isMultiInstance() );
            collection = n->getGroup();
            parentName = n->getScriptName_mt_safe();
        }
        std::list<std::pair<NodeSerializationPtr, NodePtr> > newNodesMap;
        std::list<std::pair<std::string, NodeGuiPtr> > newNodes;
        std::list<NodeGuiSerializationPtr>::const_iterator itUi = nodesUi.begin();
        for (std::list<NodeSerializationPtr>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
            NodeGuiSerializationPtr guiS = nodesUi.empty() ?  NodeGuiSerializationPtr() : *itUi;
            NodeGuiPtr newChild = pasteNode(*it, guiS, QPointF(0, 0), collection, parentName, clone, oldNewScriptNamesMapping);
            if (newChild) {
                newNodes.push_back( std::make_pair( (*it)->getNodeScriptName(), newChild ) );
                if ( (*it)->getNodeScriptName() != newChild->getNode()->getScriptName() ) {
                    (*oldNewScriptNamesMapping)[(*it)->getNodeScriptName()] = newChild->getNode()->getScriptName();
                }
                allNodes.push_back( newChild->getNode() );
                newNodesMap.push_back(std::make_pair(*it,newChild->getNode()));
            }
            if ( !nodesUi.empty() ) {
                ++itUi;
            }
        }
        restoreConnections(nodes, newNodes, *oldNewScriptNamesMapping);

        //Restore links once all children are created for alias knobs/expressions
        for (std::list<std::pair<NodeSerializationPtr, NodePtr> > ::iterator it = newNodesMap.begin(); it != newNodesMap.end(); ++it) {
            it->second->storeKnobsLinks(*(it->first), *oldNewScriptNamesMapping);
            it->second->restoreKnobsLinks(allNodes, *oldNewScriptNamesMapping, false); // may fail
        }
    }

    return gui;
} // NodeGraphPrivate::pasteNode

void
NodeGraphPrivate::restoreConnections(const std::list<NodeSerializationPtr> & serializations,
                                     const std::list<std::pair<std::string, NodeGuiPtr> > & newNodes,
                                     const std::map<std::string, std::string> &oldNewScriptNamesMap)
{
    ///For all nodes restore its connections
    std::list<NodeSerializationPtr>::const_iterator itSer = serializations.begin();

    assert( serializations.size() == newNodes.size() );
    for (std::list<std::pair<std::string, NodeGuiPtr> >::const_iterator it = newNodes.begin();
         it != newNodes.end(); ++it, ++itSer) {
        const std::map<std::string, std::string> & inputNames = (*itSer)->getInputs();
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
            for (std::list<std::pair<std::string, NodeGuiPtr> >::const_iterator it3 = newNodes.begin();
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
NodeGraphPrivate::getNodeSet(const NodesGuiList& nodeList, std::set<NodeGuiPtr>& nodeSet)
{
    for (NodesGuiList::const_iterator it = nodeList.begin(); it != nodeList.end(); ++it) {
        if (nodeSet.find(*it) == nodeSet.end()) {
            nodeSet.insert(*it);
            BackdropGui* isBd = dynamic_cast<BackdropGui*>( it->get() );
            if (isBd) {
                NodesGuiList nodesWithin = _publicInterface->getNodesWithinBackdrop(*it);
                getNodeSet(nodesWithin, nodeSet);
            }
        }
    }
}

void
NodeGraphPrivate::toggleSelectedNodesEnabled()
{
    std::set<NodeGuiPtr> nodeSet;

    // first, put all selected nodes, including those within backdrop, in nodeSet
    getNodeSet(_selection, nodeSet);

    NodesGuiList disabledNodes;
    NodesGuiList allNodes;
    for (std::set<NodeGuiPtr>::const_iterator it = nodeSet.begin(); it != nodeSet.end(); ++it) {
        KnobBoolPtr k = (*it)->getNode()->getDisabledKnob();
        if (!k) {
            continue;
        }
        allNodes.push_back(*it);
        if ( k->getValue() ) {
            disabledNodes.push_back(*it);
        }
    }
    ///if some nodes are disabled , enable them before

    if ( disabledNodes.size() == allNodes.size() ) {
        _publicInterface->pushUndoCommand( new EnableNodesCommand(allNodes) );
    } else if (disabledNodes.size() > 0) {
        _publicInterface->pushUndoCommand( new EnableNodesCommand(disabledNodes) );
    } else {
        _publicInterface->pushUndoCommand( new DisableNodesCommand(allNodes) );
    }
}

NATRON_NAMESPACE_EXIT
