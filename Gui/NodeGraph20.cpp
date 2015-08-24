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
NodeGraph::mouseMoveEvent(QMouseEvent* e)
{
    QPointF newPos = mapToScene( e->pos() );
    
    QPointF lastMousePosScene = mapToScene(_imp->_lastMousePos.x(),_imp->_lastMousePos.y());
    
    double dx = _imp->_root->mapFromScene(newPos).x() - _imp->_root->mapFromScene(lastMousePosScene).x();
    double dy = _imp->_root->mapFromScene(newPos).y() - _imp->_root->mapFromScene(lastMousePosScene).y();

    _imp->_hasMovedOnce = true;
    
    bool mustUpdate = true;
    
    QRectF sceneR = visibleSceneRect();
    if (_imp->_evtState != eEventStateSelectionRect && _imp->_evtState != eEventStateDraggingArrow) {
        ///set cursor
        boost::shared_ptr<NodeGui> selected;
        Edge* selectedEdge = 0;
        {
            bool optionalInputsAutoHidden = areOptionalInputsAutoHidden();
            QMutexLocker l(&_imp->_nodesMutex);
            for (NodeGuiList::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
                QPointF evpt = (*it)->mapFromScene(newPos);
                
                QRectF bbox = (*it)->mapToScene((*it)->boundingRect()).boundingRect();
                if ((*it)->isActive() && bbox.intersects(sceneR)) {
                    if ((*it)->contains(evpt)) {
                        selected = (*it);
                        if (optionalInputsAutoHidden) {
                            (*it)->setOptionalInputsVisible(true);
                        } else {
                            break;
                        }
                    } else {
                        Edge* edge = (*it)->hasEdgeNearbyPoint(newPos);
                        if (edge) {
                            selectedEdge = edge;
                            if (!optionalInputsAutoHidden) {
                                break;
                            }
                        } else if (optionalInputsAutoHidden && !(*it)->getIsSelected()) {
                            (*it)->setOptionalInputsVisible(false);
                        }
                    }
                }
                
            }
        }
        if (selected) {
            setCursor( QCursor(Qt::OpenHandCursor) );
        } else if (selectedEdge) {
        } else if (!selectedEdge && !selected) {
            unsetCursor();
        }
    }

    bool mustUpdateNavigator = false;
    ///Apply actions
    switch (_imp->_evtState) {
    case eEventStateDraggingArrow: {
        QPointF np = _imp->_arrowSelected->mapFromScene(newPos);
        if ( _imp->_arrowSelected->isOutputEdge() ) {
            _imp->_arrowSelected->dragDest(np);
        } else {
            _imp->_arrowSelected->dragSource(np);
        }
        scrollViewIfNeeded(newPos);
        mustUpdate = true;
        break;
    }
    case eEventStateDraggingNode: {
        mustUpdate = true;
        if ( !_imp->_selection.empty() ) {
            
            bool controlDown = modCASIsControl(e);

            std::list<std::pair<NodeGuiPtr,bool> > nodesToMove;
            for (std::list<NodeGuiPtr>::iterator it = _imp->_selection.begin();
                 it != _imp->_selection.end(); ++it) {
            
                const NodeGuiPtr& node = *it;
                nodesToMove.push_back(std::make_pair(node,false));
                
                std::map<NodeGuiPtr,NodeGuiList>::iterator foundBd = _imp->_nodesWithinBDAtPenDown.find(*it);
                if (!controlDown && foundBd != _imp->_nodesWithinBDAtPenDown.end()) {
                    for (NodeGuiList::iterator it2 = foundBd->second.begin();
                         it2 != foundBd->second.end(); ++it2) {
                        ///add it only if it's not already in the list
                        bool found = false;
                        for (std::list<std::pair<NodeGuiPtr,bool> >::iterator it3 = nodesToMove.begin();
                             it3 != nodesToMove.end(); ++it3) {
                            if (it3->first == *it2) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            nodesToMove.push_back(std::make_pair(*it2,true));
                        }
                    }

                }
            }
            
            double dxScene = newPos.x() - lastMousePosScene.x();
            double dyScene = newPos.y() - lastMousePosScene.y();
            
            QPointF newNodesCenter;
            
            bool deltaSet = false;
            for (std::list<std::pair<NodeGuiPtr,bool> >::iterator it = nodesToMove.begin();
                 it != nodesToMove.end(); ++it) {
                QPointF pos = it->first->getPos_mt_safe();
                bool ignoreMagnet = it->second || nodesToMove.size() > 1;
                it->first->refreshPosition(pos.x() + dxScene, pos.y() + dyScene,ignoreMagnet,newPos);
                QPointF newNodePos = it->first->getPos_mt_safe();
                if (!ignoreMagnet) {
                    assert(nodesToMove.size() == 1);
                    _imp->_deltaSinceMousePress.rx() += newNodePos.x() - pos.x();
                    _imp->_deltaSinceMousePress.ry() += newNodePos.y() - pos.y();
                    deltaSet = true;
                }
                newNodePos = it->first->mapToScene(it->first->mapFromParent(newNodePos));
                newNodesCenter.rx() += newNodePos.x();
                newNodesCenter.ry() += newNodePos.y();
                
            }
            size_t c = nodesToMove.size();
            if (c) {
                newNodesCenter.rx() /= c;
                newNodesCenter.ry() /= c;
            }
            
            scrollViewIfNeeded(newNodesCenter);
            
            if (!deltaSet) {
                _imp->_deltaSinceMousePress.rx() += dxScene;
                _imp->_deltaSinceMousePress.ry() += dyScene;
            }


            mustUpdateNavigator = true;
            
        }
        
        if (_imp->_selection.size() == 1) {
            ///try to find a nearby edge
            boost::shared_ptr<NodeGui> selectedNode = _imp->_selection.front();
            
            boost::shared_ptr<Natron::Node> internalNode = selectedNode->getNode();
            
            bool doMergeHints = e->modifiers().testFlag(Qt::ControlModifier) && e->modifiers().testFlag(Qt::ShiftModifier);
            bool doHints = appPTR->getCurrentSettings()->isConnectionHintEnabled();
            
            BackDropGui* isBd = dynamic_cast<BackDropGui*>(selectedNode.get());
            if (isBd) {
                doMergeHints = false;
                doHints = false;
            }
            
            if (!doMergeHints) {
                ///for nodes already connected don't show hint
                if ( ( internalNode->getMaxInputCount() == 0) && internalNode->hasOutputConnected() ) {
                    doHints = false;
                } else if ( ( internalNode->getMaxInputCount() > 0) && internalNode->hasAllInputsConnected() && internalNode->hasOutputConnected() ) {
                    doHints = false;
                }
            }
            
            if (doHints) {
                QRectF selectedNodeBbox = selectedNode->boundingRectWithEdges();//selectedNode->mapToParent( selectedNode->boundingRect() ).boundingRect();
                double tolerance = 10;
                selectedNodeBbox.adjust(-tolerance, -tolerance, tolerance, tolerance);
                
                boost::shared_ptr<NodeGui> nodeToShowMergeRect;
                
                boost::shared_ptr<Natron::Node> selectedNodeInternalNode = selectedNode->getNode();
                bool selectedNodeIsReader = selectedNodeInternalNode->getLiveInstance()->isReader();
                Edge* edge = 0;
                {
                    QMutexLocker l(&_imp->_nodesMutex);
                    for (NodeGuiList::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
                        
                        bool isAlreadyAnOutput = false;
                        const std::list<Natron::Node*>& outputs = internalNode->getGuiOutputs();
                        for (std::list<Natron::Node*>::const_iterator it2 = outputs.begin(); it2 != outputs.end(); ++it2) {
                            if (*it2 == (*it)->getNode().get()) {
                                isAlreadyAnOutput = true;
                                break;
                            }
                        }
                        if (isAlreadyAnOutput) {
                            continue;
                        }
                        QRectF nodeBbox = (*it)->boundingRectWithEdges();
                        if ( (*it) != selectedNode && (*it)->isVisible() && nodeBbox.intersects(sceneR)) {
                            
                            if (doMergeHints) {
                                
                                //QRectF nodeRect = (*it)->mapToParent((*it)->boundingRect()).boundingRect();
                                
                                boost::shared_ptr<Natron::Node> internalNode = (*it)->getNode();
                                
                                
                                if (!internalNode->isOutputNode() && nodeBbox.intersects(selectedNodeBbox)) {
                                    
                                    bool nHasInput = internalNode->hasInputConnected();
                                    int nMaxInput = internalNode->getMaxInputCount();
                                    bool selectedHasInput = selectedNodeInternalNode->hasInputConnected();
                                    int selectedMaxInput = selectedNodeInternalNode->getMaxInputCount();
                                    double nPAR = internalNode->getLiveInstance()->getPreferredAspectRatio();
                                    double selectedPAR = selectedNodeInternalNode->getLiveInstance()->getPreferredAspectRatio();
                                    double nFPS = internalNode->getLiveInstance()->getPreferredFrameRate();
                                    double selectedFPS = selectedNodeInternalNode->getLiveInstance()->getPreferredFrameRate();
                                    
                                    bool isValid = true;
                                    
                                    if (selectedPAR != nPAR || std::abs(nFPS - selectedFPS) > 0.01) {
                                        if (nHasInput || selectedHasInput) {
                                            isValid = false;
                                        } else if (!nHasInput && nMaxInput == 0 && !selectedHasInput && selectedMaxInput == 0) {
                                            isValid = false;
                                        }
                                    }
                                    if (isValid) {
                                        nodeToShowMergeRect = *it;
                                    }
                                } else {
                                    (*it)->setMergeHintActive(false);
                                }
                                
                            } else {
                                
                                
                                
                                edge = (*it)->hasEdgeNearbyRect(selectedNodeBbox);
                                
                                ///if the edge input is the selected node don't continue
                                if ( edge && ( edge->getSource() == selectedNode) ) {
                                    edge = 0;
                                }
                                
                                if ( edge && edge->isOutputEdge() ) {
                                    
                                    if (selectedNodeIsReader) {
                                        continue;
                                    }
                                    int prefInput = selectedNodeInternalNode->getPreferredInputForConnection();
                                    if (prefInput == -1) {
                                        edge = 0;
                                    } else {
                                        Natron::Node::CanConnectInputReturnValue ret = selectedNodeInternalNode->canConnectInput(edge->getSource()->getNode(),
                                                                                                                                 prefInput);
                                        if (ret != Natron::Node::eCanConnectInput_ok) {
                                            edge = 0;
                                        }
                                    }
                                }
                                
                                if ( edge && !edge->isOutputEdge() ) {
                                    
                                    if ((*it)->getNode()->getLiveInstance()->isReader()) {
                                        edge = 0;
                                        continue;
                                    }
                                    
                                    if ((*it)->getNode()->getLiveInstance()->isInputRotoBrush(edge->getInputNumber())) {
                                        edge = 0;
                                        continue;
                                    }
                                    
                                    Natron::Node::CanConnectInputReturnValue ret = edge->getDest()->getNode()->canConnectInput(selectedNodeInternalNode, edge->getInputNumber());
                                    if (ret == Natron::Node::eCanConnectInput_inputAlreadyConnected &&
                                        !selectedNodeInternalNode->getLiveInstance()->isReader()) {
                                        ret = Natron::Node::eCanConnectInput_ok;
                                    }
                                    
                                    if (ret != Natron::Node::eCanConnectInput_ok) {
                                        edge = 0;
                                    }
                                    
                                }
                                
                                if (edge) {
                                    edge->setUseHighlight(true);
                                    break;
                                }
                            }
                        }
                    }
                } // QMutexLocker l(&_imp->_nodesMutex);
                
                if ( _imp->_highLightedEdge && ( _imp->_highLightedEdge != edge) ) {
                    _imp->_highLightedEdge->setUseHighlight(false);
                    _imp->_hintInputEdge->hide();
                    _imp->_hintOutputEdge->hide();
                }

                _imp->_highLightedEdge = edge;

                if ( edge && edge->getSource() && edge->getDest() ) {
                    ///setup the hints edge

                    ///find out if the node is already connected to what the edge is connected
                    bool alreadyConnected = false;
                    const std::vector<boost::shared_ptr<Natron::Node> > & inpNodes = selectedNode->getNode()->getGuiInputs();
                    for (U32 i = 0; i < inpNodes.size(); ++i) {
                        if ( inpNodes[i] == edge->getSource()->getNode() ) {
                            alreadyConnected = true;
                            break;
                        }
                    }

                    if ( !_imp->_hintInputEdge->isVisible() ) {
                        if (!alreadyConnected) {
                            int prefInput = selectedNode->getNode()->getPreferredInputForConnection();
                            _imp->_hintInputEdge->setInputNumber(prefInput != -1 ? prefInput : 0);
                            _imp->_hintInputEdge->setSourceAndDestination(edge->getSource(), selectedNode);
                            _imp->_hintInputEdge->setVisible(true);
                        }
                        _imp->_hintOutputEdge->setInputNumber( edge->getInputNumber() );
                        _imp->_hintOutputEdge->setSourceAndDestination( selectedNode, edge->getDest() );
                        _imp->_hintOutputEdge->setVisible(true);
                    } else {
                        if (!alreadyConnected) {
                            _imp->_hintInputEdge->initLine();
                        }
                        _imp->_hintOutputEdge->initLine();
                    }
                } else if (edge) {
                    ///setup only 1 of the hints edge

                    if ( _imp->_highLightedEdge && !_imp->_hintInputEdge->isVisible() ) {
                        if ( edge->isOutputEdge() ) {
                            int prefInput = selectedNode->getNode()->getPreferredInputForConnection();
                            _imp->_hintInputEdge->setInputNumber(prefInput != -1 ? prefInput : 0);
                            _imp->_hintInputEdge->setSourceAndDestination(edge->getSource(), selectedNode);
                        } else {
                            _imp->_hintInputEdge->setInputNumber( edge->getInputNumber() );
                            _imp->_hintInputEdge->setSourceAndDestination( selectedNode,edge->getDest() );
                        }
                        _imp->_hintInputEdge->setVisible(true);
                    } else if ( _imp->_highLightedEdge && _imp->_hintInputEdge->isVisible() ) {
                        _imp->_hintInputEdge->initLine();
                    }
                } else if (nodeToShowMergeRect) {
                    nodeToShowMergeRect->setMergeHintActive(true);
                    selectedNode->setMergeHintActive(true);
                    _imp->_mergeHintNode = nodeToShowMergeRect;
                } else {
                    selectedNode->setMergeHintActive(false);
                    _imp->_mergeHintNode.reset();
                }
                
                
            } // if (doHints) {
        } //  if (_imp->_selection.nodes.size() == 1) {
        setCursor( QCursor(Qt::ClosedHandCursor) );
        break;
    }
    case eEventStateMovingArea: {
        mustUpdateNavigator = true;
        _imp->_root->moveBy(dx, dy);
        setCursor( QCursor(Qt::SizeAllCursor) );
        mustUpdate = true;
        break;
    }
    case eEventStateResizingBackdrop: {
        mustUpdateNavigator = true;
        assert(_imp->_backdropResized);
        QPointF p = _imp->_backdropResized->scenePos();
        int w = newPos.x() - p.x();
        int h = newPos.y() - p.y();
        scrollViewIfNeeded(newPos);
        mustUpdate = true;
        pushUndoCommand( new ResizeBackDropCommand(_imp->_backdropResized,w,h) );
        break;
    }
    case eEventStateSelectionRect: {
        QPointF lastSelectionScene = mapToScene(_imp->_lastSelectionStartPoint);
        QPointF startDrag = _imp->_selectionRect->mapFromScene(lastSelectionScene);
        QPointF cur = _imp->_selectionRect->mapFromScene(newPos);
        double xmin = std::min( cur.x(),startDrag.x() );
        double xmax = std::max( cur.x(),startDrag.x() );
        double ymin = std::min( cur.y(),startDrag.y() );
        double ymax = std::max( cur.y(),startDrag.y() );
        scrollViewIfNeeded(newPos);
        _imp->_selectionRect->setRect(xmin,ymin,xmax - xmin,ymax - ymin);
        _imp->_selectionRect->show();
        mustUpdate = true;
        break;
    }
    case eEventStateDraggingNavigator: {
        QPointF mousePosSceneCoordinates;
        bool insideNavigator = isNearbyNavigator(e->pos(), mousePosSceneCoordinates);
        if (insideNavigator) {
            _imp->_refreshOverlays = true;
            centerOn(mousePosSceneCoordinates);
            _imp->_lastMousePos = e->pos();
            update();
            return;
        }
        
    } break;
    case eEventStateZoomingArea: {
        int delta = 2*((e->x() - _imp->_lastMousePos.x()) - (e->y() - _imp->_lastMousePos.y()));
        setTransformationAnchor(QGraphicsView::AnchorViewCenter);
        wheelEventInternal(modCASIsControl(e),delta);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        mustUpdate = true;
    } break;
    default:
            mustUpdate = false;
        break;
    } // switch

    
    _imp->_lastMousePos = e->pos();

    if (mustUpdateNavigator) {
        _imp->_refreshOverlays = true;
        mustUpdate = true;
    }
    
    if (mustUpdate) {
        update();
    }
    QGraphicsView::mouseMoveEvent(e);
} // mouseMoveEvent


