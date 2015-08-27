/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QGraphicsProxyWidget>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QGraphicsTextItem>
#include <QFileSystemModel>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QDialogButtonBox>
#include <QUndoStack>
#include <QToolButton>
#include <QThread>
#include <QDropEvent>
#include <QApplication>
#include <QCheckBox>
#include <QMimeData>
#include <QLineEdit>
#include <QDebug>
#include <QtCore/QRectF>
#include <QRegExp>
#include <QtCore/QTimer>
#include <QAction>
#include <QPainter>
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QMutex>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include <SequenceParsing.h>

#include "Engine/AppManager.h"
#include "Engine/BackDrop.h"
#include "Engine/Dot.h"
#include "Engine/FrameEntry.h"
#include "Engine/Hash64.h"
#include "Engine/KnobFile.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeSerialization.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/ViewerInstance.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/BackDropGui.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveWidget.h"
#include "Gui/DockablePanel.h"
#include "Gui/Edge.h"
#include "Gui/FloatingWidget.h"
#include "Gui/Gui.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"
#include "Gui/Histogram.h"
#include "Gui/KnobGui.h"
#include "Gui/Label.h"
#include "Gui/Menu.h"
#include "Gui/NodeBackDropSerialization.h"
#include "Gui/NodeClipBoard.h"
#include "Gui/NodeCreationDialog.h"
#include "Gui/NodeGraphUndoRedo.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeGuiSerialization.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/TabWidget.h"
#include "Gui/TimeLineGui.h"
#include "Gui/ToolButton.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

#include "Global/QtCompat.h"

using namespace Natron;
using std::cout; using std::endl;


