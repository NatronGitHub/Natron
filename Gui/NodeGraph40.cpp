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

#include "NodeGraph.h"
#include "NodeGraphPrivate.h"

#include <sstream> // stringstream
#include <stdexcept>

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QtCore/QThread>
#include <QFile>
#include <QUrl>
#include <QtCore/QMimeData>

#include "Engine/Node.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Image.h"
#include "Engine/NodeGroup.h"
#include "Engine/RotoLayer.h"
#include "Engine/Project.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Settings.h"

#include "Gui/BackdropGui.h"
#include "Gui/Edge.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/ScriptEditor.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/NodeGui.h"

#include "Serialization/NodeSerialization.h"
#include "Serialization/NodeClipBoard.h"
#include "Serialization/ProjectSerialization.h"
#include "Serialization/SerializationIO.h"

#include "Global/QtCompat.h"

NATRON_NAMESPACE_ENTER
void
NodeGraph::togglePreviewsForSelectedNodes()
{
    bool empty = false;
    {
        QMutexLocker l(&_imp->_nodesMutex);
        empty = _imp->_selection.empty();
        for (NodesGuiWList::iterator it = _imp->_selection.begin();
             it != _imp->_selection.end();
             ++it) {
            NodeGuiPtr n = it->lock();
            if (n) {
                n->togglePreview();
            }
        }
    }

    if (empty) {
        Dialogs::warningDialog( tr("Toggle Preview").toStdString(), tr("You must select a node first").toStdString() );
    }
}

void
NodeGraph::showSelectedNodeSettingsPanel()
{
    if (_imp->_selection.size() == 1) {
        showNodePanel(false, false, _imp->_selection.front().lock());
    }
}

void
NodeGraph::switchInputs1and2ForSelectedNodes()
{
    QMutexLocker l(&_imp->_nodesMutex);

    for (NodesGuiWList::iterator it = _imp->_selection.begin();
         it != _imp->_selection.end();
         ++it) {
        NodeGuiPtr n = it->lock();
        if (n) {
            n->onSwitchInputActionTriggered();
        }
    }
}

void
NodeGraph::centerOnItem(QGraphicsItem* item)
{
    _imp->_refreshOverlays = true;
    centerOn(item);
}

void
NodeGraph::copyNodes(const NodesGuiList& nodes,
                     SERIALIZATION_NAMESPACE::NodeClipBoard& clipboard)
{
    _imp->copyNodesInternal(nodes, &clipboard.nodes);
}

void
NodeGraph::copySelectedNodes()
{
    if ( _imp->_selection.empty() ) {
        Dialogs::warningDialog( tr("Copy").toStdString(), tr("You must select at least a node to copy first.").toStdString() );

        return;
    }

    SERIALIZATION_NAMESPACE::NodeClipBoard cb;
    _imp->copyNodesInternal(getSelectedNodes(), &cb.nodes);

    std::ostringstream ss;

    try {
        SERIALIZATION_NAMESPACE::write(ss, cb, NATRON_CLIPBOARD_HEADER);
    } catch (...) {
        qDebug() << "Failed to copy selection to system clipboard";
    }

    QMimeData* mimedata = new QMimeData;
    QByteArray data( ss.str().c_str() );
    mimedata->setData(QLatin1String("text/plain"), data);
    QClipboard* clipboard = QApplication::clipboard();

    //ownership is transferred to the clipboard
    clipboard->setMimeData(mimedata);
}

void
NodeGraph::cutSelectedNodes()
{
    if ( _imp->_selection.empty() ) {
        Dialogs::warningDialog( tr("Cut").toStdString(), tr("You must select at least a node to cut first.").toStdString() );

        return;
    }
    copySelectedNodes();
    deleteSelection();
}