void
NodeGraph::mouseDoubleClickEvent(QMouseEvent* e)
{
    
    QPointF lastMousePosScene = mapToScene(_imp->_lastMousePos);

    NodeGuiList nodes = getAllActiveNodes_mt_safe();
    
    ///Matches sorted by depth
    std::map<double,NodeGuiPtr> matches;
    for (NodeGuiList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        QPointF evpt = (*it)->mapFromScene(lastMousePosScene);
        if ( (*it)->isVisible() && (*it)->isActive() && (*it)->contains(evpt) ) {
            matches.insert(std::make_pair((*it)->zValue(), *it));
        }
    }
    if (!matches.empty()) {
        const NodeGuiPtr& node = matches.rbegin()->second;
        if (modCASIsControl(e)) {
            node->ensurePanelCreated();
            if (node->getSettingPanel()) {
                node->getSettingPanel()->floatPanel();
            }
        } else {
            node->setVisibleSettingsPanel(true);
            if (node->getSettingPanel()) {
                _imp->_gui->putSettingsPanelFirst( node->getSettingPanel() );
            } else {
                ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>(node->getNode()->getLiveInstance());
                if (isViewer) {
                    ViewerGL* viewer = dynamic_cast<ViewerGL*>(isViewer->getUiContext());
                    assert(viewer);
                    ViewerTab* tab = viewer->getViewerTab();
                    assert(tab);
                    
                    TabWidget* foundTab = 0;
                    QWidget* parent = tab->parentWidget();
                    while (parent) {
                        foundTab = dynamic_cast<TabWidget*>(parent);
                        if (foundTab) {
                            break;
                        }
                        parent = parent->parentWidget();
                    }
                    if (foundTab) {
                        foundTab->setCurrentWidget(tab);
                    } else {
                        
                        //try to find a floating window
                        FloatingWidget* floating = 0;
                        parent = tab->parentWidget();
                        while (parent) {
                            floating = dynamic_cast<FloatingWidget*>(parent);
                            if (floating) {
                                break;
                            }
                            parent = parent->parentWidget();
                        }
                        if (floating) {
                            floating->activateWindow();
                        }
                    }
                }
            }
        }
        if ( !node->wasBeginEditCalled() ) {
            node->beginEditKnobs();
        }
        
        if (modCASIsShift(e)) {
            NodeGroup* isGrp = dynamic_cast<NodeGroup*>(node->getNode()->getLiveInstance());
            if (isGrp && isGrp->isSubGraphEditable()) {
                NodeGraphI* graph_i = isGrp->getNodeGraph();
                assert(graph_i);
                NodeGraph* graph = dynamic_cast<NodeGraph*>(graph_i);
                if (graph) {
                    TabWidget* isParentTab = dynamic_cast<TabWidget*>(graph->parentWidget());
                    if (isParentTab) {
                        isParentTab->setCurrentWidget(graph);
                    } else {
                        NodeGraph* lastSelectedGraph = _imp->_gui->getLastSelectedGraph();
                        ///We're in the double click event, it should've entered the focus in event beforehand!
                        assert(lastSelectedGraph == this);
                        
                        isParentTab = dynamic_cast<TabWidget*>(lastSelectedGraph->parentWidget());
                        assert(isParentTab);
                        isParentTab->appendTab(graph,graph);
                        
                    }
                    QTimer::singleShot(25, graph, SLOT(centerOnAllNodes()));
                }
            }
        }
        
        getGui()->getApp()->redrawAllViewers();
    }

}