void
NodeGraph::mouseReleaseEvent(QMouseEvent* e)
{
    EventStateEnum state = _imp->_evtState;
    
    _imp->_evtState = eEventStateNone;
    
    bool hasMovedOnce = modCASIsControl(e) || _imp->_hasMovedOnce;
    if (state == eEventStateDraggingArrow && hasMovedOnce) {
        
        QRectF sceneR = visibleSceneRect();
        
        bool foundSrc = false;
        assert(_imp->_arrowSelected);
        boost::shared_ptr<NodeGui> nodeHoldingEdge = _imp->_arrowSelected->isOutputEdge() ?
                                                     _imp->_arrowSelected->getSource() : _imp->_arrowSelected->getDest();
        assert(nodeHoldingEdge);
        
        std::list<boost::shared_ptr<NodeGui> > nodes = getAllActiveNodes_mt_safe();
        QPointF ep = mapToScene( e->pos() );
        
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            boost::shared_ptr<NodeGui> & n = *it;
            
            BackDropGui* isBd = dynamic_cast<BackDropGui*>(n.get());
            if (isBd) {
                continue;
            }
            
            QRectF bbox = n->mapToScene(n->boundingRect()).boundingRect();
            
            if (n->isActive() && n->isVisible() && bbox.intersects(sceneR) &&
                n->isNearby(ep) &&
                n->getNode()->getScriptName() != nodeHoldingEdge->getNode()->getScriptName()) {
                
                if ( !_imp->_arrowSelected->isOutputEdge() ) {
                    
                    Natron::Node::CanConnectInputReturnValue linkRetCode =
                    nodeHoldingEdge->getNode()->canConnectInput(n->getNode(), _imp->_arrowSelected->getInputNumber());
                    if (linkRetCode != Natron::Node::eCanConnectInput_ok && linkRetCode != Natron::Node::eCanConnectInput_inputAlreadyConnected) {
                        if (linkRetCode == Natron::Node::eCanConnectInput_differentPars) {
                            
                            QString error = QString(tr("You cannot connect ") +  "%1" + tr(" to ") + "%2"  + tr(" because they don't have the same pixel aspect ratio (")
                                                    + "%3 / %4 " +  tr(") and ") + "%1 " + " doesn't support inputs with different pixel aspect ratio.")
                            .arg(nodeHoldingEdge->getNode()->getLabel().c_str())
                            .arg(n->getNode()->getLabel().c_str())
                            .arg(nodeHoldingEdge->getNode()->getLiveInstance()->getPreferredAspectRatio())
                            .arg(n->getNode()->getLiveInstance()->getPreferredAspectRatio());
                            Natron::errorDialog(tr("Different pixel aspect").toStdString(),
                                                error.toStdString());
                        } else if (linkRetCode == Natron::Node::eCanConnectInput_differentFPS) {

                            QString error = QString(tr("You cannot connect ") +  "%1" + tr(" to ") + "%2"  + tr(" because they don't have the same frame rate (") + "%3 / %4). Either change the FPS from the Read node parameters or change the settings of the project.")
                            .arg(nodeHoldingEdge->getNode()->getLabel().c_str())
                            .arg(n->getNode()->getLabel().c_str())
                            .arg(nodeHoldingEdge->getNode()->getLiveInstance()->getPreferredFrameRate())
                            .arg(n->getNode()->getLiveInstance()->getPreferredFrameRate());
                            Natron::errorDialog(tr("Different frame rate").toStdString(),
                                                error.toStdString());

                        } else if (linkRetCode == Natron::Node::eCanConnectInput_groupHasNoOutput) {
                            QString error = QString(tr("You cannot connect ") + "%1 " + tr(" to ") + " %2 " + tr("because it is a group which does "
                                                                                                                 "not have an Output node."))
                            .arg(nodeHoldingEdge->getNode()->getLabel().c_str())
                            .arg(n->getNode()->getLabel().c_str());
                            Natron::errorDialog(tr("Different frame rate").toStdString(),
                                                error.toStdString());

                        }
                        break;
                    }
         
                    if (linkRetCode == Natron::Node::eCanConnectInput_ok && nodeHoldingEdge->getNode()->getLiveInstance()->isReader() &&
                        n->getNode()->getPluginID() != PLUGINID_OFX_RUNSCRIPT) {
                        Natron::warningDialog(tr("Reader input").toStdString(), tr("Connecting an input to a Reader node "
                                                                                   "is only useful when using the RunScript node "
                                                                                   "so that the Reader automatically reads an image "
                                                                                   "when the render of the RunScript is done.").toStdString());
                    }
                    _imp->_arrowSelected->stackBefore( n.get() );
                    pushUndoCommand( new ConnectCommand(this,_imp->_arrowSelected,_imp->_arrowSelected->getSource(),n) );
                } else {
                    ///Find the input edge of the node we just released the mouse over,
                    ///and use that edge to connect to the source of the selected edge.
                    int preferredInput = n->getNode()->getPreferredInputForConnection();
                    if (preferredInput != -1) {
                        
                        Natron::Node::CanConnectInputReturnValue linkRetCode = n->getNode()->canConnectInput(nodeHoldingEdge->getNode(), preferredInput);
                        if (linkRetCode != Natron::Node::eCanConnectInput_ok  && linkRetCode != Natron::Node::eCanConnectInput_inputAlreadyConnected) {
                            
                            if (linkRetCode == Natron::Node::eCanConnectInput_differentPars) {
                                
                                QString error = QString(tr("You cannot connect ") +  "%1" + " to " + "%2"  + tr(" because they don't have the same pixel aspect ratio (")
                                                        + "%3 / %4 " +  tr(") and ") + "%1 " + " doesn't support inputs with different pixel aspect ratio.")
                                .arg(n->getNode()->getLabel().c_str())
                                .arg(nodeHoldingEdge->getNode()->getLabel().c_str())
                                .arg(n->getNode()->getLiveInstance()->getPreferredAspectRatio())
                                .arg(nodeHoldingEdge->getNode()->getLiveInstance()->getPreferredAspectRatio());
                                Natron::errorDialog(tr("Different pixel aspect").toStdString(),
                                                    error.toStdString());
                            } else if (linkRetCode == Natron::Node::eCanConnectInput_differentFPS) {

                                QString error = QString(tr("You cannot connect ") +  "%1" + " to " + "%2"  + tr(" because they don't have the same frame rate (") + "%3 / %4). Either change the FPS from the Read node parameters or change the settings of the project.")
                                .arg(nodeHoldingEdge->getNode()->getLabel().c_str())
                                .arg(n->getNode()->getLabel().c_str())
                                .arg(nodeHoldingEdge->getNode()->getLiveInstance()->getPreferredFrameRate())
                                .arg(n->getNode()->getLiveInstance()->getPreferredFrameRate());
                                Natron::errorDialog(tr("Different frame rate").toStdString(),
                                                    error.toStdString());

                            } else if (linkRetCode == Natron::Node::eCanConnectInput_groupHasNoOutput) {
                                QString error = QString(tr("You cannot connect ") + "%1 " + tr(" to ") + " %2 " + tr("because it is a group which does "
                                                                                                                     "not have an Output node."))
                                .arg(nodeHoldingEdge->getNode()->getLabel().c_str())
                                .arg(n->getNode()->getLabel().c_str());
                                Natron::errorDialog(tr("Different frame rate").toStdString(),
                                                    error.toStdString());
                                
                            }


                            
                            break;
                        }
                        if (linkRetCode == Natron::Node::eCanConnectInput_ok && n->getNode()->getLiveInstance()->isReader() &&
                            nodeHoldingEdge->getNode()->getPluginID() != PLUGINID_OFX_RUNSCRIPT) {
                            Natron::warningDialog(tr("Reader input").toStdString(), tr("Connecting an input to a Reader node "
                                                                                       "is only useful when using the RunScript node "
                                                                                       "so that the Reader automatically reads an image "
                                                                                       "when the render of the RunScript is done.").toStdString());
                        }
                        Edge* foundInput = n->getInputArrow(preferredInput);
                        assert(foundInput);
                        pushUndoCommand( new ConnectCommand( this,foundInput,
                                                                  foundInput->getSource(),_imp->_arrowSelected->getSource() ) );
                    }
                }
                foundSrc = true;
                
                break;
            }
        }
        ///if we disconnected the input edge, use the undo/redo stack.
        ///Output edges can never be really connected, they're just there
        ///So the user understands some nodes can have output
        if ( !foundSrc && !_imp->_arrowSelected->isOutputEdge() && _imp->_arrowSelected->getSource() ) {
            pushUndoCommand( new ConnectCommand( this,_imp->_arrowSelected,_imp->_arrowSelected->getSource(),
                                                      boost::shared_ptr<NodeGui>() ) );
        }
        
        
        
        nodeHoldingEdge->refreshEdges();
        scene()->update();
    } else if (state == eEventStateDraggingNode) {
        if ( !_imp->_selection.empty() ) {
            
            std::list<NodeGuiPtr> nodesToMove;
            for (std::list<NodeGuiPtr>::iterator it = _imp->_selection.begin();
                 it != _imp->_selection.end(); ++it) {
                
                const NodeGuiPtr& node = *it;
                nodesToMove.push_back(node);
                
                std::map<NodeGuiPtr,NodeGuiList>::iterator foundBd = _imp->_nodesWithinBDAtPenDown.find(*it);
                if (!modCASIsControl(e) && foundBd != _imp->_nodesWithinBDAtPenDown.end()) {
                    for (NodeGuiList::iterator it2 = foundBd->second.begin();
                         it2 != foundBd->second.end(); ++it2) {
                        ///add it only if it's not already in the list
                        bool found = false;
                        for (std::list<NodeGuiPtr>::iterator it3 = nodesToMove.begin();
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
            if (_imp->_deltaSinceMousePress.x() != 0 || _imp->_deltaSinceMousePress.y() != 0) {
                pushUndoCommand(new MoveMultipleNodesCommand(nodesToMove,_imp->_deltaSinceMousePress.x(),_imp->_deltaSinceMousePress.y()));
            }
            
            ///now if there was a hint displayed, use it to actually make connections.
            if (_imp->_highLightedEdge) {
                
                _imp->_highLightedEdge->setUseHighlight(false);

                boost::shared_ptr<NodeGui> selectedNode = _imp->_selection.front();
                
                _imp->_highLightedEdge->setUseHighlight(false);
                if ( _imp->_highLightedEdge->isOutputEdge() ) {
                    int prefInput = selectedNode->getNode()->getPreferredInputForConnection();
                    if (prefInput != -1) {
                        Edge* inputEdge = selectedNode->getInputArrow(prefInput);
                        assert(inputEdge);
                        pushUndoCommand( new ConnectCommand( this,inputEdge,inputEdge->getSource(),
                                                                    _imp->_highLightedEdge->getSource() ) );
                    }
                } else {
                    boost::shared_ptr<NodeGui> src = _imp->_highLightedEdge->getSource();
                    pushUndoCommand( new ConnectCommand(this,_imp->_highLightedEdge,_imp->_highLightedEdge->getSource(),
                                                               selectedNode) );

                    ///find out if the node is already connected to what the edge is connected
                    bool alreadyConnected = false;
                    const std::vector<boost::shared_ptr<Natron::Node> > & inpNodes = selectedNode->getNode()->getGuiInputs();
                    if (src) {
                        for (U32 i = 0; i < inpNodes.size(); ++i) {
                            if ( inpNodes[i] == src->getNode() ) {
                                alreadyConnected = true;
                                break;
                            }
                        }
                    }

                    if (src && !alreadyConnected) {
                        ///push a second command... this is a bit dirty but I don't have time to add a whole new command just for this
                        int prefInput = selectedNode->getNode()->getPreferredInputForConnection();
                        if (prefInput != -1) {
                            Edge* inputEdge = selectedNode->getInputArrow(prefInput);
                            assert(inputEdge);
                            pushUndoCommand( new ConnectCommand(this,inputEdge,inputEdge->getSource(),src) );
                        }
                    }
                }

                _imp->_highLightedEdge = 0;
                _imp->_hintInputEdge->hide();
                _imp->_hintOutputEdge->hide();
            } else if (_imp->_mergeHintNode) {
                _imp->_mergeHintNode->setMergeHintActive(false);
                boost::shared_ptr<NodeGui> selectedNode = _imp->_selection.front();
                selectedNode->setMergeHintActive(false);
                
                if (getGui()) {
                    
                    QRectF selectedNodeBbox = selectedNode->mapToScene(selectedNode->boundingRect()).boundingRect();
                    QRectF mergeHintNodeBbox = _imp->_mergeHintNode->mapToScene(_imp->_mergeHintNode->boundingRect()).boundingRect();
                    QPointF mergeHintCenter = mergeHintNodeBbox.center();
                    
                    ///Place the selected node on the right of the hint node
                    selectedNode->setPosition(mergeHintCenter.x() + mergeHintNodeBbox.width() / 2. + NATRON_NODE_DUPLICATE_X_OFFSET,
                                              mergeHintCenter.y() - selectedNodeBbox.height() / 2.);
                    
                    selectedNodeBbox = selectedNode->mapToScene(selectedNode->boundingRect()).boundingRect();
                    
                    QPointF selectedNodeCenter = selectedNodeBbox.center();
                    ///Place the new merge node exactly in the middle of the 2, with an Y offset
                    QPointF newNodePos((mergeHintCenter.x() + selectedNodeCenter.x()) / 2. - 40,
                                       std::max((mergeHintCenter.y() + mergeHintNodeBbox.height() / 2.),
                                                selectedNodeCenter.y() + selectedNodeBbox.height() / 2.) + NodeGui::DEFAULT_OFFSET_BETWEEN_NODES);
                    
                    
                    CreateNodeArgs args(PLUGINID_OFX_MERGE,
                                        "",
                                        -1,
                                        -1,
                                        false,
                                        newNodePos.x(),newNodePos.y(),
                                        true,
                                        true,
                                        true,
                                        QString(),
                                        CreateNodeArgs::DefaultValuesList(),
                                        getGroup());

                
                    
                    boost::shared_ptr<Natron::Node> mergeNode = getGui()->getApp()->createNode(args);
                    
                    if (mergeNode) {
                        
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
        _imp->_selectionRect->hide();
        _imp->editSelectionFromSelectionRectangle( modCASIsShift(e) );
    }
    _imp->_nodesWithinBDAtPenDown.clear();

    unsetCursor();
} // mouseReleaseEvent

void
NodeGraph::scrollViewIfNeeded(const QPointF& scenePos)
{

    //Doesn't work for now
    return;
#if 0
    QRectF visibleRect = visibleSceneRect();
    
   
    
    static const int delta = 50;
    
    int deltaX = 0;
    int deltaY = 0;
    if (scenePos.x() < visibleRect.left()) {
        deltaX = -delta;
    } else if (scenePos.x() > visibleRect.right()) {
        deltaX = delta;
    }
    if (scenePos.y() < visibleRect.top()) {
        deltaY = -delta;
    } else if (scenePos.y() > visibleRect.bottom()) {
        deltaY = delta;
    }
    if (deltaX != 0 || deltaY != 0) {
        QPointF newCenter = visibleRect.center();
        newCenter.rx() += deltaX;
        newCenter.ry() += deltaY;
        centerOn(newCenter);
    }
#endif
}

