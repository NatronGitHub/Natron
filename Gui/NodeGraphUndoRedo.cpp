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

#include "NodeGraphUndoRedo.h"

#include <algorithm> // min, max
#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QDebug>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/CreateNodeArgs.h"
#include "Engine/GroupInput.h"
#include "Engine/GroupOutput.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/RotoLayer.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewerInstance.h"

#include "Gui/NodeGui.h"
#include "Gui/NodeGraph.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/Edge.h"
#include "Gui/GuiApplicationManager.h"

#include "Serialization/NodeSerialization.h"
#include "Serialization/NodeClipBoard.h"

#define MINIMUM_VERTICAL_SPACE_BETWEEN_NODES 10

NATRON_NAMESPACE_ENTER;

MoveMultipleNodesCommand::MoveMultipleNodesCommand(const NodesGuiList & nodes,
                                                   double dx,
                                                   double dy,
                                                   QUndoCommand *parent)
    : QUndoCommand(parent)
    , _firstRedoCalled(false)
    , _nodes(nodes)
    , _dx(dx)
    , _dy(dy)
{
    assert( !nodes.empty() );
}

void
MoveMultipleNodesCommand::move(double dx,
                               double dy)
{
    for (NodesGuiList::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        QPointF pos = (*it)->pos();
        (*it)->setPosition(pos.x() + dx, pos.y() + dy);
    }
}

void
MoveMultipleNodesCommand::undo()
{
    move(-_dx, -_dy);
    setText( tr("Move nodes") );
}

void
MoveMultipleNodesCommand::redo()
{
    if (_firstRedoCalled) {
        move(_dx, _dy);
    }
    _firstRedoCalled = true;
    setText( tr("Move nodes") );
}

AddMultipleNodesCommand::AddMultipleNodesCommand(NodeGraph* graph,
                                                 const std::list<NodeGuiPtr > & nodes,
                                                 QUndoCommand *parent)
    : QUndoCommand(parent)
    , _nodes()
    , _graph(graph)
    , _firstRedoCalled(false)
    , _isUndone(false)
{
    for (std::list<NodeGuiPtr >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        _nodes.push_back(*it);
    }
}

AddMultipleNodesCommand::AddMultipleNodesCommand(NodeGraph* graph,
                                                 const NodeGuiPtr & node,
                                                 QUndoCommand* parent)
    : QUndoCommand(parent)
    , _nodes()
    , _graph(graph)
    , _firstRedoCalled(false)
    , _isUndone(false)
{
    _nodes.push_back(node);
}

AddMultipleNodesCommand::~AddMultipleNodesCommand()
{
    if (_isUndone) {
        for (std::list<boost::weak_ptr<NodeGui> >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
            NodeGuiPtr node = it->lock();
            if (node) {
                node->getNode()->destroyNode(false, false);
            }
        }
    }
}

void
AddMultipleNodesCommand::undo()
{
    _isUndone = true;
    std::list<ViewerInstancePtr> viewersToRefresh;


    for (std::list<boost::weak_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        NodeGuiPtr node = it->lock();
        std::list<ViewerInstancePtr> viewers;
        node->getNode()->hasViewersConnected(&viewers);
        for (std::list<ViewerInstancePtr>::iterator it2 = viewers.begin(); it2 != viewers.end(); ++it2) {
            std::list<ViewerInstancePtr>::iterator foundViewer = std::find(viewersToRefresh.begin(), viewersToRefresh.end(), *it2);
            if ( foundViewer == viewersToRefresh.end() ) {
                viewersToRefresh.push_back(*it2);
            }
        }
        node->getNode()->deactivate(NodesList(), //outputs to disconnect
                                    true, //disconnect all nodes, disregarding the first parameter.
                                    true, //reconnect outputs to inputs of this node?
                                    true, //hide nodeGui?
                                    false); // triggerRender
    }

    _graph->clearSelection();

    _graph->getGui()->getApp()->triggerAutoSave();

    for (std::list<ViewerInstancePtr>::iterator it = viewersToRefresh.begin(); it != viewersToRefresh.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }


    setText( tr("Add node") );
}

void
AddMultipleNodesCommand::redo()
{
    _isUndone = false;
    std::list<ViewerInstancePtr> viewersToRefresh;
    std::list<NodeGuiPtr > nodes;
    for (std::list<boost::weak_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        nodes.push_back( it->lock() );
    }
    if (_firstRedoCalled) {
        for (std::list<NodeGuiPtr >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
            (*it)->getNode()->activate(NodesList(), //inputs to restore
                                       true, //restore all inputs ?
                                       false); //triggerRender
        }
    }


    if ( (nodes.size() != 1) || !nodes.front()->getNode()->isEffectNodeGroup() ) {
        _graph->setSelection(nodes);
    }

    _graph->getGui()->getApp()->recheckInvalidExpressions();
    _graph->getGui()->getApp()->triggerAutoSave();

    for (std::list<NodeGuiPtr >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        std::list<ViewerInstancePtr> viewers;
        (*it)->getNode()->hasViewersConnected(&viewers);
        for (std::list<ViewerInstancePtr>::iterator it2 = viewers.begin(); it2 != viewers.end(); ++it2) {
            std::list<ViewerInstancePtr>::iterator foundViewer = std::find(viewersToRefresh.begin(), viewersToRefresh.end(), *it2);
            if ( foundViewer == viewersToRefresh.end() ) {
                viewersToRefresh.push_back(*it2);
            }
        }
    }


    for (std::list<ViewerInstancePtr>::iterator it = viewersToRefresh.begin(); it != viewersToRefresh.end(); ++it) {
        if ( (*it)->getUiContext() ) {
            (*it)->renderCurrentFrame(true);
        }
    }


    _firstRedoCalled = true;
    setText( tr("Add node") );
}

RemoveMultipleNodesCommand::RemoveMultipleNodesCommand(NodeGraph* graph,
                                                       const std::list<NodeGuiPtr > & nodes,
                                                       QUndoCommand *parent)
    : QUndoCommand(parent)
    , _nodes()
    , _graph(graph)
    , _isRedone(false)
{
    for (std::list<NodeGuiPtr >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodeToRemove n;
        n.node = *it;

        ///find all outputs to restore
        const NodesWList & outputs = (*it)->getNode()->getOutputs();
        for (NodesWList::const_iterator it2 = outputs.begin(); it2 != outputs.end(); ++it2) {
            NodePtr output = it2->lock();
            if (!output) {
                continue;
            }
            bool restore = true;
            for (std::list<NodeGuiPtr >::const_iterator it3 = nodes.begin(); it3 != nodes.end(); ++it3) {
                if ( (*it3)->getNode() == output ) {
                    ///we found the output in the selection, don't restore it
                    restore = false;
                }
            }
            if (restore) {
                n.outputsToRestore.push_back(output);
            }
        }
        _nodes.push_back(n);
    }
}

RemoveMultipleNodesCommand::~RemoveMultipleNodesCommand()
{
    if (_isRedone) {
        for (std::list<NodeToRemove>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
            NodeGuiPtr n = it->node.lock();
            if (n) {
                n->getNode()->destroyNode(false, false);
            }
        }
    }
}

void
RemoveMultipleNodesCommand::undo()
{
    std::list<ViewerInstancePtr> viewersToRefresh;
    std::list<SequenceTime> allKeysToAdd;
    std::list<NodeToRemove>::iterator next = _nodes.begin();

    if ( next != _nodes.end() ) {
        ++next;
    }
    for (std::list<NodeToRemove>::iterator it = _nodes.begin();
         it != _nodes.end();
         ++it) {
        NodeGuiPtr node = it->node.lock();
        NodesList outputsToRestore;
        for (NodesWList::const_iterator it2 = it->outputsToRestore.begin(); it2 != it->outputsToRestore.end(); ++it2) {
            NodePtr output = it2->lock();
            if (output) {
                outputsToRestore.push_back(output);
            }
        }

        node->getNode()->activate(outputsToRestore, false, false);

        std::list<ViewerInstancePtr> viewers;
        node->getNode()->hasViewersConnected(&viewers);
        for (std::list<ViewerInstancePtr>::iterator it2 = viewers.begin(); it2 != viewers.end(); ++it2) {
            std::list<ViewerInstancePtr>::iterator foundViewer = std::find(viewersToRefresh.begin(), viewersToRefresh.end(), *it2);
            if ( foundViewer == viewersToRefresh.end() ) {
                viewersToRefresh.push_back(*it2);
            }
        }

        // increment for next iteration
        if ( next != _nodes.end() ) {
            ++next;
        }
    } // for(it)
    for (std::list<ViewerInstancePtr>::iterator it = viewersToRefresh.begin(); it != viewersToRefresh.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }
    _graph->getGui()->getApp()->triggerAutoSave();
    _graph->getGui()->getApp()->redrawAllViewers();
    _graph->updateNavigator();

    _isRedone = false;
    _graph->scene()->update();
    setText( tr("Remove node") );
} // RemoveMultipleNodesCommand::undo