bool
NodeGraph::tryReadClipboard(const QPointF& pos, std::istream& ss)
{

    
    // Check if this is a regular clipboard
    // This will also check if this is a single node
    try {
        SERIALIZATION_NAMESPACE::NodeClipBoard cb;
        SERIALIZATION_NAMESPACE::read(std::string(), ss, &cb);

        std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > > nodesToPaste;
        for (SERIALIZATION_NAMESPACE::NodeSerializationList::const_iterator it = cb.nodes.begin(); it != cb.nodes.end(); ++it) {
            nodesToPaste.push_back(std::make_pair(NodePtr(), *it));
        }
        _imp->pasteNodesInternal(nodesToPaste, pos, NodeGraphPrivate::PasteNodesFlags(NodeGraphPrivate::ePasteNodesFlagRelativeToCentroid | NodeGraphPrivate::ePasteNodesFlagUseUndoCommand));
    } catch (...) {
        
        // Check if this was copy/pasted from a project directly
        try {
            ss.seekg(0);
            SERIALIZATION_NAMESPACE::ProjectSerialization isProject;
            SERIALIZATION_NAMESPACE::read(std::string(), ss, &isProject);

            std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > > nodesToPaste;
            for (SERIALIZATION_NAMESPACE::NodeSerializationList::const_iterator it = isProject._nodes.begin(); it != isProject._nodes.end(); ++it) {
                nodesToPaste.push_back(std::make_pair(NodePtr(), *it));
            }

            _imp->pasteNodesInternal(nodesToPaste, pos, NodeGraphPrivate::PasteNodesFlags(NodeGraphPrivate::ePasteNodesFlagRelativeToCentroid | NodeGraphPrivate::ePasteNodesFlagUseUndoCommand));
        } catch (...) {
            
            return false;
        }
    }
    

    return true;

}

bool
NodeGraph::pasteClipboard(const QPointF& pos)
{
    QPointF position;
    if (pos.x() == INT_MIN || pos.y() == INT_MIN) {
        position =  mapToScene( mapFromGlobal( QCursor::pos() ) );
    } else {
        position = pos;
    }
    position = _imp->_root->mapFromScene(position);

    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimedata = clipboard->mimeData();

    // If this is a list of files, try to open them
    if ( mimedata->hasUrls() ) {
        QList<QUrl> urls = mimedata->urls();
        getGui()->handleOpenFilesFromUrls( urls, QCursor::pos() );
        return true;
    }

    if ( !mimedata->hasFormat( QLatin1String("text/plain") ) ) {
        return false;
    }
    QByteArray data = mimedata->data( QLatin1String("text/plain") );
    std::string s = QString::fromUtf8(data).toStdString();
    std::istringstream ss(s);
    return tryReadClipboard(position, ss);
    
}


void
NodeGraph::duplicateSelectedNodes(const QPointF& pos)
{
    if ( _imp->_selection.empty() && _imp->_selection.empty() ) {
        Dialogs::warningDialog( tr("Duplicate").toStdString(), tr("You must select at least a node to duplicate first.").toStdString() );

        return;
    }

    std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > > nodesToPaste;
    _imp->copyNodesInternal(getSelectedNodes(), &nodesToPaste);


    _imp->pasteNodesInternal(nodesToPaste, pos, NodeGraphPrivate::PasteNodesFlags(NodeGraphPrivate::ePasteNodesFlagRelativeToCentroid | NodeGraphPrivate::ePasteNodesFlagUseUndoCommand));
}

void
NodeGraph::duplicateSelectedNodes()
{
    QPointF scenePos = mapToScene( mapFromGlobal( QCursor::pos() ) );

    duplicateSelectedNodes(scenePos);
}

void
NodeGraph::cloneSelectedNodes(const QPointF& pos)
{
    if ( _imp->_selection.empty() ) {
        Dialogs::warningDialog( tr("Clone").toStdString(), tr("You must select at least a node to clone first.").toStdString() );

        return;
    }
    std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr > > nodesToPaste;
    _imp->copyNodesInternal(getSelectedNodes(), &nodesToPaste);

    _imp->pasteNodesInternal(nodesToPaste, pos, NodeGraphPrivate::PasteNodesFlags(NodeGraphPrivate::ePasteNodesFlagCloneNodes | NodeGraphPrivate::ePasteNodesFlagRelativeToCentroid | NodeGraphPrivate::ePasteNodesFlagUseUndoCommand));
} // NodeGraph::cloneSelectedNodes

void
NodeGraph::cloneSelectedNodes()
{
    QPointF scenePos = mapToScene( mapFromGlobal( QCursor::pos() ) );

    cloneSelectedNodes(scenePos);
} // cloneSelectedNodes

