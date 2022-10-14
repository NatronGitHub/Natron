/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

#include "NodeGraphUndoRedo.h"

#include <algorithm> // min, max
#include <limits>
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
#include "Engine/NodeSerialization.h"
#include "Engine/Project.h"
#include "Engine/RotoLayer.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewerInstance.h"

#include "Gui/NodeClipBoard.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeGraph.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/Edge.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/MultiInstancePanel.h"

#define MINIMUM_VERTICAL_SPACE_BETWEEN_NODES 10

NATRON_NAMESPACE_ENTER

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
        QPointF pos = (*it)->getPos_mt_safe();
        (*it)->setPosition(pos.x() + dx, pos.y() + dy);
    }
    setText( tr("Move node(s)") );
}

void
MoveMultipleNodesCommand::undo()
{
    move(-_dx, -_dy);
}

void
MoveMultipleNodesCommand::redo()
{
    if (_firstRedoCalled) {
        move(_dx, _dy);
    }
    _firstRedoCalled = true;
}

AddMultipleNodesCommand::AddMultipleNodesCommand(NodeGraph* graph,
                                                 const std::list<NodeGuiPtr> & nodes,
                                                 QUndoCommand *parent)
    : QUndoCommand(parent)
    , _nodes()
    , _graph(graph)
    , _firstRedoCalled(false)
    , _isUndone(false)
{
    for (std::list<NodeGuiPtr> ::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
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
        for (std::list<NodeGuiWPtr>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
            NodeGuiPtr node = it->lock();
            if (node) {
                node->getNode()->destroyNode(false, false);
            }
        }
    }
    setText( tr("Add node") );
}

