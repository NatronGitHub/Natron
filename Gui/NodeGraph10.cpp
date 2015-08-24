//  Natron
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
NodeGraph::mousePressEvent(QMouseEvent* e)
{
    assert(e);
    _imp->_hasMovedOnce = false;
    _imp->_deltaSinceMousePress = QPointF(0,0);
    if ( buttonDownIsMiddle(e) ) {
        _imp->_evtState = eEventStateMovingArea;

        return;
    }

    bool didSomething = false;

    _imp->_lastMousePos = e->pos();
    QPointF lastMousePosScene = mapToScene(_imp->_lastMousePos.x(),_imp->_lastMousePos.y());

    if (((e->buttons() & Qt::MiddleButton) &&
         (buttonMetaAlt(e) == Qt::AltModifier || (e->buttons() & Qt::LeftButton))) ||
        ((e->buttons() & Qt::LeftButton) &&
         (buttonMetaAlt(e) == (Qt::AltModifier|Qt::MetaModifier)))) {
        // Alt + middle or Left + middle or Crtl + Alt + Left = zoom
        _imp->_evtState = eEventStateZoomingArea;
        return;
    }
    
    NodeGuiPtr selected;
    Edge* selectedEdge = 0;
    Edge* selectedBendPoint = 0;
    {
        
        QMutexLocker l(&_imp->_nodesMutex);
        
        ///Find matches, sorted by depth
        std::map<double,NodeGuiPtr> matches;
        for (NodeGuiList::reverse_iterator it = _imp->_nodes.rbegin(); it != _imp->_nodes.rend(); ++it) {
            QPointF evpt = (*it)->mapFromScene(lastMousePosScene);
            if ( (*it)->isVisible() && (*it)->isActive() ) {
                
                BackDropGui* isBd = dynamic_cast<BackDropGui*>(it->get());
                if (isBd) {
                    if (isBd->isNearbyNameFrame(evpt)) {
                        matches.insert(std::make_pair((*it)->zValue(),*it));
                    } else if (isBd->isNearbyResizeHandle(evpt)) {
                        selected = *it;
                        _imp->_backdropResized = *it;
                        _imp->_evtState = eEventStateResizingBackdrop;
                        break;
                    }
                } else {
                    if ((*it)->contains(evpt)) {
                        matches.insert(std::make_pair((*it)->zValue(),*it));
                    }
                }
                
            }
        }
        if (!matches.empty() && _imp->_evtState != eEventStateResizingBackdrop) {
            selected = matches.rbegin()->second;
        }
        if (!selected) {
            ///try to find a selected edge
            for (NodeGuiList::reverse_iterator it = _imp->_nodes.rbegin(); it != _imp->_nodes.rend(); ++it) {
                Edge* bendPointEdge = (*it)->hasBendPointNearbyPoint(lastMousePosScene);

                if (bendPointEdge) {
                    selectedBendPoint = bendPointEdge;
                    break;
                }
                Edge* edge = (*it)->hasEdgeNearbyPoint(lastMousePosScene);
                if (edge) {
                    selectedEdge = edge;
                }
                
            }
        }
    }
    
    if (selected) {
        didSomething = true;
        if ( buttonDownIsLeft(e) ) {
            
            BackDropGui* isBd = dynamic_cast<BackDropGui*>(selected.get());
            if (!isBd) {
                _imp->_magnifiedNode = selected;
            }
            
            if ( !selected->getIsSelected() ) {
                selectNode( selected, modCASIsShift(e) );
            } else if ( modCASIsShift(e) ) {
                NodeGuiList::iterator it = std::find(_imp->_selection.begin(),
                                                     _imp->_selection.end(),selected);
                if ( it != _imp->_selection.end() ) {
                    (*it)->setUserSelected(false);
                    _imp->_selection.erase(it);
                }
            }
            if (_imp->_evtState != eEventStateResizingBackdrop) {
                _imp->_evtState = eEventStateDraggingNode;
            }
            ///build the _nodesWithinBDAtPenDown map
            _imp->_nodesWithinBDAtPenDown.clear();
            for (NodeGuiList::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
                BackDropGui* isBd = dynamic_cast<BackDropGui*>(it->get());
                if (isBd) {
                    NodeGuiList nodesWithin = getNodesWithinBackDrop(*it);
                    _imp->_nodesWithinBDAtPenDown.insert(std::make_pair(*it,nodesWithin));
                }
            }

            _imp->_lastNodeDragStartPoint = selected->getPos_mt_safe();
        } else if ( buttonDownIsRight(e) ) {
            if ( !selected->getIsSelected() ) {
                selectNode(selected,true); ///< don't wipe the selection
            }
        }
    } else if (selectedBendPoint) {
        _imp->setNodesBendPointsVisible(false);
        
        CreateNodeArgs args(PLUGINID_NATRON_DOT,
                            std::string(),
                            -1,
                            -1,
                            false, //< don't autoconnect
                            INT_MIN,
                            INT_MIN,
                            false, //<< don't push an undo command
                            true,
                            true,
                            QString(),
                            CreateNodeArgs::DefaultValuesList(),
                            _imp->group.lock());
        boost::shared_ptr<Natron::Node> dotNode = _imp->_gui->getApp()->createNode(args);
        assert(dotNode);
        boost::shared_ptr<NodeGuiI> dotNodeGui_i = dotNode->getNodeGui();
        boost::shared_ptr<NodeGui> dotNodeGui = boost::dynamic_pointer_cast<NodeGui>(dotNodeGui_i);
        assert(dotNodeGui);
        
        std::list<boost::shared_ptr<NodeGui> > nodesList;
        nodesList.push_back(dotNodeGui);
        
        ///Now connect the node to the edge input
        boost::shared_ptr<Natron::Node> inputNode = selectedBendPoint->getSource()->getNode();
        assert(inputNode);
        ///disconnect previous connection
        boost::shared_ptr<Natron::Node> outputNode = selectedBendPoint->getDest()->getNode();
        assert(outputNode);
        
        int inputNb = outputNode->inputIndex( inputNode.get() );
        assert(inputNb != -1);
        bool ok = _imp->_gui->getApp()->getProject()->disconnectNodes(inputNode.get(), outputNode.get());
        assert(ok);
        
        ok = _imp->_gui->getApp()->getProject()->connectNodes(0, inputNode, dotNode.get());
        assert(ok);
        
        _imp->_gui->getApp()->getProject()->connectNodes(inputNb,dotNode,outputNode.get());
        
        QPointF pos = dotNodeGui->mapToParent( dotNodeGui->mapFromScene(lastMousePosScene) );

        dotNodeGui->refreshPosition( pos.x(), pos.y() );
        if ( !dotNodeGui->getIsSelected() ) {
            selectNode( dotNodeGui, modCASIsShift(e) );
        }
        pushUndoCommand( new AddMultipleNodesCommand( this,nodesList) );
        
        
        _imp->_evtState = eEventStateDraggingNode;
        _imp->_lastNodeDragStartPoint = dotNodeGui->getPos_mt_safe();
        didSomething = true;
    } else if (selectedEdge) {
        _imp->_arrowSelected = selectedEdge;
        didSomething = true;
        _imp->_evtState = eEventStateDraggingArrow;
    }
    
    ///Test if mouse is inside the navigator
    {
        QPointF mousePosSceneCoordinates;
        bool insideNavigator = isNearbyNavigator(e->pos(), mousePosSceneCoordinates);
        if (insideNavigator) {
            updateNavigator();
            _imp->_refreshOverlays = true;
            centerOn(mousePosSceneCoordinates);
            _imp->_evtState = eEventStateDraggingNavigator;
            didSomething = true;
        }
    }

    ///Don't forget to reset back to null the _backdropResized pointer
    if (_imp->_evtState != eEventStateResizingBackdrop) {
        _imp->_backdropResized.reset();
    }

    if ( buttonDownIsRight(e) ) {
        showMenu( mapToGlobal( e->pos() ) );
        didSomething = true;
    }
    if (!didSomething) {
        if ( buttonDownIsLeft(e) ) {
            if ( !modCASIsShift(e) ) {
                deselect();
            }
            _imp->_evtState = eEventStateSelectionRect;
            _imp->_lastSelectionStartPoint = _imp->_lastMousePos;
            QPointF clickPos = _imp->_selectionRect->mapFromScene(lastMousePosScene);
            _imp->_selectionRect->setRect(clickPos.x(), clickPos.y(), 0, 0);
            //_imp->_selectionRect->show();
        } else if ( buttonDownIsMiddle(e) ) {
            _imp->_evtState = eEventStateMovingArea;
            QGraphicsView::mousePressEvent(e);
        }
    }
} // mousePressEvent

