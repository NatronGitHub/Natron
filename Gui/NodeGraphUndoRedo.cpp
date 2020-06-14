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

NATRON_NAMESPACE_ENTER

MoveMultipleNodesCommand::MoveMultipleNodesCommand(const NodesGuiList & nodes,
                                                   double dx,
                                                   double dy,
                                                   QUndoCommand *parent)
    : QUndoCommand(parent)
    , _firstRedoCalled(false)
    , _nodes()
    , _dx(dx)
    , _dy(dy)
{
    assert( !nodes.empty() );
    for (NodesGuiList::const_iterator it = nodes.begin(); it!=nodes.end(); ++it) {
        _nodes.push_back(*it);
    }
}

void
MoveMultipleNodesCommand::move(double dx,
                               double dy)
{
    for (std::list<NodeGuiWPtr>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        NodeGuiPtr n = it->lock();
        if (!n) {
            continue;
        }
        QPointF pos = n->pos();
        n->setPosition(pos.x() + dx, pos.y() + dy);
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
                                                 const NodesList& nodes,
                                                 QUndoCommand *parent)
    : QUndoCommand(parent)
    , _nodes()
    , _graph(graph)
    , _firstRedoCalled(false)
    , _isUndone(false)
{
    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodeToAdd n;
        n.node = *it;
        _nodes.push_back(n);
    }
    setText( tr("Add node") );

}

AddMultipleNodesCommand::AddMultipleNodesCommand(NodeGraph* graph,
                                                 const NodePtr & node,
                                                 QUndoCommand* parent)
    : QUndoCommand(parent)
    , _nodes()
    , _graph(graph)
    , _firstRedoCalled(false)
    , _isUndone(false)
{
    NodeToAdd n;
    n.node = node;
    _nodes.push_back(n);
    setText( tr("Add node") );

}

AddMultipleNodesCommand::~AddMultipleNodesCommand()
{
}

void
AddMultipleNodesCommand::undo()
{
    _isUndone = true;
    std::list<ViewerInstancePtr> viewersToRefresh;


    for (std::list<NodeToAdd>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        NodePtr node = it->node.lock();
        if (!node) {
            continue;
        }
        it->serialization = boost::make_shared<SERIALIZATION_NAMESPACE::NodeSerialization>();
        it->serialization->_encodeFlags = SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationFlagsSerializeOutputs;
        node->toSerialization(it->serialization.get());
        if (_nodes.size() == 1) {
            node->connectOutputsToMainInput();
        }
        node->destroyNode();
    }

    _graph->clearSelection();

    _graph->getGui()->getApp()->triggerAutoSave();
    //_graph->getGui()->getApp()->renderAllViewers();


}

void
AddMultipleNodesCommand::redo()
{
    _isUndone = false;

    NodesList createdNodes;

    if (_firstRedoCalled) {
        SERIALIZATION_NAMESPACE::NodeSerializationList serializationList;

        for (std::list<NodeToAdd>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
            assert(it->serialization);
            serializationList.push_back(it->serialization);
        }
        _graph->getGroup()->createNodesFromSerialization(serializationList, NodeCollection::eCreateNodesFromSerializationFlagsConnectToExternalNodes, &createdNodes);

        std::list<NodeToAdd>::iterator itNodes = _nodes.begin();
        assert(createdNodes.size() == _nodes.size());
        for (NodesList::iterator it = createdNodes.begin(); it!=createdNodes.end(); ++it, ++itNodes) {
            itNodes->node = *it;
        }
    } else {
        for (std::list<NodeToAdd>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
            NodePtr n = it->node.lock();
            if (n) {
                createdNodes.push_back(n);
            }
        }
    }


    if ( (createdNodes.size() != 1) || !createdNodes.front()->isEffectNodeGroup() ) {
        _graph->setSelection(createdNodes);
    }

    _graph->getGui()->getApp()->triggerAutoSave();
    //_graph->getGui()->getApp()->renderAllViewers();


    _firstRedoCalled = true;
}

RemoveMultipleNodesCommand::RemoveMultipleNodesCommand(NodeGraph* graph,
                                                       const std::list<NodeGuiPtr> & nodes,
                                                       QUndoCommand *parent)
    : QUndoCommand(parent)
    , _nodes()
    , _graph(graph)
    , _isRedone(false)
{
    for (std::list<NodeGuiPtr>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodeToRemove n;
        n.node = (*it)->getNode();
        _nodes.push_back(n);
    }
    setText( tr("Remove node") );
}

RemoveMultipleNodesCommand::~RemoveMultipleNodesCommand()
{

}

