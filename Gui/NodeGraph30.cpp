/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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

#include <cmath> // abs
#include <stdexcept>

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWheelEvent>
#include <QToolButton>
#include <QApplication>
#include <QTabBar>
#include <QTreeWidget>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Node.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/NodeGroup.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewerInstance.h"

#include "Gui/Edge.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/Histogram.h"
#include "Gui/NodeGui.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

#include "Global/QtCompat.h"

NATRON_NAMESPACE_ENTER
void
NodeGraph::connectCurrentViewerToSelection(int inputNB,
                                           bool isASide)
{
    ViewerTab* lastUsedViewer =  getLastSelectedViewer();

    if (lastUsedViewer) {
        NodeCollectionPtr collection = lastUsedViewer->getInternalNode()->getNode()->getGroup();
        if ( collection && (collection->getNodeGraph() != this) ) {
            //somehow the group doesn't belong to this nodegraph , pick another one
            const std::list<ViewerTab*>& tabs = getGui()->getViewersList();
            lastUsedViewer = 0;
            for (std::list<ViewerTab*>::const_iterator it = tabs.begin(); it != tabs.end(); ++it) {
                NodeCollectionPtr otherCollection = (*it)->getInternalNode()->getNode()->getGroup();
                if ( otherCollection && (otherCollection->getNodeGraph() == this) ) {
                    lastUsedViewer = *it;
                    break;
                }
            }
        }
    }


    ViewerNodePtr viewerNode;
    if (lastUsedViewer) {
        viewerNode = lastUsedViewer->getInternalNode();
    } else {
        CreateNodeArgsPtr args(CreateNodeArgs::create( PLUGINID_NATRON_VIEWER_GROUP,
                             getGroup() ));
        args->setProperty<bool>(kCreateNodeArgsPropSubGraphOpened, false);
        NodePtr node = getGui()->getApp()->createNode(args);

        if (!node) {
            return;
        }
        viewerNode = node->isEffectViewerNode();
    }

    if (!viewerNode) {
        return;
    }

    ///get a ptr to the NodeGui
    NodeGuiIPtr gui_i = viewerNode->getNode()->getNodeGui();
    NodeGuiPtr gui = boost::dynamic_pointer_cast<NodeGui>(gui_i);
    assert(gui);
    ///if there's no selected node or the viewer is selected, then try refreshing that input nb if it is connected.
    bool viewerAlreadySelected = _imp->findSelectedNode(gui) != _imp->_selection.end();
    if (_imp->_selection.empty() || (_imp->_selection.size() > 1) || viewerAlreadySelected) {
        viewerNode->connectInputToIndex(inputNB, isASide ? 0 : 1);
        return;
    }

    NodeGuiPtr selected = _imp->_selection.front().lock();
    if (selected && !selected->getNode()) {
        return;
    }

    if ( !selected->getNode()->canOthersConnectToThisNode() ) {
        return;
    }

    ///if the node doesn't have the input 'inputNb' created yet, populate enough input
    ///so it can be created.
    Edge* foundInput = gui->getInputArrow(inputNB);
    assert(foundInput);

    ///and push a connect command to the selected node.
    pushUndoCommand( new ConnectCommand(this, foundInput, foundInput->getSource(), selected, isASide ? 0 : 1) );

} // connectCurrentViewerToSelection

void
NodeGraph::enterEvent(QEvent* e)
{
    enterEventBase();
    QGraphicsView::enterEvent(e);
    _imp->_nodeCreationShortcutEnabled = true;
}

void
NodeGraph::leaveEvent(QEvent* e)
{
    leaveEventBase();
    QGraphicsView::leaveEvent(e);

    _imp->_nodeCreationShortcutEnabled = false;
    // setFocus();
}