void
NodeGraph::decloneSelectedNodes()
{
    if ( _imp->_selection.empty() ) {
        Dialogs::warningDialog( tr("Declone").toStdString(), tr("You must select at least a node to declone first.").toStdString() );

        return;
    }
    std::map<NodeGuiPtr, NodePtr> nodesToDeclone;


    for (NodesGuiWList::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
        NodeGuiPtr n = it->lock();
        if (!n) {
            continue;
        }
        BackdropGuiPtr isBd = toBackdropGui(n);
        if (isBd) {
            // Also clone all nodes within the backdrop
            NodesGuiList nodesWithinBD = getNodesWithinBackdrop(n);
            for (NodesGuiList::iterator it2 = nodesWithinBD.begin(); it2 != nodesWithinBD.end(); ++it2) {
                std::map<NodeGuiPtr, NodePtr>::iterator found = nodesToDeclone.find(*it2);
                if ( found == nodesToDeclone.end() ) {
                    std::list<NodePtr> linkedNodes;
                    (*it2)->getNode()->getCloneLinkedNodes(&linkedNodes);
                    if (!linkedNodes.empty()) {
                        nodesToDeclone[*it2] = linkedNodes.front();
                    }

                }
            }
        }
        std::list<NodePtr> linkedNodes;
        n->getNode()->getCloneLinkedNodes(&linkedNodes);
        if (!linkedNodes.empty()) {
            nodesToDeclone[n] = linkedNodes.front();
        }
    }

    pushUndoCommand( new DecloneMultipleNodesCommand(this, nodesToDeclone) );
}

void
NodeGraph::setUndoRedoStackLimit(int limit)
{
    _imp->_undoStack->clear();
    _imp->_undoStack->setUndoLimit(limit);
}

void
NodeGraph::deleteNodePermanantly(const NodeGuiPtr& n)
{
    assert(n);
   
    {
        QMutexLocker l(&_imp->_nodesMutex);
        NodesGuiList::iterator it = std::find(_imp->_nodes.begin(), _imp->_nodes.end(), n);
        if ( it != _imp->_nodes.end() ) {
            _imp->_nodes.erase(it);
        }
    }

    NodesGuiWList::iterator found = _imp->findSelectedNode(n);
    if ( found != _imp->_selection.end() ) {
        n->setUserSelected(false);
        _imp->_selection.erase(found);
    }
} // deleteNodePermanantly

void
NodeGraph::invalidateAllNodesParenting()
{
    for (NodesGuiList::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        (*it)->setParentItem(NULL);
        if ( (*it)->scene() ) {
            (*it)->scene()->removeItem((*it).get());
        }
    }

}

void
NodeGraph::centerOnAllNodes()
{
    assert( QThread::currentThread() == qApp->thread() );
    double xmin = INT_MAX;
    double xmax = INT_MIN;
    double ymin = INT_MAX;
    double ymax = INT_MIN;
    //_imp->_root->setPos(0,0);

    if ( _imp->_selection.empty() ) {
        QMutexLocker l(&_imp->_nodesMutex);

        for (NodesGuiList::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            if ( /*(*it)->isActive() &&*/ (*it)->isVisible() ) {
                QSize size = (*it)->getSize();
                QPointF pos = (*it)->mapToScene( (*it)->mapFromParent( (*it)->pos() ) );
                xmin = std::min( xmin, pos.x() );
                xmax = std::max( xmax, pos.x() + size.width() );
                ymin = std::min( ymin, pos.y() );
                ymax = std::max( ymax, pos.y() + size.height() );
            }
        }
    } else {
        for (NodesGuiWList::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
            NodeGuiPtr n =it->lock();
            if ( n && n->isVisible() ) {
                QSize size = n->getSize();
                QPointF pos = n->mapToScene( n->mapFromParent( n->pos() ) );
                xmin = std::min( xmin, pos.x() );
                xmax = std::max( xmax, pos.x() + size.width() );
                ymin = std::min( ymin, pos.y() );
                ymax = std::max( ymax, pos.y() + size.height() );
            }
        }
    }
    QRectF bbox( xmin, ymin, (xmax - xmin), (ymax - ymin) );
    fitInView(bbox, Qt::KeepAspectRatio);

    double currentZoomFactor = transform().mapRect( QRectF(0, 0, 1, 1) ).width();
    assert(currentZoomFactor != 0);
    //we want to fit at scale 1 at most
    if (currentZoomFactor > 1.) {
        double scaleFactor = 1. / currentZoomFactor;
        setTransformationAnchor(QGraphicsView::AnchorViewCenter);
        scale(scaleFactor, scaleFactor);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    }

    _imp->_refreshOverlays = true;
    update();
} // NodeGraph::centerOnAllNodes


void
NodeGraph::onMustRefreshNodeLinksLaterReceived()
{
    if (_imp->refreshNodesLinkRequest == 0) {
        return;
    }
    _imp->refreshNodesLinkRequest = 0;
    refreshNodeLinksNow();
}