void
RemoveMultipleNodesCommand::undo()
{

    SERIALIZATION_NAMESPACE::NodeSerializationList serializationList;
    for (std::list<NodeToRemove>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        assert(it->serialization);
        serializationList.push_back(it->serialization);
    }


    NodesList createdNodes;
    _graph->getGroup()->createNodesFromSerialization(serializationList, NodeCollection::eCreateNodesFromSerializationFlagsConnectToExternalNodes, &createdNodes);

    {
        assert(createdNodes.size() == _nodes.size());
        std::list<NodeToRemove>::iterator itNodes = _nodes.begin();
        for (NodesList::iterator it = createdNodes.begin(); it!=createdNodes.end(); ++it, ++itNodes) {
            itNodes->node = *it;
        }
    }


    _graph->getGui()->getApp()->triggerAutoSave();
    //_graph->getGui()->getApp()->renderAllViewers();
    //_graph->getGui()->getApp()->redrawAllViewers();

    _isRedone = false;

} // RemoveMultipleNodesCommand::undo

void
RemoveMultipleNodesCommand::redo()
{
    _isRedone = true;

    for (std::list<NodeToRemove>::iterator it = _nodes.begin();
         it != _nodes.end();
         ++it) {
        NodePtr node = it->node.lock();
        if (!node) {
            continue;
        }
        it->serialization = boost::make_shared<SERIALIZATION_NAMESPACE::NodeSerialization>();
        it->serialization->_encodeFlags = SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationFlagsSerializeOutputs;
        node->toSerialization(it->serialization.get());
        if (_nodes.size() == 1) {
            node->connectOutputsToMainInput();
        }
        node->destroyNode();

    } // for(it)

    _graph->getGui()->getApp()->triggerAutoSave();
    //_graph->getGui()->getApp()->renderAllViewers();
    //_graph->getGui()->getApp()->redrawAllViewers();

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
        for (int i = 0; i < internalDst->getNInputs(); ++i) {
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
                std::list<int> inputsConnectedToOldSrc = internalOldSrc->getInputIndicesConnectedToThisNode(internalDst);
                for (std::list<int>::const_iterator it = inputsConnectedToOldSrc.begin(); it != inputsConnectedToOldSrc.end(); ++it) {
                    internalDst->disconnectInput(*it);
                }
            }
        } else if (internalOldSrc && !internalNewSrc) {
            std::list<int> inputsConnectedToOldSrc = internalOldSrc->getInputIndicesConnectedToThisNode(internalDst);
            for (std::list<int>::const_iterator it = inputsConnectedToOldSrc.begin(); it != inputsConnectedToOldSrc.end(); ++it) {
                internalDst->disconnectInput(*it);
            }
        } else if (!internalOldSrc && internalNewSrc) {
            Node::CanConnectInputReturnValue ret = internalDst->canConnectInput(internalNewSrc, inputNb);
            bool connectionOk = ret == Node::eCanConnectInput_ok ||
                                ret == Node::eCanConnectInput_differentFPS ||
                                ret == Node::eCanConnectInput_differentPars;
            if (connectionOk) {
                internalDst->connectInput(internalNewSrc, inputNb);
            } else {
                std::list<int> inputsConnectedToOldSrc = internalOldSrc->getInputIndicesConnectedToThisNode(internalDst);
                for (std::list<int>::const_iterator it = inputsConnectedToOldSrc.begin(); it != inputsConnectedToOldSrc.end(); ++it) {
                    internalDst->disconnectInput(*it);
                }
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
    const std::vector<NodeWPtr> & inpNodes = newSrcInternal->getInputs();
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

   _graph->getGui()->getApp()->renderAllViewers();
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
    NodeGuiPtr bd = _bd.lock();
    if (!bd) {
        return;
    }
    bd->resize(_oldW, _oldH);
    setText( tr("Resize %1").arg( QString::fromUtf8( bd->getNode()->getLabel().c_str() ) ) );
}

void
ResizeBackdropCommand::redo()
{
    NodeGuiPtr bd = _bd.lock();
    if (!bd) {
        return;
    }
    bd->resize(_w, _h);
    setText( tr("Resize %1").arg( QString::fromUtf8( bd->getNode()->getLabel().c_str() ) ) );
}

bool
ResizeBackdropCommand::mergeWith(const QUndoCommand *command)
{
    const ResizeBackdropCommand* rCmd = dynamic_cast<const ResizeBackdropCommand*>(command);

    if (!rCmd) {
        return false;
    }
    if (rCmd->_bd.lock() != _bd.lock()) {
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

typedef boost::shared_ptr<Tree> TreePtr;
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
            bool isMask = internalNode->isInputMask(i);
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
hasNodeOutputsInList(const std::list<NodeGuiPtr>& nodes,
                     const NodeGuiPtr& node)
{
    OutputNodesMap outputs;
    node->getNode()->getOutputs(outputs);
    for (std::list<NodeGuiPtr>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if (*it != node) {
            NodePtr n = (*it)->getNode();
            OutputNodesMap::const_iterator foundOutput = outputs.find(n);
            if (foundOutput != outputs.end()) {
                return true;
            }
        }
    }

    return false;
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

    for (std::list<NodeGuiPtr>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( !hasNodeOutputsInList( nodes, (*it) ) ) {
            TreePtr newTree = boost::make_shared<Tree>();
            newTree->buildTree(*it, nodes, usedNodes);
            trees.push_back(newTree);
        }
    }

    ///For all trees find out which one has the top most level node
    QPointF topLevelPos(0, INT_MAX);
    for (TreePtrList::iterator it = trees.begin(); it != trees.end(); ++it) {
        const QPointF & treeTop = (*it)->getTopLevelNodeCenter();
        if ( treeTop.y() < topLevelPos.y() ) {
            topLevelPos = treeTop;
        }
    }

    ///now offset all trees to be top aligned at the same level
    for (TreePtrList::iterator it = trees.begin(); it != trees.end(); ++it) {
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
            QSize size = it2->first->getSize();
            n.newPos = it2->second - QPointF(size.width() / 2., size.height() / 2.);
            n.oldPos = it2->first->pos();
            _nodes.push_back(n);
        }
    }
}

void
RearrangeNodesCommand::undo()
{
    for (std::list<NodeToRearrange>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        NodeGuiPtr node = it->node.lock();
        if (!node) {
            continue;
        }
        node->refreshPosition(it->oldPos.x(), it->oldPos.y(), true);
    }
    setText( tr("Rearrange nodes") );
}

void
RearrangeNodesCommand::redo()
{
    for (std::list<NodeToRearrange>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        NodeGuiPtr node = it->node.lock();
        if (!node) {
            continue;
        }
        node->refreshPosition(it->newPos.x(), it->newPos.y(), true);
    }
    setText( tr("Rearrange nodes") );
}

DisableNodesCommand::DisableNodesCommand(const std::list<NodeGuiPtr> & nodes,
                                         QUndoCommand *parent)
    : QUndoCommand(parent)
    , _nodes()
{
    for (std::list<NodeGuiPtr>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        _nodes.push_back(*it);
    }
}

void
DisableNodesCommand::undo()
{
    for (std::list<boost::weak_ptr<NodeGui> >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->lock()->getNode()->getEffectInstance()->setNodeDisabled(false);
    }
    setText( tr("Disable nodes") );
}

void
DisableNodesCommand::redo()
{
    for (std::list<boost::weak_ptr<NodeGui> >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->lock()->getNode()->getEffectInstance()->setNodeDisabled(true);
    }
    setText( tr("Disable nodes") );
}

EnableNodesCommand::EnableNodesCommand(const std::list<NodeGuiPtr> & nodes,
                                       QUndoCommand *parent)
    : QUndoCommand(parent)
    , _nodes()
{
    for (std::list<NodeGuiPtr> ::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        _nodes.push_back(*it);
    }
}

void
EnableNodesCommand::undo()
{
    for (std::list<boost::weak_ptr<NodeGui> >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->lock()->getNode()->getEffectInstance()->setNodeDisabled(true);
    }
    setText( tr("Enable nodes") );
}

void
EnableNodesCommand::redo()
{
    for (std::list<boost::weak_ptr<NodeGui> >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->lock()->getNode()->getEffectInstance()->setNodeDisabled(false);
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


///////////////

ExtractNodeUndoRedoCommand::ExtractNodeUndoRedoCommand(NodeGraph* graph,
                                                       const NodesGuiList& nodes)
    : QUndoCommand()
    , _graph(graph)
    , _sortedNodes()
{
    NodesList internalNodes;
    for (NodesGuiList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        internalNodes.push_back((*it)->getNode());
    }

    NodeCollection::extractTopologicallySortedTreesFromNodes(false /*enterGroups*/, internalNodes, &_sortedNodes, 0);
}

ExtractNodeUndoRedoCommand::~ExtractNodeUndoRedoCommand()
{
}

void
ExtractNodeUndoRedoCommand::undo()
{

    for (NodeCollection::TopologicallySortedNodesList::iterator it = _sortedNodes.begin(); it != _sortedNodes.end(); ++it) {

        NodePtr node = (*it)->node;

            // Restore input links
        int inputNb = 0;
        for (NodeCollection::TopologicalSortNode::InputsVec::iterator it2 = (*it)->inputs.begin(); it2 != (*it)->inputs.end(); ++it2, ++inputNb) {
            if (*it2 && !(*it2)->isPartOfGivenNodes) {
                node->swapInput((*it2)->node, inputNb);
            }
        }

        // Restore output links and inverse the offset on the position that we applied
        for (NodeCollection::TopologicalSortNode::ExternalOutputsMap::iterator it2 = (*it)->externalOutputs.begin(); it2 != (*it)->externalOutputs.end(); ++it2) {
            for (std::list<int>::const_iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3) {
                it2->first->node->swapInput(node, *it3);
            }
        }

        node->movePosition(-200, 0);
    }

    _graph->getGui()->getApp()->renderAllViewers();
    _graph->getGui()->getApp()->triggerAutoSave();
    setText( tr("Extract node") );
} // ExtractNodeUndoRedoCommand::undo

void
ExtractNodeUndoRedoCommand::redo()
{

    std::list<NodeCollection::TopologicalSortNodePtr> outputNodes;
    std::list<NodeCollection::TopologicalSortNodePtr> inputNodes;
    for (NodeCollection::TopologicallySortedNodesList::iterator it = _sortedNodes.begin(); it != _sortedNodes.end(); ++it) {

        NodePtr node = (*it)->node;

        // Does this node has external links ?
        {
            int nExternalLinks = 0;
            for (NodeCollection::TopologicalSortNode::InputsVec::iterator it2 = (*it)->inputs.begin(); it2 != (*it)->inputs.end(); ++it2) {
                if (*it2 && !(*it2)->isPartOfGivenNodes) {
                    ++nExternalLinks;
                }
            }
            if (nExternalLinks == 1) {
                inputNodes.push_back(*it);
            }
        }
        if (!(*it)->externalOutputs.empty()) {
            outputNodes.push_back(*it);
        }

        // Offset the node position to indicate to the user that it was extracted.
        node->movePosition(200, 0);
    }

    // If we extracted a tree with exactly 1 output and 1 input, we can reconnect the output of the tree
    // to the input of the tree.
    if (outputNodes.size() == 1 && inputNodes.size() == 1) {
        const NodeCollection::TopologicalSortNodePtr& treeInput = inputNodes.front();

        // Find in the tree input, which inputNb is the external input
        NodePtr externalLinkNode;
        {
            int i = 0;
            for (NodeCollection::TopologicalSortNode::InputsVec::iterator it2 = treeInput->inputs.begin(); it2 != treeInput->inputs.end(); ++it2, ++i) {
                if (*it2 && !(*it2)->isPartOfGivenNodes) {
                    externalLinkNode = (*it2)->node;
                    break;
                }
            }
        }


        const NodeCollection::TopologicalSortNodePtr& treeOutput = outputNodes.front();

        for (NodeCollection::TopologicalSortNode::ExternalOutputsMap::iterator it = treeOutput->externalOutputs.begin(); it != treeOutput->externalOutputs.end(); ++it) {
            for (std::list<int>::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                it->first->node->swapInput(externalLinkNode, *it2);
            }
        }

    } else {
        // Disconnect outputs of the tree

        for (std::list<NodeCollection::TopologicalSortNodePtr>::const_iterator it = outputNodes.begin(); it != outputNodes.end(); ++it) {

            for (NodeCollection::TopologicalSortNode::ExternalOutputsMap::iterator it2 = (*it)->externalOutputs.begin(); it2 != (*it)->externalOutputs.end(); ++it2) {
                for (std::list<int>::const_iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3) {
                    it2->first->node->disconnectInput(*it3);
                }
            }
        }
    }

    // Disconnect inputs of the tree

    for (std::list<NodeCollection::TopologicalSortNodePtr>::const_iterator it = inputNodes.begin(); it != inputNodes.end(); ++it) {
        int inputNb = 0;
        for (NodeCollection::TopologicalSortNode::InputsVec::iterator it2 = (*it)->inputs.begin(); it2 != (*it)->inputs.end(); ++it2, ++inputNb) {
            if (*it && !(*it)->isPartOfGivenNodes) {
                (*it)->node->disconnectInput(inputNb);
            }
        }
    }

    _graph->getGui()->getApp()->renderAllViewers();
    _graph->getGui()->getApp()->triggerAutoSave();


    setText( tr("Extract node") );
} // ExtractNodeUndoRedoCommand::redo

GroupFromSelectionCommand::GroupFromSelectionCommand(const NodesList & nodes)
    : QUndoCommand()
    , _oldGroup()
    , _newGroup()
{
    setText( tr("Group from selection") );

    assert( !nodes.empty() );
    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodeCollectionPtr group = (*it)->getGroup();

        // All nodes must belong to the same group
        assert(!_oldGroup.lock() || _oldGroup.lock() == group);
        _oldGroup = group;
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

    NodeCollectionPtr oldGroup = _oldGroup.lock();
    NodeGraph* oldGraph = dynamic_cast<NodeGraph*>(oldGroup->getNodeGraph());

    // Restore original selection
    for (NodesWList::iterator it = _originalNodes.begin(); it != _originalNodes.end(); ++it) {
        NodePtr node = it->lock();
        if (node) {
            node->moveToGroup(oldGroup);
            NodeGuiPtr nodeGui = toNodeGui(node->getNodeGui());
            if (nodeGui) {
                nodesToSelect.push_back(nodeGui);
            }
        }
    }

    // Restore links to the selection
    for (NodeCollection::TopologicallySortedNodesList::const_iterator it = _sortedNodes.begin(); it != _sortedNodes.end(); ++it) {
        for (std::size_t i = 0; i < (*it)->inputs.size(); ++i) {
            NodePtr inputNode;
            if ((*it)->inputs[i]) {
                inputNode = (*it)->inputs[i]->node;
            }
            (*it)->node->swapInput(inputNode, i);
        }
        for (NodeCollection::TopologicalSortNode::ExternalOutputsMap::const_iterator it2 = (*it)->externalOutputs.begin(); it2 != (*it)->externalOutputs.end(); ++it2) {
            for (std::list<int>::const_iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3) {
                it2->first->node->swapInput((*it)->node, *it3);
            }
        }

    }

    oldGraph->setSelection(nodesToSelect);

    // Destroy the created group
    {
        NodePtr groupNode = _newGroup.lock();
        if (groupNode) {
            groupNode->destroyNode();
        }
        _newGroup.reset();
    }

} // GroupFromSelectionCommand::undo

void
GroupFromSelectionCommand::redo()
{
    // The group position will be at the centroid of all selected nodes
    RectD originalNodesBbox;
    bool originalNodesBboxSet = false;

    NodesList originalNodes;
    for (NodesWList::const_iterator it = _originalNodes.begin(); it != _originalNodes.end(); ++it) {
        NodePtr n = it->lock();
        if (!n) {
            continue;
        }
        originalNodes.push_back(n);

        double x,y,w,h;
        n->getPosition(&x, &y);
        n->getSize(&w, &h);

        if (!originalNodesBboxSet) {
            originalNodesBbox.x1 = x;
            originalNodesBbox.x2 = x + w;
            originalNodesBbox.y1 = y;
            originalNodesBbox.y2 = y + h;
            originalNodesBboxSet = true;
        } else {
            originalNodesBbox.x1 = std::min(originalNodesBbox.x1, x);
            originalNodesBbox.x2 = std::max(originalNodesBbox.x2,x + w);
            originalNodesBbox.y1 = std::min(originalNodesBbox.x1, x);
            originalNodesBbox.y2 = std::max(originalNodesBbox.y2,y + h);
        }
    }

    Point bboxCenter;
    bboxCenter.x = (originalNodesBbox.x1 + originalNodesBbox.x2) / 2.;
    bboxCenter.y = (originalNodesBbox.y1 + originalNodesBbox.y2) / 2.;

    // Create the actual Group node within this Group
    NodeCollectionPtr oldContainer = _oldGroup.lock();
    if (!oldContainer) {
        return;
    }
    NodeGraph* oldContainerGraph = dynamic_cast<NodeGraph*>(oldContainer->getNodeGraph());
    assert(oldContainerGraph);
    if (!oldContainerGraph) {
        return;
    }

    CreateNodeArgsPtr groupArgs(CreateNodeArgs::create(PLUGINID_NATRON_GROUP, oldContainer ));
    groupArgs->setProperty<bool>(kCreateNodeArgsPropNodeGroupDisableCreateInitialNodes, true);
    groupArgs->setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
    groupArgs->setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    groupArgs->setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);


    NodePtr containerNode = oldContainer->getApplication()->createNode(groupArgs);
    if (!containerNode) {
        return;
    }
    NodeGroupPtr containerGroup = containerNode->isEffectNodeGroup();
    assert(containerGroup);
    if (!containerGroup) {
        return;
    }

    NodeGraph* newContainerGraph = dynamic_cast<NodeGraph*>(containerGroup->getNodeGraph());
    assert(newContainerGraph);
    if (!newContainerGraph) {
        return;
    }

    _newGroup = containerNode;


    // Set the position of the group to the centroid of selected nodes
    containerNode->setPosition(bboxCenter.x, bboxCenter.y);

    // Move all the selected nodes to the newly created Group
    for (NodesList::const_iterator it = originalNodes.begin(); it!=originalNodes.end(); ++it) {
        (*it)->moveToGroup(containerGroup);
    }

    // Just moving nodes into the group is not enough: we need to create the appropriate number
    // of Input nodes in the group to match the input of the selection so that the graph is not broken
    // and undoing this operation would restore the state.

    // First, extract all different trees within the selected node so we can identify outputs and inputs

    std::list<NodeCollection::TopologicalSortNodePtr> outputNodesList;
    NodeCollection::extractTopologicallySortedTreesFromNodes(false /*enterGroups*/, originalNodes, &_sortedNodes, &outputNodesList);

    int inputNb = 0;



    for (NodeCollection::TopologicallySortedNodesList::const_iterator it = _sortedNodes.begin(); it != _sortedNodes.end(); ++it) {

        // For each input node of each tree branch within the group, add a Input node in input of that branch
        // to actually create the input on the Group node
        int i = 0;
        for (NodeCollection::TopologicalSortNode::InputsVec::const_iterator it2 = (*it)->inputs.begin() ; it2 != (*it)->inputs.end(); ++it2, ++i) {
            if (!*it2 || (*it2)->isPartOfGivenNodes) {
                continue;
            }
            // only create input if input is enabled/visible
            if ( !(*it)->node->isInputVisible(i) ) {
                continue;
            }
            NodePtr originalInput = (*it2)->node;
            if (!originalInput) {
                continue;
            }
            //Create an input node corresponding to this input
            CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_INPUT, containerGroup));
            args->setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
            args->setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
            args->setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);


            NodePtr input = containerNode->getApp()->createNode(args);
            assert(input);

            // Name the Input node with the label of the node and the input label
            std::string inputLabel =  (*it)->node->getLabel() + '_' + (*it)->node->getInputLabel(i);
            input->setLabel(inputLabel);

            // Position the input node
            double offsetX, offsetY;
            double outputX, outputY;
            (*it)->node->getPosition(&outputX, &outputY);
            double inputX, inputY;
            originalInput->getPosition(&inputX, &inputY);
            offsetX = inputX - outputX;
            offsetY = inputY - outputY;

            input->movePosition(offsetX, offsetY);

            // Connect the node to the Input node, leaving the original input disconnected
            (*it)->node->swapInput(input, i);


            if (originalInput) {
                // Connect the newly created input of the group node to the original input
                containerGroup->getNode()->connectInput(originalInput, inputNb);
            }
            ++inputNb;
        } // for each external input


        // Connect each external output node to the group node
        for (NodeCollection::TopologicalSortNode::ExternalOutputsMap::iterator it2 = (*it)->externalOutputs.begin(); it2 != (*it)->externalOutputs.end(); ++it2) {
            for (std::list<int>::const_iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3) {
                it2->first->node->swapInput(containerNode, *it3);
            }
        }

    }

    //Create only a single output

    {
        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_OUTPUT, containerGroup));
        args->setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
        args->setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
        args->setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        NodePtr output = containerNode->getApp()->createNode(args);

        assert(output);
        if (originalNodesBboxSet) {
            output->setPosition(bboxCenter.x, bboxCenter.y + 200);
        }

        // If only a single tree, connect the output node to the bottom of the tree
        if (outputNodesList.size() == 1) {
            output->swapInput(outputNodesList.front()->node, 0);
        }
    }


    // Select the group node in the old graph
    std::list<NodeGuiPtr> nodesToSelect;
    NodeGuiPtr nodeGroupGui = toNodeGui(containerNode->getNodeGui());
    if (nodeGroupGui) {
        nodesToSelect.push_back(nodeGroupGui);
    }
    oldContainerGraph->setSelection(nodesToSelect);


    // Ensure all viewers are refreshed
    containerNode->getApp()->renderAllViewers();

    // Center the new nodegraph on all its nodes
    if (newContainerGraph) {
        newContainerGraph->centerOnAllNodes();
    }
} // GroupFromSelectionCommand::redo

