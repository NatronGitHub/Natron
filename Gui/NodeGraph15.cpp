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

#include "NodeGraph.h"
#include "NodeGraphPrivate.h"

#include <stdexcept>

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QMouseEvent>
#include <QCursor>
#include <QApplication>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON

#include "Engine/AppInstance.h" // CreateNodeArgs
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/Settings.h"

#include "Gui/BackdropGui.h"
#include "Gui/Edge.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiMacros.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/NodeGui.h"

#include "Global/QtCompat.h"

#define NATRON_GRAPH_AUTO_SCROLL_OFFSET 20
#define NATRON_AUTO_SCROLL_TIMEOUT_MS 50


NATRON_NAMESPACE_ENTER
static bool
handleConnectionError(const NodeGuiPtr& outputNode,
                      const NodeGuiPtr& inputNode,
                      int inputNb)
{
    Node::CanConnectInputReturnValue linkRetCode = outputNode->getNode()->canConnectInput(inputNode->getNode(), inputNb);

    if ( (linkRetCode != Node::eCanConnectInput_ok) && (linkRetCode != Node::eCanConnectInput_inputAlreadyConnected) ) {
        if (linkRetCode == Node::eCanConnectInput_differentPars) {
            QString error = QCoreApplication::translate("NodeGraph", "%1 and %2 do not have the same pixel aspect ratio (%3 vs. %4), and %1 does not support inputs with different pixel aspect ratio.\n"
                                                        "This might yield unwanted results.")
                            .arg( QString::fromUtf8( outputNode->getNode()->getLabel().c_str() ) )
                            .arg( QString::fromUtf8( inputNode->getNode()->getLabel().c_str() ) )
                            .arg( outputNode->getNode()->getEffectInstance()->getAspectRatio(-1) )
                            .arg( inputNode->getNode()->getEffectInstance()->getAspectRatio(-1) );
            Dialogs::warningDialog( QCoreApplication::translate("NodeGraph", "Different pixel aspect").toStdString(),
                                    error.toStdString() );

            return true;
        } else if (linkRetCode == Node::eCanConnectInput_differentFPS) {
            QString error = QCoreApplication::translate("NodeGraph", "%1 and %2 do not have the same frame rate (%3 vs. %4).\n"
                                                        "This might yield unwanted results.\n"
                                                        "Either change the FPS from the Read node parameters or change the settings of the project.")
                            .arg( QString::fromUtf8( outputNode->getNode()->getLabel().c_str() ) )
                            .arg( QString::fromUtf8( inputNode->getNode()->getLabel().c_str() ) )
                            .arg( outputNode->getNode()->getEffectInstance()->getFrameRate() )
                            .arg( inputNode->getNode()->getEffectInstance()->getFrameRate() );
            Dialogs::warningDialog( QCoreApplication::translate("NodeGraph", "Different frame rate").toStdString(),
                                    error.toStdString() );

            return true;
        } else if (linkRetCode == Node::eCanConnectInput_groupHasNoOutput) {
            QString error = QCoreApplication::translate("NodeGraph", "You cannot connect %1 to %2, because it is a group which does not have an Output node.")
                            .arg( QString::fromUtf8( outputNode->getNode()->getLabel().c_str() ) )
                            .arg( QString::fromUtf8( inputNode->getNode()->getLabel().c_str() ) );
            Dialogs::errorDialog( QCoreApplication::translate("NodeGraph", "Different frame rate").toStdString(),
                                  error.toStdString() );
        } else if (linkRetCode == Node::eCanConnectInput_multiResNotSupported) {
            QString error = QCoreApplication::translate("NodeGraph", "You cannot connect %1 to %2 because multi-resolution is not supported on %1, "
                                                        "which means that it cannot receive images with a lower left corner different than (0,0) and cannot have "
                                                        "multiple inputs/outputs with different image sizes.\n"
                                                        "To overcome this, use a Resize or Crop node upstream to change the image size. In some cases you might also need to make sure the input image size is the project size.").arg( QString::fromUtf8( outputNode->getNode()->getLabel().c_str() ) )
                            .arg( QString::fromUtf8( inputNode->getNode()->getLabel().c_str() ) );
            Dialogs::errorDialog( QCoreApplication::translate("NodeGraph", "Multi-resolution not supported").toStdString(),
                                  error.toStdString() );;
        }

        return false;
    }

    if ( (linkRetCode == Node::eCanConnectInput_ok) && outputNode->getNode()->getEffectInstance()->isReader() &&
         ( inputNode->getNode()->getPluginID() != PLUGINID_OFX_RUNSCRIPT) &&
         !inputNode->getNode()->getEffectInstance()->isWriter() ) {
        Dialogs::warningDialog( QCoreApplication::translate("NodeGraph", "Reader input").toStdString(),
                                QCoreApplication::translate("NodeGraph", "Connecting an input to a Reader node "
                                                            "is only useful when connecting to the output of a Writer "
                                                            "or RunScript node "
                                                            "so that the Reader automatically reads an image "
                                                            "when the render of the Writer or RunScript is done.").toStdString() );
    }

    return true;
} // handleConnectionError

