/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#include <stdexcept>

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QMouseEvent>
#include <QApplication>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/CreateNodeArgs.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewerInstance.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/Edge.h"
#include "Gui/FloatingWidget.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h" // isKeyBind
#include "Gui/GuiMacros.h"
#include "Gui/NodeCreationDialog.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/TabWidget.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

#include "Global/QtCompat.h"

NATRON_NAMESPACE_ENTER

void
NodeGraph::showNodePanel(bool casIsCtrl, bool casIsShift, NodeGui* nearbyNode)
{
    if (!nearbyNode) {
        return;
    }
    if ( casIsCtrl ) {
        nearbyNode->ensurePanelCreated();
        if ( nearbyNode->getSettingPanel() ) {
            nearbyNode->getSettingPanel()->floatPanel();
        }
    } else {
        nearbyNode->setVisibleSettingsPanel(true);
        if ( nearbyNode->getSettingPanel() ) {
            getGui()->putSettingsPanelFirst( nearbyNode->getSettingPanel() );
        } else {
            ViewerInstance* isViewer = nearbyNode->getNode()->isEffectViewer();
            if (isViewer) {
                ViewerGL* viewer = dynamic_cast<ViewerGL*>( isViewer->getUiContext() );
                assert(viewer);
                if (viewer) {
                    ViewerTab* tab = viewer->getViewerTab();
                    assert(tab);

                    TabWidget* foundTab = 0;
                    QWidget* parent = 0;
                    if (tab) {
                        parent = tab->parentWidget();
                    }
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
                        if (tab) {
                            parent = tab->parentWidget();
                        }
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
                } // if (viewer)
            }
        }
    }
    if ( !nearbyNode->wasBeginEditCalled() ) {
        nearbyNode->beginEditKnobs();
    }

    if ( casIsShift ) {
        NodeGroup* isGrp = nearbyNode->getNode()->isEffectGroup();
        if ( isGrp && isGrp->isSubGraphUserVisible() ) {
            NodeGraphI* graph_i = isGrp->getNodeGraph();
            assert(graph_i);
            NodeGraph* graph = dynamic_cast<NodeGraph*>(graph_i);
            if (graph) {
                TabWidget* isParentTab = dynamic_cast<TabWidget*>( graph->parentWidget() );
                if (isParentTab) {
                    isParentTab->setCurrentWidget(graph);
                } else {
                    NodeGraph* lastSelectedGraph = getGui()->getLastSelectedGraph();
                    ///We're in the double click event, it should've entered the focus in event beforehand!
                    assert(lastSelectedGraph == this);

                    isParentTab = dynamic_cast<TabWidget*>( lastSelectedGraph->parentWidget() );
                    assert(isParentTab);
                    if (isParentTab) {
                        isParentTab->appendTab(graph, graph);
                    }
                }
                QTimer::singleShot( 25, graph, SLOT(centerOnAllNodes()) );
            }
        }
    }

    getGui()->getApp()->redrawAllViewers();
}

void
NodeGraph::mouseDoubleClickEvent(QMouseEvent* e)
{
    NodeGui* nearbyNode;
    Edge* nearbyEdge;
    NearbyItemEnum nearbyItemCode = hasItemNearbyMouse(e->pos(), &nearbyNode, &nearbyEdge);

    if (nearbyItemCode == eNearbyItemNode || nearbyItemCode == eNearbyItemBackdropFrame) {
        showNodePanel(modCASIsControl(e), modCASIsShift(e), nearbyNode);
    }
} // NodeGraph::mouseDoubleClickEvent

bool
NodeGraph::event(QEvent* e)
{
    if ( !getGui() ) {
        return false;
    }
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent* ke = dynamic_cast<QKeyEvent*>(e);
        assert(ke);
        if (ke && (ke->key() == Qt::Key_Tab) && _imp->_nodeCreationShortcutEnabled) {
            NodeCreationDialog* nodeCreation = new NodeCreationDialog(_imp->_lastNodeCreatedName, this);

            ///This allows us to have a non-modal dialog: when the user clicks outside of the dialog,
            ///it closes it.
            QObject::connect( nodeCreation, SIGNAL(dialogFinished(bool)), this, SLOT(onNodeCreationDialogFinished(bool)) );
            nodeCreation->show();

            takeClickFocus();
            ke->accept();

            return true;
        }
    }

    return QGraphicsView::event(e);
}