InlineGroupCommand::InlineGroupCommand(const NodeCollectionPtr& newGroup, const NodesList & groupNodes)
: QUndoCommand()
, _newGroup(newGroup)
, _oldGroups()
{
    setText( tr("Inline Group(s)") );

    // For each group, remember the gropu node inputs, outputs etc...
    for (NodesList::const_iterator it = groupNodes.begin(); it != groupNodes.end(); ++it) {
        NodeGroupPtr group = (*it)->isEffectNodeGroup();
        assert(group);
        if (!group) {
            continue;
        }
        InlinedGroup inlinedGroup;
        inlinedGroup.oldGroupNode = group;
        inlinedGroup.groupInputs = (*it)->getInputs();


        std::vector<NodePtr> inputNodes;
        group->getInputs(&inputNodes);


        NodePtr outputNode = group->getOutputNode();
        for (std::size_t i = 0; i < inputNodes.size(); ++i) {

            OutputNodesMap inputOutputs;
            inputNodes[i]->getOutputs(inputOutputs);
            for (OutputNodesMap::const_iterator it2 = inputOutputs.begin(); it2 != inputOutputs.end(); ++it2) {
                const NodePtr& inputOutput = it2->first;

                for (std::list<int>::const_iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3) {
                    InlinedGroup::InputOutput p;
                    p.output = inputOutput;
                    p.inputNodes = inputOutput->getInputs();
                    p.inputIndex = i;
                    p.outputInputIndex = *it3;
                    inlinedGroup.inputsMap.push_back(p);
                }

            }

        }
        if (outputNode) {
            inlinedGroup.outputNodeInput = outputNode->getInput(0);
        }

        OutputNodesMap groupOutputs;
        (*it)->getOutputs(groupOutputs);
        for (OutputNodesMap::const_iterator it2 = groupOutputs.begin(); it2 != groupOutputs.end(); ++it2) {
            const NodePtr& groupOutput = it2->first;

            for (std::list<int>::const_iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3) {
                InlinedGroup::GroupNodeOutput outp;
                outp.output = groupOutput;
                outp.inputIndex = *it3;
                //outp.outputNodeInputs = groupOutput->getInputs();
                //groupOutput->getPosition(&outp.position[0], &outp.position[1]);
                inlinedGroup.groupOutputs.push_back(outp);
            }
        }


        NodesList nodes = group->getNodes();
        // Only move the nodes that are not GroupInput and GroupOutput
        // Compute the BBox of the inlined nodes so we can make space
        {
            bool bboxSet = false;
            for (NodesList::iterator it2 = nodes.begin(); it2 != nodes.end(); ++it2) {
                GroupInputPtr inp = (*it2)->isEffectGroupInput();
                GroupOutputPtr output = (*it2)->isEffectGroupOutput();
                if ( !inp && !output) {

                    InlinedGroup::MovedNode mnode;
                    double x,y;
                    (*it2)->getPosition(&x, &y);
                    mnode.position[0] = x;
                    mnode.position[1] = y;
                    if (!bboxSet) {
                        inlinedGroup.movedNodesBbox.x1 = inlinedGroup.movedNodesBbox.x2 = x;
                        inlinedGroup.movedNodesBbox.y1 = inlinedGroup.movedNodesBbox.y2 = y;
                        bboxSet = true;
                    } else {
                        inlinedGroup.movedNodesBbox.x1 = std::min(inlinedGroup.movedNodesBbox.x1, x);
                        inlinedGroup.movedNodesBbox.x2 = std::max(inlinedGroup.movedNodesBbox.x2, x);
                        inlinedGroup.movedNodesBbox.y1 = std::min(inlinedGroup.movedNodesBbox.y1, y);
                        inlinedGroup.movedNodesBbox.y2 = std::max(inlinedGroup.movedNodesBbox.y2, y);
                    }
                    mnode.node = *it2;
                    inlinedGroup.movedNodes.push_back(mnode);
                }

            }
        }

        (*it)->getPosition(&inlinedGroup.groupNodePos[0], &inlinedGroup.groupNodePos[1]);

        _oldGroups.push_back(inlinedGroup);
    }

}