void
NodeGraph::mouseReleaseEvent(QMouseEvent* e)
{
    EventStateEnum state = _imp->_evtState;

    _imp->_evtState = eEventStateNone;

    _imp->autoScrollTimer.stop();

    bool hasMovedOnce = modCASIsControl(e) || _imp->_hasMovedOnce;
    if ( (state == eEventStateDraggingArrow) && hasMovedOnce ) {
        bool foundSrc = false;
        assert(_imp->_arrowSelected);
        NodeGuiPtr nodeHoldingEdge = _imp->_arrowSelected->isOutputEdge() ?
                                     _imp->_arrowSelected->getSource() : _imp->_arrowSelected->getDest();
        assert(nodeHoldingEdge);


        NodeGui* nearbyNode;
        Edge* nearbyEdge;
        NearbyItemEnum nearbyItemCode = hasItemNearbyMouse(e->pos(), &nearbyNode, &nearbyEdge);

        if (nearbyItemCode == eNearbyItemNode) {
            NodeGuiPtr targetNode = nearbyNode->shared_from_this();
            if (targetNode != nodeHoldingEdge) {
                if ( !_imp->_arrowSelected->isOutputEdge() ) {
                    bool ok = handleConnectionError( nodeHoldingEdge, targetNode, _imp->_arrowSelected->getInputNumber() );
                    _imp->_arrowSelected->stackBefore( targetNode.get() );
                    if (ok) {
                        foundSrc = true;
                        pushUndoCommand( new ConnectCommand(this, _imp->_arrowSelected, _imp->_arrowSelected->getSource(), targetNode) );
                    }
                } else {
                    // Find the input edge of the node we just released the mouse over,
                    // and use that edge to connect to the source of the selected edge.
                    int preferredInput = targetNode->getNode()->getPreferredInputForConnection();
                    if (preferredInput != -1) {
                        bool ok = handleConnectionError(targetNode, nodeHoldingEdge, preferredInput);
                        if (ok) {
                            Edge* foundInput = targetNode->getInputArrow(preferredInput);
                            assert(foundInput);
                            foundSrc = true;
                            pushUndoCommand( new ConnectCommand( this, foundInput,
                                                                 foundInput->getSource(), _imp->_arrowSelected->getSource() ) );
                        }
                    }
                }
            }
        }

        // If we disconnected the input edge, use the undo/redo stack.
        // Output edges can never be really connected, they're just there
        // So the user understands some nodes can have output
        if ( !foundSrc && !_imp->_arrowSelected->isOutputEdge() && _imp->_arrowSelected->getSource() ) {
            pushUndoCommand( new ConnectCommand( this, _imp->_arrowSelected, _imp->_arrowSelected->getSource(),
                                                 NodeGuiPtr() ) );
        }


        nodeHoldingEdge->refreshEdges();
        scene()->update();
    } else if (state == eEventStateDraggingNode) {
        if ( !_imp->_hasMovedOnce ) {
            if ( modCASIsControl(e) ) { // control-click is the same as double-click, see NodeGraph::mouseDoubleClickEvent
                NodeGui* nearbyNode;
                Edge* nearbyEdge;
                NearbyItemEnum nearbyItemCode = hasItemNearbyMouse(e->pos(), &nearbyNode, &nearbyEdge);

                if (nearbyItemCode == eNearbyItemNode || nearbyItemCode == eNearbyItemBackdropFrame) {
                    showNodePanel(false, modCASIsShift(e), nearbyNode);
                }
            }
        } else if ( !_imp->_selection.empty() ) {
            NodesGuiList nodesToMove;
            for (NodesGuiList::iterator it = _imp->_selection.begin();
                 it != _imp->_selection.end(); ++it) {
                const NodeGuiPtr& node = *it;
                nodesToMove.push_back(node);

                std::map<NodeGuiPtr, NodesGuiList>::iterator foundBd = _imp->_nodesWithinBDAtPenDown.find(*it);
                if ( !modCASIsControl(e) && ( foundBd != _imp->_nodesWithinBDAtPenDown.end() ) ) {
                    for (NodesGuiList::iterator it2 = foundBd->second.begin();
                         it2 != foundBd->second.end(); ++it2) {
                        ///add it only if it's not already in the list
                        bool found = false;
                        for (NodesGuiList::iterator it3 = nodesToMove.begin();
                             it3 != nodesToMove.end(); ++it3) {
                            if (*it3 == *it2) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            nodesToMove.push_back(*it2);
                        }
                    }
                }
            }
            if ( (_imp->_deltaSinceMousePress.x() != 0) || (_imp->_deltaSinceMousePress.y() != 0) ) {
                pushUndoCommand( new MoveMultipleNodesCommand( nodesToMove, _imp->_deltaSinceMousePress.x(), _imp->_deltaSinceMousePress.y() ) );
            }

            ///now if there was a hint displayed, use it to actually make connections.
            if (_imp->_highLightedEdge) {
                _imp->_highLightedEdge->setUseHighlight(false);

                NodeGuiPtr selectedNode = _imp->_selection.front();

                _imp->_highLightedEdge->setUseHighlight(false);
                if ( _imp->_highLightedEdge->isOutputEdge() ) {
                    int prefInput = selectedNode->getNode()->getPreferredInputForConnection();
                    if (prefInput != -1) {
                        Edge* inputEdge = selectedNode->getInputArrow(prefInput);
                        assert(inputEdge);
                        pushUndoCommand( new ConnectCommand( this, inputEdge, inputEdge->getSource(),
                                                             _imp->_highLightedEdge->getSource() ) );
                    }
                } else {
                    pushUndoCommand( new InsertNodeCommand(this, _imp->_highLightedEdge, selectedNode) );
                } // if ( _imp->_highLightedEdge->isOutputEdge() )

                _imp->_highLightedEdge = 0;
                _imp->_hintInputEdge->hide();
                _imp->_hintOutputEdge->hide();
            } else if (_imp->_mergeHintNode) {
                _imp->_mergeHintNode->setMergeHintActive(false);
                NodeGuiPtr selectedNode = _imp->_selection.front();
                selectedNode->setMergeHintActive(false);

                if ( getGui() ) {
                    QRectF selectedNodeBbox = selectedNode->mapToScene( selectedNode->boundingRect() ).boundingRect();
                    QRectF mergeHintNodeBbox = _imp->_mergeHintNode->mapToScene( _imp->_mergeHintNode->boundingRect() ).boundingRect();
                    QPointF mergeHintCenter = mergeHintNodeBbox.center();

                    ///Place the selected node on the right of the hint node
                    QPointF selectedNodePos(mergeHintCenter.x() + mergeHintNodeBbox.width() / 2. + TO_DPIX(NATRON_NODE_DUPLICATE_X_OFFSET),
                                            mergeHintCenter.y() - selectedNodeBbox.height() / 2.);

                    selectedNodePos = selectedNode->mapToParent( selectedNode->mapFromScene(selectedNodePos) );
                    selectedNode->setPosition( selectedNodePos.x(), selectedNodePos.y() );

                    selectedNodeBbox = selectedNode->mapToScene( selectedNode->boundingRect() ).boundingRect();

                    QPointF selectedNodeCenter = selectedNodeBbox.center();
                    CreateNodeArgs args( PLUGINID_OFX_MERGE, getGroup() );
                    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
                    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
                    
                    NodePtr mergeNode = getGui()->getApp()->createNode(args);

                    if (mergeNode) {
                        NodeGuiIPtr nodeUI = mergeNode->getNodeGui();
                        assert(nodeUI);
                        NodeGuiPtr nodeGui = boost::dynamic_pointer_cast<NodeGui>(nodeUI);
                        assert(nodeGui);

                        double nodeW, nodeH;
                        nodeGui->getSize(&nodeW, &nodeH);
                        ///Place the new merge node exactly in the middle of the 2, with an Y offset
                        QPointF newNodePos( ( mergeHintCenter.x() + selectedNodeCenter.x() ) / 2. - nodeW / 2.,
                                            std::max( (mergeHintCenter.y() + mergeHintNodeBbox.height() / 2.),
                                                      selectedNodeCenter.y() + selectedNodeBbox.height() / 2. ) + TO_DPIY(NodeGui::DEFAULT_OFFSET_BETWEEN_NODES) );


                        newNodePos = nodeGui->mapToParent( nodeGui->mapFromScene(newNodePos) );
                        nodeGui->setPosition( newNodePos.x(), newNodePos.y() );
                        int aIndex = mergeNode->getInputNumberFromLabel("A");
                        int bIndex = mergeNode->getInputNumberFromLabel("B");
                        assert(aIndex != -1 && bIndex != -1);
                        mergeNode->connectInput(selectedNode->getNode(), aIndex);
                        mergeNode->connectInput(_imp->_mergeHintNode->getNode(), bIndex);
                    }
                }

                _imp->_mergeHintNode.reset();
            }
        }
    } else if (state == eEventStateSelectionRect) {
        _imp->editSelectionFromSelectionRectangle( modCASIsShift(e) );

    }
    _imp->_nodesWithinBDAtPenDown.clear();

    _imp->cursorSet = false;
    unsetCursor();
    update();
} // mouseReleaseEvent

