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

#include <sstream>
#include <stdexcept>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
// /usr/local/include/boost/serialization/shared_ptr.hpp:112:5: warning: unused typedef 'boost_static_assert_typedef_112' [-Wunused-local-typedef]
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#endif

#include <QApplication>
#include <QClipboard>
#include <QMimeData>

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeSerialization.h"
#include "Engine/RotoLayer.h"
#include "Engine/ViewerInstance.h"

#include "Gui/BackdropGui.h"
#include "Gui/CurveEditor.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/NodeClipBoard.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeGuiSerialization.h"

#include "Global/QtCompat.h"

NATRON_NAMESPACE_ENTER;


void
NodeGraph::togglePreviewsForSelectedNodes()
{
    bool empty = false;
    {
        QMutexLocker l(&_imp->_nodesMutex);
        empty = _imp->_selection.empty();
        for (NodesGuiList::iterator it = _imp->_selection.begin();
             it != _imp->_selection.end();
             ++it) {
            (*it)->togglePreview();
        }
    }
    if (empty) {
        Dialogs::warningDialog(tr("Toggle Preview").toStdString(), tr("You must select a node first").toStdString());
    }
}

void
NodeGraph::switchInputs1and2ForSelectedNodes()
{
    QMutexLocker l(&_imp->_nodesMutex);

    for (NodesGuiList::iterator it = _imp->_selection.begin();
         it != _imp->_selection.end();
         ++it) {
        (*it)->onSwitchInputActionTriggered();
    }
}

void
NodeGraph::centerOnItem(QGraphicsItem* item)
{
    _imp->_refreshOverlays = true;
    centerOn(item);
}

void
NodeGraph::copyNodes(const NodesGuiList& nodes,NodeClipBoard& clipboard)
{
    _imp->copyNodesInternal(nodes, clipboard);
}