bool
NodeGraph::event(QEvent* e)
{
    if (!_imp->_gui) {
        return false;
    }
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent* ke = dynamic_cast<QKeyEvent*>(e);
        assert(ke);
        if (ke && (ke->key() == Qt::Key_Tab) && _imp->_nodeCreationShortcutEnabled) {
            NodeCreationDialog* nodeCreation = new NodeCreationDialog(_imp->_lastNodeCreatedName,this);

            ///This allows us to have a non-modal dialog: when the user clicks outside of the dialog,
            ///it closes it.
            QObject::connect( nodeCreation,SIGNAL( accepted() ),this,SLOT( onNodeCreationDialogFinished() ) );
            QObject::connect( nodeCreation,SIGNAL( rejected() ),this,SLOT( onNodeCreationDialogFinished() ) );
            nodeCreation->show();


            ke->accept();

            return true;
        }
    }

    return QGraphicsView::event(e);
}

void
NodeGraph::onNodeCreationDialogFinished()
{
    NodeCreationDialog* dialog = qobject_cast<NodeCreationDialog*>( sender() );

    if (dialog) {
        QDialog::DialogCode ret = (QDialog::DialogCode)dialog->result();
        int major;
        QString res = dialog->getNodeName(&major);
        _imp->_lastNodeCreatedName = res;
        dialog->deleteLater();

        switch (ret) {
        case QDialog::Accepted: {
            
            const Natron::PluginsMap & allPlugins = appPTR->getPluginsList();
            Natron::PluginsMap::const_iterator found = allPlugins.find(res.toStdString());
            if (found != allPlugins.end()) {
                QPointF posHint = mapToScene( mapFromGlobal( QCursor::pos() ) );
                getGui()->getApp()->createNode( CreateNodeArgs( res,
                                                               "",
                                                               major,
                                                               -1,
                                                               true,
                                                               posHint.x(),
                                                               posHint.y(),
                                                               true,
                                                               true,
                                                               true,
                                                               QString(),
                                                               CreateNodeArgs::DefaultValuesList(),
                                                               getGroup()) );
            }
            break;
        }
        case QDialog::Rejected:
        default:
            break;
        }
    }
}

