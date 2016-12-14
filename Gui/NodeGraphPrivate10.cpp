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
    
    
    QPointF avgNodesPosition((xmin + xmax) / 2., (ymin + ymax) / 2.);
    
    NodesGuiList newNodesList;

    std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > > allCreatedNodes;
    {
        CreatingNodeTreeFlag_RAII createNodeTree( _publicInterface->getGui()->getApp() );
        for (SERIALIZATION_NAMESPACE::NodeSerializationList::const_iterator it = clipboard.begin();
             it != clipboard.end(); ++it) {
            const std::string& oldScriptName = (*it)->_nodeScriptName;
            NodeGuiPtr node = NodeGraphPrivate::pasteNode(*it, avgNodesPosition, scenePos, group.lock(), NodePtr());
            
            if (!node) {
                continue;
            }
            if (newNodes) {
                newNodes->push_back( std::make_pair(oldScriptName, node) );
            }
            newNodesList.push_back(node);
            allCreatedNodes.push_back(std::make_pair(node->getNode(), *it));
        }

    }
    
    // Restore connections
#pragma message WARN("Todo: set the position of the node in serialization")
    Project::restoreGroupFromSerialization(clipboard, group.lock(), true /*restoreLinks*/);

    if (useUndoCommand) {
        _publicInterface->pushUndoCommand( new AddMultipleNodesCommand(_publicInterface, newNodesList) );
    }
} // pasteNodesInternal

NodeGuiPtr
NodeGraphPrivate::pasteNode(const SERIALIZATION_NAMESPACE::NodeSerializationPtr & internalSerialization,
                            const QPointF& averageNodesPosition,
                            const QPointF& position,
                            const NodeCollectionPtr& groupContainer,
                            const NodePtr& cloneMaster)
{
    assert(groupContainer && internalSerialization);

    // Create the duplicate node
    NodePtr duplicateNode;
    {
        CreateNodeArgsPtr args(CreateNodeArgs::create(internalSerialization->_pluginID, groupContainer));
        args->setProperty<SERIALIZATION_NAMESPACE::NodeSerializationPtr >(kCreateNodeArgsPropNodeSerialization, internalSerialization);
        args->setProperty<int>(kCreateNodeArgsPropPluginVersion, internalSerialization->_pluginMajorVersion, 0);
        args->setProperty<int>(kCreateNodeArgsPropPluginVersion, internalSerialization->_pluginMinorVersion, 1);
        args->setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
        args->setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        args->setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
        //args->setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, false);

        duplicateNode = groupContainer->getApplication()->createNode(args);

        if (!duplicateNode) {
            return NodeGuiPtr();
        }
    }

    // This is the node gui of the duplicate node
    NodeGuiPtr duplicateNodeUI = boost::dynamic_pointer_cast<NodeGui>(duplicateNode->getNodeGui());
    assert(duplicateNodeUI);

    // Position the node to the indicated position with the same offset of the selection center than the original node
    if (position.x() != INT_MIN && position.y() != INT_MIN) {
        QPointF duplicatedNodeInitialPos = duplicateNodeUI->scenePos();
        QPointF offset = duplicatedNodeInitialPos - averageNodesPosition;
        QPointF newPos = position + offset;
        newPos = duplicateNodeUI->mapToParent(duplicateNodeUI->mapFromScene(newPos));
        duplicateNodeUI->setPosition( newPos.x(), newPos.y() );
    }

    if (cloneMaster) {
        duplicateNode->linkToNode(cloneMaster);
    }



    return duplicateNodeUI;
} // NodeGraphPrivate::pasteNode

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