void
NodeGraph::wheelEventInternal(bool ctrlDown,
                              double delta)
{
    double scaleFactor = pow( NATRON_WHEEL_ZOOM_PER_DELTA, delta);
    QTransform transfo = transform();
    double currentZoomFactor = transfo.mapRect( QRectF(0, 0, 1, 1) ).width();
    double newZoomfactor = currentZoomFactor * scaleFactor;

    if ( ( (newZoomfactor < 0.01) && (scaleFactor < 1.) ) || ( (newZoomfactor > 50) && (scaleFactor > 1.) ) ) {
        return;
    }

    if (ctrlDown && _imp->_magnifiedNode.lock()) {
        if (!_imp->_magnifOn) {
            _imp->_magnifOn = true;
            _imp->_nodeSelectedScaleBeforeMagnif = _imp->_magnifiedNode.lock()->scale();
        }
        _imp->_magnifiedNode.lock()->setScale_natron(_imp->_magnifiedNode.lock()->scale() * scaleFactor);
    } else {
        _imp->_accumDelta += delta;
        if (std::abs(_imp->_accumDelta) > 60) {
            scaleFactor = pow( NATRON_WHEEL_ZOOM_PER_DELTA, _imp->_accumDelta );
            // setSceneRect(NATRON_SCENE_MIN,NATRON_SCENE_MIN,NATRON_SCENE_MAX,NATRON_SCENE_MAX);
            scale(scaleFactor, scaleFactor);
            _imp->_accumDelta = 0;
        }
        _imp->_refreshOverlays = true;
    }
}

void
NodeGraph::wheelEvent(QWheelEvent* e)
{
    if (e->orientation() != Qt::Vertical) {
        return;
    }
    wheelEventInternal( modCASIsControl(e), e->delta() );
    _imp->_lastMousePos = e->pos();
    update();
}

void
NodeGraph::keyReleaseEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Control) {
        if (_imp->_magnifOn) {
            _imp->_magnifOn = false;
            _imp->_magnifiedNode.lock()->setScale_natron(_imp->_nodeSelectedScaleBeforeMagnif);
        }
        if (_imp->_bendPointsVisible) {
            _imp->setNodesBendPointsVisible(false);
        }
    }

    handleUnCaughtKeyUpEvent(e);
    QGraphicsView::keyReleaseEvent(e);
}

void
NodeGraph::removeNode(const NodeGuiPtr & node)
{
    if (node->getNode()->isEffectViewerInstance()) {
        Dialogs::errorDialog( tr("Delete").toStdString(), tr("Removing the internal viewer process node is not a valid action").toStdString());
        return;
    }
    NodeGroupPtr isGrp = node->getNode()->isEffectNodeGroup();
    const KnobsVec & knobs = node->getNode()->getKnobs();


    for (U32 i = 0; i < knobs.size(); ++i) {
        KnobDimViewKeySet listeners;
        knobs[i]->getListeners(listeners, KnobI::eListenersTypeExpression);
        ///For all listeners make sure they belong to a node
        std::string foundLinkedErrorMessage;
        for (KnobDimViewKeySet::iterator it2 = listeners.begin(); it2 != listeners.end(); ++it2) {
            KnobIPtr listener = it2->knob.lock();
            if (!listener) {
                continue;
            }
            EffectInstancePtr isEffect = toEffectInstance( listener->getHolder() );
            if (!isEffect || !isEffect->getNode()->getGroup() || !isEffect->getNode()->getNodeGui()) {
                continue;
            }
            if ( isGrp && (isEffect->getNode()->getGroup() == isGrp) ) {
                continue;
            }

            if ( isEffect && ( isEffect != node->getNode()->getEffectInstance() ) ) {
                std::string masterNodeName = node->getNode()->getFullyQualifiedName();
                std::string master = masterNodeName + "." + knobs[i]->getName();
                std::string slave = isEffect->getNode()->getFullyQualifiedName() + "." + listener->getName();
                foundLinkedErrorMessage = tr("%1 is linked to %2 with an expression. Deleting %3 may break this link.\nContinue anyway?").arg(QString::fromUtf8(slave.c_str())).arg(QString::fromUtf8(master.c_str())).arg(QString::fromUtf8(masterNodeName.c_str())).toStdString();
                break;
            }




        }
        if (!foundLinkedErrorMessage.empty()) {
            StandardButtonEnum reply = Dialogs::questionDialog( tr("Delete").toStdString(), foundLinkedErrorMessage, false );
            if (reply == eStandardButtonNo) {
                return;
            }
            break;
        }
    }

    node->setUserSelected(false);
    NodesGuiList nodesToRemove;
    nodesToRemove.push_back(node);
    pushUndoCommand( new RemoveMultipleNodesCommand(this, nodesToRemove) );
} // NodeGraph::removeNode