void
RemoveMultipleNodesCommand::redo()
{
    _isRedone = true;

    std::list<ViewerInstancePtr> viewersToRefresh;
    std::list<NodeToRemove>::iterator next = _nodes.begin();
    if ( next != _nodes.end() ) {
        ++next;
    }
    for (std::list<NodeToRemove>::iterator it = _nodes.begin();
         it != _nodes.end();
         ++it) {
        NodeGuiPtr node = it->node.lock();
        ///Make a copy before calling deactivate which will modify the list
        NodesWList outputs = node->getNode()->getOutputs();
        std::list<ViewerInstancePtr> viewers;
        node->getNode()->hasViewersConnected(&viewers);
        for (std::list<ViewerInstancePtr>::iterator it2 = viewers.begin(); it2 != viewers.end(); ++it2) {
            std::list<ViewerInstancePtr>::iterator foundViewer = std::find(viewersToRefresh.begin(), viewersToRefresh.end(), *it2);
            if ( foundViewer == viewersToRefresh.end() ) {
                viewersToRefresh.push_back(*it2);
            }
        }

        NodesList outputsToRestore;
        for (NodesWList::const_iterator it2 = it->outputsToRestore.begin(); it2 != it->outputsToRestore.end(); ++it2) {
            NodePtr output = it2->lock();
            if (output) {
                outputsToRestore.push_back(output);
            }
        }

        node->getNode()->deactivate(outputsToRestore, false, _nodes.size() == 1, true, false);


        if (_nodes.size() == 1) {
            ///If we're deleting a single node and there's a viewer in output,reconnect the viewer to another connected input it has
            for (NodesWList::const_iterator it2 = outputs.begin(); it2 != outputs.end(); ++it2) {
                NodePtr output = it2->lock();

                if (!output) {
                    continue;
                }

                ///the output must be in the outputs to restore
                NodesList::const_iterator found = std::find(outputsToRestore.begin(), outputsToRestore.end(), output);

                if ( found != outputsToRestore.end() ) {
                    ViewerNodePtr isViewer = output->isEffectViewerNode();
                    ///if the node is an viewer, when disconnecting the active input just activate another input instead
                    if (isViewer) {
                        const std::vector<NodeWPtr> & inputs = output->getInputs();
                        ///set as active input the first non null input
                        for (std::size_t i = 0; i < inputs.size(); ++i) {
                            NodePtr input = inputs[i].lock();
                            if (input) {
                                isViewer->connectInputToIndex(i, 0);
                                ///make sure we don't refresh it a second time
                                std::list<ViewerInstancePtr>::iterator foundViewer =
                                    std::find( viewersToRefresh.begin(), viewersToRefresh.end(), isViewer->getInternalViewerNode() );
                                if ( foundViewer != viewersToRefresh.end() ) {
                                    viewersToRefresh.erase(foundViewer);
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }

        // increment for next iteration
        if ( next != _nodes.end() ) {
            ++next;
        }
    } // for(it)

    for (std::list<ViewerInstancePtr>::iterator it = viewersToRefresh.begin(); it != viewersToRefresh.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }

    _graph->getGui()->getApp()->triggerAutoSave();
    _graph->getGui()->getApp()->redrawAllViewers();
    _graph->updateNavigator();

    _graph->scene()->update();
    setText( tr("Remove node") );
} // redo

ConnectCommand::ConnectCommand(NodeGraph* graph,
                               Edge* edge,
                               const NodeGuiPtr & oldSrc,
                               const NodeGuiPtr & newSrc,
                               int viewerInternalIndex,
                               QUndoCommand *parent)
    : QUndoCommand(parent),
    _oldSrc(oldSrc),
    _newSrc(newSrc),
    _dst( edge->getDest() ),
    _graph(graph),
    _inputNb( edge->getInputNumber() ),
    _viewerInternalIndex(viewerInternalIndex)
{
    assert( _dst.lock() );
}

void
ConnectCommand::undo()
{
    NodeGuiPtr newSrc = _newSrc.lock();
    NodeGuiPtr oldSrc = _oldSrc.lock();
    NodeGuiPtr dst = _dst.lock();

    doConnect(newSrc,
              oldSrc,
              dst,
              _inputNb,
              _viewerInternalIndex);

    if (newSrc) {
        setText( tr("Connect %1 to %2")
                 .arg( QString::fromUtf8( dst->getNode()->getLabel().c_str() ) ).arg( QString::fromUtf8( newSrc->getNode()->getLabel().c_str() ) ) );
    } else {
        setText( tr("Disconnect %1")
                 .arg( QString::fromUtf8( dst->getNode()->getLabel().c_str() ) ) );
    }


    ViewerInstancePtr isDstAViewer = dst->getNode()->isEffectViewerInstance();
    if (!isDstAViewer) {
        _graph->getGui()->getApp()->triggerAutoSave();
    }
    _graph->update();
} // undo

void
ConnectCommand::redo()
{
    NodeGuiPtr newSrc = _newSrc.lock();
    NodeGuiPtr oldSrc = _oldSrc.lock();
    NodeGuiPtr dst = _dst.lock();

    doConnect(oldSrc,
              newSrc,
              dst,
              _inputNb,
              _viewerInternalIndex);

    if (newSrc) {
        setText( tr("Connect %1 to %2")
                 .arg( QString::fromUtf8( dst->getNode()->getLabel().c_str() ) ).arg( QString::fromUtf8( newSrc->getNode()->getLabel().c_str() ) ) );
    } else {
        setText( tr("Disconnect %1")
                 .arg( QString::fromUtf8( dst->getNode()->getLabel().c_str() ) ) );
    }


    ViewerNodePtr isDstAViewer = dst->getNode()->isEffectViewerNode();
    if (!isDstAViewer) {
        _graph->getGui()->getApp()->triggerAutoSave();
    }
    _graph->update();
} // redo

void
ConnectCommand::doConnect(const NodeGuiPtr &oldSrc,
                          const NodeGuiPtr &newSrc,
                          const NodeGuiPtr& dst,
                          int inputNb,
                          int viewerInternalIndex)
{
    NodePtr internalDst =  dst->getNode();
    NodePtr internalNewSrc = newSrc ? newSrc->getNode() : NodePtr();
    NodePtr internalOldSrc = oldSrc ? oldSrc->getNode() : NodePtr();
    ViewerNodePtr isViewer = internalDst->isEffectViewerNode();


    if (isViewer) {
        ///if the node is an inspector  disconnect any current connection between the inspector and the _newSrc
        for (int i = 0; i < internalDst->getMaxInputCount(); ++i) {
            if ( (i != inputNb) && (internalDst->getInput(i) == internalNewSrc) ) {
                internalDst->disconnectInput(i);
            }
        }
    }
    if (internalOldSrc && internalNewSrc) {
        internalDst->swapInput(internalNewSrc, inputNb);
    } else {
        if (internalOldSrc && internalNewSrc) {
            Node::CanConnectInputReturnValue ret = internalDst->canConnectInput(internalNewSrc, inputNb);
            bool connectionOk = ret == Node::eCanConnectInput_ok ||
                                ret == Node::eCanConnectInput_differentFPS ||
                                ret == Node::eCanConnectInput_differentPars;
            if (connectionOk) {
                internalDst->swapInput(internalNewSrc, inputNb);
            } else {
                internalDst->disconnectInput( internalDst->getInputIndex( internalOldSrc ) );
            }
        } else if (internalOldSrc && !internalNewSrc) {
            internalDst->disconnectInput( internalDst->getInputIndex( internalOldSrc ) );
        } else if (!internalOldSrc && internalNewSrc) {
            Node::CanConnectInputReturnValue ret = internalDst->canConnectInput(internalNewSrc, inputNb);
            bool connectionOk = ret == Node::eCanConnectInput_ok ||
                                ret == Node::eCanConnectInput_differentFPS ||
                                ret == Node::eCanConnectInput_differentPars;
            if (connectionOk) {
                internalDst->connectInput(internalNewSrc, inputNb);
            } else {
                internalDst->disconnectInput( internalDst->getInputIndex( internalOldSrc ) );
            }
        }
    }

    if (isViewer) {
        if (viewerInternalIndex == 0 || viewerInternalIndex == 1) {
            isViewer->connectInputToIndex(inputNb, viewerInternalIndex);
        }
    }

    dst->refreshEdges();
    dst->refreshEdgesVisility();

    if (newSrc) {
        newSrc->refreshEdgesVisility();
    }
    if (oldSrc) {
        oldSrc->refreshEdgesVisility();
    }
} // ConnectCommand::doConnect

InsertNodeCommand::InsertNodeCommand(NodeGraph* graph,
                                     Edge* edge,
                                     const NodeGuiPtr & newSrc,
                                     QUndoCommand *parent)
    : ConnectCommand(graph, edge, edge->getSource(), newSrc, -1, parent)
    , _inputEdge(0)
{
    assert(newSrc);
    setText( tr("Insert node") );
}

void
InsertNodeCommand::undo()
{
    NodeGuiPtr oldSrc = _oldSrc.lock();
    NodeGuiPtr newSrc = _newSrc.lock();
    NodeGuiPtr dst = _dst.lock();

    assert(newSrc);

    NodePtr oldSrcInternal = oldSrc ? oldSrc->getNode() : NodePtr();
    NodePtr newSrcInternal = newSrc->getNode();
    NodePtr dstInternal = dst->getNode();
    assert(newSrcInternal && dstInternal);

    doConnect(newSrc, oldSrc, dst, _inputNb, -1);

    if (_inputEdge) {
        doConnect( _inputEdge->getSource(), NodeGuiPtr(), _inputEdge->getDest(), _inputEdge->getInputNumber(), -1);
    }

    ViewerInstancePtr isDstAViewer = dst->getNode()->isEffectViewerInstance();
    if (!isDstAViewer) {
        _graph->getGui()->getApp()->triggerAutoSave();
    }
    _graph->update();
}

void
InsertNodeCommand::redo()
{
    NodeGuiPtr oldSrc = _oldSrc.lock();
    NodeGuiPtr newSrc = _newSrc.lock();
    NodeGuiPtr dst = _dst.lock();

    assert(newSrc);

    NodePtr oldSrcInternal = oldSrc ? oldSrc->getNode() : NodePtr();
    NodePtr newSrcInternal = newSrc->getNode();
    NodePtr dstInternal = dst->getNode();
    assert(newSrcInternal && dstInternal);

    newSrcInternal->beginInputEdition();
    dstInternal->beginInputEdition();

    doConnect(oldSrc, newSrc, dst, _inputNb, -1);


    ///find out if the node is already connected to what the edge is connected
    bool alreadyConnected = false;
    const std::vector<NodeWPtr > & inpNodes = newSrcInternal->getInputs();
    if (oldSrcInternal) {
        for (std::size_t i = 0; i < inpNodes.size(); ++i) {
            if (inpNodes[i].lock() == oldSrcInternal) {
                alreadyConnected = true;
                break;
            }
        }
    }

    _inputEdge = 0;
    if (oldSrcInternal && !alreadyConnected) {

        int prefInput = newSrcInternal->getPreferredInputForConnection();
        if (prefInput != -1) {
            _inputEdge = newSrc->getInputArrow(prefInput);
            assert(_inputEdge);
            doConnect( _inputEdge->getSource(), oldSrc, _inputEdge->getDest(), _inputEdge->getInputNumber(), -1 );
        }
    }

    ViewerInstancePtr isDstAViewer = dst->getNode()->isEffectViewerInstance();
    if (!isDstAViewer) {
        _graph->getGui()->getApp()->triggerAutoSave();
    }

    newSrcInternal->endInputEdition(false);
    dstInternal->endInputEdition(false);

    std::list<ViewerInstancePtr> viewers;
    dstInternal->hasViewersConnected(&viewers);
    for (std::list<ViewerInstancePtr>::iterator it2 = viewers.begin(); it2 != viewers.end(); ++it2) {
        (*it2)->renderCurrentFrame(true);
    }

    _graph->update();
} // InsertNodeCommand::redo

ResizeBackdropCommand::ResizeBackdropCommand(const NodeGuiPtr& bd,
                                             int w,
                                             int h,
                                             QUndoCommand *parent)
    : QUndoCommand(parent)
    , _bd(bd)
    , _w(w)
    , _h(h)
    , _oldW(0)
    , _oldH(0)
{
    QRectF bbox = bd->boundingRect();

    _oldW = bbox.width();
    _oldH = bbox.height();
}

ResizeBackdropCommand::~ResizeBackdropCommand()
{
}

void
ResizeBackdropCommand::undo()
{
    _bd->resize(_oldW, _oldH);
    setText( tr("Resize %1").arg( QString::fromUtf8( _bd->getNode()->getLabel().c_str() ) ) );
}

void
ResizeBackdropCommand::redo()
{
    _bd->resize(_w, _h);
    setText( tr("Resize %1").arg( QString::fromUtf8( _bd->getNode()->getLabel().c_str() ) ) );
}

bool
ResizeBackdropCommand::mergeWith(const QUndoCommand *command)
{
    const ResizeBackdropCommand* rCmd = dynamic_cast<const ResizeBackdropCommand*>(command);

    if (!rCmd) {
        return false;
    }
    if (rCmd->_bd != _bd) {
        return false;
    }
    _w = rCmd->_w;
    _h = rCmd->_h;

    return true;
}

DecloneMultipleNodesCommand::DecloneMultipleNodesCommand(NodeGraph* graph,
                                                         const  std::map<NodeGuiPtr, NodePtr> & nodes,
                                                         QUndoCommand *parent)
    : QUndoCommand(parent)
    , _nodes()
    , _graph(graph)
{
    for ( std::map<NodeGuiPtr, NodePtr>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodeToDeclone n;
        n.node = it->first;
        n.master = it->second;
        _nodes.push_back(n);
    }
}

DecloneMultipleNodesCommand::~DecloneMultipleNodesCommand()
{
}

void
DecloneMultipleNodesCommand::undo()
{
    for (std::list<NodeToDeclone>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->node.lock()->getNode()->linkToNode( it->master.lock());
    }

    _graph->getGui()->getApp()->triggerAutoSave();
    setText( tr("Declone node") );
}

void
DecloneMultipleNodesCommand::redo()
{
    for (std::list<NodeToDeclone>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->node.lock()->getNode()->unlinkAllKnobs();
    }

    _graph->getGui()->getApp()->triggerAutoSave();
    setText( tr("Declone node") );
}

NATRON_NAMESPACE_ANONYMOUS_ENTER

typedef std::pair<NodeGuiPtr, QPointF> TreeNode; ///< all points are expressed as being the CENTER of the node!

class Tree
{
    std::list<TreeNode> nodes;
    QPointF topLevelNodeCenter; //< in scene coords

public:

    Tree()
        : nodes()
        , topLevelNodeCenter(0, INT_MAX)
    {
    }

    void buildTree(const NodeGuiPtr & output,
                   const NodesGuiList& selectedNodes,
                   std::list<NodeGui*> & usedNodes)
    {
        QPointF outputPos = output->pos();
        QSize nodeSize = output->getSize();

        outputPos += QPointF(nodeSize.width() / 2., nodeSize.height() / 2.);
        addNode(output, outputPos);

        buildTreeInternal(selectedNodes, output.get(), output->mapToScene( output->mapFromParent(outputPos) ), usedNodes);
    }

    const std::list<TreeNode> & getNodes() const
    {
        return nodes;
    }

    const QPointF & getTopLevelNodeCenter() const
    {
        return topLevelNodeCenter;
    }

    void moveAllTree(const QPointF & delta)
    {
        for (std::list<TreeNode>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            it->second += delta;
        }
    }

private:

    void addNode(const NodeGuiPtr & node,
                 const QPointF & point)
    {
        nodes.push_back( std::make_pair(node, point) );
    }

    void buildTreeInternal(const NodesGuiList& selectedNodes,
                           NodeGui* currentNode, const QPointF & currentNodeScenePos, std::list<NodeGui*> & usedNodes);
};

typedef std::list< boost::shared_ptr<Tree> > TreeList;

void
Tree::buildTreeInternal(const NodesGuiList& selectedNodes,
                        NodeGui* currentNode,
                        const QPointF & currentNodeScenePos,
                        std::list<NodeGui*> & usedNodes)
{
    QSize nodeSize = currentNode->getSize();
    NodePtr internalNode = currentNode->getNode();
    const std::vector<Edge*> & inputs = currentNode->getInputsArrows();
    NodeGuiPtr firstNonMaskInput;
    NodesGuiList otherNonMaskInputs;
    NodesGuiList maskInputs;

    for (U32 i = 0; i < inputs.size(); ++i) {
        NodeGuiPtr source = inputs[i]->getSource();

        ///Check if the source is selected
        NodesGuiList::const_iterator foundSelected = std::find(selectedNodes.begin(), selectedNodes.end(), source);
        if ( foundSelected == selectedNodes.end() ) {
            continue;
        }

        if (source) {
            bool isMask = internalNode->getEffectInstance()->isInputMask(i);
            if (!firstNonMaskInput && !isMask) {
                firstNonMaskInput = source;
                for (std::list<TreeNode>::iterator it2 = nodes.begin(); it2 != nodes.end(); ++it2) {
                    if (it2->first == firstNonMaskInput) {
                        firstNonMaskInput.reset();
                        break;
                    }
                }
            } else if (!isMask) {
                bool found = false;
                for (std::list<TreeNode>::iterator it2 = nodes.begin(); it2 != nodes.end(); ++it2) {
                    if (it2->first == source) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    otherNonMaskInputs.push_back(source);
                }
            } else if (isMask) {
                bool found = false;
                for (std::list<TreeNode>::iterator it2 = nodes.begin(); it2 != nodes.end(); ++it2) {
                    if (it2->first == source) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    maskInputs.push_back(source);
                }
            } else {
                ///this can't be happening
                assert(false);
            }
        }
    }


    ///The node has already been processed in another tree, skip it.
    if ( std::find(usedNodes.begin(), usedNodes.end(), currentNode) == usedNodes.end() ) {
        ///mark it
        usedNodes.push_back(currentNode);


        QPointF firstNonMaskInputPos;
        std::list<QPointF> otherNonMaskInputsPos;
        std::list<QPointF> maskInputsPos;

        ///Position the first non mask input
        if (firstNonMaskInput) {
            firstNonMaskInputPos = firstNonMaskInput->mapToParent( firstNonMaskInput->mapFromScene(currentNodeScenePos) );
            firstNonMaskInputPos.ry() -= ( (nodeSize.height() / 2.) +
                                           MINIMUM_VERTICAL_SPACE_BETWEEN_NODES +
                                           (firstNonMaskInput->getSize().height() / 2.) );

            ///and add it to the tree, with parent relative coordinates
            addNode(firstNonMaskInput, firstNonMaskInputPos);

            firstNonMaskInputPos = firstNonMaskInput->mapToScene( firstNonMaskInput->mapFromParent(firstNonMaskInputPos) );
        }

        ///Position all other non mask inputs
        int index = 0;
        for (NodesGuiList::iterator it = otherNonMaskInputs.begin(); it != otherNonMaskInputs.end(); ++it, ++index) {
            QPointF p = (*it)->mapToParent( (*it)->mapFromScene(currentNodeScenePos) );

            p.rx() -= ( (nodeSize.width() + (*it)->getSize().width() / 2.) ) * (index + 1);

            ///and add it to the tree
            addNode(*it, p);


            p = (*it)->mapToScene( (*it)->mapFromParent(p) );
            otherNonMaskInputsPos.push_back(p);
        }

        ///Position all mask inputs
        index = 0;
        for (NodesGuiList::iterator it = maskInputs.begin(); it != maskInputs.end(); ++it, ++index) {
            QPointF p = (*it)->mapToParent( (*it)->mapFromScene(currentNodeScenePos) );
            ///Note that here we subsctract nodeSize.width(): Actually we substract twice nodeSize.width() / 2: once to get to the left of the node
            ///and another time to add the space of half a node
            p.rx() += ( (nodeSize.width() + (*it)->getSize().width() / 2.) ) * (index + 1);

            ///and add it to the tree
            addNode(*it, p);

            p = (*it)->mapToScene( (*it)->mapFromParent(p) );
            maskInputsPos.push_back(p);
        }

        ///Now that we built the tree at this level, call this function again on the inputs that we just processed
        if (firstNonMaskInput) {
            buildTreeInternal(selectedNodes, firstNonMaskInput.get(), firstNonMaskInputPos, usedNodes);
        }

        std::list<QPointF>::iterator pointsIt = otherNonMaskInputsPos.begin();
        for (NodesGuiList::iterator it = otherNonMaskInputs.begin(); it != otherNonMaskInputs.end(); ++it, ++pointsIt) {
            buildTreeInternal(selectedNodes, it->get(), *pointsIt, usedNodes);
        }

        pointsIt = maskInputsPos.begin();
        for (NodesGuiList::iterator it = maskInputs.begin(); it != maskInputs.end(); ++it, ++pointsIt) {
            buildTreeInternal(selectedNodes, it->get(), *pointsIt, usedNodes);
        }
    }
    ///update the top level node center if the node doesn't have any input
    if ( !firstNonMaskInput && otherNonMaskInputs.empty() && maskInputs.empty() ) {
        ///QGraphicsView Y axis is top-->down oriented
        if ( currentNodeScenePos.y() < topLevelNodeCenter.y() ) {
            topLevelNodeCenter = currentNodeScenePos;
        }
    }
} // buildTreeInternal

static bool
hasNodeOutputsInList(const std::list<NodeGuiPtr >& nodes,
                     const NodeGuiPtr& node)
{
    const NodesWList& outputs = node->getNode()->getOutputs();
    bool foundOutput = false;

    for (std::list<NodeGuiPtr >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if (*it != node) {
            NodePtr n = (*it)->getNode();

            for (NodesWList::const_iterator it2 = outputs.begin(); it2 != outputs.end(); ++it2) {
                if (it2->lock() == n) {
                    foundOutput = true;
                    break;
                }
            }
            if (foundOutput) {
                break;
            }
        }
    }

    return foundOutput;
}

static bool
hasNodeInputsInList(const std::list<NodeGuiPtr >& nodes,
                    const NodeGuiPtr& node)
{
    const std::vector<NodeWPtr >& inputs = node->getNode()->getInputs();
    bool foundInput = false;

    for (std::list<NodeGuiPtr >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if (*it != node) {
            NodePtr n = (*it)->getNode();

            for (std::size_t i = 0; i < inputs.size(); ++i) {
                if (inputs[i].lock() == n) {
                    foundInput = true;
                    break;
                }
            }
            if (foundInput) {
                break;
            }
        }
    }

    return foundInput;
}

NATRON_NAMESPACE_ANONYMOUS_EXIT


RearrangeNodesCommand::RearrangeNodesCommand(const std::list<NodeGuiPtr > & nodes,
                                             QUndoCommand *parent)
    : QUndoCommand(parent)
    , _nodes()
{
    ///1) Separate the nodes in trees they belong to, once a node has been "used" by a tree, mark it
    ///and don't try to reposition it for another tree
    ///2) For all trees : recursively position each nodes so that each input of a node is positionned as following:
    /// a) The first non mask input is positionned above the node
    /// b) All others non mask inputs are positionned on the left of the node, each one separated by the space of half a node
    /// c) All masks are positionned on the right of the node, each one separated by the space of half a node
    ///3) Move all trees so that they are next to each other and their "top level" node
    ///(the input that is at the highest position in the Y coordinate) is at the same
    ///Y level (node centers have the same Y)

    std::list<NodeGui*> usedNodes;

    ///A list of Tree
    ///Each tree is a lit of nodes with a boolean indicating if it was already positionned( "used" ) by another tree, if set to
    ///true we don't do anything
    /// Each node that doesn't have any output is a potential tree.
    TreeList trees;

    for (std::list<NodeGuiPtr >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( !hasNodeOutputsInList( nodes, (*it) ) ) {
            boost::shared_ptr<Tree> newTree(new Tree);
            newTree->buildTree(*it, nodes, usedNodes);
            trees.push_back(newTree);
        }
    }

    ///For all trees find out which one has the top most level node
    QPointF topLevelPos(0, INT_MAX);
    for (TreeList::iterator it = trees.begin(); it != trees.end(); ++it) {
        const QPointF & treeTop = (*it)->getTopLevelNodeCenter();
        if ( treeTop.y() < topLevelPos.y() ) {
            topLevelPos = treeTop;
        }
    }

    ///now offset all trees to be top aligned at the same level
    for (TreeList::iterator it = trees.begin(); it != trees.end(); ++it) {
        QPointF treeTop = (*it)->getTopLevelNodeCenter();
        if (treeTop.y() == INT_MAX) {
            treeTop.setY( topLevelPos.y() );
        }
        QPointF delta( 0, topLevelPos.y() - treeTop.y() );
        if ( (delta.x() != 0) || (delta.y() != 0) ) {
            (*it)->moveAllTree(delta);
        }

        ///and insert the final result into the _nodes list
        const std::list<TreeNode> & treeNodes = (*it)->getNodes();
        for (std::list<TreeNode>::const_iterator it2 = treeNodes.begin(); it2 != treeNodes.end(); ++it2) {
            NodeToRearrange n;
            n.node = it2->first;
            QSize size = n.node->getSize();
            n.newPos = it2->second - QPointF(size.width() / 2., size.height() / 2.);
            n.oldPos = n.node->pos();
            _nodes.push_back(n);
        }
    }
}

void
RearrangeNodesCommand::undo()
{
    for (std::list<NodeToRearrange>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->node->refreshPosition(it->oldPos.x(), it->oldPos.y(), true);
    }
    setText( tr("Rearrange nodes") );
}

void
RearrangeNodesCommand::redo()
{
    for (std::list<NodeToRearrange>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->node->refreshPosition(it->newPos.x(), it->newPos.y(), true);
    }
    setText( tr("Rearrange nodes") );
}

DisableNodesCommand::DisableNodesCommand(const std::list<NodeGuiPtr > & nodes,
                                         QUndoCommand *parent)
    : QUndoCommand(parent)
    , _nodes()
{
    for (std::list<NodeGuiPtr >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        _nodes.push_back(*it);
    }
}

void
DisableNodesCommand::undo()
{
    for (std::list<boost::weak_ptr<NodeGui> >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->lock()->getNode()->setNodeDisabled(false);
    }
    setText( tr("Disable nodes") );
}

void
DisableNodesCommand::redo()
{
    for (std::list<boost::weak_ptr<NodeGui> >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->lock()->getNode()->setNodeDisabled(true);
    }
    setText( tr("Disable nodes") );
}

EnableNodesCommand::EnableNodesCommand(const std::list<NodeGuiPtr > & nodes,
                                       QUndoCommand *parent)
    : QUndoCommand(parent)
    , _nodes()
{
    for (std::list<NodeGuiPtr > ::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        _nodes.push_back(*it);
    }
}

void
EnableNodesCommand::undo()
{
    for (std::list<boost::weak_ptr<NodeGui> >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->lock()->getNode()->setNodeDisabled(true);
    }
    setText( tr("Enable nodes") );
}

void
EnableNodesCommand::redo()
{
    for (std::list<boost::weak_ptr<NodeGui> >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->lock()->getNode()->setNodeDisabled(false);
    }
    setText( tr("Enable nodes") );
}

RenameNodeUndoRedoCommand::RenameNodeUndoRedoCommand(const NodeGuiPtr & node,
                                                     const QString& oldName,
                                                     const QString& newName)
    : QUndoCommand()
    , _node(node)
    , _oldName(oldName)
    , _newName(newName)
{
    assert(node);
    setText( tr("Rename node") );
}

RenameNodeUndoRedoCommand::~RenameNodeUndoRedoCommand()
{
}

void
RenameNodeUndoRedoCommand::undo()
{
    NodeGuiPtr node = _node.lock();

    node->setName(_oldName);
}

void
RenameNodeUndoRedoCommand::redo()
{
    NodeGuiPtr node = _node.lock();

    node->setName(_newName);
}

static void
addTreeInputs(const std::list<NodeGuiPtr >& nodes,
              const NodeGuiPtr& node,
              ExtractedTree& tree,
              std::list<NodeGuiPtr >& markedNodes)
{
    if ( std::find(markedNodes.begin(), markedNodes.end(), node) != markedNodes.end() ) {
        return;
    }

    if ( std::find(nodes.begin(), nodes.end(), node) == nodes.end() ) {
        return;
    }

    if ( !hasNodeInputsInList(nodes, node) ) {
        ExtractedInput input;
        input.node = node;
        input.inputs = node->getNode()->getInputs();
        tree.inputs.push_back(input);
        markedNodes.push_back(node);
    } else {
        tree.inbetweenNodes.push_back(node);
        markedNodes.push_back(node);
        const std::vector<Edge*>& inputs = node->getInputsArrows();
        for (std::vector<Edge*>::const_iterator it2 = inputs.begin(); it2 != inputs.end(); ++it2) {
            NodeGuiPtr input = (*it2)->getSource();
            if (input) {
                addTreeInputs(nodes, input, tree, markedNodes);
            }
        }
    }
}

static void
extractTreesFromNodes(const std::list<NodeGuiPtr >& nodes,
                      std::list<ExtractedTree>& trees)
{
    std::list<NodeGuiPtr > markedNodes;

    for (std::list<NodeGuiPtr >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        bool isOutput = !hasNodeOutputsInList(nodes, *it);
        if (isOutput) {
            ExtractedTree tree;
            tree.output.node = *it;
            NodePtr n = (*it)->getNode();
            const NodesWList& outputs = n->getOutputs();
            for (NodesWList::const_iterator it2 = outputs.begin(); it2 != outputs.end(); ++it2) {
                NodePtr output = it2->lock();
                if (!output) {
                    continue;
                }
                int idx = output->inputIndex(n);
                tree.output.outputs.push_back( std::make_pair(idx, output) );
            }

            const std::vector<Edge*>& inputs = (*it)->getInputsArrows();
            for (U32 i = 0; i < inputs.size(); ++i) {
                NodeGuiPtr input = inputs[i]->getSource();
                if (input) {
                    addTreeInputs(nodes, input, tree, markedNodes);
                }
            }

            if ( tree.inputs.empty() ) {
                ExtractedInput input;
                input.node = *it;
                input.inputs = n->getInputs();
                tree.inputs.push_back(input);
            }

            trees.push_back(tree);
        }
    }
}

///////////////

ExtractNodeUndoRedoCommand::ExtractNodeUndoRedoCommand(NodeGraph* graph,
                                                       const std::list<NodeGuiPtr >& nodes)
    : QUndoCommand()
    , _graph(graph)
    , _trees()
{
    extractTreesFromNodes(nodes, _trees);
}

ExtractNodeUndoRedoCommand::~ExtractNodeUndoRedoCommand()
{
}

void
ExtractNodeUndoRedoCommand::undo()
{
    std::set<ViewerInstancePtr> viewers;

    for (std::list<ExtractedTree>::iterator it = _trees.begin(); it != _trees.end(); ++it) {
        NodeGuiPtr output = it->output.node.lock();
        ///Connect and move output
        for (std::list<std::pair<int, NodeWPtr > >::iterator it2 = it->output.outputs.begin(); it2 != it->output.outputs.end(); ++it2) {
            NodePtr node = it2->second.lock();
            if (!node) {
                continue;
            }
            node->disconnectInput(it2->first);
            node->connectInput(output->getNode(), it2->first);
        }

        QPointF curPos = output->pos();
        output->refreshPosition(curPos.x() - 200, curPos.y(), true);

        ///Connect and move inputs
        for (std::list<ExtractedInput>::iterator it2 = it->inputs.begin(); it2 != it->inputs.end(); ++it2) {
            NodeGuiPtr input = it2->node.lock();
            for (U32 i  =  0; i < it2->inputs.size(); ++i) {
                if ( it2->inputs[i].lock() ) {
                    input->getNode()->connectInput(it2->inputs[i].lock(), i);
                }
            }

            if (input != output) {
                QPointF curPos = input->pos();
                input->refreshPosition(curPos.x() - 200, curPos.y(), true);
            }
        }

        ///Move all other nodes

        for (std::list<boost::weak_ptr<NodeGui> >::iterator it2 = it->inbetweenNodes.begin(); it2 != it->inbetweenNodes.end(); ++it2) {
            NodeGuiPtr node = it2->lock();
            QPointF curPos = node->pos();
            node->refreshPosition(curPos.x() - 200, curPos.y(), true);
        }

        std::list<ViewerInstancePtr> tmp;
        output->getNode()->hasViewersConnected(&tmp);

        for (std::list<ViewerInstancePtr>::iterator it2 = tmp.begin(); it2 != tmp.end(); ++it2) {
            viewers.insert(*it2);
        }
    }

    for (std::set<ViewerInstancePtr>::iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }

    _graph->getGui()->getApp()->triggerAutoSave();
    setText( tr("Extract node") );
} // ExtractNodeUndoRedoCommand::undo

void
ExtractNodeUndoRedoCommand::redo()
{
    std::set<ViewerInstancePtr> viewers;

    for (std::list<ExtractedTree>::iterator it = _trees.begin(); it != _trees.end(); ++it) {
        std::list<ViewerInstancePtr> tmp;
        NodeGuiPtr output = it->output.node.lock();
        output->getNode()->hasViewersConnected(&tmp);

        for (std::list<ViewerInstancePtr>::iterator it2 = tmp.begin(); it2 != tmp.end(); ++it2) {
            viewers.insert(*it2);
        }

        bool outputsAlreadyDisconnected = false;

        ///Reconnect outputs to the input of the input of the ExtractedInputs if inputs.size() == 1
        if ( (it->output.outputs.size() == 1) && (it->inputs.size() == 1) ) {
            const ExtractedInput& selectedInput = it->inputs.front();
            const std::vector<NodeWPtr > &inputs = selectedInput.inputs;
            NodeGuiPtr selectedInputNode = selectedInput.node.lock();
            NodePtr inputToConnectTo;
            for (U32 i = 0; i < inputs.size(); ++i) {
                if ( inputs[i].lock() && !selectedInputNode->getNode()->getEffectInstance()->isInputOptional(i) ) {
                    inputToConnectTo = inputs[i].lock();
                    break;
                }
            }

            if (inputToConnectTo) {
                for (std::list<std::pair<int, NodeWPtr > >::iterator it2 = it->output.outputs.begin(); it2 != it->output.outputs.end(); ++it2) {
                    NodePtr node = it2->second.lock();
                    if (!node) {
                        continue;
                    }
                    node->disconnectInput(it2->first);
                    node->connectInput(inputToConnectTo, it2->first);
                }
                outputsAlreadyDisconnected = true;
            }
        }

        ///Disconnect and move output
        if (!outputsAlreadyDisconnected) {
            for (std::list<std::pair<int, NodeWPtr > >::iterator it2 = it->output.outputs.begin(); it2 != it->output.outputs.end(); ++it2) {
                NodePtr node = it2->second.lock();
                if (node) {
                    node->disconnectInput(it2->first);
                }
            }
        }

        QPointF curPos = output->pos();
        output->refreshPosition(curPos.x() + 200, curPos.y(), true);


        ///Disconnect and move inputs
        for (std::list<ExtractedInput>::iterator it2 = it->inputs.begin(); it2 != it->inputs.end(); ++it2) {
            NodeGuiPtr node = it2->node.lock();
            for (U32 i  =  0; i < it2->inputs.size(); ++i) {
                if ( it2->inputs[i].lock() ) {
                    node->getNode()->disconnectInput(i);
                }
            }
            if (node != output) {
                QPointF curPos = node->pos();
                node->refreshPosition(curPos.x() + 200, curPos.y(), true);
            }
        }

        ///Move all other nodes

        for (std::list<boost::weak_ptr<NodeGui> >::iterator it2 = it->inbetweenNodes.begin(); it2 != it->inbetweenNodes.end(); ++it2) {
            NodeGuiPtr node = it2->lock();
            QPointF curPos = node->pos();
            node->refreshPosition(curPos.x() + 200, curPos.y(), true);
        }
    }

    for (std::set<ViewerInstancePtr>::iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }

    _graph->getGui()->getApp()->triggerAutoSave();


    setText( tr("Extract node") );
} // ExtractNodeUndoRedoCommand::redo

GroupFromSelectionCommand::GroupFromSelectionCommand(NodeGraph* graph,
                                                     const NodesGuiList & nodes)
    : QUndoCommand()
    , _graph(graph)
    , _group()
    , _firstRedoCalled(false)
    , _isRedone(false)
{
    setText( tr("Group from selection") );

    assert( !nodes.empty() );
    for (NodesGuiList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        _originalNodes.push_back(*it);
    }

}

GroupFromSelectionCommand::~GroupFromSelectionCommand()
{
}

void
GroupFromSelectionCommand::undo()
{
    std::list<NodeGuiPtr> nodesToSelect;

    for (OutputLinksMap::iterator it = _outputLinks.begin(); it != _outputLinks.end(); ++it) {
        NodePtr outputNode = it->first.lock();
        if (!outputNode) {
            continue;
        }
        NodePtr input = it->second.inputNode.lock();
        if (!input) {
            continue;
        }
        outputNode->swapInput(input, it->second.inputIdx);
    }


    for (std::list<boost::weak_ptr<NodeGui> >::iterator it = _originalNodes.begin(); it != _originalNodes.end(); ++it) {
        NodeGuiPtr node = it->lock();
        if (node) {
            node->getNode()->activate(NodesList(), true, false);
            nodesToSelect.push_back(node);
        }
    }
    _graph->setSelection(nodesToSelect);
    _group.lock()->getNode()->deactivate(NodesList(),
                                         true,
                                         false,
                                         true,
                                         true,
                                         false);

    _isRedone = false;
}

void
GroupFromSelectionCommand::redo()
{
    // The group position will be at the centroid of all selected nodes
    QPointF groupPosition;

    NodesGuiList originalNodes;
    for (std::list<boost::weak_ptr<NodeGui> >::const_iterator it = _originalNodes.begin(); it != _originalNodes.end(); ++it) {
        NodeGuiPtr n = it->lock();
        if (!n) {
            continue;
        }
        originalNodes.push_back(n);

        QPointF nodePos = n->pos();
        groupPosition += nodePos;
    }

    unsigned sz = originalNodes.size();
    if (sz) {
        groupPosition.rx() /= sz;
        groupPosition.ry() /= sz;
    }

    NodeGroupPtr isGrp;
    if (_firstRedoCalled) {
        isGrp = toNodeGroup(_group.lock()->getNode()->getEffectInstance());
        assert(isGrp);
        if (!isGrp) {
            throw std::logic_error("GroupFromSelectionCommand::redo()");
        }
    } else {
        NodesList internalNewNodes;
        // Create the actual Group node
        CreateNodeArgsPtr groupArgs(CreateNodeArgs::create(PLUGINID_NATRON_GROUP, _graph->getGroup() ));
        groupArgs->setProperty<bool>(kCreateNodeArgsPropNodeGroupDisableCreateInitialNodes, true);
        groupArgs->setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
        groupArgs->setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
        groupArgs->setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);

        NodePtr containerNode = _graph->getGui()->getApp()->createNode(groupArgs);

        isGrp = toNodeGroup( containerNode->getEffectInstance()->shared_from_this() );
        assert(isGrp);
        if (!isGrp) {
            throw std::logic_error("GroupFromSelectionCommand::redo()");
        }
        NodeGuiIPtr container_i = containerNode->getNodeGui();
        assert(container_i);
        _group = boost::dynamic_pointer_cast<NodeGui>(container_i);
        assert( _group.lock() );

        // Set the position of the group
        container_i->setPosition( groupPosition.x(), groupPosition.y() );

        // Copy all the selected nodes and paste them in the newly created Group
        std::list<std::pair<std::string, NodeGuiPtr > > newNodes;
        _graph->copyNodesAndCreateInGroup(originalNodes, isGrp, newNodes);

        for (std::list<std::pair<std::string, NodeGuiPtr > >::iterator it = newNodes.begin(); it != newNodes.end(); ++it) {
            internalNewNodes.push_back( it->second->getNode() );
        }

        // Extract all trees from these new nodes
        std::list<Project::NodesTree> trees;
        Project::extractTreesFromNodes(internalNewNodes, trees);
        bool hasCreatedOutput = false;
        int inputNb = 0;
        for (std::list<Project::NodesTree>::iterator it = trees.begin(); it != trees.end(); ++it) {
            NodeGuiPtr foundOriginalOutputNode;
            for (NodesGuiList::const_iterator it3 = originalNodes.begin(); it3 != originalNodes.end(); ++it3) {
                if ( (*it3)->getNode()->getScriptName_mt_safe() == it->output.node->getScriptName_mt_safe() ) {
                    foundOriginalOutputNode = *it3;
                    break;
                }
            }
            assert(foundOriginalOutputNode);
            if (!foundOriginalOutputNode) {
                continue;
            }

            // For each input node of each tree branch within the group, add a Input node in input of that branch
            // to actually create the input on the Group node
            for (std::list<Project::TreeInput>::iterator it2 = it->inputs.begin(); it2 != it->inputs.end(); ++it2) {
                // Find the equivalent node in the original nodes and see which inputs we need to create
                NodeGuiPtr foundOriginalNode;
                for (NodesGuiList::const_iterator it3 = originalNodes.begin(); it3 != originalNodes.end(); ++it3) {
                    if ( (*it3)->getNode()->getScriptName_mt_safe() == it2->node->getScriptName_mt_safe() ) {
                        foundOriginalNode = *it3;
                        break;
                    }
                }
                assert(foundOriginalNode);
                if (!foundOriginalNode) {
                    continue;
                }

                // For each connected input of the original node, create a corresponding Input node with the appropriate name
                NodePtr originalNodeInternal = foundOriginalNode->getNode();
                const std::vector<NodeWPtr >& originalNodeInputs = originalNodeInternal->getInputs();
                for (std::size_t i = 0; i < originalNodeInputs.size(); ++i) {
                    NodePtr originalInput = originalNodeInputs[i].lock();
                    if (originalInput) {
                        //Create an input node corresponding to this input
                        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_INPUT, isGrp));
                        args->setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
                        args->setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
                        args->setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);


                        NodePtr input = _graph->getGui()->getApp()->createNode(args);
                        assert(input);
                        std::string inputLabel = originalNodeInternal->getLabel() + '_' + originalNodeInternal->getInputLabel(i);
                        input->setLabel(inputLabel);

                        // Position the input node correctly
                        double offsetX, offsetY;
                        {
                            double inputX, inputY;
                            originalInput->getPosition(&inputX, &inputY);
                            double originalX, originalY;
                            foundOriginalNode->getPosition(&originalX, &originalY);
                            offsetX = inputX - originalX;
                            offsetY = inputY - originalY;
                        }
                        double thisInputX, thisInputY;
                        it2->node->getPosition(&thisInputX, &thisInputY);

                        thisInputX += offsetX;
                        thisInputY += offsetY;

                        input->setPosition(thisInputX, thisInputY);

                        it2->node->connectInput(input, i);

                        isGrp->getNode()->connectInput(originalInput, inputNb);

                        ++inputNb;
                    }
                } // for all node's inputs
            } // for all inputs in the tree
            //Create only a single output

            if (!hasCreatedOutput) {
                CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_OUTPUT, isGrp));
                args->setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
                args->setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
                args->setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
                NodePtr output = _graph->getGui()->getApp()->createNode(args);
       
                assert(output);

                double thisNodeX, thisNodeY;
                it->output.node->getPosition(&thisNodeX, &thisNodeY);
                double thisNodeW, thisNodeH;
                it->output.node->getSize(&thisNodeW, &thisNodeH);

                thisNodeY += thisNodeH * 2;

                output->setPosition(thisNodeX, thisNodeY);

                output->connectInput(it->output.node, 0);

                hasCreatedOutput = true;
            }
            // Connect all outputs of the original node to the new Group
            NodesWList originalOutputs;
            foundOriginalOutputNode->getNode()->getOutputs_mt_safe(originalOutputs);
            for (NodesWList::const_iterator it2 = originalOutputs.begin(); it2 != originalOutputs.end(); ++it2) {
                NodePtr output = it2->lock();
                if (!output) {
                    continue;
                }
                int inputIdx = output->getInputIndex(foundOriginalOutputNode->getNode());
                assert(inputIdx != -1);
                output->swapInput(isGrp->getNode(), inputIdx);
                OutputLink& link = _outputLinks[*it2];
                link.inputNode = foundOriginalOutputNode->getNode();
                link.inputIdx = inputIdx;
            }
        } // for all trees
    } // !_firstRedoCalled

    if (_firstRedoCalled) {
        for (OutputLinksMap::iterator it = _outputLinks.begin(); it != _outputLinks.end(); ++it) {
            NodePtr outputNode = it->first.lock();
            if (!outputNode) {
                continue;
            }
            outputNode->swapInput(isGrp->getNode(), it->second.inputIdx);
        }
    }



    for (NodesGuiList::iterator it = originalNodes.begin(); it != originalNodes.end(); ++it) {
        (*it)->getNode()->deactivate(NodesList(),
                                     true,
                                     false,
                                     true,
                                     false,
                                     false);
    }

    std::list<NodeGuiPtr> nodesToSelect;
    NodeGuiPtr nodeGroup = _group.lock();
    if (_firstRedoCalled && nodeGroup) {
        nodeGroup->getNode()->activate(NodesList(), true, false);
    }
    if (nodeGroup) {
        nodesToSelect.push_back(nodeGroup);
    }
    _graph->setSelection(nodesToSelect);


    std::list<ViewerInstancePtr> viewers;
    _graph->getGroup()->getViewers(&viewers);
    for (std::list<ViewerInstancePtr>::iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }
    NodeGraphI* graph_i = isGrp->getNodeGraph();
    assert(graph_i);
    NodeGraph* graph = dynamic_cast<NodeGraph*>(graph_i);
    assert(graph);
    if (graph) {
        graph->centerOnAllNodes();
    }
    _firstRedoCalled = true;
    _isRedone = true;
}