void
NodeGraph::refreshNodeLinksLater()
{
    ++_imp->refreshNodesLinkRequest;
    Q_EMIT mustRefreshNodeLinksLater();
}

bool
NodeGraph::isNodeCloneLinked(const NodePtr& node)
{
    for (NodeGraphPrivate::LinkedNodesList::const_iterator it = _imp->linkedNodes.begin(); it!=_imp->linkedNodes.end(); ++it) {
        if (it->isCloneLink && (it->nodes[0].lock() == node || it->nodes[1].lock() == node)) {
            return true;
        }
    }
    return false;
}

void
NodeGraph::refreshNodeLinksNow()
{
    // First clear all previous links
    for (NodeGraphPrivate::LinkedNodesList::const_iterator it = _imp->linkedNodes.begin(); it != _imp->linkedNodes.end(); ++it) {
        delete it->arrow;
    }
    _imp->linkedNodes.clear();

    if (!_imp->_knobLinksVisible) {
        return;
    }

    NodesGuiList nodes;
    {
        QMutexLocker k(&_imp->_nodesMutex);
        nodes = _imp->_nodes;
    }

    QColor simpleLinkColor, cloneLinkColor;
    {
        double exprColor[3], cloneColor[3];
        appPTR->getCurrentSettings()->getExprColor(&exprColor[0], &exprColor[1], &exprColor[2]);
        appPTR->getCurrentSettings()->getCloneColor(&cloneColor[0], &cloneColor[1], &cloneColor[2]);
        simpleLinkColor.setRgbF(Image::clamp(exprColor[0], 0., 1.),
                                Image::clamp(exprColor[1], 0., 1.),
                                Image::clamp(exprColor[2], 0., 1.));
        cloneLinkColor.setRgbF(Image::clamp(cloneColor[0], 0., 1.),
                                Image::clamp(cloneColor[1], 0., 1.),
                                Image::clamp(cloneColor[2], 0., 1.));
    }
    
    
    for (NodesGuiList::const_iterator it = nodes.begin(); it!=nodes.end(); ++it) {
        NodePtr thisNode = (*it)->getNode();

        std::list<std::pair<NodePtr, bool> > linkedNodes;
        thisNode->getLinkedNodes(&linkedNodes);

        for (std::list<std::pair<NodePtr, bool> >::const_iterator it2 = linkedNodes.begin(); it2 != linkedNodes.end(); ++it2) {

            // If the linked node doesn't have a gui or it is not part of this nodegraph don't create a link at all
            NodeGuiPtr otherNodeGui = toNodeGui(it2->first->getNodeGui());
            if (!otherNodeGui || otherNodeGui->getDagGui() != this) {
                continue;
            }

            // Try to find an existing link
            NodeGraphPrivate::LinkedNodes link;
            link.nodes[0] = thisNode;
            link.nodes[1] = it2->first;
            link.isCloneLink = it2->second;

            NodeGraphPrivate::LinkedNodesList::iterator foundExistingLink = _imp->linkedNodes.end();
            {
                for (NodeGraphPrivate::LinkedNodesList::iterator it = _imp->linkedNodes.begin(); it != _imp->linkedNodes.end(); ++it) {
                    NodePtr a1 = it->nodes[0].lock();
                    NodePtr a2 = it->nodes[1].lock();
                    NodePtr b1 = link.nodes[0].lock();
                    NodePtr b2 = link.nodes[1].lock();
                    if ((a1 == b1 || a1 == b2) &&
                        (a2 == b1 || a2 == b2)) {
                        foundExistingLink = it;
                        break;
                    }
                }
            }

            if (foundExistingLink == _imp->linkedNodes.end()) {
                // A link did not exist, create it
                link.arrow = new LinkArrow( otherNodeGui, *it, (*it)->parentItem() );
                link.arrow->setWidth(2);
                link.arrow->setArrowHeadVisible(false);
                if (link.isCloneLink) {
                    link.arrow->setColor(cloneLinkColor);
                } else {
                    link.arrow->setColor(simpleLinkColor);
                }
                _imp->linkedNodes.push_back(link);
            } else {
                // A link existed, if this is a clone link and it was not before, upgrade it
                if (!foundExistingLink->isCloneLink && link.isCloneLink) {
                    foundExistingLink->arrow->setColor(cloneLinkColor);
                }
            }

        }

        (*it)->refreshLinkIndicators(linkedNodes);
    }

} // refreshNodeLinksNow


NATRON_NAMESPACE_EXIT