void
NodeGraph::copySelectedNodes()
{
    if ( _imp->_selection.empty()) {
        Dialogs::warningDialog( tr("Copy").toStdString(), tr("You must select at least a node to copy first.").toStdString() );

        return;
    }

    NodeClipBoard& cb = appPTR->getNodeClipBoard();
    _imp->copyNodesInternal(_imp->_selection,cb);
    
    std::ostringstream ss;
    try {
        boost::archive::xml_oarchive oArchive(ss);
        oArchive << boost::serialization::make_nvp("Clipboard",cb);
    } catch (...) {
        qDebug() << "Failed to copy selection to system clipboard";
    }
    QMimeData* mimedata = new QMimeData;
    QByteArray data(ss.str().c_str());
    mimedata->setData("text/natron-nodes", data);
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

void
NodeGraph::pasteCliboard(const NodeClipBoard& clipboard,std::list<std::pair<std::string,NodeGuiPtr > >* newNodes)
{
    QPointF position = _imp->_root->mapFromScene(mapToScene(mapFromGlobal(QCursor::pos())));
    _imp->pasteNodesInternal(clipboard,position, false, newNodes);
}

void
NodeGraph::pasteNodeClipBoards(const QPointF& pos)
{
    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimedata = clipboard->mimeData();
    if (!mimedata->hasFormat("text/natron-nodes")) {
        return;
    }
    QByteArray data = mimedata->data("text/natron-nodes");
    
    std::list<std::pair<std::string,NodeGuiPtr > > newNodes;
    
    
    
    NodeClipBoard& cb = appPTR->getNodeClipBoard();
    
    std::string s = QString(data).toStdString();
    try {
        std::stringstream ss(s);
        boost::archive::xml_iarchive iArchive(ss);
        iArchive >> boost::serialization::make_nvp("Clipboard",cb);
    } catch (...) {
        qDebug() << "Failed to load clipboard";
        return;
    }
    _imp->pasteNodesInternal(cb,pos, true, &newNodes);
}

void
NodeGraph::pasteNodeClipBoards()
{
    QPointF position = _imp->_root->mapFromScene(mapToScene(mapFromGlobal(QCursor::pos())));
    pasteNodeClipBoards(position);
}


void
NodeGraph::duplicateSelectedNodes(const QPointF& pos)
{
    if ( _imp->_selection.empty() && _imp->_selection.empty() ) {
        Dialogs::warningDialog( tr("Duplicate").toStdString(), tr("You must select at least a node to duplicate first.").toStdString() );
        
        return;
    }
    
    ///Don't use the member clipboard as the user might have something copied
    NodeClipBoard tmpClipboard;
    _imp->copyNodesInternal(_imp->_selection,tmpClipboard);
    std::list<std::pair<std::string,NodeGuiPtr > > newNodes;
    _imp->pasteNodesInternal(tmpClipboard,pos,true,&newNodes);

}


void
NodeGraph::duplicateSelectedNodes()
{
    QPointF scenePos = _imp->_root->mapFromScene(mapToScene(mapFromGlobal(QCursor::pos())));
    duplicateSelectedNodes(scenePos);
}

void
NodeGraph::cloneSelectedNodes(const QPointF& scenePos)
{
    if (_imp->_selection.empty()) {
        Dialogs::warningDialog( tr("Clone").toStdString(), tr("You must select at least a node to clone first.").toStdString() );
        return;
    }
    
    double xmax = INT_MIN;
    double xmin = INT_MAX;
    double ymin = INT_MAX;
    double ymax = INT_MIN;
    NodesGuiList nodesToCopy = _imp->_selection;
    for (NodesGuiList::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
        if ( (*it)->getNode()->getMasterNode()) {
            Dialogs::errorDialog( tr("Clone").toStdString(), tr("You cannot clone a node which is already a clone.").toStdString() );
            return;
        }
        QRectF bbox = (*it)->mapToScene((*it)->boundingRect()).boundingRect();
        if ( ( bbox.x() + bbox.width() ) > xmax ) {
            xmax = ( bbox.x() + bbox.width() );
        }
        if (bbox.x() < xmin) {
            xmin = bbox.x();
        }
        
        if ( ( bbox.y() + bbox.height() ) > ymax ) {
            ymax = ( bbox.y() + bbox.height() );
        }
        if (bbox.y() < ymin) {
            ymin = bbox.y();
        }
        
        ///Also copy all nodes within the backdrop
        BackdropGui* isBd = dynamic_cast<BackdropGui*>(it->get());
        if (isBd) {
            NodesGuiList nodesWithinBD = getNodesWithinBackdrop(*it);
            for (NodesGuiList::iterator it2 = nodesWithinBD.begin(); it2 != nodesWithinBD.end(); ++it2) {
                NodesGuiList::iterator found = std::find(nodesToCopy.begin(),nodesToCopy.end(),*it2);
                if ( found == nodesToCopy.end() ) {
                    nodesToCopy.push_back(*it2);
                }
            }
        }
    }
    
    for (NodesGuiList::iterator it = nodesToCopy.begin(); it != nodesToCopy.end(); ++it) {
        if ( (*it)->getNode()->getEffectInstance()->isSlave() ) {
            Dialogs::errorDialog( tr("Clone").toStdString(), tr("You cannot clone a node which is already a clone.").toStdString() );
            
            return;
        }
        ViewerInstance* isViewer = (*it)->getNode()->isEffectViewer();
        if (isViewer) {
            Dialogs::errorDialog( tr("Clone").toStdString(), tr("Cloning a viewer is not a valid operation.").toStdString() );
            
            return;
        }
        if ( (*it)->getNode()->isMultiInstance() ) {
            QString err = QString("%1 cannot be cloned.").arg( (*it)->getNode()->getLabel().c_str() );
            Dialogs::errorDialog( tr("Clone").toStdString(),
                                tr( err.toStdString().c_str() ).toStdString() );
            
            return;
        }
    }
    
    QPointF offset(scenePos.x() - ((xmax + xmin) / 2.), scenePos.y() -  ((ymax + ymin) / 2.));
    std::list<std::pair<std::string,NodeGuiPtr > > newNodes;
    std::list <boost::shared_ptr<NodeSerialization> > serializations;
    
    std::list <NodeGuiPtr > newNodesList;
    
    std::map<std::string,std::string> oldNewScriptNameMapping;
    for (NodesGuiList::iterator it = nodesToCopy.begin(); it != nodesToCopy.end(); ++it) {
        boost::shared_ptr<NodeSerialization>  internalSerialization( new NodeSerialization( (*it)->getNode() ) );
        boost::shared_ptr<NodeGuiSerialization> guiSerialization(new NodeGuiSerialization);
        (*it)->serialize(guiSerialization.get());
        NodeGuiPtr clone = _imp->pasteNode(internalSerialization, guiSerialization, offset,
                                                           _imp->group.lock(),std::string(),true, &oldNewScriptNameMapping );
        
        newNodes.push_back(std::make_pair(internalSerialization->getNodeScriptName(),clone));
        newNodesList.push_back(clone);
        serializations.push_back(internalSerialization);
        
        oldNewScriptNameMapping[internalSerialization->getNodeScriptName()] = clone->getNode()->getScriptName();
        
    }
    
    
    assert( serializations.size() == newNodes.size() );
    ///restore connections
    _imp->restoreConnections(serializations, newNodes, oldNewScriptNameMapping);
    
    
    pushUndoCommand( new AddMultipleNodesCommand(this,newNodesList) );
}

void
NodeGraph::cloneSelectedNodes()
{
    QPointF scenePos = _imp->_root->mapFromScene(mapToScene(mapFromGlobal(QCursor::pos())));
    cloneSelectedNodes(scenePos);
    
} // cloneSelectedNodes

void
NodeGraph::decloneSelectedNodes()
{
    if ( _imp->_selection.empty() ) {
        Dialogs::warningDialog( tr("Declone").toStdString(), tr("You must select at least a node to declone first.").toStdString() );

        return;
    }
    NodesGuiList nodesToDeclone;


    for (NodesGuiList::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
        
        BackdropGui* isBd = dynamic_cast<BackdropGui*>(it->get());
        if (isBd) {
            ///Also copy all nodes within the backdrop
            NodesGuiList nodesWithinBD = getNodesWithinBackdrop(*it);
            for (NodesGuiList::iterator it2 = nodesWithinBD.begin(); it2 != nodesWithinBD.end(); ++it2) {
                NodesGuiList::iterator found = std::find(nodesToDeclone.begin(),nodesToDeclone.end(),*it2);
                if ( found == nodesToDeclone.end() ) {
                    nodesToDeclone.push_back(*it2);
                }
            }
        }
        if ( (*it)->getNode()->getEffectInstance()->isSlave() ) {
            nodesToDeclone.push_back(*it);
        }
    }

    pushUndoCommand( new DecloneMultipleNodesCommand(this,nodesToDeclone) );
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
    NodesGuiList::iterator it = std::find(_imp->_nodesTrash.begin(),_imp->_nodesTrash.end(),n);

    if ( it != _imp->_nodesTrash.end() ) {
        _imp->_nodesTrash.erase(it);
    }

    {
        QMutexLocker l(&_imp->_nodesMutex);
        NodesGuiList::iterator it = std::find(_imp->_nodes.begin(),_imp->_nodes.end(),n);
        if ( it != _imp->_nodes.end() ) {
            _imp->_nodes.erase(it);
        }
    }
    
    NodesGuiList::iterator found = std::find(_imp->_selection.begin(),_imp->_selection.end(),n);
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
            (*it)->scene()->removeItem( it->get() );
        }
    }
    for (NodesGuiList::iterator it = _imp->_nodesTrash.begin(); it != _imp->_nodesTrash.end(); ++it) {
        (*it)->setParentItem(NULL);
        if ( (*it)->scene() ) {
            (*it)->scene()->removeItem( it->get() );
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

    if (_imp->_selection.empty()) {
        QMutexLocker l(&_imp->_nodesMutex);

        for (NodesGuiList::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            if ( /*(*it)->isActive() &&*/ (*it)->isVisible() ) {
                QSize size = (*it)->getSize();
                QPointF pos = (*it)->mapToScene((*it)->mapFromParent((*it)->getPos_mt_safe()));
                xmin = std::min( xmin, pos.x() );
                xmax = std::max( xmax,pos.x() + size.width() );
                ymin = std::min( ymin,pos.y() );
                ymax = std::max( ymax,pos.y() + size.height() );
            }
        }
        
    } else {
        for (NodesGuiList::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
            if ( /*(*it)->isActive() && */(*it)->isVisible() ) {
                QSize size = (*it)->getSize();
                QPointF pos = (*it)->mapToScene((*it)->mapFromParent((*it)->getPos_mt_safe()));
                xmin = std::min( xmin, pos.x() );
                xmax = std::max( xmax,pos.x() + size.width() );
                ymin = std::min( ymin,pos.y() );
                ymax = std::max( ymax,pos.y() + size.height() ); 
            }
        }

    }
    QRectF bbox( xmin,ymin,(xmax - xmin),(ymax - ymin) );
    fitInView(bbox,Qt::KeepAspectRatio);
    
    double currentZoomFactor = transform().mapRect( QRectF(0, 0, 1, 1) ).width();
    assert(currentZoomFactor != 0);
    //we want to fit at scale 1 at most
    if (currentZoomFactor > 1.) {
        double scaleFactor = 1. / currentZoomFactor;
        setTransformationAnchor(QGraphicsView::AnchorViewCenter);
        scale(scaleFactor,scaleFactor);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    }

    _imp->_refreshOverlays = true;
    update();
}

NATRON_NAMESPACE_EXIT;