InlineGroupCommand::InlineGroupCommand(NodeGraph* graph,
                                       const std::list<NodeGuiPtr > & groupNodes)
    : QUndoCommand()
    , _graph(graph)
    , _groupNodes()
    , _firstRedoCalled(false)
{
    setText( tr("Inline group(s)") );

    for (std::list<NodeGuiPtr >::const_iterator it = groupNodes.begin(); it != groupNodes.end(); ++it) {
        NodeGroupPtr group = (*it)->getNode()->isEffectNodeGroup();
        assert(group);
        if (!group) {
            continue;
        }
        InlinedGroup expandedGroup;

        /*
         * First-off copy all the nodes within the group, except the Inputs and Ouput nodes
         */
        SERIALIZATION_NAMESPACE::NodeClipBoard cb;
        NodesList nodes = group->getNodes();
        std::vector<NodePtr> groupInputs;
        NodePtr groupOutput = group->getOutputNode();
        group->getInputs(&groupInputs);

        std::list<NodeGuiPtr > nodesToCopy;
        for (NodesList::iterator it2 = nodes.begin(); it2 != nodes.end(); ++it2) {
            GroupInputPtr inp = (*it2)->isEffectGroupInput();
            GroupOutputPtr output = (*it2)->isEffectGroupOutput();
            if ( !inp && !output) {
                NodeGuiIPtr gui_i = (*it2)->getNodeGui();
                assert(gui_i);
                NodeGuiPtr nodeGui = boost::dynamic_pointer_cast<NodeGui>(gui_i);
                assert(nodeGui);
                nodesToCopy.push_back(nodeGui);
            }
        }

        {
            NodeGraphI *graph_i = group->getNodeGraph();
            assert(graph_i);
            {
                NodeGraph* thisGroupGraph = dynamic_cast<NodeGraph*>(graph_i);
                assert(thisGroupGraph);
                if (thisGroupGraph) {
                    thisGroupGraph->copyNodes(nodesToCopy, cb);
                }
            }
        }
        std::list<std::pair<std::string, NodeGuiPtr > > newNodes;
        _graph->pasteCliboard(cb, &newNodes);

        NodeGuiIPtr groupGui_i = group->getNode()->getNodeGui();
        assert(groupGui_i);
        NodeGuiPtr groupGui = boost::dynamic_pointer_cast<NodeGui>(groupGui_i);
        assert(groupGui);
        expandedGroup.group = groupGui;

        QPointF groupNodePos = groupGui->mapToScene( groupGui->mapFromParent( groupGui->pos() ) );

        //This is the BBox of the new inlined nodes
        RectD bbox;
        bbox.setupInfinity();
        for (std::list<std::pair<std::string, NodeGuiPtr > >::iterator it2 = newNodes.begin();
             it2 != newNodes.end(); ++it2) {
            QPointF p = it2->second->mapToScene( it2->second->mapFromParent( it2->second->pos() ) );
            bbox.x1 = std::min( bbox.x1, p.x() );
            bbox.x2 = std::max( bbox.x2, p.x() );

            //Qt coord system is top down
            bbox.y1 = std::min( bbox.y1, p.y() );
            bbox.y2 = std::max( bbox.y2, p.y() );
            expandedGroup.inlinedNodes.push_back(it2->second);
        }

        QPointF bboxCenter( (bbox.x1 + bbox.x2) / 2., (bbox.y1 + bbox.y2) / 2. );
        // Remember the links from the Group node we are expending to its inputs and outputs

        // This is the y coord. of the bottom-most input
        double inputY = INT_MIN;
        int maxInputs = group->getNode()->getMaxInputCount();
        assert( maxInputs == (int)groupInputs.size() );
        for (int i = 0; i < maxInputs; ++i) {
            NodePtr input = group->getNode()->getInput(i);
            if (input) {
                NodeToConnect ntc;

                assert(groupInputs[i]);
                std::map<NodePtr, int> outputConnected;
                groupInputs[i]->getOutputsConnectedToThisNode(&outputConnected);

                NodeGuiIPtr inputGui_i = input->getNodeGui();
                assert(inputGui_i);
                NodeGuiPtr inputGui = boost::dynamic_pointer_cast<NodeGui>(inputGui_i);
                assert(inputGui);
                ntc.input = inputGui;

                QPointF p = inputGui->mapToScene( inputGui->mapFromParent( inputGui->pos() ) );
                if (p.y() > inputY) {
                    inputY = p.y();
                }

                for (std::map<NodePtr, int>::iterator it2 = outputConnected.begin(); it2 != outputConnected.end(); ++it2) {
                    NodeGuiPtr outputGui;
                    ///Find the new node that was inlined, based on the script name of the old node in the group
                    for (std::list<std::pair<std::string, NodeGuiPtr > >::iterator it3 = newNodes.begin(); it3 != newNodes.end(); ++it3) {
                        if ( it3->first == it2->first->getScriptName() ) {
                            outputGui = it3->second;
                            break;
                        }
                    }
                    assert(outputGui);
                    ntc.outputs.insert( std::make_pair(outputGui, it2->second) );
                }

                expandedGroup.connections[i] = ntc;
            }
        }

        std::list<NodeGuiPtr > outputsConnectedToGroup;
        QPointF firstInputPos;

        // This is the y coord of the top most output
        double outputY = INT_MAX;
        if (groupOutput) {
            NodePtr groupOutputInput = groupOutput->getInput(0);
            if (groupOutputInput) {
                NodeToConnect outputConnection;
                NodeGuiPtr inputGui;
                ///Find the new node that was inlined, based on the script name of the old node in the group
                for (std::list<std::pair<std::string, NodeGuiPtr > >::iterator it3 = newNodes.begin(); it3 != newNodes.end(); ++it3) {
                    if ( it3->first == groupOutputInput->getScriptName() ) {
                        inputGui = it3->second;
                        break;
                    }
                }

                if (!inputGui) {
                    continue;
                }
                firstInputPos = inputGui->mapToScene( inputGui->mapFromParent( inputGui->pos() ) );
                outputConnection.input = inputGui;

                std::map<NodePtr, int> outputConnected;
                group->getNode()->getOutputsConnectedToThisNode(&outputConnected);
                for (std::map<NodePtr, int>::iterator it2 = outputConnected.begin(); it2 != outputConnected.end(); ++it2) {
                    NodeGuiIPtr outputGui_i = it2->first->getNodeGui();
                    assert(outputGui_i);
                    NodeGuiPtr outputGui = boost::dynamic_pointer_cast<NodeGui>(outputGui_i);
                    assert(outputGui);
                    outputsConnectedToGroup.push_back(outputGui);

                    QPointF p = outputGui->mapToScene( outputGui->mapFromParent( outputGui->pos() ) );
                    if (p.y() < outputY) {
                        outputY = p.y();
                    }
                    outputConnection.outputs.insert( std::make_pair(outputGui, it2->second) );
                }
                expandedGroup.connections[-1] = outputConnection;
            }
        }

        // If there is no output to the group, the output is considered to be infinite (so we don't move any node)
        if (outputY == INT_MIN) {
            outputY = INT_MAX;
        }

        const double ySpaceAvailable = outputY  - inputY;
        const double ySpaceNeeded = bbox.y2 - bbox.y1 + TO_DPIX(100);

        //  Move recursively the outputs of the group nodes so that it does not overlap the inlining of the group
        QRectF rectToClear(bbox.x1, bbox.y1, bbox.x2 - bbox.x1, ySpaceNeeded - ySpaceAvailable);
        for (std::list<NodeGuiPtr >::iterator it2 = outputsConnectedToGroup.begin();
             it2 != outputsConnectedToGroup.end(); ++it2) {
            (*it2)->moveBelowPositionRecursively(rectToClear);
        }


        // Move all created nodes by this delta to fit in the space we've just made
        for (std::list<std::pair<std::string, NodeGuiPtr > >::iterator it2 = newNodes.begin();
             it2 != newNodes.end(); ++it2) {
            QPointF p = it2->second->mapToScene( it2->second->mapFromParent( it2->second->pos() ) );
            QPointF delta = p - bboxCenter;
            p = groupNodePos + delta;
            p = it2->second->mapToParent( it2->second->mapFromScene(p) );
            it2->second->setPosition( p.x(), p.y() );
        }


        _groupNodes.push_back(expandedGroup);
    } // for (std::list<NodeGuiPtr >::const_iterator it = groupNodes.begin(); it != groupNodes.end(); ++it) {
}

