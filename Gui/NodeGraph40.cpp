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
NodeGraph::togglePreviewsForSelectedNodes()
{
    QMutexLocker l(&_imp->_nodesMutex);

    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.begin();
         it != _imp->_selection.end();
         ++it) {
        (*it)->togglePreview();
    }
}

void
NodeGraph::switchInputs1and2ForSelectedNodes()
{
    QMutexLocker l(&_imp->_nodesMutex);

    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.begin();
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
NodeGraph::copyNodes(const std::list<boost::shared_ptr<NodeGui> >& nodes,NodeClipBoard& clipboard)
{
    _imp->copyNodesInternal(nodes, clipboard);
}

void
NodeGraph::copySelectedNodes()
{
    if ( _imp->_selection.empty()) {
        Natron::warningDialog( tr("Copy").toStdString(), tr("You must select at least a node to copy first.").toStdString() );

        return;
    }

    _imp->copyNodesInternal(_imp->_selection,appPTR->getNodeClipBoard());
}


void
NodeGraph::cutSelectedNodes()
{
    if ( _imp->_selection.empty() ) {
        Natron::warningDialog( tr("Cut").toStdString(), tr("You must select at least a node to cut first.").toStdString() );

        return;
    }
    copySelectedNodes();
    deleteSelection();
}

void
NodeGraph::pasteCliboard(const NodeClipBoard& clipboard,std::list<std::pair<std::string,boost::shared_ptr<NodeGui> > >* newNodes)
{
    QPointF position = _imp->_root->mapFromScene(mapToScene(mapFromGlobal(QCursor::pos())));
    _imp->pasteNodesInternal(clipboard,position, false, newNodes);
}

void
NodeGraph::pasteNodeClipBoards(const QPointF& pos)
{
    std::list<std::pair<std::string,boost::shared_ptr<NodeGui> > > newNodes;
    _imp->pasteNodesInternal(appPTR->getNodeClipBoard(),pos, true, &newNodes);
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
        Natron::warningDialog( tr("Duplicate").toStdString(), tr("You must select at least a node to duplicate first.").toStdString() );
        
        return;
    }
    
    ///Don't use the member clipboard as the user might have something copied
    NodeClipBoard tmpClipboard;
    _imp->copyNodesInternal(_imp->_selection,tmpClipboard);
    std::list<std::pair<std::string,boost::shared_ptr<NodeGui> > > newNodes;
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
        Natron::warningDialog( tr("Clone").toStdString(), tr("You must select at least a node to clone first.").toStdString() );
        return;
    }
    
    double xmax = INT_MIN;
    double xmin = INT_MAX;
    double ymin = INT_MAX;
    double ymax = INT_MIN;
    NodeGuiList nodesToCopy = _imp->_selection;
    for (NodeGuiList::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
        if ( (*it)->getNode()->getMasterNode()) {
            Natron::errorDialog( tr("Clone").toStdString(), tr("You cannot clone a node which is already a clone.").toStdString() );
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
        BackDropGui* isBd = dynamic_cast<BackDropGui*>(it->get());
        if (isBd) {
            NodeGuiList nodesWithinBD = getNodesWithinBackDrop(*it);
            for (NodeGuiList::iterator it2 = nodesWithinBD.begin(); it2 != nodesWithinBD.end(); ++it2) {
                NodeGuiList::iterator found = std::find(nodesToCopy.begin(),nodesToCopy.end(),*it2);
                if ( found == nodesToCopy.end() ) {
                    nodesToCopy.push_back(*it2);
                }
            }
        }
    }
    
    for (NodeGuiList::iterator it = nodesToCopy.begin(); it != nodesToCopy.end(); ++it) {
        if ( (*it)->getNode()->getLiveInstance()->isSlave() ) {
            Natron::errorDialog( tr("Clone").toStdString(), tr("You cannot clone a node which is already a clone.").toStdString() );
            
            return;
        }
        ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>((*it)->getNode()->getLiveInstance());
        if (isViewer) {
            Natron::errorDialog( tr("Clone").toStdString(), tr("Cloning a viewer is not a valid operation.").toStdString() );
            
            return;
        }
        if ( (*it)->getNode()->isMultiInstance() ) {
            QString err = QString("%1 cannot be cloned.").arg( (*it)->getNode()->getLabel().c_str() );
            Natron::errorDialog( tr("Clone").toStdString(),
                                tr( err.toStdString().c_str() ).toStdString() );
            
            return;
        }
    }
    
    QPointF offset(scenePos.x() - ((xmax + xmin) / 2.), scenePos.y() -  ((ymax + ymin) / 2.));
    std::list<std::pair<std::string,boost::shared_ptr<NodeGui> > > newNodes;
    std::list <boost::shared_ptr<NodeSerialization> > serializations;
    
    std::list <boost::shared_ptr<NodeGui> > newNodesList;
    
    for (NodeGuiList::iterator it = nodesToCopy.begin(); it != nodesToCopy.end(); ++it) {
        boost::shared_ptr<NodeSerialization>  internalSerialization( new NodeSerialization( (*it)->getNode() ) );
        NodeGuiSerialization guiSerialization;
        (*it)->serialize(&guiSerialization);
        boost::shared_ptr<NodeGui> clone = _imp->pasteNode( *internalSerialization, guiSerialization, offset,
                                                           _imp->group.lock(),std::string(),true );
        
        newNodes.push_back(std::make_pair(internalSerialization->getNodeScriptName(),clone));
        newNodesList.push_back(clone);
        serializations.push_back(internalSerialization);
        
        ///The script-name of the copy node is different than the one of the original one, update all input connections in the serialization
        for (std::list<boost::shared_ptr<NodeSerialization> >::iterator it2 = serializations.begin(); it2!=serializations.end(); ++it2) {
            (*it2)->switchInput(internalSerialization->getNodeScriptName(), clone->getNode()->getScriptName());
        }
        
        
    }
    
    
    assert( serializations.size() == newNodes.size() );
    ///restore connections
    _imp->restoreConnections(serializations, newNodes);
    
    
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
        Natron::warningDialog( tr("Declone").toStdString(), tr("You must select at least a node to declone first.").toStdString() );

        return;
    }
    std::list<boost::shared_ptr<NodeGui> > nodesToDeclone;


    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
        
        BackDropGui* isBd = dynamic_cast<BackDropGui*>(it->get());
        if (isBd) {
            ///Also copy all nodes within the backdrop
            NodeGuiList nodesWithinBD = getNodesWithinBackDrop(*it);
            for (NodeGuiList::iterator it2 = nodesWithinBD.begin(); it2 != nodesWithinBD.end(); ++it2) {
                NodeGuiList::iterator found = std::find(nodesToDeclone.begin(),nodesToDeclone.end(),*it2);
                if ( found == nodesToDeclone.end() ) {
                    nodesToDeclone.push_back(*it2);
                }
            }
        }
        if ( (*it)->getNode()->getLiveInstance()->isSlave() ) {
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
NodeGraph::deleteNodepluginsly(boost::shared_ptr<NodeGui> n)
{
    assert(n);
    boost::shared_ptr<Natron::Node> internalNode = n->getNode();

    if (internalNode) {
        internalNode->deactivate(std::list< Natron::Node* >(),false,false,true,false);
    }
    std::list<boost::shared_ptr<NodeGui> >::iterator it = std::find(_imp->_nodesTrash.begin(),_imp->_nodesTrash.end(),n);

    if ( it != _imp->_nodesTrash.end() ) {
        _imp->_nodesTrash.erase(it);
    }

    {
        QMutexLocker l(&_imp->_nodesMutex);
        std::list<boost::shared_ptr<NodeGui> >::iterator it = std::find(_imp->_nodes.begin(),_imp->_nodes.end(),n);
        if ( it != _imp->_nodes.end() ) {
            _imp->_nodes.erase(it);
        }
    }


    n->deleteReferences();
    n->discardGraphPointer();

    if ( getGui() ) {
        
        if (internalNode->isRotoPaintingNode()) {
            getGui()->removeRotoInterface(n.get(),true);
        }
        
        if (internalNode->isPointTrackerNode()) {
            getGui()->removeTrackerInterface(n.get(), true);
        }

        ///now that we made the command dirty, delete the node everywhere in Natron
        getGui()->getApp()->deleteNode(n);


        getGui()->getCurveEditor()->removeNode( n.get() );
        std::list<boost::shared_ptr<NodeGui> >::iterator found = std::find(_imp->_selection.begin(),_imp->_selection.end(),n);
        if ( found != _imp->_selection.end() ) {
            n->setUserSelected(false);
            _imp->_selection.erase(found);
        }
        
        if (internalNode && internalNode->getLiveInstance()) {
            NodeGroup* isGrp = dynamic_cast<NodeGroup*>(internalNode->getLiveInstance());
            if (isGrp) {
                NodeGraphI* graph_i = isGrp->getNodeGraph();
                if (graph_i) {
                    NodeGraph* graph = dynamic_cast<NodeGraph*>(graph_i);
                    assert(graph);
                    if (graph) {
                        getGui()->removeGroupGui(graph, true);
                    }
                }
            }
        }
    }
    
    if (internalNode) {
        ///remove the node from the clipboard if it is
        NodeClipBoard &cb = appPTR->getNodeClipBoard();
        for (std::list< boost::shared_ptr<NodeSerialization> >::iterator it = cb.nodes.begin();
             it != cb.nodes.end(); ++it) {
            if ( (*it)->getNode() == internalNode ) {
                cb.nodes.erase(it);
                break;
            }
        }
        
        for (std::list<boost::shared_ptr<NodeGuiSerialization> >::iterator it = cb.nodesUI.begin();
             it != cb.nodesUI.end(); ++it) {
            if ( (*it)->getFullySpecifiedName() == internalNode->getFullyQualifiedName() ) {
                cb.nodesUI.erase(it);
                break;
            }
        }
    }
} // deleteNodepluginsly

void
NodeGraph::invalidateAllNodesParenting()
{
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        (*it)->setParentItem(NULL);
        if ( (*it)->scene() ) {
            (*it)->scene()->removeItem( it->get() );
        }
    }
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodesTrash.begin(); it != _imp->_nodesTrash.end(); ++it) {
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

        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
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
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
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
    
    currentZoomFactor = transform().mapRect( QRectF(0, 0, 1, 1) ).width();
    if (currentZoomFactor < 0.4) {
        setVisibleNodeDetails(false);
    } else if (currentZoomFactor >= 0.4) {
        setVisibleNodeDetails(true);
    }

    _imp->_refreshOverlays = true;
    update();
}
