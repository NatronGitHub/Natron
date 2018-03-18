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

#include "ViewerTab.h"
#include "ViewerTabPrivate.h"

#include <cassert>
#include <cmath>
#include <stdexcept>

#include <QtCore/QDebug>

#include <QVBoxLayout>
#include <QCheckBox>
#include <QToolBar>

#include "Engine/Settings.h"
#include "Engine/Node.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerNode.h"

#include "Gui/Button.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/InfoViewerWidget.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeViewerContext.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TimeLineGui.h"
#include "Gui/ViewerGL.h"


#ifndef M_LN2
#define M_LN2       0.693147180559945309417232121458176568  /* loge(2)        */
#endif


NATRON_NAMESPACE_ENTER




NodeGuiPtr
ViewerTab::getCurrentNodeViewerInterface(const PluginPtr& plugin) const
{
    for (std::list<ViewerTabPrivate::PluginViewerContext>::const_iterator it = _imp->currentNodeContext.begin(); it!=_imp->currentNodeContext.end(); ++it) {
        if (it->plugin.lock() == plugin || it->pyPlug.lock() == plugin) {
            return it->currentNode.lock();
        }
    }
    return NodeGuiPtr();
}

/**
 * @brief Creates a new viewer interface context for this node. This is not shared among viewers.
 **/
void
ViewerTab::createNodeViewerInterface(const NodeGuiPtr& n)
{
    if (!n) {
        return;
    }
    std::map<NodeGuiWPtr, NodeViewerContextPtr>::iterator found = _imp->nodesContext.find(n);
    if ( found != _imp->nodesContext.end() ) {
        // Already exists
        return;
    }

    ViewerNodePtr isViewerNode = n->getNode()->isEffectViewerNode();
    if (isViewerNode && isViewerNode != getInternalNode()) {
        return;
    }

    boost::shared_ptr<NodeViewerContext> nodeContext = NodeViewerContext::create(n, this);
    nodeContext->createGui();
    _imp->nodesContext.insert( std::make_pair(n, nodeContext) );

    if ( n->isSettingsPanelVisible() ) {
        setPluginViewerInterface(n);
    } else {
        QToolBar *toolbar = nodeContext->getToolBar();
        if (toolbar) {
            toolbar->hide();
        }
        QWidget* w = nodeContext->getContainerWidget();
        if (w) {
            w->hide();
        }
    }
}

/**
 * @brief Set the current viewer interface for a given plug-in to be the one of the given node
 **/