InlineGroupCommand::~InlineGroupCommand()
{
}

void
InlineGroupCommand::undo()
{
    std::set<ViewerInstancePtr> viewers;
    std::list<NodeGuiPtr> nodesToSelect;

    for (std::list<InlinedGroup>::iterator it = _groupNodes.begin(); it != _groupNodes.end(); ++it) {
        NodeGuiPtr groupNode = it->group.lock();
        if (groupNode) {
            groupNode->getNode()->activate(NodesList(), true, false);
            std::list<ViewerInstancePtr> connectedViewers;
            groupNode->getNode()->hasViewersConnected(&connectedViewers);
            for (std::list<ViewerInstancePtr>::iterator it2 = connectedViewers.begin(); it2 != connectedViewers.end(); ++it2) {
                viewers.insert(*it2);
            }
            for (std::list<boost::weak_ptr<NodeGui> >::iterator it2 = it->inlinedNodes.begin();
                 it2 != it->inlinedNodes.end(); ++it2) {
                NodeGuiPtr node = (*it2).lock();
                if (node) {
                    node->getNode()->deactivate(NodesList(), false, false, true, false, false);
                }
            }
        }
        nodesToSelect.push_back(groupNode);
    }
    _graph->setSelection(nodesToSelect);
    for (std::set<ViewerInstancePtr>::iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }

    _graph->getGui()->getApp()->triggerAutoSave();
}

