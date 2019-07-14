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

#include "NodeGraph.h"
#include "NodeGraphPrivate.h"

#include <cstdlib>
#include <set>
#include <map>
#include <vector>
#include <locale>
#include <algorithm> // min, max
#include <stdexcept>

#include "Engine/CreateNodeArgs.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/ViewerInstance.h"

#include "Gui/BackdropGui.h"
#include "Gui/Edge.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/NodeGui.h"

NATRON_NAMESPACE_ENTER
//using std::cout; using std::endl;

void
NodeGraph::moveNodeToCenterOfVisiblePortion(const NodeGuiPtr &n)
{
    QRectF viewPos = visibleSceneRect();
    QPointF position;
    position.setX( ( viewPos.bottomRight().x() + viewPos.topLeft().x() ) / 2. );
    position.setY( ( viewPos.topLeft().y() + viewPos.bottomRight().y() ) / 2. );

    position = n->mapFromScene(position);
    position = n->mapToParent(position);
    n->setPosition( position.x(), position.y() );
}


void
NodeGraph::setNodeToDefaultPosition(const NodeGuiPtr& node, const NodesGuiList& selectedNodes, const CreateNodeArgs& args)
{
    NodePtr internalNode = node->getNode();

    // Serializatino, don't do anything
    SERIALIZATION_NAMESPACE::NodeSerializationPtr serialization = args.getPropertyUnsafe<SERIALIZATION_NAMESPACE::NodeSerializationPtr>(kCreateNodeArgsPropNodeSerialization);
    if (serialization) {
        return;
    }

    bool hasPositionnedNode = false;


    // Try to autoconnect if there is a selection
    bool autoConnect = args.getPropertyUnsafe<bool>(kCreateNodeArgsPropAutoConnect);
    if ( selectedNodes.empty() || serialization) {
        autoConnect = false;
    }

    if (autoConnect) {
        BackdropGuiPtr isBd = toBackdropGui(node);
        if (!isBd) {
            NodeGuiPtr selectedNode;
            if ( !serialization && (selectedNodes.size() == 1) ) {
                selectedNode = selectedNodes.front();
                BackdropGuiPtr isBdGui = toBackdropGui(selectedNode);
                if (isBdGui) {
                    selectedNode.reset();
                }
            }
            if (selectedNode && node->getNode()->autoConnect(selectedNode->getNode())) {
                hasPositionnedNode = true;
            }
        }
    }

    if (!hasPositionnedNode) {
        // If there's a position hint, use it to position the node
        double xPosHint = args.getPropertyUnsafe<double>(kCreateNodeArgsPropNodeInitialPosition, 0);
        double yPosHint = args.getPropertyUnsafe<double>(kCreateNodeArgsPropNodeInitialPosition, 1);

        if ((xPosHint != INT_MIN) && (yPosHint != INT_MIN)) {
            QPointF pos = node->mapToParent( node->mapFromScene( QPointF(xPosHint, yPosHint) ) );
            node->refreshPosition( pos.x(), pos.y(), true );
            hasPositionnedNode = true;
        }
    }



    // Ok fallback with the node in the middle of the node graph
    if (!hasPositionnedNode) {
        moveNodeToCenterOfVisiblePortion(node);
    }


}

NATRON_NAMESPACE_EXIT
