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

NATRON_NAMESPACE_ENTER;
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

bool
NodeGraph::doAutoConnectHeuristic(const NodeGuiPtr &node, const NodeGuiPtr &selected)
{
    BackdropGuiPtr isBd = toBackdropGui(node);

    if (isBd) {
        return false;
    }


    if (!selected) {
        return false;
    }

    // 2 possible values:
    // 1 = pop the node above the selected node and move the inputs of the selected node a little
    // 2 = pop the node below the selected node and move the outputs of the selected node a little
    enum AutoConnectBehaviorEnum {
        eAutoConnectBehaviorAbove,
        eAutoConnectBehaviorBelow
    };

    AutoConnectBehaviorEnum behavior;

    //        1) selected is output
    //          a) created is output --> fail
    //          b) created is input --> connect input
    //          c) created is regular --> connect input
    //        2) selected is input
    //          a) created is output --> connect output
    //          b) created is input --> fail
    //          c) created is regular --> connect output
    //        3) selected is regular
    //          a) created is output--> connect output
    //          b) created is input --> connect input
    //          c) created is regular --> connect output

    ///1)
    if ( selected->getNode()->isOutputNode() ) {
        ///case 1-a) just do default we don't know what else to do
        if ( node->getNode()->isOutputNode() ) {
            return false;
        } else {
            ///for either cases 1-b) or 1-c) we just connect the created node as input of the selected node.
            behavior = eAutoConnectBehaviorAbove;
        }
    }
    ///2) and 3) are similar except for case b)
    else {
        ///case 2 or 3- a): connect the created node as output of the selected node.
        if ( node->getNode()->isOutputNode() ) {
            behavior = eAutoConnectBehaviorBelow;
        }
        ///case b)
        else if ( node->getNode()->isInputNode() ) {
            if ( selected->getNode()->getEffectInstance()->isReader() ) {
                ///case 2-b) just do default we don't know what else to do
                return false;
            } else {
                ///case 3-b): connect the created node as input of the selected node
                behavior = eAutoConnectBehaviorAbove;
            }
        }
        ///case c) connect created as output of the selected node
        else {
            behavior = eAutoConnectBehaviorBelow;
        }
    }


    NodePtr createdNodeInternal = node->getNode();
    NodePtr selectedNodeInternal;
    if (selected) {
        selectedNodeInternal = selected->getNode();
    }


    // If behaviour is 1 , just check that we can effectively connect the node to avoid moving them for nothing
    // otherwise fail
    if (behavior == eAutoConnectBehaviorAbove) {
        const std::vector<NodeWPtr > & inputs = selected->getNode()->getGuiInputs();
        bool oneInputEmpty = false;
        for (std::size_t i = 0; i < inputs.size(); ++i) {
            if ( !inputs[i].lock() ) {
                oneInputEmpty = true;
                break;
            }
        }
        if (!oneInputEmpty) {
            return false;
        }
    }


    ProjectPtr proj = getGui()->getApp()->getProject();


    ///default
    QPointF position;
    if (behavior == eAutoConnectBehaviorAbove) {
        ///pop it above the selected node

        ///If this is the first connected input, insert it in a "linear" way so the tree remains vertical
        int nbConnectedInput = 0;
        const std::vector<Edge*> & selectedNodeInputs = selected->getInputsArrows();
        for (std::vector<Edge*>::const_iterator it = selectedNodeInputs.begin(); it != selectedNodeInputs.end(); ++it) {
            NodeGuiPtr input;
            if (*it) {
                input = (*it)->getSource();
            }
            if (input) {
                ++nbConnectedInput;
            }
        }

        ///connect it to the first input
        QSize selectedNodeSize = selected->getSize();
        QSize createdNodeSize = node->getSize();


        if (nbConnectedInput == 0) {
            QPointF selectedNodeMiddlePos = selected->scenePos() +
                                            QPointF(selectedNodeSize.width() / 2, selectedNodeSize.height() / 2);


            position.setX(selectedNodeMiddlePos.x() - createdNodeSize.width() / 2);
            position.setY( selectedNodeMiddlePos.y() - selectedNodeSize.height() / 2 - NodeGui::DEFAULT_OFFSET_BETWEEN_NODES
                           - createdNodeSize.height() );

            QRectF createdNodeRect( position.x(), position.y(), createdNodeSize.width(), createdNodeSize.height() );

            ///now that we have the position of the node, move the inputs of the selected node to make some space for this node

            for (std::vector<Edge*>::const_iterator it = selectedNodeInputs.begin(); it != selectedNodeInputs.end(); ++it) {
                if ( (*it)->hasSource() ) {
                    (*it)->getSource()->moveAbovePositionRecursively(createdNodeRect);
                }
            }

            int selectedInput = selectedNodeInternal->getPreferredInputForConnection();
            if (selectedInput != -1) {
                bool ok = proj->connectNodes(selectedInput, createdNodeInternal, selectedNodeInternal, true);
                Q_UNUSED(ok);
            }
        } else {
            //ViewerInstancePtr isSelectedViewer = selectedNodeInternal->isEffectViewerInstance();
            //Don't pop a dot, it will most likely annoy the user, just fallback on behavior 0
            /*    position.setX( ( viewPos.bottomRight().x() + viewPos.topLeft().x() ) / 2. );
                position.setY( ( viewPos.topLeft().y() + viewPos.bottomRight().y() ) / 2. );
               }
               else {
             */
            QRectF selectedBbox = selected->mapToScene( selected->boundingRect() ).boundingRect();
            QPointF selectedCenter = selectedBbox.center();
            double y = selectedCenter.y() - selectedNodeSize.height() / 2.
                       - NodeGui::DEFAULT_OFFSET_BETWEEN_NODES - createdNodeSize.height();
            double x = selectedCenter.x() + nbConnectedInput * 150;

            position.setX(x  - createdNodeSize.width() / 2.);
            position.setY(y);

            int index = selectedNodeInternal->getPreferredInputForConnection();
            if (index != -1) {
                bool ok = proj->connectNodes(index, createdNodeInternal, selectedNodeInternal, true);
                Q_UNUSED(ok);
            }
            //} // if (isSelectedViewer) {*/
        } // if (nbConnectedInput == 0) {
    } else if (behavior == eAutoConnectBehaviorBelow) {

        // Pop it below the selected node

        const NodesWList& outputs = selectedNodeInternal->getGuiOutputs();
        if ( !createdNodeInternal->isOutputNode() || outputs.empty() ) {
            QSize selectedNodeSize = selected->getSize();
            QSize createdNodeSize = node->getSize();
            QPointF selectedNodeMiddlePos = selected->scenePos() +
                                            QPointF(selectedNodeSize.width() / 2, selectedNodeSize.height() / 2);

            ///actually move the created node where the selected node is
            position.setX(selectedNodeMiddlePos.x() - createdNodeSize.width() / 2);
            position.setY(selectedNodeMiddlePos.y() + (selectedNodeSize.height() / 2) + NodeGui::DEFAULT_OFFSET_BETWEEN_NODES);

            QRectF createdNodeRect( position.x(), position.y(), createdNodeSize.width(), createdNodeSize.height() );

            ///and move the selected node below recusively
            for (NodesWList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
                NodePtr output = it->lock();
                if (!output) {
                    continue;
                }
                NodeGuiIPtr output_i = output->getNodeGui();
                if (!output_i) {
                    continue;
                }
                NodeGuiPtr outputGui = toNodeGui( output_i );
                assert(outputGui);
                if (outputGui) {
                    outputGui->moveBelowPositionRecursively(createdNodeRect);
                }
            }

            ///Connect the created node to the selected node
            ///finally we connect the created node to the selected node
            int createdInput = createdNodeInternal->getPreferredInputForConnection();
            if (createdInput != -1) {
                ignore_result( createdNodeInternal->connectInput(selectedNodeInternal, createdInput) );
            }

            if ( !createdNodeInternal->isOutputNode() ) {
                ///we find all the nodes that were previously connected to the selected node,
                ///and connect them to the created node instead.
                std::map<NodePtr, int> outputsConnectedToSelectedNode;
                selectedNodeInternal->getOutputsConnectedToThisNode(&outputsConnectedToSelectedNode);

                for (std::map<NodePtr, int>::iterator it = outputsConnectedToSelectedNode.begin();
                     it != outputsConnectedToSelectedNode.end(); ++it) {
                    if ( (it->first != createdNodeInternal) ) {


                        ignore_result( it->first->replaceInput(createdNodeInternal, it->second) );

                    }
                }
            }
        } else {
            ///the created node is an output node and the selected node already has several outputs, create it aside
            QSize createdNodeSize = node->getSize();
            QRectF selectedBbox = selected->mapToScene( selected->boundingRect() ).boundingRect();
            QPointF selectedCenter = selectedBbox.center();
            double y = selectedCenter.y() + selectedBbox.height() / 2.
                       + NodeGui::DEFAULT_OFFSET_BETWEEN_NODES;
            double x = selectedCenter.x() + (int)outputs.size() * 150;

            position.setX(x  - createdNodeSize.width() / 2.);
            position.setY(y);


            //Don't pop a dot, it will most likely annoy the user, just fallback on behavior 0

            int index = createdNodeInternal->getPreferredInputForConnection();
            bool ok = proj->connectNodes(index, selectedNodeInternal, createdNodeInternal, true);
            Q_UNUSED(ok);
        }
    }
    position = node->mapFromScene(position);
    position = node->mapToParent(position);
    node->setPosition( position.x(), position.y() );

    return true;
} // doAutoConnectHeuristic

void
NodeGraph::setNodeToDefaultPosition(const NodeGuiPtr& node, const NodesGuiList& selectedNodes, const CreateNodeArgs& args)
{
    NodePtr internalNode = node->getNode();

    // Serializatino, don't do anything
    SERIALIZATION_NAMESPACE::NodeSerializationPtr serialization = args.getProperty<SERIALIZATION_NAMESPACE::NodeSerializationPtr >(kCreateNodeArgsPropNodeSerialization);
    if (serialization) {
        return;
    }

    bool hasPositionnedNode = false;


    // Try to autoconnect if there is a selection
    bool autoConnect = args.getProperty<bool>(kCreateNodeArgsPropAutoConnect);
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
            if (doAutoConnectHeuristic(node, selectedNode)) {
                hasPositionnedNode = true;
            }
        }
    }

    if (!hasPositionnedNode) {
        // If there's a position hint, use it to position the node
        double xPosHint = args.getProperty<double>(kCreateNodeArgsPropNodeInitialPosition, 0);
        double yPosHint = args.getProperty<double>(kCreateNodeArgsPropNodeInitialPosition, 1);

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

NATRON_NAMESPACE_EXIT;