void
InlineGroupCommand::redo()
{
    std::set<ViewerInstancePtr> viewers;
    std::list<NodeGuiPtr> nodesToSelect;

    for (std::list<InlinedGroup>::iterator it = _groupNodes.begin(); it != _groupNodes.end(); ++it) {
        NodeGuiPtr groupNode = it->group.lock();
        if (groupNode) {
            std::list<ViewerInstancePtr> connectedViewers;
            groupNode->getNode()->hasViewersConnected(&connectedViewers);
            for (std::list<ViewerInstancePtr>::iterator it2 = connectedViewers.begin(); it2 != connectedViewers.end(); ++it2) {
                viewers.insert(*it2);
            }
            groupNode->getNode()->deactivate(NodesList(), true, false, true, false, false);
            for (std::list<boost::weak_ptr<NodeGui> >::iterator it2 = it->inlinedNodes.begin();
                 it2 != it->inlinedNodes.end(); ++it2) {
                NodeGuiPtr node = (*it2).lock();
                if (node) {
                    if  (_firstRedoCalled) {
                        node->getNode()->activate(NodesList(), false, false);
                    }
                    nodesToSelect.push_back(node);
                }
            }

            for (std::map<int, NodeToConnect>::iterator it2 = it->connections.begin(); it2 != it->connections.end(); ++it2) {
                NodeGuiPtr input = it2->second.input.lock();
                if (!input) {
                    continue;
                }
                for (std::map<boost::weak_ptr<NodeGui>, int>::iterator it3 = it2->second.outputs.begin();
                     it3 != it2->second.outputs.end(); ++it3) {
                    NodeGuiPtr node = it3->first.lock();
                    if (node) {
                        node->getNode()->disconnectInput(it3->second);
                        NodeCollection::connectNodes(it3->second, input->getNode(), node->getNode(), false);
                    }
                }
            }
        }
    }
    _graph->setSelection(nodesToSelect);

    for (std::set<ViewerInstancePtr>::iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }
    _graph->getGui()->getApp()->triggerAutoSave();
    _firstRedoCalled = true;
}