bool
NodeGraph::isNearbyNavigator(const QPoint& widgetPos,QPointF& scenePos) const
{
    if (!_imp->_navigator->isVisible()) {
        return false;
    }
    
    QRect visibleWidget = visibleWidgetRect();
    
    int navWidth = std::ceil(width() * NATRON_NAVIGATOR_BASE_WIDTH);
    int navHeight = std::ceil(height() * NATRON_NAVIGATOR_BASE_HEIGHT);

    QPoint btmRightWidget = visibleWidget.bottomRight();
    QPoint navTopLeftWidget = btmRightWidget - QPoint(navWidth,navHeight );
    
    if (widgetPos.x() >= navTopLeftWidget.x() && widgetPos.x() < btmRightWidget.x() &&
        widgetPos.y() >= navTopLeftWidget.y() && widgetPos.y() <= btmRightWidget.y()) {
        
        ///The bbox of all nodes in the nodegraph
        QRectF sceneR = _imp->calcNodesBoundingRect();
        
        ///The visible portion of the nodegraph
        QRectF viewRect = visibleSceneRect();
        sceneR = sceneR.united(viewRect);
        
        ///Make sceneR and viewRect keep the same aspect ratio as the navigator
        double xScale = navWidth / sceneR.width();
        double yScale =  navHeight / sceneR.height();
        double scaleFactor = std::max(0.001,std::min(xScale,yScale));

        ///Make the widgetPos relative to the navTopLeftWidget
        QPoint clickNavPos(widgetPos.x() - navTopLeftWidget.x(), widgetPos.y() - navTopLeftWidget.y());
        
        scenePos.rx() = clickNavPos.x() / scaleFactor;
        scenePos.ry() = clickNavPos.y() / scaleFactor;
        
        ///Now scenePos is in scene coordinates, but relative to the sceneR top left
        scenePos.rx() += sceneR.x();
        scenePos.ry() += sceneR.y();
        return true;
    }
    
    return false;
    
}

void
NodeGraph::pushUndoCommand(QUndoCommand* command)
{
    _imp->_undoStack->setActive();
    _imp->_undoStack->push(command);
}

bool
NodeGraph::areOptionalInputsAutoHidden() const
{
    return appPTR->getCurrentSettings()->areOptionalInputsAutoHidden();
}

void
NodeGraph::deselect()
{
    {
        QMutexLocker l(&_imp->_nodesMutex);
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
            (*it)->setUserSelected(false);
        }
    }
  
    _imp->_selection.clear();

    if (_imp->_magnifiedNode && _imp->_magnifOn) {
        _imp->_magnifOn = false;
        _imp->_magnifiedNode->setScale_natron(_imp->_nodeSelectedScaleBeforeMagnif);
    }
}

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