InlineGroupCommand::~InlineGroupCommand()
{
}

void
InlineGroupCommand::undo()
{

    AppInstancePtr app;
    for (std::list<InlinedGroup>::iterator it = _oldGroups.begin(); it != _oldGroups.end(); ++it) {

        assert(it->groupNodeSerialization);
        NodeCollectionPtr newGroup = _newGroup.lock();
        if (!newGroup) {
            continue;
        }
        NodePtr groupNode;
        {
            SERIALIZATION_NAMESPACE::NodeSerializationList serializationList;
            serializationList.push_back(it->groupNodeSerialization);
            NodesList createdNodes;
            newGroup->createNodesFromSerialization(serializationList, NodeCollection::eCreateNodesFromSerializationFlagsConnectToExternalNodes, &createdNodes);
            if (!createdNodes.empty()) {
                groupNode = createdNodes.front();
            }
        }
        if (!groupNode) {
            continue;
        }

        NodeGroupPtr group = groupNode->isEffectNodeGroup();
        it->oldGroupNode = group;
        if (!group) {
            continue;
        }
        app = group->getApp();

        // Re-position back all moved nodes
        for (std::list<InlinedGroup::MovedNode>::const_iterator it2 = it->movedNodes.begin(); it2 != it->movedNodes.end(); ++it2) {
            NodePtr movedNode = it2->node.lock();
            if (!movedNode) {
                continue;
            }
            movedNode->moveToGroup(group);
            movedNode->setPosition(it2->position[0], it2->position[1]);
        }


        // Re-connect all input outputs to the GroupInput
        for (std::vector<InlinedGroup::InputOutput>::const_iterator it2 = it->inputsMap.begin(); it2 != it->inputsMap.end(); ++it2) {
            NodePtr inputOutput = it2->output.lock();
            for (std::size_t i = 0; i < it2->inputNodes.size(); ++i) {
                inputOutput->swapInput(it2->inputNodes[i].lock(), i);
            }
        }

        // Re-connect the output node to the group output input
        NodePtr outputNode = group->getOutputNode();
        if (outputNode) {
            outputNode->swapInput(it->outputNodeInput.lock(), 0);
        }

        // Re-connect all Group node outputs
        for (std::list<InlinedGroup::GroupNodeOutput>::const_iterator it2 = it->groupOutputs.begin(); it2 != it->groupOutputs.end(); ++it2) {
            NodePtr output = it2->output.lock();
            if (!output) {
                continue;
            }
            output->swapInput(groupNode,it2->inputIndex);
        }

    } // for each group

    if (app) {
        app->triggerAutoSave();
        app->renderAllViewers();
    }
} // undo