RestoreNodeToDefaultCommand::RestoreNodeToDefaultCommand(const NodesGuiList & nodes)
: QUndoCommand()
, _nodes()
{
    for (NodesGuiList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodeDefaults d;
        d.node = *it;
        d.serialization.reset(new SERIALIZATION_NAMESPACE::NodeSerialization);
        (*it)->getNode()->toSerialization(d.serialization.get());
        _nodes.push_back(d);
    }
    setText(tr("Restore node(s) to default"));
}

RestoreNodeToDefaultCommand::~RestoreNodeToDefaultCommand()
{
    
}

void
RestoreNodeToDefaultCommand::undo()
{
    for (std::list<NodeDefaults>::const_iterator it = _nodes.begin(); it!=_nodes.end(); ++it) {
        NodeGuiPtr node = it->node.lock();
        if (!node) {
            continue;
        }
        NodePtr internalNode = node->getNode();
        if (!internalNode) {
            continue;
        }
        internalNode->loadKnobsFromSerialization(*it->serialization);
    }
}

void
RestoreNodeToDefaultCommand::redo()
{
    for (std::list<NodeDefaults>::const_iterator it = _nodes.begin(); it!=_nodes.end(); ++it) {
        NodeGuiPtr node = it->node.lock();
        if (!node) {
            continue;
        }
        NodePtr internalNode = node->getNode();
        if (!internalNode) {
            continue;
        }
        internalNode->restoreNodeToDefaultState(CreateNodeArgsPtr());
    }
}



NATRON_NAMESPACE_EXIT;