void
ViewerTab::setPluginViewerInterface(const NodeGuiPtr& n)
{
    if (!n) {
        return;
    }
    std::map<NodeGuiWPtr, NodeViewerContextPtr>::iterator it = _imp->nodesContext.find(n);
    if ( it == _imp->nodesContext.end() ) {
        return;
    }

    std::list<ViewerTabPrivate::PluginViewerContext>::iterator foundActive = _imp->findActiveNodeContextForNode(n->getNode());
    NodeGuiPtr activeNodeForPlugin;
    if ( foundActive != _imp->currentNodeContext.end() ) {
        activeNodeForPlugin = foundActive->currentNode.lock();
    }

    // If already active, return
    if (activeNodeForPlugin == n) {
        return;
    }

    QToolBar* newToolbar = it->second->getToolBar();
    QWidget* newContainer = it->second->getContainerWidget();

    // Plugin has no viewer interface
    if (!newToolbar && !newContainer) {
        return;
    }

    ///remove any existing roto gui
    if (activeNodeForPlugin) {
        removeNodeViewerInterface(activeNodeForPlugin, false /*permanently*/, /*setAnother*/ false);
    }


    ViewerNodePtr internalNode = getInternalNode();
    // Add the widgets

    if (newToolbar) {
        _imp->viewerLayout->insertWidget(0, newToolbar);
        if (internalNode->isLeftToolbarVisible()) {
            newToolbar->show();
        }
    }

    // If there are other interface active, add it after them
    int index;
    if ( _imp->currentNodeContext.empty() ) {
        // insert before the viewer
        index = _imp->mainLayout->indexOf(_imp->viewerContainer);
    } else {
        // Remove the oldest opened interface if we reached the maximum
        int maxNodeContextOpened = appPTR->getCurrentSettings()->getMaxOpenedNodesViewerContext();
        while ( (int)_imp->currentNodeContext.size() >= (maxNodeContextOpened + 1) ) { // + 1 so we allow the Viewer interface to stay
            const ViewerTabPrivate::PluginViewerContext* oldestNodeViewerInterface = 0;
            for (std::list<ViewerTabPrivate::PluginViewerContext>::iterator it = _imp->currentNodeContext.begin(); it != _imp->currentNodeContext.end(); ++it) {
                NodeGuiPtr node = it->currentNode.lock();
                if (!node) {
                    continue;
                }
                // Do not remove the viewer interface
                if (node->getNode()->isEffectViewerNode()) {
                    continue;
                }
                oldestNodeViewerInterface = &(*it);
            }
            if (oldestNodeViewerInterface) {
                removeNodeViewerInterface(oldestNodeViewerInterface->currentNode.lock(), false /*permanently*/, false /*setAnother*/);
            }
        }
        if ( _imp->currentNodeContext.empty() ) {
            // insert before the viewer
            index = _imp->mainLayout->indexOf(_imp->viewerContainer);
        } else {
            QWidget* container = _imp->currentNodeContext.back().currentContext->getContainerWidget();
            index = _imp->mainLayout->indexOf(container);
            
            assert(index != -1);
            if (index >= 0) {
                ++index;
            }
        }
    }
    assert(index >= 0);
    if (index >= 0) {
        assert(newContainer);
        if (newContainer) {
            _imp->mainLayout->insertWidget(index, newContainer);

            if (internalNode->isTopToolbarVisible()) {
                newContainer->show();
            }

        }
    }

    if (n->getNode()->isEffectViewerNode()) {
        // Add the player toolbar
        int viewerIndex = _imp->mainLayout->indexOf(_imp->viewerContainer);
        ++viewerIndex;
        bool playerVisible = internalNode->isPlayerVisible();
        QWidget* playerToolbar = it->second->getPlayerToolbar();
        if (!playerVisible) {
            playerToolbar->hide();
        }
        _imp->mainLayout->insertWidget(viewerIndex, playerToolbar);
    }

    ViewerTabPrivate::PluginViewerContext p;
    p.plugin = n->getNode()->getOriginalPlugin();
    p.pyPlug = n->getNode()->getPyPlugPlugin();
    p.currentNode = n;
    p.currentContext = it->second;
    _imp->currentNodeContext.push_back(p);

    _imp->viewer->redraw();
} // ViewerTab::setPluginViewerInterface

void
ViewerTab::removeViewerInterface(const NodeGuiPtr& n,
                           bool permanently)
{
    if (n != _imp->viewerNode.lock()->getNode()->getNodeGui()) {
        return;
    }
    removeNodeViewerInterfaceInternal(n, permanently, false);
}