void
AddMultipleNodesCommand::undo()
{
    _isUndone = true;
    std::list<ViewerInstance*> viewersToRefresh;


    for (std::list<NodeGuiWPtr>::const_iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        NodeGuiPtr node = it->lock();
        std::list<ViewerInstance* > viewers;
        node->getNode()->hasViewersConnected(&viewers);
        for (std::list<ViewerInstance* >::iterator it2 = viewers.begin(); it2 != viewers.end(); ++it2) {
            std::list<ViewerInstance*>::iterator foundViewer = std::find(viewersToRefresh.begin(), viewersToRefresh.end(), *it2);
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

    for (std::list<ViewerInstance* >::iterator it = viewersToRefresh.begin(); it != viewersToRefresh.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }
    _graph->updateNavigator();
}

void
AddMultipleNodesCommand::redo()
{
    _isUndone = false;
    std::list<ViewerInstance*> viewersToRefresh;
    std::list<NodeGuiPtr> nodes;
    for (std::list<NodeGuiWPtr>::const_iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        nodes.push_back( it->lock() );
    }
    if (_firstRedoCalled) {
        for (std::list<NodeGuiPtr> ::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
            (*it)->getNode()->activate(NodesList(), //inputs to restore
                                       true, //restore all inputs ?
                                       false); //triggerRender
        }
    }


    if ( (nodes.size() != 1) || !nodes.front()->getNode()->isEffectGroup() ) {
        _graph->setSelection(nodes);
    }

    _graph->getGui()->getApp()->recheckInvalidExpressions();
    _graph->getGui()->getApp()->triggerAutoSave();

    for (std::list<NodeGuiPtr> ::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        std::list<ViewerInstance* > viewers;
        (*it)->getNode()->hasViewersConnected(&viewers);
        for (std::list<ViewerInstance* >::iterator it2 = viewers.begin(); it2 != viewers.end(); ++it2) {
            std::list<ViewerInstance*>::iterator foundViewer = std::find(viewersToRefresh.begin(), viewersToRefresh.end(), *it2);
            if ( foundViewer == viewersToRefresh.end() ) {
                viewersToRefresh.push_back(*it2);
            }
        }
    }


    for (std::list<ViewerInstance* >::iterator it = viewersToRefresh.begin(); it != viewersToRefresh.end(); ++it) {
        if ( (*it)->getUiContext() ) {
            (*it)->renderCurrentFrame(true);
        }
    }

    _firstRedoCalled = true;
    _graph->updateNavigator();
}

RemoveMultipleNodesCommand::RemoveMultipleNodesCommand(NodeGraph* graph,
                                                       const std::list<NodeGuiPtr> & nodes,
                                                       QUndoCommand *parent)
    : QUndoCommand(parent)
    , _nodes()
    , _graph(graph)
    , _isRedone(false)
{
    for (std::list<NodeGuiPtr> ::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodeToRemove n;
        n.node = *it;

        ///find all outputs to restore
        const NodesWList & outputs = (*it)->getNode()->getGuiOutputs();
        for (NodesWList::const_iterator it2 = outputs.begin(); it2 != outputs.end(); ++it2) {
            NodePtr output = it2->lock();
            if (!output) {
                continue;
            }
            bool restore = true;
            for (std::list<NodeGuiPtr> ::const_iterator it3 = nodes.begin(); it3 != nodes.end(); ++it3) {
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
    setText( tr("Remove node(s)") );
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
    std::list<ViewerInstance*> viewersToRefresh;
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
        if ( node->isSettingsPanelVisible() ) {
            node->getNode()->showKeyframesOnTimeline( next == _nodes.end() );
        }
        std::list<ViewerInstance* > viewers;
        node->getNode()->hasViewersConnected(&viewers);
        for (std::list<ViewerInstance* >::iterator it2 = viewers.begin(); it2 != viewers.end(); ++it2) {
            std::list<ViewerInstance*>::iterator foundViewer = std::find(viewersToRefresh.begin(), viewersToRefresh.end(), *it2);
            if ( foundViewer == viewersToRefresh.end() ) {
                viewersToRefresh.push_back(*it2);
            }
        }

        // increment for next iteration
        if ( next != _nodes.end() ) {
            ++next;
        }
    } // for(it)
    for (std::list<ViewerInstance* >::iterator it = viewersToRefresh.begin(); it != viewersToRefresh.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }
    _graph->getGui()->getApp()->triggerAutoSave();
    _graph->getGui()->getApp()->redrawAllViewers();
    _graph->updateNavigator();

    _isRedone = false;
    _graph->scene()->update();
    _graph->updateNavigator();
} // RemoveMultipleNodesCommand::undo

void
RemoveMultipleNodesCommand::redo()
{
    _isRedone = true;

    std::list<ViewerInstance*> viewersToRefresh;
    std::list<NodeToRemove>::iterator next = _nodes.begin();
    if ( next != _nodes.end() ) {
        ++next;
    }
    for (std::list<NodeToRemove>::iterator it = _nodes.begin();
         it != _nodes.end();
         ++it) {
        NodeGuiPtr node = it->node.lock();
        ///Make a copy before calling deactivate which will modify the list
        NodesWList outputs = node->getNode()->getGuiOutputs();
        std::list<ViewerInstance* > viewers;
        node->getNode()->hasViewersConnected(&viewers);
        for (std::list<ViewerInstance* >::iterator it2 = viewers.begin(); it2 != viewers.end(); ++it2) {
            std::list<ViewerInstance*>::iterator foundViewer = std::find(viewersToRefresh.begin(), viewersToRefresh.end(), *it2);
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
                    InspectorNode* inspector = dynamic_cast<InspectorNode*>( output.get() );
                    ///if the node is an inspector, when disconnecting the active input just activate another input instead
                    if (inspector) {
                        const std::vector<NodeWPtr> & inputs = inspector->getGuiInputs();
                        ///set as active input the first non null input
                        for (std::size_t i = 0; i < inputs.size(); ++i) {
                            NodePtr input = inputs[i].lock();
                            if (input) {
                                inspector->setActiveInputAndRefresh(i, true);
                                ///make sure we don't refresh it a second time
                                std::list<ViewerInstance*>::iterator foundViewer =
                                    std::find( viewersToRefresh.begin(), viewersToRefresh.end(), inspector->isEffectViewer() );
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
        if ( node->isSettingsPanelVisible() ) {
            node->getNode()->hideKeyframesFromTimeline( next == _nodes.end() );
        }

        // increment for next iteration
        if ( next != _nodes.end() ) {
            ++next;
        }
    } // for(it)

    for (std::list<ViewerInstance* >::iterator it = viewersToRefresh.begin(); it != viewersToRefresh.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }

    _graph->getGui()->getApp()->triggerAutoSave();
    _graph->getGui()->getApp()->redrawAllViewers();
    _graph->updateNavigator();

    _graph->scene()->update();
    _graph->updateNavigator();
} // redo

ConnectCommand::ConnectCommand(NodeGraph* graph,
                               Edge* edge,
                               const NodeGuiPtr & oldSrc,
                               const NodeGuiPtr & newSrc,
                               QUndoCommand *parent)
    : QUndoCommand(parent),
    _oldSrc(oldSrc),
    _newSrc(newSrc),
    _dst( edge->getDest() ),
    _graph(graph),
    _inputNb( edge->getInputNumber() )
{
    NodeGuiPtr dst = _dst.lock();
    assert(dst);
    if (newSrc) {
        setText( tr("Connect %1 to %2")
                 .arg( QString::fromUtf8( dst->getNode()->getLabel().c_str() ) ).arg( QString::fromUtf8( newSrc->getNode()->getLabel().c_str() ) ) );
    } else {
        setText( tr("Disconnect %1")
                 .arg( QString::fromUtf8( dst->getNode()->getLabel().c_str() ) ) );
    }
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
              _inputNb);

    ViewerInstance* isDstAViewer = dst->getNode()->isEffectViewer();
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
              _inputNb);

    ViewerInstance* isDstAViewer = dst->getNode()->isEffectViewer();
    if (!isDstAViewer) {
        _graph->getGui()->getApp()->triggerAutoSave();
    }
    _graph->update();
} // redo

void
ConnectCommand::doConnect(const NodeGuiPtr &oldSrc,
                          const NodeGuiPtr &newSrc,
                          const NodeGuiPtr& dst,
                          int inputNb)
{
    NodePtr internalDst =  dst->getNode();
    NodePtr internalNewSrc = newSrc ? newSrc->getNode() : NodePtr();
    NodePtr internalOldSrc = oldSrc ? oldSrc->getNode() : NodePtr();
    ViewerInstance* isViewer = internalDst->isEffectViewer();


    if (isViewer) {
        ///if the node is an inspector  disconnect any current connection between the inspector and the _newSrc
        for (int i = 0; i < internalDst->getNInputs(); ++i) {
            if ( (i != inputNb) && (internalDst->getInput(i) == internalNewSrc) ) {
                internalDst->disconnectInput(i);
            }
        }
    }
    if (internalOldSrc && internalNewSrc) {
        internalDst->replaceInput(internalNewSrc, inputNb);
    } else {
        if (internalOldSrc && internalNewSrc) {
            Node::CanConnectInputReturnValue ret = internalDst->canConnectInput(internalNewSrc, inputNb);
            bool connectionOk = ret == Node::eCanConnectInput_ok ||
                                ret == Node::eCanConnectInput_differentFPS ||
                                ret == Node::eCanConnectInput_differentPars;
            if (connectionOk) {
                internalDst->replaceInput(internalNewSrc, inputNb);
            } else {
                internalDst->disconnectInput( internalDst->getInputIndex( internalOldSrc.get() ) );
            }
        } else if (internalOldSrc && !internalNewSrc) {
            internalDst->disconnectInput( internalDst->getInputIndex( internalOldSrc.get() ) );
        } else if (!internalOldSrc && internalNewSrc) {
            Node::CanConnectInputReturnValue ret = internalDst->canConnectInput(internalNewSrc, inputNb);
            bool connectionOk = ret == Node::eCanConnectInput_ok ||
                                ret == Node::eCanConnectInput_differentFPS ||
                                ret == Node::eCanConnectInput_differentPars;
            if (connectionOk) {
                internalDst->connectInput(internalNewSrc, inputNb);
            } else {
                internalDst->disconnectInput( internalDst->getInputIndex( internalOldSrc.get() ) );
            }
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
    : ConnectCommand(graph, edge, edge->getSource(), newSrc, parent)
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

    doConnect(newSrc, oldSrc, dst, _inputNb);

    if (_inputEdge) {
        doConnect( _inputEdge->getSource(), NodeGuiPtr(), _inputEdge->getDest(), _inputEdge->getInputNumber() );
    }

    ViewerInstance* isDstAViewer = dst->getNode()->isEffectViewer();
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

    doConnect(oldSrc, newSrc, dst, _inputNb);


    ///find out if the node is already connected to what the edge is connected
    bool alreadyConnected = false;
    const std::vector<NodeWPtr> & inpNodes = newSrcInternal->getGuiInputs();
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
        ///push a second command... this is a bit dirty but I don't have time to add a whole new command just for this
        int prefInput = newSrcInternal->getPreferredInputForConnection();
        if (prefInput != -1) {
            _inputEdge = newSrc->getInputArrow(prefInput);
            assert(_inputEdge);
            doConnect( _inputEdge->getSource(), oldSrc, _inputEdge->getDest(), _inputEdge->getInputNumber() );
        }
    }

    ViewerInstance* isDstAViewer = dst->getNode()->isEffectViewer();
    if (!isDstAViewer) {
        _graph->getGui()->getApp()->triggerAutoSave();
    }

    newSrcInternal->endInputEdition(false);
    dstInternal->endInputEdition(false);

    std::list<ViewerInstance* > viewers;
    dstInternal->hasViewersConnected(&viewers);
    for (std::list<ViewerInstance* >::iterator it2 = viewers.begin(); it2 != viewers.end(); ++it2) {
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
    setText( tr("Resize %1").arg( QString::fromUtf8( _bd->getNode()->getLabel().c_str() ) ) );
}

ResizeBackdropCommand::~ResizeBackdropCommand()
{
}

void
ResizeBackdropCommand::undo()
{
    _bd->resize(_oldW, _oldH);
}

void
ResizeBackdropCommand::redo()
{
    _bd->resize(_w, _h);
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
                                                         const std::list<NodeGuiPtr> & nodes,
                                                         QUndoCommand *parent)
    : QUndoCommand(parent)
    , _nodes()
    , _graph(graph)
{
    for (std::list<NodeGuiPtr> ::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodeToDeclone n;
        n.node = *it;
        n.master = (*it)->getNode()->getMasterNode();
        assert( n.master.lock() );
        _nodes.push_back(n);
    }
    setText( tr("Declone node(s)") );
}

DecloneMultipleNodesCommand::~DecloneMultipleNodesCommand()
{
}

void
DecloneMultipleNodesCommand::undo()
{
    for (std::list<NodeToDeclone>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->node.lock()->getNode()->getEffectInstance()->slaveAllKnobs( it->master.lock()->getEffectInstance().get(), false );
    }

    _graph->getGui()->getApp()->triggerAutoSave();
}

void
DecloneMultipleNodesCommand::redo()
{
    for (std::list<NodeToDeclone>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->node.lock()->getNode()->getEffectInstance()->unslaveAllKnobs();
    }

    _graph->getGui()->getApp()->triggerAutoSave();
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
        , topLevelNodeCenter(0, std::numeric_limits<int>::max())
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

typedef std::shared_ptr<Tree> TreePtr;
typedef std::list<TreePtr> TreePtrList;

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
            ///Note that here we subsctract nodeSize.width(): Actually we subtract twice nodeSize.width() / 2: once to get to the left of the node
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
hasNodeOutputsInList(const std::list<NodeGuiPtr> & nodes,
                     const NodeGuiPtr& node)
{
    const NodesWList& outputs = node->getNode()->getGuiOutputs();
    bool foundOutput = false;

    for (std::list<NodeGuiPtr> ::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
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
hasNodeInputsInList(const std::list<NodeGuiPtr> & nodes,
                    const NodeGuiPtr& node)
{
    const std::vector<NodeWPtr>& inputs = node->getNode()->getGuiInputs();
    bool foundInput = false;

    for (std::list<NodeGuiPtr> ::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
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


RearrangeNodesCommand::RearrangeNodesCommand(const std::list<NodeGuiPtr> & nodes,
                                             QUndoCommand *parent)
    : QUndoCommand(parent)
    , _nodes()
{
    ///1) Separate the nodes in trees they belong to, once a node has been "used" by a tree, mark it
    ///and don't try to reposition it for another tree
    ///2) For all trees : recursively position each nodes so that each input of a node is positioned as following:
    /// a) The first non mask input is positioned above the node
    /// b) All others non mask inputs are positioned on the left of the node, each one separated by the space of half a node
    /// c) All masks are positioned on the right of the node, each one separated by the space of half a node
    ///3) Move all trees so that they are next to each other and their "top level" node
    ///(the input that is at the highest position in the Y coordinate) is at the same
    ///Y level (node centers have the same Y)

    std::list<NodeGui*> usedNodes;

    ///A list of Tree
    ///Each tree is a lit of nodes with a boolean indicating if it was already positioned( "used" ) by another tree, if set to
    ///true we don't do anything
    /// Each node that doesn't have any output is a potential tree.
    TreePtrList trees;

    for (std::list<NodeGuiPtr> ::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( !hasNodeOutputsInList( nodes, (*it) ) ) {
            TreePtr newTree = std::make_shared<Tree>();
            newTree->buildTree(*it, nodes, usedNodes);
            trees.push_back(newTree);
        }
    }

    ///For all trees find out which one has the top most level node
    QPointF topLevelPos(0, std::numeric_limits<int>::max());
    for (TreePtrList::iterator it = trees.begin(); it != trees.end(); ++it) {
        const QPointF & treeTop = (*it)->getTopLevelNodeCenter();
        if ( treeTop.y() < topLevelPos.y() ) {
            topLevelPos = treeTop;
        }
    }

    ///now offset all trees to be top aligned at the same level
    for (TreePtrList::iterator it = trees.begin(); it != trees.end(); ++it) {
        QPointF treeTop = (*it)->getTopLevelNodeCenter();
        if (treeTop.y() == std::numeric_limits<int>::max()) {
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
    setText( tr("Rearrange nodes") );
}

void
RearrangeNodesCommand::undo()
{
    for (std::list<NodeToRearrange>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->node->refreshPosition(it->oldPos.x(), it->oldPos.y(), true);
    }
}

void
RearrangeNodesCommand::redo()
{
    for (std::list<NodeToRearrange>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->node->refreshPosition(it->newPos.x(), it->newPos.y(), true);
    }
}

DisableNodesCommand::DisableNodesCommand(const std::list<NodeGuiPtr> & nodes,
                                         QUndoCommand *parent)
    : QUndoCommand(parent)
    , _nodes()
{
    for (std::list<NodeGuiPtr> ::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        _nodes.push_back(*it);
    }
    setText( tr("Disable node(s)") );
}

void
DisableNodesCommand::undo()
{
    for (std::list<NodeGuiWPtr>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->lock()->getNode()->setNodeDisabled(false);
    }
}

void
DisableNodesCommand::redo()
{
    for (std::list<NodeGuiWPtr>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->lock()->getNode()->setNodeDisabled(true);
    }
}

EnableNodesCommand::EnableNodesCommand(const std::list<NodeGuiPtr> & nodes,
                                       QUndoCommand *parent)
    : QUndoCommand(parent)
    , _nodes()
{
    for (std::list<NodeGuiPtr> ::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        _nodes.push_back(*it);
    }
    setText( tr("Enable node(s)") );
}

void
EnableNodesCommand::undo()
{
    for (std::list<NodeGuiWPtr>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->lock()->getNode()->setNodeDisabled(true);
    }
}

void
EnableNodesCommand::redo()
{
    for (std::list<NodeGuiWPtr>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->lock()->getNode()->setNodeDisabled(false);
    }
}

LoadNodePresetsCommand::LoadNodePresetsCommand(const NodeGuiPtr & node,
                                               const std::list<NodeSerializationPtr>& serialization,
                                               QUndoCommand *parent)
    : QUndoCommand(parent)
    , _firstRedoCalled(false)
    , _isUndone(false)
    , _node(node)
    , _newSerializations(serialization)
{
    setText( tr("Load presets") );
}

void
LoadNodePresetsCommand::getListAsShared(const std::list<NodeWPtr>& original,
                                        std::list<NodePtr>& shared) const
{
    for (std::list<NodeWPtr>::const_iterator it = original.begin(); it != original.end(); ++it) {
        shared.push_back( it->lock() );
    }
}

LoadNodePresetsCommand::~LoadNodePresetsCommand()
{
}

void
LoadNodePresetsCommand::undo()
{
    _isUndone = true;

    NodeGuiPtr node = _node.lock();
    NodePtr internalNode = node->getNode();
    MultiInstancePanelPtr panel = node->getMultiInstancePanel();
    internalNode->loadKnobs(*_oldSerialization.front(), true);
    if (panel) {
        std::list<NodePtr> newChildren, oldChildren;
        getListAsShared(_newChildren, newChildren);
        getListAsShared(_oldChildren, oldChildren);
        panel->removeInstances(newChildren);
        panel->addInstances(oldChildren);
    }
    internalNode->getEffectInstance()->incrHashAndEvaluate(true, true);
    internalNode->getApp()->triggerAutoSave();
}

void
LoadNodePresetsCommand::redo()
{
    NodeGuiPtr node = _node.lock();
    NodePtr internalNode = node->getNode();
    MultiInstancePanelPtr panel = node->getMultiInstancePanel();

    if (!_firstRedoCalled) {
        ///Serialize the current state of the node
        node->serializeInternal(_oldSerialization);

        if (panel) {
            const std::list<std::pair<NodeWPtr, bool> >& children = panel->getInstances();
            for (std::list<std::pair<NodeWPtr, bool> >::const_iterator it = children.begin();
                 it != children.end(); ++it) {
                _oldChildren.push_back( it->first.lock() );
            }
        }

        int k = 0;

        for (std::list<NodeSerializationPtr>::const_iterator it = _newSerializations.begin();
             it != _newSerializations.end(); ++it, ++k) {
            if (k > 0) {  /// this is a multi-instance child, create it
                NodePtr newNode = panel->createNewInstance(false);
                newNode->loadKnobs(**it);
                std::list<SequenceTime> keys;
                newNode->getAllKnobsKeyframes(&keys);
                newNode->getApp()->addMultipleKeyframeIndicatorsAdded(keys, true);
                _newChildren.push_back(newNode);
            }
        }
    }

    internalNode->loadKnobs(*_newSerializations.front(), true);
    if (panel) {
        std::list<NodePtr> oldChildren, newChildren;
        getListAsShared(_oldChildren, oldChildren);
        getListAsShared(_newChildren, newChildren);
        panel->removeInstances(oldChildren);
        if (_firstRedoCalled) {
            panel->addInstances(newChildren);
        }
    }

    NodesList allNodes;
    internalNode->getApp()->getProject()->getActiveNodesExpandGroups(&allNodes);

    std::map<std::string, std::string> oldNewScriptNames;
    internalNode->storeKnobsLinks(*_newSerializations.front(), oldNewScriptNames);
    internalNode->restoreKnobsLinks(allNodes, oldNewScriptNames, true); // may not fail
    internalNode->getEffectInstance()->incrHashAndEvaluate(true, true);
    internalNode->getApp()->triggerAutoSave();
    _firstRedoCalled = true;
} // LoadNodePresetsCommand::redo

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
addTreeInputs(const std::list<NodeGuiPtr> & nodes,
              const NodeGuiPtr& node,
              ExtractedTree& tree,
              std::list<NodeGuiPtr> & markedNodes)
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
        input.inputs = node->getNode()->getGuiInputs();
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
extractTreesFromNodes(const std::list<NodeGuiPtr> & nodes,
                      std::list<ExtractedTree>& trees)
{
    std::list<NodeGuiPtr> markedNodes;

    for (std::list<NodeGuiPtr> ::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        bool isOutput = !hasNodeOutputsInList(nodes, *it);
        if (isOutput) {
            ExtractedTree tree;
            tree.output.node = *it;
            NodePtr n = (*it)->getNode();
            const NodesWList& outputs = n->getGuiOutputs();
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
                input.inputs = n->getGuiInputs();
                tree.inputs.push_back(input);
            }

            trees.push_back(tree);
        }
    }
}

///////////////

ExtractNodeUndoRedoCommand::ExtractNodeUndoRedoCommand(NodeGraph* graph,
                                                       const std::list<NodeGuiPtr> & nodes)
    : QUndoCommand()
    , _graph(graph)
    , _trees()
{
    extractTreesFromNodes(nodes, _trees);
    setText( tr("Extract node") );
}

ExtractNodeUndoRedoCommand::~ExtractNodeUndoRedoCommand()
{
}

void
ExtractNodeUndoRedoCommand::undo()
{
    std::set<ViewerInstance* > viewers;

    for (std::list<ExtractedTree>::iterator it = _trees.begin(); it != _trees.end(); ++it) {
        NodeGuiPtr output = it->output.node.lock();
        ///Connect and move output
        for (std::list<std::pair<int, NodeWPtr> >::iterator it2 = it->output.outputs.begin(); it2 != it->output.outputs.end(); ++it2) {
            NodePtr node = it2->second.lock();
            if (!node) {
                continue;
            }
            node->disconnectInput(it2->first);
            node->connectInput(output->getNode(), it2->first);
        }

        QPointF curPos = output->getPos_mt_safe();
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
                QPointF curPos = input->getPos_mt_safe();
                input->refreshPosition(curPos.x() - 200, curPos.y(), true);
            }
        }

        ///Move all other nodes

        for (std::list<NodeGuiWPtr>::iterator it2 = it->inbetweenNodes.begin(); it2 != it->inbetweenNodes.end(); ++it2) {
            NodeGuiPtr node = it2->lock();
            QPointF curPos = node->getPos_mt_safe();
            node->refreshPosition(curPos.x() - 200, curPos.y(), true);
        }

        std::list<ViewerInstance* > tmp;
        output->getNode()->hasViewersConnected(&tmp);

        for (std::list<ViewerInstance* >::iterator it2 = tmp.begin(); it2 != tmp.end(); ++it2) {
            viewers.insert(*it2);
        }
    }

    for (std::set<ViewerInstance* >::iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }

    _graph->getGui()->getApp()->triggerAutoSave();
} // ExtractNodeUndoRedoCommand::undo

void
ExtractNodeUndoRedoCommand::redo()
{
    std::set<ViewerInstance* > viewers;

    for (std::list<ExtractedTree>::iterator it = _trees.begin(); it != _trees.end(); ++it) {
        std::list<ViewerInstance* > tmp;
        NodeGuiPtr output = it->output.node.lock();
        output->getNode()->hasViewersConnected(&tmp);

        for (std::list<ViewerInstance* >::iterator it2 = tmp.begin(); it2 != tmp.end(); ++it2) {
            viewers.insert(*it2);
        }

        bool outputsAlreadyDisconnected = false;

        ///Reconnect outputs to the input of the input of the ExtractedInputs if inputs.size() == 1
        if ( (it->output.outputs.size() == 1) && (it->inputs.size() == 1) ) {
            const ExtractedInput& selectedInput = it->inputs.front();
            const std::vector<NodeWPtr> &inputs = selectedInput.inputs;
            NodeGuiPtr selectedInputNode = selectedInput.node.lock();
            NodePtr inputToConnectTo;
            for (U32 i = 0; i < inputs.size(); ++i) {
                if ( inputs[i].lock() && !selectedInputNode->getNode()->getEffectInstance()->isInputOptional(i) &&
                     !selectedInputNode->getNode()->getEffectInstance()->isInputRotoBrush(i) ) {
                    inputToConnectTo = inputs[i].lock();
                    break;
                }
            }

            if (inputToConnectTo) {
                for (std::list<std::pair<int, NodeWPtr> >::iterator it2 = it->output.outputs.begin(); it2 != it->output.outputs.end(); ++it2) {
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
            for (std::list<std::pair<int, NodeWPtr> >::iterator it2 = it->output.outputs.begin(); it2 != it->output.outputs.end(); ++it2) {
                NodePtr node = it2->second.lock();
                if (node) {
                    node->disconnectInput(it2->first);
                }
            }
        }

        QPointF curPos = output->getPos_mt_safe();
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
                QPointF curPos = node->getPos_mt_safe();
                node->refreshPosition(curPos.x() + 200, curPos.y(), true);
            }
        }

        ///Move all other nodes

        for (std::list<NodeGuiWPtr>::iterator it2 = it->inbetweenNodes.begin(); it2 != it->inbetweenNodes.end(); ++it2) {
            NodeGuiPtr node = it2->lock();
            QPointF curPos = node->getPos_mt_safe();
            node->refreshPosition(curPos.x() + 200, curPos.y(), true);
        }
    }

    for (std::set<ViewerInstance* >::iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }

    _graph->getGui()->getApp()->triggerAutoSave();
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
        outputNode->replaceInput(input, it->second.inputIdx);
    }


    for (std::list<NodeGuiWPtr>::iterator it = _originalNodes.begin(); it != _originalNodes.end(); ++it) {
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
    for (std::list<NodeGuiWPtr>::const_iterator it = _originalNodes.begin(); it != _originalNodes.end(); ++it) {
        NodeGuiPtr n = it->lock();
        if (!n) {
            continue;
        }
        originalNodes.push_back(n);

        QPointF nodePos = n->getPos_mt_safe();
        groupPosition += nodePos;
    }

    unsigned sz = originalNodes.size();
    if (sz) {
        groupPosition.rx() /= sz;
        groupPosition.ry() /= sz;
    }

    NodeGroupPtr isGrp;
    if (_firstRedoCalled) {
        isGrp = std::dynamic_pointer_cast<NodeGroup>(_group.lock()->getNode()->getEffectInstance());
        assert(isGrp);
        if (!isGrp) {
            throw std::logic_error("GroupFromSelectionCommand::redo()");
        }
    } else {
        NodesList internalNewNodes;
        // Create the actual Group node
        CreateNodeArgs groupArgs( PLUGINID_NATRON_GROUP, _graph->getGroup() );
        groupArgs.setProperty<bool>(kCreateNodeArgsPropNodeGroupDisableCreateInitialNodes, true);
        groupArgs.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
        groupArgs.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
        groupArgs.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);

        NodePtr containerNode = _graph->getGui()->getApp()->createNode(groupArgs);

        isGrp = std::dynamic_pointer_cast<NodeGroup>( containerNode->getEffectInstance()->shared_from_this() );
        assert(isGrp);
        if (!isGrp) {
            throw std::logic_error("GroupFromSelectionCommand::redo()");
        }
        NodeGuiIPtr container_i = containerNode->getNodeGui();
        assert(container_i);
        _group = std::dynamic_pointer_cast<NodeGui>(container_i);
        assert( _group.lock() );

        // Set the position of the group
        container_i->setPosition( groupPosition.x(), groupPosition.y() );

        // Copy all the selected nodes and paste them in the newly created Group
        std::list<std::pair<std::string, NodeGuiPtr> > newNodes;
        _graph->copyNodesAndCreateInGroup(originalNodes, isGrp, newNodes);

        for (std::list<std::pair<std::string, NodeGuiPtr> >::iterator it = newNodes.begin(); it != newNodes.end(); ++it) {
            internalNewNodes.push_back( it->second->getNode() );
        }

        // Extract all trees from these new nodes
        std::list<Project::NodesTree> trees;
        Project::extractTreesFromNodes(internalNewNodes, trees);
        bool hasCreatedOutput = false;
        int inputNb = 0;
        for (std::list<Project::NodesTree>::iterator it = trees.begin(); it != trees.end(); ++it) {
            NodeGuiPtr foundOriginalOutputNode;
            if (!it->output.node->isActivated()) {
                continue;
            }
            const std::string outputNodeName = it->output.node->getScriptName_mt_safe();
            for (NodesGuiList::const_iterator it3 = originalNodes.begin(); it3 != originalNodes.end(); ++it3) {
                NodePtr n = (*it3)->getNode();
                if ( n && n->isActivated() && n->getScriptName_mt_safe() == outputNodeName) {
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
                if (!it2->node->isActivated()) {
                    continue;
                }
                const std::string originalNodeName = it2->node->getScriptName_mt_safe();
                for (NodesGuiList::const_iterator it3 = originalNodes.begin(); it3 != originalNodes.end(); ++it3) {
                    NodePtr n = (*it3)->getNode();
                    if ( n && n->isActivated() && n->getScriptName_mt_safe() == originalNodeName) {
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
                const std::vector<NodeWPtr>& originalNodeInputs = originalNodeInternal->getInputs();
                for (std::size_t i = 0; i < originalNodeInputs.size(); ++i) {
                    if ( !originalNodeInternal->isInputVisible(i) ) {
                        // do not create an Input node for an invisible input (example: Shadertoy node, which hides its inputs)
                        continue;
                    }
                    NodePtr originalInput = originalNodeInputs[i].lock();

                    //Create an input node corresponding to this input
                    CreateNodeArgs args(PLUGINID_NATRON_INPUT, isGrp);
                    args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
                    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
                    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);


                    NodePtr input = _graph->getGui()->getApp()->createNode(args);
                    assert(input);
                    std::string inputLabel = originalNodeInternal->getLabel() + '_' + originalNodeInternal->getInputLabel(i);
                    input->setLabel(inputLabel);

                    // Position the input node correctly
                    double offsetX, offsetY;


                    double originalX, originalY;
                    foundOriginalNode->getPosition(&originalX, &originalY);
                    if (originalInput) {
                        double inputX, inputY;
                        originalInput->getPosition(&inputX, &inputY);
                        offsetX = inputX - originalX;
                        offsetY = inputY - originalY;
                    } else {
                        offsetX = originalX;
                        offsetY = originalY;
                    }
                    double thisInputX, thisInputY;
                    it2->node->getPosition(&thisInputX, &thisInputY);
                    
                    thisInputX += offsetX;
                    thisInputY += offsetY;

                    input->setPosition(thisInputX, thisInputY);

                    it2->node->connectInput(input, i);
                    if (originalInput) {
                        isGrp->getNode()->connectInput(originalInput, inputNb);
                    }
                    ++inputNb;

                } // for all node's inputs
            } // for all inputs in the tree
            //Create only a single output

            if (!hasCreatedOutput) {
                CreateNodeArgs args(PLUGINID_NATRON_OUTPUT, isGrp);
                args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
                args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
                args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
                NodePtr output = _graph->getGui()->getApp()->createNode(args);
                try {
                    output->setScriptName("Output");
                } catch (...) {
                }

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
                int inputIdx = output->getInputIndex(foundOriginalOutputNode->getNode().get());
                assert(inputIdx != -1);
                output->replaceInput(isGrp->getNode(), inputIdx);
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
            outputNode->replaceInput(isGrp->getNode(), it->second.inputIdx);
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


    std::list<ViewerInstance*> viewers;
    _graph->getGroup()->getViewers(&viewers);
    for (std::list<ViewerInstance*>::iterator it = viewers.begin(); it != viewers.end(); ++it) {
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
                                       const std::list<NodeGuiPtr> & groupNodes)
    : QUndoCommand()
    , _graph(graph)
    , _groupNodes()
    , _firstRedoCalled(false)
{
    setText( tr("Inline group(s)") );

    for (std::list<NodeGuiPtr> ::const_iterator it = groupNodes.begin(); it != groupNodes.end(); ++it) {
        NodeGroup* group = (*it)->getNode()->isEffectGroup();
        assert(group);
        if (!group) {
            continue;
        }
        InlinedGroup expandedGroup;

        /*
         * First-off copy all the nodes within the group, except the Inputs and Output nodes
         */
        NodeClipBoard cb;
        NodesList nodes = group->getNodes();
        std::vector<NodePtr> groupInputs;
        NodePtr groupOutput = group->getOutputNode(true);
        group->getInputs(&groupInputs, true);

        std::list<NodeGuiPtr> nodesToCopy;
        for (NodesList::iterator it2 = nodes.begin(); it2 != nodes.end(); ++it2) {
            GroupInput* inp = dynamic_cast<GroupInput*>( (*it2)->getEffectInstance().get() );
            GroupOutput* output = dynamic_cast<GroupOutput*>( (*it2)->getEffectInstance().get() );
            if ( !inp && !output && !(*it2)->getParentMultiInstance() ) {
                NodeGuiIPtr gui_i = (*it2)->getNodeGui();
                assert(gui_i);
                NodeGuiPtr nodeGui = std::dynamic_pointer_cast<NodeGui>(gui_i);
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
        std::list<std::pair<std::string, NodeGuiPtr> > newNodes;
        _graph->pasteCliboard(cb, &newNodes);

        NodeGuiIPtr groupGui_i = group->getNode()->getNodeGui();
        assert(groupGui_i);
        NodeGuiPtr groupGui = std::dynamic_pointer_cast<NodeGui>(groupGui_i);
        assert(groupGui);
        expandedGroup.group = groupGui;

        QPointF groupNodePos = groupGui->mapToScene( groupGui->mapFromParent( groupGui->getPos_mt_safe() ) );

        //This is the BBox of the new inlined nodes
        RectD bbox;
        bbox.setupInfinity();
        for (std::list<std::pair<std::string, NodeGuiPtr> >::iterator it2 = newNodes.begin();
             it2 != newNodes.end(); ++it2) {
            QPointF p = it2->second->mapToScene( it2->second->mapFromParent( it2->second->getPos_mt_safe() ) );
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
        double inputY = std::numeric_limits<int>::min();
        int maxInputs = group->getNode()->getNInputs();
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
                NodeGuiPtr inputGui = std::dynamic_pointer_cast<NodeGui>(inputGui_i);
                assert(inputGui);
                ntc.input = inputGui;

                QPointF p = inputGui->mapToScene( inputGui->mapFromParent( inputGui->getPos_mt_safe() ) );
                if (p.y() > inputY) {
                    inputY = p.y();
                }

                for (std::map<NodePtr, int>::iterator it2 = outputConnected.begin(); it2 != outputConnected.end(); ++it2) {
                    NodeGuiPtr outputGui;
                    ///Find the new node that was inlined, based on the script name of the old node in the group
                    for (std::list<std::pair<std::string, NodeGuiPtr> >::iterator it3 = newNodes.begin(); it3 != newNodes.end(); ++it3) {
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

        std::list<NodeGuiPtr> outputsConnectedToGroup;
        QPointF firstInputPos;

        // This is the y coord of the top most output
        double outputY = std::numeric_limits<int>::max();
        if (groupOutput) {
            NodePtr groupOutputInput = groupOutput->getInput(0);
            if (groupOutputInput) {
                NodeToConnect outputConnection;
                NodeGuiPtr inputGui;
                ///Find the new node that was inlined, based on the script name of the old node in the group
                for (std::list<std::pair<std::string, NodeGuiPtr> >::iterator it3 = newNodes.begin(); it3 != newNodes.end(); ++it3) {
                    if ( it3->first == groupOutputInput->getScriptName() ) {
                        inputGui = it3->second;
                        break;
                    }
                }

                if (!inputGui) {
                    continue;
                }
                firstInputPos = inputGui->mapToScene( inputGui->mapFromParent( inputGui->getPos_mt_safe() ) );
                outputConnection.input = inputGui;

                std::map<NodePtr, int> outputConnected;
                group->getNode()->getOutputsConnectedToThisNode(&outputConnected);
                for (std::map<NodePtr, int>::iterator it2 = outputConnected.begin(); it2 != outputConnected.end(); ++it2) {
                    NodeGuiIPtr outputGui_i = it2->first->getNodeGui();
                    assert(outputGui_i);
                    NodeGuiPtr outputGui = std::dynamic_pointer_cast<NodeGui>(outputGui_i);
                    assert(outputGui);
                    outputsConnectedToGroup.push_back(outputGui);

                    QPointF p = outputGui->mapToScene( outputGui->mapFromParent( outputGui->getPos_mt_safe() ) );
                    if (p.y() < outputY) {
                        outputY = p.y();
                    }
                    outputConnection.outputs.insert( std::make_pair(outputGui, it2->second) );
                }
                expandedGroup.connections[-1] = outputConnection;
            }
        }

        // If there is no output to the group, the output is considered to be infinite (so we don't move any node)
        if (outputY == std::numeric_limits<int>::min()) {
            outputY = std::numeric_limits<int>::max();
        }

        const double ySpaceAvailable = outputY  - inputY;
        const double ySpaceNeeded = bbox.y2 - bbox.y1 + TO_DPIX(100);

        //  Move recursively the outputs of the group nodes so that it does not overlap the inlining of the group
        QRectF rectToClear(bbox.x1, bbox.y1, bbox.x2 - bbox.x1, ySpaceNeeded - ySpaceAvailable);
        for (std::list<NodeGuiPtr> ::iterator it2 = outputsConnectedToGroup.begin();
             it2 != outputsConnectedToGroup.end(); ++it2) {
            (*it2)->moveBelowPositionRecursively(rectToClear);
        }


        // Move all created nodes by this delta to fit in the space we've just made
        for (std::list<std::pair<std::string, NodeGuiPtr> >::iterator it2 = newNodes.begin();
             it2 != newNodes.end(); ++it2) {
            QPointF p = it2->second->mapToScene( it2->second->mapFromParent( it2->second->getPos_mt_safe() ) );
            QPointF delta = p - bboxCenter;
            p = groupNodePos + delta;
            p = it2->second->mapToParent( it2->second->mapFromScene(p) );
            it2->second->setPosition( p.x(), p.y() );
        }


        _groupNodes.push_back(expandedGroup);
    } // for (std::list<NodeGuiPtr> ::const_iterator it = groupNodes.begin(); it != groupNodes.end(); ++it) {
}

InlineGroupCommand::~InlineGroupCommand()
{
}

void
InlineGroupCommand::undo()
{
    std::set<ViewerInstance*> viewers;
    std::list<NodeGuiPtr> nodesToSelect;

    for (std::list<InlinedGroup>::iterator it = _groupNodes.begin(); it != _groupNodes.end(); ++it) {
        NodeGuiPtr groupNode = it->group.lock();
        if (groupNode) {
            groupNode->getNode()->activate(NodesList(), true, false);
            std::list<ViewerInstance*> connectedViewers;
            groupNode->getNode()->hasViewersConnected(&connectedViewers);
            for (std::list<ViewerInstance*>::iterator it2 = connectedViewers.begin(); it2 != connectedViewers.end(); ++it2) {
                viewers.insert(*it2);
            }
            for (std::list<NodeGuiWPtr>::iterator it2 = it->inlinedNodes.begin();
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
    for (std::set<ViewerInstance*>::iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }

    _graph->getGui()->getApp()->triggerAutoSave();
}

void
InlineGroupCommand::redo()
{
    std::set<ViewerInstance*> viewers;
    std::list<NodeGuiPtr> nodesToSelect;

    for (std::list<InlinedGroup>::iterator it = _groupNodes.begin(); it != _groupNodes.end(); ++it) {
        NodeGuiPtr groupNode = it->group.lock();
        if (groupNode) {
            std::list<ViewerInstance*> connectedViewers;
            groupNode->getNode()->hasViewersConnected(&connectedViewers);
            for (std::list<ViewerInstance*>::iterator it2 = connectedViewers.begin(); it2 != connectedViewers.end(); ++it2) {
                viewers.insert(*it2);
            }
            groupNode->getNode()->deactivate(NodesList(), true, false, true, false, false);
            for (std::list<NodeGuiWPtr>::iterator it2 = it->inlinedNodes.begin();
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
                for (NodeGuisMap::iterator it3 = it2->second.outputs.begin();
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

    for (std::set<ViewerInstance*>::iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }
    _graph->getGui()->getApp()->triggerAutoSave();
    _firstRedoCalled = true;
}

NATRON_NAMESPACE_EXIT