void
NodeGraph::scrollViewIfNeeded(const QPointF& scenePos)
{
    QPoint widgetPos = mapFromScene(scenePos);
    QRectF visibleRect = visibleWidgetRect();
    int scrollDeltaZone = 50;
    int dx = 0, dy = 0;

    if (widgetPos.x() <= visibleRect.x() + scrollDeltaZone) {
        dx = -NATRON_GRAPH_AUTO_SCROLL_OFFSET;
    } else if (widgetPos.x() >= ( visibleRect.x() + visibleRect.width() ) - scrollDeltaZone) {
        dx = NATRON_GRAPH_AUTO_SCROLL_OFFSET;
    }
    if (widgetPos.y() <= visibleRect.y() + scrollDeltaZone) {
        dy = -NATRON_GRAPH_AUTO_SCROLL_OFFSET;
    } else if (widgetPos.y() >= ( visibleRect.y() + visibleRect.height() ) - scrollDeltaZone) {
        dy = NATRON_GRAPH_AUTO_SCROLL_OFFSET;
    }


    if ( (dx != 0) || (dy != 0) ) {
        moveRootInternal(-dx, -dy);
        /*if (_imp->_evtState == eEventStateDraggingArrow && _imp->_arrowSelected) {

           } else if (_imp->_evtState == eEventStateDraggingNode && !_imp->_selection.empty()) {
            Qt::KeyboardModifiers mods = qApp->keyboardModifiers();
            QPointF lastMousePosScene = mapToScene(_imp->_lastMousePos);
            QPoint newPos = _imp->_lastMousePos + QPoint(dx,dy);
            QPointF newPosScene = mapToScene(newPos);
            moveSelectedNodesBy(mods.testFlag(Qt::ShiftModifier), mods.testFlag(Qt::ControlModifier), lastMousePosScene, newPosScene, visibleSceneRect(), false);
            _imp->_lastMousePos = newPos;
           }*/

        _imp->_refreshOverlays = true;
        update();
    } else {
        _imp->autoScrollTimer.stop();
    }
}