void
NodeGraph::onNodeCreationDialogFinished(bool accepted)
{
    NodeCreationDialog* dialog = qobject_cast<NodeCreationDialog*>( sender() );

    if (dialog) {
        int major;
        QString res = dialog->getNodeName(&major);

        if (accepted) {
            _imp->_lastNodeCreatedName = res;
            const PluginsMap & allPlugins = appPTR->getPluginsList();
            PluginsMap::const_iterator found = allPlugins.find( res.toStdString() );
            if ( found != allPlugins.end() ) {
                QPointF posHint = mapToScene( mapFromGlobal( QCursor::pos() ) );
                CreateNodeArgs args( res.toStdString(), getGroup() );
                args.setProperty<double>(kCreateNodeArgsPropNodeInitialPosition, posHint.x(), 0);
                args.setProperty<double>(kCreateNodeArgsPropNodeInitialPosition, posHint.y(), 1);
                args.setProperty<int>(kCreateNodeArgsPropPluginVersion, major, 0);
                getGui()->getApp()->createNode(args);
            }
        }

        dialog->close();

    }
}

void
NodeGraph::keyPressEvent(QKeyEvent* e)
{
    NodeCollectionPtr collection = getGroup();
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>( collection.get() );
    bool groupEdited = true;

    if (isGroup) {
        groupEdited = isGroup->getNode()->hasPyPlugBeenEdited();
    }

    if (!groupEdited) {
        return;
    }

    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();
    bool accept = true;

#ifndef NATRON_ENABLE_IO_META_NODES
    if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateReader, modifiers, key) ) {
        getGui()->createReader();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateWriter, modifiers, key) ) {
        getGui()->createWriter();
    } else