void
NodeGraph::deleteSelection()
{
    if ( !_imp->_selection.empty() ) {
        NodesGuiList nodesToRemove = getSelectedNodes();


        ///For all backdrops also move all the nodes contained within it
        for (NodesGuiList::iterator it = nodesToRemove.begin(); it != nodesToRemove.end(); ++it) {
            const NodeGuiPtr& node = *it;
            if (!node) {
                continue;
            }
            NodesGuiList nodesWithinBD = getNodesWithinBackdrop(node);
            for (NodesGuiList::iterator it2 = nodesWithinBD.begin(); it2 != nodesWithinBD.end(); ++it2) {
                NodesGuiList::iterator found = std::find(nodesToRemove.begin(), nodesToRemove.end(), *it2);
                if ( found == nodesToRemove.end() ) {
                    nodesToRemove.push_back(*it2);
                }
            }
        }


        for (NodesGuiList::iterator it = nodesToRemove.begin(); it != nodesToRemove.end(); ++it) {

            if ((*it)->getNode()->isEffectViewerInstance()) {
                Dialogs::errorDialog( tr("Delete").toStdString(), tr("Removing the internal viewer process node is not a valid action").toStdString());
                return;
            }
            const KnobsVec & knobs = (*it)->getNode()->getKnobs();
            bool mustBreak = false;
            NodeGroupPtr isGrp = (*it)->getNode()->isEffectNodeGroup();

            for (U32 i = 0; i < knobs.size(); ++i) {
                KnobDimViewKeySet listeners;
                knobs[i]->getListeners(listeners, KnobI::eListenersTypeExpression);

                ///For all listeners make sure they belong to a node
                std::string foundLinkedErrorMessage;
                for (KnobDimViewKeySet::iterator it2 = listeners.begin(); it2 != listeners.end(); ++it2) {
                    KnobIPtr listener = it2->knob.lock();
                    if (!listener) {
                        continue;
                    }
                    EffectInstancePtr isEffect = toEffectInstance( listener->getHolder() );

                    if (!isEffect || !isEffect->getNode()->getGroup()  || !isEffect->getNode()->getNodeGui()) {
                        continue;
                    }
                    if ( isGrp && (isEffect->getNode()->getGroup() == isGrp) ) {
                        continue;
                    }

                    if ( isEffect && ( isEffect != (*it)->getNode()->getEffectInstance() ) ) {
                        bool isInSelection = false;
                        for (NodesGuiList::iterator it3 = nodesToRemove.begin(); it3 != nodesToRemove.end(); ++it3) {
                            if ((*it3)->getNode()->getEffectInstance() == isEffect) {
                                isInSelection = true;
                                break;
                            }
                        }
                        if (!isInSelection) {
                            std::string masterNodeName = (*it)->getNode()->getFullyQualifiedName();
                            std::string master = masterNodeName + "." + knobs[i]->getName();
                            std::string slave = isEffect->getNode()->getFullyQualifiedName() + "." + listener->getName();
                            foundLinkedErrorMessage = tr("%1 is linked to %2 with an expression. Deleting %3 may break this link.\nContinue anyway?").arg(QString::fromUtf8(slave.c_str())).arg(QString::fromUtf8(master.c_str())).arg(QString::fromUtf8(masterNodeName.c_str())).toStdString();
                            break;
                        }
                    }
                }
                if (!foundLinkedErrorMessage.empty()) {
                    StandardButtonEnum reply = Dialogs::questionDialog( tr("Delete").toStdString(),
                                                                       foundLinkedErrorMessage, false);
                    if (reply == eStandardButtonNo) {
                        return;
                    }
                    mustBreak = true;
                    break;
                }
            }
            if (mustBreak) {
                break;
            }
        }


        for (NodesGuiList::iterator it = nodesToRemove.begin(); it != nodesToRemove.end(); ++it) {
            (*it)->setUserSelected(false);
        }


        pushUndoCommand( new RemoveMultipleNodesCommand(this, nodesToRemove) );
        _imp->_selection.clear();
    }
} // deleteSelection

