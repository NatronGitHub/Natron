    /* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#include "NodeGraphPrivate.h"
#include "NodeGraph.h"

#include <stdexcept>
#include <sstream> // stringstream

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


NATRON_NAMESPACE_ENTER


void
NodeGraphPrivate::pasteNodesInternal(const std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > >& originalNodes,
                                     const QPointF& newCentroidScenePos,
                                     PasteNodesFlags flags)
{
    if (originalNodes.empty()) {
        return;
    }

    if (flags & ePasteNodesFlagRelativeToCentroid) {

        assert(newCentroidScenePos.x() != INT_MIN && newCentroidScenePos.x() != INT_MAX &&
               newCentroidScenePos.y() != INT_MIN && newCentroidScenePos.y() != INT_MAX);

        // Compute the centroid of the selected node so that we can place duplicates exactly at the same position relative to new centroid
        double xmax = INT_MIN;
        double xmin = INT_MAX;
        double ymin = INT_MAX;
        double ymax = INT_MIN;


        for (std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > >::const_iterator it = originalNodes.begin();
             it != originalNodes.end(); ++it) {

            if (it->second->_nodePositionCoords[0] == INT_MIN || it->second->_nodePositionCoords[1] == INT_MIN) {
                continue;
            }
            
            if ( (it->second->_nodePositionCoords[0] + it->second->_nodeSize[0]) > xmax ) {
                xmax = it->second->_nodePositionCoords[0];
            }
            if (it->second->_nodePositionCoords[0] < xmin) {
                xmin = it->second->_nodePositionCoords[0];
            }
            if ( (it->second->_nodePositionCoords[1] + it->second->_nodeSize[1]) > ymax ) {
                ymax = it->second->_nodePositionCoords[1];
            }
            if (it->second->_nodePositionCoords[1] < ymin) {
                ymin = it->second->_nodePositionCoords[1];
            }
        }

        // Adjust the serialization of all nodes so that it contains the new position relative to the centroid
        QPointF originalCentroidScenePos((xmin + xmax) / 2., (ymin + ymax) / 2.);


        for (std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > >::const_iterator it = originalNodes.begin();
             it != originalNodes.end(); ++it) {

            // If the position was not serialized, do not attempt to position the node.
            if (it->second->_nodePositionCoords[0] == INT_MIN || it->second->_nodePositionCoords[1] == INT_MIN) {
                continue;
            }

            QPointF originalNodePos(it->second->_nodePositionCoords[0], it->second->_nodePositionCoords[1]);

            // Compute the distance from the original
            QPointF offset = originalNodePos - originalCentroidScenePos;

            // New pos is relative to the centroid with the same delta
            QPointF newPos = newCentroidScenePos + offset;

            it->second->_nodePositionCoords[0] = newPos.x();
            it->second->_nodePositionCoords[1] = newPos.y();
        }
    }

    SERIALIZATION_NAMESPACE::NodeSerializationList serializationList;
    for (std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > >::const_iterator it = originalNodes.begin();
         it != originalNodes.end(); ++it) {
        serializationList.push_back(it->second);
    }

    
    // Create nodes
    NodesList createdNodes;
    group.lock()->createNodesFromSerialization(serializationList, NodeCollection::eCreateNodesFromSerializationFlagsNone, &createdNodes);

    // Link the created node to the original node if needed
    if (flags & ePasteNodesFlagCloneNodes) {
        assert(createdNodes.size() == serializationList.size());
        std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > >::const_iterator itOrig = originalNodes.begin();
        for (NodesList::const_iterator it = createdNodes.begin(); it!=createdNodes.end(); ++it, ++itOrig) {
            const NodePtr& createdNode = *it;
            if (!createdNode) {
                continue;
            }
            NodePtr originalNode = itOrig->first;
            if (!originalNode) {
                continue;
            }
            
            createdNode->linkToNode(originalNode);

        }
    }

    // Add an undo command if we need to
    if (flags & ePasteNodesFlagUseUndoCommand) {
        if (!createdNodes.empty()) {
            _publicInterface->pushUndoCommand( new AddMultipleNodesCommand(_publicInterface, createdNodes) );
        }
    }
} // pasteNodesInternal



void
NodeGraphPrivate::toggleSelectedNodesEnabled()
{
    NodesGuiList toProcess;

    for (NodesGuiWList::iterator it = _selection.begin(); it != _selection.end(); ++it) {
        NodeGuiPtr n = it->lock();
        if (!n) {
            continue;
        }
        KnobBoolPtr k = n->getNode()->getEffectInstance()->getDisabledKnob();
        if (!k) {
            continue;
        }
        if ( k->getValue() ) {
            toProcess.push_back(n);
        }
    }
    ///if some nodes are disabled , enable them before

    if ( toProcess.size() == _selection.size() ) {
        _publicInterface->pushUndoCommand( new EnableNodesCommand(_publicInterface->getSelectedNodes()) );
    } else if (toProcess.size() > 0) {
        _publicInterface->pushUndoCommand( new EnableNodesCommand(toProcess) );
    } else {
        _publicInterface->pushUndoCommand( new DisableNodesCommand(_publicInterface->getSelectedNodes()) );
    }
}

NATRON_NAMESPACE_EXIT
