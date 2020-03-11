/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"
#include "Engine/RotoLayer.h"

#include "Gui/Edge.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h" // appPTR
#include "Gui/NodeGui.h"
#include "Gui/NodeGraph.h"

#include "Serialization/NodeSerialization.h"
#include "Serialization/NodeClipBoard.h"

NATRON_NAMESPACE_ENTER


NodeGraphPrivate::NodeGraphPrivate(NodeGraph* p,
                                   const NodeCollectionPtr& group)
    : _publicInterface(p)
    , group(group)
    , _lastMousePos()
    , _lastSelectionStartPointScene()
    , _evtState(eEventStateNone)
    , _magnifiedNode()
    , _nodeSelectedScaleBeforeMagnif(1.)
    , _magnifOn(false)
    , _arrowSelected(NULL)
    , _nodesMutex()
    , _nodes()
    , _nodeCreationShortcutEnabled(false)
    , _lastPluginCreatedID()
    , _root(NULL)
    , _nodeRoot(NULL)
    , _cacheSizeText(NULL)
    , cacheSizeHidden(true)
    , _refreshCacheTextTimer()
    , _navigator(NULL)
    , _undoStack()
    , _menu(NULL)
    , _tL(NULL)
    , _tR(NULL)
    , _bR(NULL)
    , _bL(NULL)
    , _refreshOverlays(false)
    , _highLightedEdge(NULL)
    , _mergeHintNode()
    , _hintInputEdge(NULL)
    , _hintOutputEdge(NULL)
    , _backdropResized()
    , _selection()
    , cursorSet(false)
    , _nodesWithinBDAtPenDown()
    , _selectionRect()
    , _bendPointsVisible(false)
    , _knobLinksVisible(true)
    , _accumDelta(0)
    , _detailsVisible(false)
    , _deltaSinceMousePress(0, 0)
    , _hasMovedOnce(false)
    , lastSelectedViewer(0)
    , isDoingPreviewRender(false)
    , autoScrollTimer()
    , linkedNodes()
    , refreshNodesLinkRequest(0)
{
    appPTR->getIcon(NATRON_PIXMAP_LOCKED, &unlockIcon);
}

QPoint
NodeGraphPrivate::getPyPlugUnlockPos() const
{
    return QPoint(_publicInterface->width() - unlockIcon.width() - 10,   10);
}

void
NodeGraphPrivate::resetSelection()
{
    for (NodesGuiWList::iterator it = _selection.begin(); it != _selection.end(); ++it) {
        NodeGuiPtr n = it->lock();
        if (n) {
            n->setUserSelected(false);
        }
    }

    _selection.clear();
}

void
NodeGraphPrivate::editSelectionFromSelectionRectangle(bool addToSelection)
{
    if (!addToSelection) {
        resetSelection();
    }

    const QRectF& selection = _selectionRect;

    for (NodesGuiList::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        QRectF bbox = (*it)->mapToScene( (*it)->boundingRect() ).boundingRect();
        if ( selection.contains(bbox) ) {
            NodesGuiWList::iterator foundInSel = findSelectedNode(*it);
            if ( foundInSel != _selection.end() ) {
                continue;
            }

            _selection.push_back(*it);
            (*it)->setUserSelected(true);
        }
    }
}

bool
NodeGraphPrivate::rearrangeSelectedNodes()
{
    if ( !_selection.empty() ) {
        _publicInterface->pushUndoCommand( new RearrangeNodesCommand(_publicInterface->getSelectedNodes()) );

        return true;
    }

    return false;
}

void
NodeGraphPrivate::setNodesBendPointsVisible(bool visible)
{
    _bendPointsVisible = visible;

    for (NodesGuiList::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        const std::vector<Edge*> & edges = (*it)->getInputsArrows();
        for (std::vector<Edge*>::const_iterator it2 = edges.begin(); it2 != edges.end(); ++it2) {
            if (visible) {
                if ( !(*it2)->isOutputEdge() && (*it2)->hasSource() && ( (*it2)->line().length() > 50 ) ) {
                    (*it2)->setBendPointVisible(visible);
                }
            } else {
                if ( (*it2) && !(*it2)->isOutputEdge() ) {
                    (*it2)->setBendPointVisible(visible);
                }
            }
        }
    }
}

QRectF
NodeGraphPrivate::calcNodesBoundingRect()
{
    QRectF ret;
    QMutexLocker l(&_nodesMutex);

    for (NodesGuiList::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        if ( (*it)->isVisible() ) {
            ret = ret.united( (*it)->boundingRectWithEdges() );
        }
    }

    return ret;
}


void
NodeGraphPrivate::copyNodesInternal(const NodesGuiList& selection,
                                   SERIALIZATION_NAMESPACE::NodeSerializationList* clipboard)
{
    clipboard->clear();
    std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr> > tmp;
    copyNodesInternal(selection, &tmp);
    for (std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr> >::const_iterator it = tmp.begin(); it!=tmp.end(); ++it) {
        clipboard->push_back(it->second);
    }
}

void
NodeGraphPrivate::copyNodesInternal(const NodesGuiList& selection, std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr> >* clipboard)
{
    // Clear the list
    clipboard->clear();

    NodesGuiList nodesToCopy = selection;
    for (NodesGuiList::iterator it = nodesToCopy.begin(); it != nodesToCopy.end(); ++it) {
        // Also copy all nodes within the backdrop
        NodesGuiList nodesWithinBD = _publicInterface->getNodesWithinBackdrop(*it);
        for (NodesGuiList::iterator it2 = nodesWithinBD.begin(); it2 != nodesWithinBD.end(); ++it2) {
            NodesGuiList::iterator found = std::find(nodesToCopy.begin(), nodesToCopy.end(), *it2);
            if ( found == nodesToCopy.end() ) {
                nodesToCopy.push_back(*it2);
            }
        }
    }

    for (NodesGuiList::iterator it = nodesToCopy.begin(); it != nodesToCopy.end(); ++it) {
        if ( (*it)->isVisible() ) {
            NodePtr internalNode = (*it)->getNode();
            SERIALIZATION_NAMESPACE::NodeSerializationPtr ns = boost::make_shared<SERIALIZATION_NAMESPACE::NodeSerialization>();
            internalNode->toSerialization(ns.get());
            clipboard->push_back(std::make_pair(internalNode, ns));
        }
    }
}

NATRON_NAMESPACE_EXIT