void
InlineGroupCommand::redo()
{

    NodeCollectionPtr newGroup = _newGroup.lock();
    AppInstancePtr app;
    for (std::list<InlinedGroup>::iterator it = _oldGroups.begin(); it != _oldGroups.end(); ++it) {
        NodeGroupPtr group = it->oldGroupNode.lock();
        if (!group) {
            continue;
        }

        app = group->getApp();

        std::vector<NodePtr> groupInputs;
        NodePtr groupOutput = group->getOutputNode();
        group->getInputs(&groupInputs);



        // Remember the links from the Group node we are expending to its inputs and outputs

        // This is the y coord. of the bottom-most input
        double inputY = INT_MIN;
        for (std::size_t i = 0; i < it->groupInputs.size(); ++i) {
            NodePtr input = it->groupInputs[i].lock();
            if (!input) {
                continue;
            }
            double x,y;
            input->getPosition(&x, &y);

            // Qt coord system is top down
            inputY = std::max(y, inputY);
        }

        // This is the y coord of the top most output
        double outputY = INT_MAX;
        {
            NodePtr groupOutputInput = it->outputNodeInput.lock();
            if (groupOutputInput) {
                double x;
                groupOutputInput->getPosition(&x, &outputY);
            }
        }

        const double ySpaceAvailable = outputY  - inputY;
        const double ySpaceNeeded = it->movedNodesBbox.y2 - it->movedNodesBbox.y1 + TO_DPIX(100);

        //  Move recursively the outputs of the group nodes so that it does not overlap the inlining of the group
        const RectD rectToClear(it->movedNodesBbox.x1, it->movedNodesBbox.y1, it->movedNodesBbox.x2, it->movedNodesBbox.y1 + ySpaceNeeded - ySpaceAvailable);

        for (std::list<InlinedGroup::GroupNodeOutput>::const_iterator it2 = it->groupOutputs.begin(); it2 != it->groupOutputs.end(); ++it2) {
            NodePtr groupOutput = it2->output.lock();
            if (!groupOutput) {
                continue;
            }
            groupOutput->moveBelowPositionRecursively(rectToClear);


        }

        const QPointF bboxCenter( (it->movedNodesBbox.x1 + it->movedNodesBbox.x2) / 2., (it->movedNodesBbox.y1 + it->movedNodesBbox.y2) / 2. );

        // Move the node to the new group
        // Also move all created nodes by this delta to fit in the space we've just made
        for (std::list<InlinedGroup::MovedNode>::const_iterator it2 = it->movedNodes.begin(); it2 != it->movedNodes.end(); ++it2) {
            QPointF curPos(it2->position[0], it2->position[1]);
            QPointF delta = curPos - bboxCenter;
            curPos = QPointF(it->groupNodePos[0], it->groupNodePos[1]) + delta;
            NodePtr movedNode = it2->node.lock();
            if (movedNode) {
                movedNode->moveToGroup(newGroup);
                movedNode->setPosition(curPos.x(), curPos.y());
            }
        }

        // Connect all GroupInput output nodes to the original Group node inputs
        for (std::size_t i = 0; i < it->inputsMap.size(); ++i) {
            NodePtr inputOutput = it->inputsMap[i].output.lock();
            if (!inputOutput) {
                continue;
            }
            assert(it->inputsMap[i].inputIndex >= 0 && it->inputsMap[i].inputIndex < (int)it->groupInputs.size());
            inputOutput->swapInput(it->groupInputs[it->inputsMap[i].inputIndex].lock(), it->inputsMap[i].outputInputIndex);
        }

        // Connect all original group node outputs to the  outputNodeInput
        for (std::list<InlinedGroup::GroupNodeOutput>::const_iterator it2 = it->groupOutputs.begin(); it2 != it->groupOutputs.end(); ++it2) {
            NodePtr output = it2->output.lock();
            if (!output) {
                continue;
            }
            output->swapInput(it->outputNodeInput.lock(), it2->inputIndex);
        }

        // Deactivate the group node
        it->groupNodeSerialization = boost::make_shared<SERIALIZATION_NAMESPACE::NodeSerialization>();
        it->groupNodeSerialization->_encodeFlags = SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationFlagsSerializeOutputs;
        group->getNode()->toSerialization(it->groupNodeSerialization.get());
        group->getNode()->connectOutputsToMainInput();
        group->getNode()->destroyNode();


    } // for each group


    if (app) {
        app->triggerAutoSave();
        app->renderAllViewers();
    }
} // redo

RestoreNodeToDefaultCommand::RestoreNodeToDefaultCommand(const NodesGuiList & nodes)
: QUndoCommand()
, _nodes()
{
    for (NodesGuiList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodeDefaults d;
        d.node = *it;
        d.serialization = boost::make_shared<SERIALIZATION_NAMESPACE::NodeSerialization>();
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
        internalNode->loadKnobsFromSerialization(*it->serialization, false);
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


NATRON_NAMESPACE_EXIT