#endif
    if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphRemoveNodes, modifiers, key) ) {
        deleteSelection();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphForcePreview, modifiers, key) ) {
        forceRefreshAllPreviews();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCopy, modifiers, key) ) {
        copySelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphPaste, modifiers, key) ) {
        if ( !pasteNodeClipBoards() ) {
            accept = false;
        }
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
    } else if ( (key == Qt::Key_Control) && (e->modifiers() == Qt::ControlModifier) ) {
        _imp->setNodesBendPointsVisible(true);
        accept = false;
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectUp, modifiers, key) ||
                isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphNavigateUpstream, modifiers, key) ) {
        ///We try to find if the last selected node has an input, if so move selection (or add to selection)
        ///the first valid input node
        if ( !_imp->_selection.empty() ) {
            NodeGuiPtr lastSelected = ( *_imp->_selection.rbegin() );
            const std::vector<Edge*> & inputs = lastSelected->getInputsArrows();
            for (U32 i = 0; i < inputs.size(); ++i) {
                if ( inputs[i]->hasSource() ) {
                    NodeGuiPtr input = inputs[i]->getSource();
                    if ( input->getIsSelected() && modCASIsShift(e) ) {
                        NodesGuiList::iterator found = std::find(_imp->_selection.begin(),
                                                                 _imp->_selection.end(), lastSelected);
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
            NodeGuiPtr lastSelected = ( *_imp->_selection.rbegin() );
            const NodesWList & outputs = lastSelected->getNode()->getGuiOutputs();
            if ( !outputs.empty() ) {
                NodePtr firstOutput = outputs.front().lock();
                if (firstOutput) {
                    NodeGuiIPtr output_i = firstOutput->getNodeGui();
                    NodeGuiPtr output = boost::dynamic_pointer_cast<NodeGui>(output_i);
                    if (output) {
                        if ( output->getIsSelected() && modCASIsShift(e) ) {
                            NodesGuiList::iterator found = std::find(_imp->_selection.begin(),
                                                                     _imp->_selection.end(), lastSelected);
                            if ( found != _imp->_selection.end() ) {
                                lastSelected->setUserSelected(false);
                                _imp->_selection.erase(found);
                            }
                        } else {
                            selectNode( output, modCASIsShift(e) );
                        }
                    }
                } // firstOutput
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
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNext, modifiers, key) ) {
        if ( getLastSelectedViewer() ) {
            getLastSelectedViewer()->nextFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious, modifiers, key) ) {
        if ( getLastSelectedViewer() ) {
            getLastSelectedViewer()->previousFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevKF, modifiers, key) ) {
        getGui()->getApp()->goToPreviousKeyframe();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextKF, modifiers, key) ) {
        getGui()->getApp()->goToNextKeyframe();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphRearrangeNodes, modifiers, key) ) {
        if ( !_imp->rearrangeSelectedNodes() ) {
            accept = false;
        }
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDisableNodes, modifiers, key) ) {
        toggleSelectedNodesEnabled();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphShowExpressions, modifiers, key) ) {
        toggleKnobLinksVisible();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphToggleAutoPreview, modifiers, key) ) {
        toggleAutoPreview();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphToggleAutoTurbo, modifiers, key) ) {
        toggleAutoTurbo();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphAutoHideInputs, modifiers, key) ) {
        toggleAutoHideInputs(true);
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphFindNode, modifiers, key) ) {
        popFindDialog( QCursor::pos() );
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphRenameNode, modifiers, key) ) {
        renameNode();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphExtractNode, modifiers, key) ) {
        pushUndoCommand( new ExtractNodeUndoRedoCommand(this, _imp->_selection) );
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphTogglePreview, modifiers, key) ) {
        togglePreviewsForSelectedNodes();
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionZoomIn, Qt::NoModifier, key) ) { // zoom in/out doesn't care about modifiers
        QWheelEvent e(mapFromGlobal( QCursor::pos() ), 120, Qt::NoButton, Qt::NoModifier); // one wheel click = +-120 delta
        wheelEvent(&e);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionZoomOut, Qt::NoModifier, key) ) { // zoom in/out doesn't care about modifiers
        QWheelEvent e(mapFromGlobal( QCursor::pos() ), -120, Qt::NoButton, Qt::NoModifier); // one wheel click = +-120 delta
        wheelEvent(&e);
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphOpenNodePanel, modifiers, key) ) {
        if (_imp->_selection.size() == 1) {
            showNodePanel(modCASIsControl(e), modCASIsControl(e), _imp->_selection.front().get());
        } else {
            accept = false;
        }
    } else {
        bool intercepted = false;

        if ( modifiers.testFlag(Qt::ControlModifier) && ( (key == Qt::Key_Up) || (key == Qt::Key_Down) ) ) {
            ///These shortcuts pans the graphics view but we don't want it
            intercepted = true;
        }

        if (!intercepted) {
            /// Search for a node which has a shortcut bound
            const PluginsMap & allPlugins = appPTR->getPluginsList();
            for (PluginsMap::const_iterator it = allPlugins.begin(); it != allPlugins.end(); ++it) {
                assert( !it->second.empty() );
                Plugin* plugin = *it->second.rbegin();

                if ( plugin->getHasShortcut() ) {
                    QString group = QString::fromUtf8(kShortcutGroupNodes);
                    QStringList groupingSplit = plugin->getGrouping();
                    for (int j = 0; j < groupingSplit.size(); ++j) {
                        group.push_back( QLatin1Char('/') );
                        group.push_back(groupingSplit[j]);
                    }
                    if ( isKeybind(group.toStdString(), plugin->getPluginID().toStdString(), modifiers, key) ) {
                        QPointF hint = mapToScene( mapFromGlobal( QCursor::pos() ) );
                        CreateNodeArgs args( plugin->getPluginID().toStdString(), getGroup() );
                        args.setProperty<double>(kCreateNodeArgsPropNodeInitialPosition, hint.x(), 0);
                        args.setProperty<double>(kCreateNodeArgsPropNodeInitialPosition, hint.y(), 1);
                        getGui()->getApp()->createNode(args);

                        intercepted = true;
                        break;
                    }
                }
            }
        }


        if (!intercepted) {
            accept = false;
        }
    }
    if (accept) {
        takeClickFocus();
        e->accept();
    } else {
        handleUnCaughtKeyPressEvent(e);
        QGraphicsView::keyPressEvent(e);
    }
} // keyPressEvent

void
NodeGraph::toggleAutoTurbo()
{
    appPTR->getCurrentSettings()->setAutoTurboModeEnabled( !appPTR->getCurrentSettings()->isAutoTurboEnabled() );
}

void
NodeGraph::selectAllNodes(bool onlyInVisiblePortion)
{
    _imp->resetSelection();
    if (onlyInVisiblePortion) {
        QRectF r = visibleSceneRect();
        for (NodesGuiList::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            QRectF bbox = (*it)->mapToScene( (*it)->boundingRect() ).boundingRect();
            if ( r.intersects(bbox) && (*it)->isActive() && (*it)->isVisible() ) {
                (*it)->setUserSelected(true);
                _imp->_selection.push_back(*it);
            }
        }
    } else {
        for (NodesGuiList::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            if ( (*it)->isActive() && (*it)->isVisible() ) {
                (*it)->setUserSelected(true);
                _imp->_selection.push_back(*it);
            }
        }
    }
}

NATRON_NAMESPACE_EXIT