void
NodeGraph::moveRootInternal(double dx,
                            double dy)
{
    _imp->_lastSelectionStartPointScene.rx() += dx;
    _imp->_lastSelectionStartPointScene.ry() += dy;

    _imp->_root->moveBy(dx, dy);
}

void
NodeGraph::checkAndStartAutoScrollTimer(const QPointF& scenePos)
{
    if ( _imp->autoScrollTimer.isActive() ) {
        return;
    }
    QPoint widgetPos = mapFromScene(scenePos);
    QRectF visibleRect = visibleWidgetRect();
    int scrollDeltaZone = 50;
    bool startScrolling = false;
    if (widgetPos.x() <= visibleRect.x() + scrollDeltaZone) {
        startScrolling = true;
    } else if (widgetPos.x() >= ( visibleRect.x() + visibleRect.width() ) - scrollDeltaZone) {
        startScrolling = true;
    }
    if (widgetPos.y() <= visibleRect.y() + scrollDeltaZone) {
        startScrolling = true;
    } else if (widgetPos.y() >= ( visibleRect.y() + visibleRect.height() ) - scrollDeltaZone) {
        startScrolling = true;
    }

    if (startScrolling) {
        _imp->autoScrollTimer.start(NATRON_AUTO_SCROLL_TIMEOUT_MS);
    }
}

void
NodeGraph::onAutoScrollTimerTriggered()
{
    bool isAutoScrollEnabled = appPTR->getCurrentSettings()->isAutoScrollEnabled();

    if (isAutoScrollEnabled) {
        QPointF cursorPos = mapToScene( mapFromGlobal( QCursor::pos() ) );
        scrollViewIfNeeded(cursorPos);
    }
}

NATRON_NAMESPACE_EXIT