void
ViewerTab::removeNodeViewerInterfaceInternal(const NodeGuiPtr& n,
                                       bool permanently,
                                       bool setAnotherFromSamePlugin)
{
    std::map<NodeGuiWPtr, NodeViewerContextPtr>::iterator found = _imp->nodesContext.find(n);

    if ( found == _imp->nodesContext.end() ) {
        return;
    }

    NodePtr internalNode = n->getNode();
    if (!internalNode) {
        return;
    }

    std::string pluginID = internalNode->getPluginID();
    NodeGuiPtr activeNodeForPlugin;
    QToolBar* activeItemToolBar = 0;
    QWidget* activeItemContainer = 0;
    QWidget* activePlayer = 0;
    {
        // Keep the iterator under this scope since we erase it
        std::list<ViewerTabPrivate::PluginViewerContext>::iterator foundActive = _imp->findActiveNodeContextForNode(internalNode);

        if ( foundActive != _imp->currentNodeContext.end() ) {
            activeNodeForPlugin = foundActive->currentNode.lock();

            if (activeNodeForPlugin == n) {
                activeItemToolBar = foundActive->currentContext->getToolBar();
                activeItemContainer = foundActive->currentContext->getContainerWidget();
                activePlayer = foundActive->currentContext->getPlayerToolbar();
                _imp->currentNodeContext.erase(foundActive);
            }
        }
    }

    // Remove the widgets of the current node
    if (activeItemToolBar) {
        int nLayoutItems = _imp->viewerLayout->count();
        for (int i = 0; i < nLayoutItems; ++i) {
            QLayoutItem* item = _imp->viewerLayout->itemAt(i);
            if (item->widget() == activeItemToolBar) {
                activeItemToolBar->hide();
                _imp->viewerLayout->removeItem(item);
                if (permanently) {
                    activeItemToolBar->deleteLater();
                }
                break;
            }
        }
    }
    if (activeItemContainer) {
        int buttonsBarIndex = _imp->mainLayout->indexOf(activeItemContainer);
        assert(buttonsBarIndex >= 0);
        if (buttonsBarIndex >= 0) {
            _imp->mainLayout->removeWidget(activeItemContainer);
        }
        activeItemContainer->hide();
        if (permanently) {
            activeItemContainer->deleteLater();
        }
    }

    if (activePlayer) {
        int playerIndex = _imp->mainLayout->indexOf(activePlayer);
        assert(playerIndex >= 0);
        if (playerIndex >= 0) {
            _imp->mainLayout->removeWidget(activePlayer);
        }
        activePlayer->hide();
        if (permanently) {
            activePlayer->deleteLater();
        }
    }

    if (setAnotherFromSamePlugin && activeNodeForPlugin == n) {
        ///If theres another roto node, set it as the current roto interface
        std::map<NodeGuiWPtr, NodeViewerContextPtr>::iterator newInterface = _imp->nodesContext.end();
        for (std::map<NodeGuiWPtr, NodeViewerContextPtr>::iterator it2 = _imp->nodesContext.begin(); it2 != _imp->nodesContext.end(); ++it2) {
            NodeGuiPtr otherNode = it2->first.lock();
            if (!otherNode) {
                continue;
            }
            if (otherNode == n) {
                continue;
            }
            if (otherNode->getNode()->getPluginID() != pluginID) {
                continue;
            }
            if ( (it2->second != found->second) && it2->first.lock()->isSettingsPanelVisible() ) {
                newInterface = it2;
                break;
            }
        }

        if ( newInterface != _imp->nodesContext.end() ) {
            setPluginViewerInterface( newInterface->first.lock() );
        }
    }

    if (permanently) {
        found->second.reset();
        _imp->nodesContext.erase(found);
    }

} // ViewerTab::removeNodeViewerInterfaceInternal

/**
 * @brief Removes the interface associated to the given node.
 * @param permanently The interface is destroyed instead of being hidden
 * @param setAnotherFromSamePlugin If true, if another node of the same plug-in is a candidate for a viewer interface, it will replace the existing
 * viewer interface for this plug-in
 **/
void
ViewerTab::removeNodeViewerInterface(const NodeGuiPtr& n,
                                     bool permanently,
                                     bool setAnotherFromSamePlugin)
{
    NodePtr node = n->getNode();
    if (node && node->isEffectViewerNode()) {
        return;
    }
    removeNodeViewerInterfaceInternal(n, permanently, setAnotherFromSamePlugin);
    
} // ViewerTab::removeNodeViewerInterface

/**
 * @brief Get the list of all nodes that have a user interface created on this viewer (but not necessarily displayed)
 * and a list for each plug-in of the active node.
 **/
void
ViewerTab::getNodesViewerInterface(std::list<NodeGuiPtr>* nodesWithUI,
                                   std::list<NodeGuiPtr>* perPluginActiveUI) const
{
    for (std::map<NodeGuiWPtr, NodeViewerContextPtr>::const_iterator it = _imp->nodesContext.begin(); it != _imp->nodesContext.end(); ++it) {
        NodeGuiPtr n = it->first.lock();
        if (n) {
            nodesWithUI->push_back(n);
        }
    }
    for (std::list<ViewerTabPrivate::PluginViewerContext>::const_iterator it = _imp->currentNodeContext.begin(); it != _imp->currentNodeContext.end(); ++it) {
        NodeGuiPtr n = it->currentNode.lock();
        if (n) {
            perPluginActiveUI->push_back(n);
        }
    }
}

void
ViewerTab::updateSelectedToolForNode(const QString& toolID,
                                     const NodeGuiPtr& node)
{
    std::map<NodeGuiWPtr, NodeViewerContextPtr>::iterator found = _imp->nodesContext.find(node);

    if ( found == _imp->nodesContext.end() ) {
        // Already exists
        return;
    }
    found->second->setCurrentTool(toolID, false /*notifyNode*/);
}

void
ViewerTab::notifyGuiClosing()
{
    _imp->timeLineGui->discardGuiPointer();
    for (std::map<NodeGuiWPtr, NodeViewerContextPtr>::iterator it = _imp->nodesContext.begin(); it != _imp->nodesContext.end(); ++it) {
        it->second->notifyGuiClosing();
    }
}




NATRON_NAMESPACE_EXIT