void
NodeGraph::deselectNode(const NodeGuiPtr& n)
{
    {
        QMutexLocker k(&_imp->_nodesMutex);
        for (NodesGuiWList::iterator it = _imp->_selection.begin(); it!= _imp->_selection.end(); ++it) {
            if ( it->lock() == n) {
                _imp->_selection.erase(it);
                break;
            }
        }
    }
    n->setUserSelected(false);

    //Stop magnification if active
    if ( (_imp->_magnifiedNode.lock() == n) && _imp->_magnifOn ) {
        _imp->_magnifOn = false;
        _imp->_magnifiedNode.lock()->setScale_natron(_imp->_nodeSelectedScaleBeforeMagnif);
    }
}

void
NodeGraph::selectNode(const NodeGuiPtr & n,
                      bool addToSelection)
{
    if ( !n->isVisible() ) {
        return;
    }
    bool alreadyInSelection = _imp->findSelectedNode(n) != _imp->_selection.end();


    assert(n);
    if (addToSelection && !alreadyInSelection) {
        _imp->_selection.push_back(n);
    } else if (!addToSelection) {
        clearSelection();
        _imp->_selection.push_back(n);
    }

    n->setUserSelected(true);

    if (!n->getNode()) {
        return;
    }
    ViewerNodePtr isViewer =  n->getNode()->isEffectViewerNode();
    if (isViewer) {
        OpenGLViewerI* viewer = isViewer->getUiContext();
        const std::list<ViewerTab*> & viewerTabs = getGui()->getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = viewerTabs.begin(); it != viewerTabs.end(); ++it) {
            if ( (*it)->getViewer() == viewer ) {
                setLastSelectedViewer( (*it) );
            }
        }
    }

    bool magnifiedNodeSelected = false;
    if (_imp->_magnifiedNode.lock()) {
        magnifiedNodeSelected = _imp->findSelectedNode(_imp->_magnifiedNode.lock()) != _imp->_selection.end();

    }
    if (magnifiedNodeSelected && _imp->_magnifOn) {
        _imp->_magnifOn = false;
        _imp->_magnifiedNode.lock()->setScale_natron(_imp->_nodeSelectedScaleBeforeMagnif);
    }
}

void
NodeGraph::setLastSelectedViewer(ViewerTab* tab)
{
    _imp->lastSelectedViewer = tab;
}

ViewerTab*
NodeGraph::getLastSelectedViewer() const
{
    return _imp->lastSelectedViewer;
}

void
NodeGraph::setSelection(const NodesGuiList& nodes)
{
    clearSelection();
    for (NodesGuiList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        selectNode(*it, true);
    }
}

void
NodeGraph::setSelection(const NodesList& nodes)
{
    NodesGuiList nodeUIs;
    for (NodesList::const_iterator it = nodes.begin(); it!=nodes.end(); ++it) {
        NodeGuiPtr n = toNodeGui((*it)->getNodeGui());
        if (!n || n->getDagGui() != this) {
            continue;
        }
        nodeUIs.push_back(n);
    }
    setSelection(nodeUIs);
}

void
NodeGraph::clearSelection()
{
    {
        QMutexLocker l(&_imp->_nodesMutex);
        for (NodesGuiWList::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
            NodeGuiPtr n = it->lock();
            if (n) {
                n->setUserSelected(false);
            }
        }
    }

    _imp->_selection.clear();
}

void
NodeGraph::updateNavigator()
{
    if ( !areAllNodesVisible() ) {
        _imp->_navigator->setPixmap( QPixmap::fromImage( getFullSceneScreenShot() ) );
        _imp->_navigator->show();
    } else {
        _imp->_navigator->hide();
    }
}

bool
NodeGraph::areAllNodesVisible()
{
    QRectF rect = visibleSceneRect();
    QMutexLocker l(&_imp->_nodesMutex);

    for (NodesGuiList::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        if ( (*it)->isVisible() ) {
            if ( !rect.contains( (*it)->boundingRectWithEdges() ) ) {
                return false;
            }
        }
    }

    return true;
}

NATRON_NAMESPACE_EXIT