void
NodeGraph::keyPressEvent(QKeyEvent* e)
{
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();

    if (key == Qt::Key_Escape) {
        return QGraphicsView::keyPressEvent(e);
    }
    
    if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionShowPaneFullScreen, modifiers, key) ) {
        QKeyEvent* ev = new QKeyEvent(QEvent::KeyPress, key, modifiers);
        QCoreApplication::postEvent(parentWidget(),ev);
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateReader, modifiers, key) ) {
        _imp->_gui->createReader();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateWriter, modifiers, key) ) {
        _imp->_gui->createWriter();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphRemoveNodes, modifiers, key) ) {
        deleteSelection();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphForcePreview, modifiers, key) ) {
        forceRefreshAllPreviews();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCopy, modifiers, key) ) {
        copySelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphPaste, modifiers, key) ) {
        pasteNodeClipBoards();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCut, modifiers, key) ) {
        cutSelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDuplicate, modifiers, key) ) {
        duplicateSelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphClone, modifiers, key) ) {
        cloneSelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDeclone, modifiers, key) ) {
        decloneSelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphFrameNodes, modifiers, key) ) {
        centerOnAllNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphEnableHints, modifiers, key) ) {
        toggleConnectionHints();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSwitchInputs, modifiers, key) ) {
        ///No need to make an undo command for this, the user just have to do it a second time to reverse the effect
        switchInputs1and2ForSelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectAll, modifiers, key) ) {
        selectAllNodes(false);
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectAllVisible, modifiers, key) ) {
        selectAllNodes(true);
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphMakeGroup, modifiers, key) ) {
        createGroupFromSelection();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphExpandGroup, modifiers, key) ) {
        expandSelectedGroups();
    } else if (key == Qt::Key_Control) {
        _imp->setNodesBendPointsVisible(true);
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectUp, modifiers, key) ||
                isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphNavigateUpstream, modifiers, key) ) {
        ///We try to find if the last selected node has an input, if so move selection (or add to selection)
        ///the first valid input node
        if ( !_imp->_selection.empty() ) {
            boost::shared_ptr<NodeGui> lastSelected = ( *_imp->_selection.rbegin() );
            const std::vector<Edge*> & inputs = lastSelected->getInputsArrows();
            for (U32 i = 0; i < inputs.size() ; ++i) {
                if ( inputs[i]->hasSource() ) {
                    boost::shared_ptr<NodeGui> input = inputs[i]->getSource();
                    if ( input->getIsSelected() && modCASIsShift(e) ) {
                        std::list<boost::shared_ptr<NodeGui> >::iterator found = std::find(_imp->_selection.begin(),
                                                                                           _imp->_selection.end(),lastSelected);
                        if ( found != _imp->_selection.end() ) {
                            lastSelected->setUserSelected(false);
                            _imp->_selection.erase(found);
                        }
                    } else {
                        selectNode( inputs[i]->getSource(), modCASIsShift(e) );
                    }
                    break;
                }
            }
        }
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectDown, modifiers, key) ||
                isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphNavigateDownstream, modifiers, key) ) {
        ///We try to find if the last selected node has an output, if so move selection (or add to selection)
        ///the first valid output node
        if ( !_imp->_selection.empty() ) {
            boost::shared_ptr<NodeGui> lastSelected = ( *_imp->_selection.rbegin() );
            const std::list<Natron::Node* > & outputs = lastSelected->getNode()->getGuiOutputs();
            if ( !outputs.empty() ) {
                boost::shared_ptr<NodeGuiI> output_i = outputs.front()->getNodeGui();
                boost::shared_ptr<NodeGui> output = boost::dynamic_pointer_cast<NodeGui>(output_i);
                if (output) {
                    if ( output->getIsSelected() && modCASIsShift(e) ) {
                        std::list<boost::shared_ptr<NodeGui> >::iterator found = std::find(_imp->_selection.begin(),
                                                                                           _imp->_selection.end(),lastSelected);
                        if ( found != _imp->_selection.end() ) {
                            lastSelected->setUserSelected(false);
                            _imp->_selection.erase(found);
                        }
                    } else {
                        selectNode( output, modCASIsShift(e) );
                    }
                }
            }
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerFirst, modifiers, key) ) {
        if ( getLastSelectedViewer() ) {
            getLastSelectedViewer()->firstFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerLast, modifiers, key) ) {
        if ( getLastSelectedViewer() ) {
            getLastSelectedViewer()->lastFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevIncr, modifiers, key) ) {
        if ( getLastSelectedViewer() ) {
            getLastSelectedViewer()->previousIncrement();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextIncr, modifiers, key) ) {
        if ( getLastSelectedViewer() ) {
            getLastSelectedViewer()->nextIncrement();
        }
    }else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNext, modifiers, key) ) {
        if ( getLastSelectedViewer() ) {
            getLastSelectedViewer()->nextFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious, modifiers, key) ) {
        if ( getLastSelectedViewer() ) {
            getLastSelectedViewer()->previousFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevKF, modifiers, key) ) {
        getGui()->getApp()->getTimeLine()->goToPreviousKeyframe();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextKF, modifiers, key) ) {
        getGui()->getApp()->getTimeLine()->goToNextKeyframe();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphRearrangeNodes, modifiers, key) ) {
        _imp->rearrangeSelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDisableNodes, modifiers, key) ) {
        _imp->toggleSelectedNodesEnabled();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphShowExpressions, modifiers, key) ) {
        toggleKnobLinksVisible();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphToggleAutoPreview, modifiers, key) ) {
        toggleAutoPreview();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphToggleAutoTurbo, modifiers, key) ) {
        toggleAutoTurbo();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphAutoHideInputs, modifiers, key) ) {
        toggleAutoHideInputs(true);
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphFindNode, modifiers, key) ) {
        popFindDialog(QCursor::pos());
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphRenameNode, modifiers, key) ) {
        popRenameDialog(QCursor::pos());
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphExtractNode, modifiers, key) ) {
        pushUndoCommand(new ExtractNodeUndoRedoCommand(this,_imp->_selection));
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphTogglePreview, modifiers, key) ) {
        togglePreviewsForSelectedNodes();
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionZoomIn, Qt::NoModifier, key) ) { // zoom in/out doesn't care about modifiers
        QWheelEvent e(mapFromGlobal(QCursor::pos()), 120, Qt::NoButton, Qt::NoModifier); // one wheel click = +-120 delta
        wheelEvent(&e);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionZoomOut, Qt::NoModifier, key) ) { // zoom in/out doesn't care about modifiers
        QWheelEvent e(mapFromGlobal(QCursor::pos()), -120, Qt::NoButton, Qt::NoModifier); // one wheel click = +-120 delta
        wheelEvent(&e);
    } else {
        bool intercepted = false;
        
        if ( modifiers.testFlag(Qt::ControlModifier) && (key == Qt::Key_Up || key == Qt::Key_Down)) {
            ///These shortcuts pans the graphics view but we don't want it
            intercepted = true;
        }
        
        if (!intercepted) {
            /// Search for a node which has a shortcut bound
            const Natron::PluginsMap & allPlugins = appPTR->getPluginsList();
            for (Natron::PluginsMap::const_iterator it = allPlugins.begin() ; it != allPlugins.end() ; ++it) {
                
                assert(!it->second.empty());
                Natron::Plugin* plugin = *it->second.rbegin();
                
                if ( plugin->getHasShortcut() ) {
                    QString group(kShortcutGroupNodes);
                    QStringList groupingSplit = plugin->getGrouping();
                    for (int j = 0; j < groupingSplit.size(); ++j) {
                        group.push_back('/');
                        group.push_back(groupingSplit[j]);
                    }
                    if ( isKeybind(group.toStdString().c_str(), plugin->getPluginID(), modifiers, key) ) {
                        QPointF hint = mapToScene( mapFromGlobal( QCursor::pos() ) );
                        getGui()->getApp()->createNode( CreateNodeArgs( plugin->getPluginID(),
                                                                       "",
                                                                       -1,-1,
                                                                       true,
                                                                       hint.x(),hint.y(),
                                                                       true,
                                                                       true,
                                                                       true,
                                                                       QString(),
                                                                       CreateNodeArgs::DefaultValuesList(),
                                                                       getGroup()) );
                        intercepted = true;
                        break;
                    }
                }
            }
        }
        
        
        if (!intercepted) {
            QGraphicsView::keyPressEvent(e);
        }
    }
} // keyPressEvent

void
NodeGraph::toggleAutoTurbo()
{
    appPTR->getCurrentSettings()->setAutoTurboModeEnabled(!appPTR->getCurrentSettings()->isAutoTurboEnabled());
}


void
NodeGraph::selectAllNodes(bool onlyInVisiblePortion)
{
    _imp->resetSelection();
    if (onlyInVisiblePortion) {
        QRectF r = visibleSceneRect();
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            QRectF bbox = (*it)->mapToScene( (*it)->boundingRect() ).boundingRect();
            if ( r.intersects(bbox) && (*it)->isActive() && (*it)->isVisible() ) {
                (*it)->setUserSelected(true);
                _imp->_selection.push_back(*it);
            }
        }
  
    } else {
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            if ( (*it)->isActive() && (*it)->isVisible() ) {
                (*it)->setUserSelected(true);
                _imp->_selection.push_back(*it);
            }
        }
    }
}

