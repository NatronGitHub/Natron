//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "NodeGraphUndoRedo.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QDebug>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewerInstance.h"
#include "Engine/NodeSerialization.h"

#include "Gui/NodeGui.h"
#include "Gui/NodeGraph.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/Edge.h"
#include "Gui/NodeBackDrop.h"
#include "Gui/MultiInstancePanel.h"
#define MINIMUM_VERTICAL_SPACE_BETWEEN_NODES 10

typedef boost::shared_ptr<NodeGui> NodeGuiPtr;

MoveMultipleNodesCommand::MoveMultipleNodesCommand(const std::list<NodeToMove> & nodes,
                                                   const std::list<NodeBackDrop*> & bds,
                                                   double dx,
                                                   double dy,
                                                   bool doMerge,
                                                   const QPointF & mouseScenePos,
                                                   QUndoCommand *parent)
    : QUndoCommand(parent)
      , _firstRedoCalled(false)
      , _nodes(nodes)
      , _bds(bds)
      , _mouseScenePos(mouseScenePos)
      , _dx(dx)
      , _dy(dy)
      , _doMerge(doMerge)
{
    assert( !nodes.empty() || !bds.empty() );
}

void
MoveMultipleNodesCommand::move(bool skipMagnet,
                               double dx,
                               double dy)
{
    for (std::list<NodeToMove>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        NodeGuiPtr node = it->node.lock();
        QPointF pos = node->getPos_mt_safe();
        node->refreshPosition(pos.x() + dx, pos.y() + dy,it->isWithinBD || _nodes.size() > 1 || skipMagnet,_mouseScenePos);
    }
    for (std::list<NodeBackDrop*>::iterator it = _bds.begin(); it != _bds.end(); ++it) {
        QPointF pos = (*it)->getPos_mt_safe();
        pos += QPointF(dx,dy);
        (*it)->setPos_mt_safe(pos);
    }
}

void
MoveMultipleNodesCommand::undo()
{
    move(true,-_dx, -_dy);
    setText( QObject::tr("Move nodes") );
}

void
MoveMultipleNodesCommand::redo()
{
    move(_firstRedoCalled,_dx, _dy);
    _firstRedoCalled = true;
    setText( QObject::tr("Move nodes") );
}

bool
MoveMultipleNodesCommand::mergeWith(const QUndoCommand *command)
{
    const MoveMultipleNodesCommand *mvCmd = dynamic_cast<const MoveMultipleNodesCommand *>(command);

    if (!mvCmd) {
        return false;
    }
    if (!mvCmd->_doMerge || !_doMerge) {
        return false;
    }
    if ( ( mvCmd->_bds.size() != _bds.size() ) || ( mvCmd->_nodes.size() != _nodes.size() ) ) {
        return false;
    }
    {
        std::list<NodeToMove >::const_iterator itOther = mvCmd->_nodes.begin();
        for (std::list<NodeToMove>::const_iterator it = _nodes.begin(); it != _nodes.end(); ++it,++itOther) {
            if (it->node.lock() != itOther->node.lock()) {
                return false;
            }
        }
    }
    {
        std::list<NodeBackDrop*>::const_iterator itOther = mvCmd->_bds.begin();
        for (std::list<NodeBackDrop*>::const_iterator it = _bds.begin(); it != _bds.end(); ++it,++itOther) {
            if (*itOther != *it) {
                return false;
            }
        }
    }
    _dx += mvCmd->_dx;
    _dy += mvCmd->_dy;

    return true;
}

AddMultipleNodesCommand::AddMultipleNodesCommand(NodeGraph* graph,
                                                 const std::list<boost::shared_ptr<NodeGui> > & nodes,
                                                 const std::list<NodeBackDrop*> & bds,
                                                 QUndoCommand *parent)
    : QUndoCommand(parent)
      , _nodes()
      , _bds(bds)
      , _graph(graph)
      , _firstRedoCalled(false)
      , _isUndone(false)
{
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        _nodes.push_back(*it);
    }
}

AddMultipleNodesCommand::AddMultipleNodesCommand(NodeGraph* graph,
                                                 const boost::shared_ptr<NodeGui> & node,
                                                 QUndoCommand* parent)
    : QUndoCommand(parent)
      , _nodes()
      , _bds()
      , _graph(graph)
      , _firstRedoCalled(false)
      , _isUndone(false)
{
    _nodes.push_back(node);
}

AddMultipleNodesCommand::AddMultipleNodesCommand(NodeGraph* graph,
                                                 NodeBackDrop* bd,
                                                 QUndoCommand* parent)
    : QUndoCommand(parent)
      , _nodes()
      , _bds()
      , _graph(graph)
      , _firstRedoCalled(false)
      , _isUndone(false)
{
    _bds.push_back(bd);
}

AddMultipleNodesCommand::~AddMultipleNodesCommand()
{
    if (_isUndone) {
        for (std::list<boost::weak_ptr<NodeGui> >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
            _graph->deleteNodepluginsly(it->lock());
        }

        for (std::list<NodeBackDrop*>::iterator it = _bds.begin(); it != _bds.end(); ++it) {
            (*it)->setParentItem(NULL);
            _graph->removeBackDrop(*it);
            delete *it;
        }
    }
}

void
AddMultipleNodesCommand::undo()
{
    _isUndone = true;
    std::list<ViewerInstance*> viewersToRefresh;

    for (std::list<NodeBackDrop*>::iterator it = _bds.begin(); it != _bds.end(); ++it) {
        (*it)->deactivate();
    }

    for (std::list<boost::weak_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        NodeGuiPtr node = it->lock();
        node->getNode()->deactivate(std::list< Natron::Node* >(), //outputs to disconnect
                                     true, //disconnect all nodes, disregarding the first parameter.
                                     true, //reconnect outputs to inputs of this node?
                                     true, //hide nodeGui?
                                     false); // triggerRender
        std::list<ViewerInstance* > viewers;
        node->getNode()->hasViewersConnected(&viewers);
        for (std::list<ViewerInstance* >::iterator it2 = viewers.begin(); it2 != viewers.end(); ++it2) {
            std::list<ViewerInstance*>::iterator foundViewer = std::find(viewersToRefresh.begin(), viewersToRefresh.end(), *it2);
            if ( foundViewer == viewersToRefresh.end() ) {
                viewersToRefresh.push_back(*it2);
            }
        }
    }
    
    _graph->clearSelection();
    
    _graph->getGui()->getApp()->triggerAutoSave();

    for (std::list<ViewerInstance* >::iterator it = viewersToRefresh.begin(); it != viewersToRefresh.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }


    setText( QObject::tr("Add node") );
}

void
AddMultipleNodesCommand::redo()
{
    _isUndone = false;
    std::list<ViewerInstance*> viewersToRefresh;
    
    std::list<boost::shared_ptr<NodeGui> > nodes;
    for (std::list<boost::weak_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        nodes.push_back(it->lock());
    }
    if (_firstRedoCalled) {
        for (std::list<NodeBackDrop*>::iterator it = _bds.begin(); it != _bds.end(); ++it) {
            (*it)->activate();
        }

        for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
            (*it)->getNode()->activate(std::list< Natron::Node* >(), //inputs to restore
                                       true, //restore all inputs ?
                                       false); //triggerRender
        }
    }
    
    
    _graph->setSelection(nodes);

    _graph->getGui()->getApp()->triggerAutoSave();

    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
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
        (*it)->renderCurrentFrame(true);
    }


    _firstRedoCalled = true;
    setText( QObject::tr("Add node") );
}

RemoveMultipleNodesCommand::RemoveMultipleNodesCommand(NodeGraph* graph,
                                                       const std::list<boost::shared_ptr<NodeGui> > & nodes,
                                                       const std::list<NodeBackDrop*> & bds,
                                                       QUndoCommand *parent)
    : QUndoCommand(parent)
      , _nodes()
      , _bds(bds)
      , _graph(graph)
      , _isRedone(false)
{
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodeToRemove n;
        n.node = *it;

        ///find all outputs to restore
        const std::list<Natron::Node*> & outputs = (*it)->getNode()->getOutputs();
        for (std::list<Natron::Node* >::const_iterator it2 = outputs.begin(); it2 != outputs.end(); ++it2) {
            bool restore = true;
            for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it3 = nodes.begin(); it3 != nodes.end(); ++it3) {
                if ( (*it3)->getNode().get() == *it2 ) {
                    ///we found the output in the selection, don't restore it
                    restore = false;
                }
            }
            if (restore) {
                n.outputsToRestore.push_back(*it2);
            }
        }
        _nodes.push_back(n);
    }
}

RemoveMultipleNodesCommand::~RemoveMultipleNodesCommand()
{
    if (_isRedone) {
        for (std::list<NodeToRemove>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
            _graph->deleteNodepluginsly(it->node.lock());
        }

        for (std::list<NodeBackDrop*>::iterator it = _bds.begin(); it != _bds.end(); ++it) {
            (*it)->setParentItem(NULL);
            _graph->removeBackDrop(*it);
            delete *it;
        }
    }
}

void
RemoveMultipleNodesCommand::undo()
{
    std::list<ViewerInstance*> viewersToRefresh;
    std::list<SequenceTime> allKeysToAdd;
    std::list<NodeToRemove>::iterator next = _nodes.begin();

    if ( !_nodes.empty() ) {
        ++next;
    }
    for (std::list<NodeToRemove>::iterator it = _nodes.begin(); it != _nodes.end(); ++it,++next) {
        
        NodeGuiPtr node = it->node.lock();
        node->getNode()->activate(it->outputsToRestore,false,false);
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

        ///On Windows going pass .end() will crash...
        if ( next == _nodes.end() ) {
            --next;
        }
    }
    for (std::list<NodeBackDrop*>::iterator it = _bds.begin(); it != _bds.end(); ++it) {
        (*it)->activate();
    }

    for (std::list<ViewerInstance* >::iterator it = viewersToRefresh.begin(); it != viewersToRefresh.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }
    _graph->getGui()->getApp()->triggerAutoSave();
    _graph->getGui()->getApp()->redrawAllViewers();
    _graph->updateNavigator();

    _isRedone = false;
    _graph->scene()->update();
    setText( QObject::tr("Remove node") );
}

void
RemoveMultipleNodesCommand::redo()
{
    _isRedone = true;

    std::list<ViewerInstance*> viewersToRefresh;
    std::list<NodeToRemove>::iterator next = _nodes.begin();
    if ( !_nodes.empty() ) {
        ++next;
    }
    for (std::list<NodeToRemove>::iterator it = _nodes.begin(); it != _nodes.end(); ++it,++next) {
        
        NodeGuiPtr node = it->node.lock();
        ///Make a copy before calling deactivate which will modify the list
        std::list<Natron::Node* > outputs = node->getNode()->getOutputs();
        
        std::list<ViewerInstance* > viewers;
        node->getNode()->hasViewersConnected(&viewers);
        for (std::list<ViewerInstance* >::iterator it2 = viewers.begin(); it2 != viewers.end(); ++it2) {
            std::list<ViewerInstance*>::iterator foundViewer = std::find(viewersToRefresh.begin(), viewersToRefresh.end(), *it2);
            if ( foundViewer == viewersToRefresh.end() ) {
                viewersToRefresh.push_back(*it2);
            }
        }

        node->getNode()->deactivate(it->outputsToRestore,false,_nodes.size() == 1,true,false);


        if (_nodes.size() == 1) {
            ///If we're deleting a single node and there's a viewer in output,reconnect the viewer to another connected input it has
            for (std::list<Natron::Node* >::const_iterator it2 = outputs.begin(); it2 != outputs.end(); ++it2) {
                assert(*it2);

                ///the output must be in the outputs to restore
                std::list<Natron::Node* >::const_iterator found =
                    std::find(it->outputsToRestore.begin(),it->outputsToRestore.end(),*it2);

                if ( found != it->outputsToRestore.end() ) {
                    InspectorNode* inspector = dynamic_cast<InspectorNode*>( *it2 );
                    ///if the node is an inspector, when disconnecting the active input just activate another input instead
                    if (inspector) {
                        const std::vector<boost::shared_ptr<Natron::Node> > & inputs = inspector->getInputs_mt_safe();
                        ///set as active input the first non null input
                        for (U32 i = 0; i < inputs.size(); ++i) {
                            if (inputs[i]) {
                                inspector->setActiveInputAndRefresh(i);
                                ///make sure we don't refresh it a second time
                                std::list<ViewerInstance*>::iterator foundViewer =
                                    std::find( viewersToRefresh.begin(), viewersToRefresh.end(),
                                               dynamic_cast<ViewerInstance*>( inspector->getLiveInstance() ) );
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

        ///On Windows going pass .end() will crash...
        if ( next == _nodes.end() ) {
            --next;
        }
    }
    for (std::list<NodeBackDrop*>::iterator it = _bds.begin(); it != _bds.end(); ++it) {
        (*it)->deactivate();
    }

    for (std::list<ViewerInstance* >::iterator it = viewersToRefresh.begin(); it != viewersToRefresh.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }

    _graph->getGui()->getApp()->triggerAutoSave();
    _graph->getGui()->getApp()->redrawAllViewers();
    _graph->updateNavigator();

    _graph->scene()->update();
    setText( QObject::tr("Remove node") );
} // redo

ConnectCommand::ConnectCommand(NodeGraph* graph,
                               Edge* edge,
                               const boost::shared_ptr<NodeGui> & oldSrc,
                               const boost::shared_ptr<NodeGui> & newSrc,
                               QUndoCommand *parent)
    : QUndoCommand(parent),
      _edge(edge),
      _oldSrc(oldSrc),
      _newSrc(newSrc),
      _dst( edge->getDest() ),
      _graph(graph),
      _inputNb( edge->getInputNumber() )
{
    assert(_dst.lock());
}

void
ConnectCommand::undo()
{
    doConnect(_newSrc.lock() ? _newSrc.lock()->getNode() : boost::shared_ptr<Natron::Node>(),
              _oldSrc.lock() ? _oldSrc.lock()->getNode() : boost::shared_ptr<Natron::Node>());
   
} // undo

void
ConnectCommand::redo()
{
    doConnect(_oldSrc.lock() ? _oldSrc.lock()->getNode() : boost::shared_ptr<Natron::Node>(),
              _newSrc.lock() ? _newSrc.lock()->getNode() : boost::shared_ptr<Natron::Node>());
} // redo



void
ConnectCommand::doConnect(const boost::shared_ptr<Natron::Node> &oldSrc,
               const boost::shared_ptr<Natron::Node> & newSrc)
{
    NodeGuiPtr dst = _dst.lock();
    boost::shared_ptr<Natron::Node> internalDst =  dst->getNode();
    InspectorNode* inspector = dynamic_cast<InspectorNode*>(internalDst.get());
    
    if (newSrc) {
        setText( QObject::tr("Connect %1 to %2")
                .arg(internalDst->getName().c_str() ).arg( newSrc->getName().c_str() ) );
    } else {
        setText( QObject::tr("Disconnect %1")
                .arg(internalDst->getName().c_str() ) );
    }

    
    if (inspector) {
        ///if the node is an inspector we have to do things differently
        
        if (oldSrc && newSrc) {
            
            ///also disconnect any current connection between the inspector and the _newSrc
            _graph->getGui()->getApp()->getProject()->disconnectNodes(newSrc.get(),inspector);
            inspector->replaceInput(newSrc, _inputNb);
            
        } else {
            if (oldSrc) {
                ///we want to connect to nothing, hence disconnect
                _graph->getGui()->getApp()->getProject()->disconnectNodes(oldSrc.get(),inspector);
            }
            if (newSrc) {
                ///also disconnect any current connection between the inspector and the _newSrc
                _graph->getGui()->getApp()->getProject()->disconnectNodes(newSrc.get(),inspector);
                _graph->getGui()->getApp()->getProject()->connectNodes(_inputNb, newSrc, inspector);
            }
        }
      
        
    } else {
        
        if (oldSrc && newSrc) {
            internalDst->replaceInput(newSrc, _inputNb);
        } else {
            if (oldSrc) {
                _graph->getGui()->getApp()->getProject()->disconnectNodes( oldSrc.get(), internalDst.get() );
            }
            if (newSrc) {
                _graph->getGui()->getApp()->getProject()->connectNodes( _inputNb, newSrc, internalDst.get() );
            }
        }
    }
    
    assert(_dst.lock());
    dst->refreshEdges();

    ///if the node has no inputs, all the viewers attached to that node should be black.
    std::list<ViewerInstance* > viewers;
    internalDst->hasViewersConnected(&viewers);
    for (std::list<ViewerInstance* >::iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }
    
    ViewerInstance* isDstAViewer = dynamic_cast<ViewerInstance*>(internalDst->getLiveInstance() );
    if (!isDstAViewer) {
        _graph->getGui()->getApp()->triggerAutoSave();
    }

}

ResizeBackDropCommand::ResizeBackDropCommand(NodeBackDrop* bd,
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

ResizeBackDropCommand::~ResizeBackDropCommand()
{
}

void
ResizeBackDropCommand::undo()
{
    _bd->resize(_oldW, _oldH);
    setText( QObject::tr("Resize %1").arg( _bd->getName() ) );
}

void
ResizeBackDropCommand::redo()
{
    _bd->resize(_w, _h);
    setText( QObject::tr("Resize %1").arg( _bd->getName() ) );
}

bool
ResizeBackDropCommand::mergeWith(const QUndoCommand *command)
{
    const ResizeBackDropCommand* rCmd = dynamic_cast<const ResizeBackDropCommand*>(command);

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
                                                         const std::list<boost::shared_ptr<NodeGui> > & nodes,
                                                         const std::list<NodeBackDrop*> & bds,
                                                         QUndoCommand *parent)
    : QUndoCommand(parent)
      , _nodes()
      , _bds()
      , _graph(graph)
{
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodeToDeclone n;
        n.node = *it;
        n.master = (*it)->getNode()->getMasterNode();
        assert(n.master.lock());
        _nodes.push_back(n);
    }

    for (std::list<NodeBackDrop*>::const_iterator it = bds.begin(); it != bds.end(); ++it) {
        BDToDeclone b;
        b.bd = *it;
        b.master = (*it)->getMaster();
        assert(b.master);
        _bds.push_back(b);
    }
}

DecloneMultipleNodesCommand::~DecloneMultipleNodesCommand()
{
}

void
DecloneMultipleNodesCommand::undo()
{
    for (std::list<NodeToDeclone>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->node.lock()->getNode()->getLiveInstance()->slaveAllKnobs( it->master.lock()->getLiveInstance() );
    }
    for (std::list<BDToDeclone>::iterator it = _bds.begin(); it != _bds.end(); ++it) {
        it->bd->slaveTo(it->master);
    }
    _graph->getGui()->getApp()->triggerAutoSave();
    setText( QObject::tr("Declone node") );
}

void
DecloneMultipleNodesCommand::redo()
{
    for (std::list<NodeToDeclone>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->node.lock()->getNode()->getLiveInstance()->unslaveAllKnobs();
    }
    for (std::list<BDToDeclone>::iterator it = _bds.begin(); it != _bds.end(); ++it) {
        it->bd->unslave();
    }
    _graph->getGui()->getApp()->triggerAutoSave();
    setText( QObject::tr("Declone node") );
}

namespace {
typedef std::pair<NodeGuiPtr,QPointF> TreeNode; ///< all points are expressed as being the CENTER of the node!

class Tree
{
    std::list<TreeNode> nodes;
    QPointF topLevelNodeCenter; //< in scene coords

public:

    Tree()
        : nodes()
          , topLevelNodeCenter(0,INT_MAX)
    {
    }

    void buildTree(const NodeGuiPtr & output,
                   const std::list<NodeGuiPtr>& selectedNodes,
                   std::list<NodeGui*> & usedNodes)
    {
        QPointF outputPos = output->pos();
        QSize nodeSize = output->getSize();

        outputPos += QPointF(nodeSize.width() / 2.,nodeSize.height() / 2.);
        addNode(output, outputPos);

        buildTreeInternal(selectedNodes, output.get(),output->mapToScene( output->mapFromParent(outputPos) ), usedNodes);
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

    void buildTreeInternal(const std::list<NodeGuiPtr>& selectedNodes,
                           NodeGui* currentNode,const QPointF & currentNodeScenePos,std::list<NodeGui*> & usedNodes);
};

typedef std::list< boost::shared_ptr<Tree> > TreeList;

void
Tree::buildTreeInternal(const std::list<NodeGuiPtr>& selectedNodes,
                        NodeGui* currentNode,
                        const QPointF & currentNodeScenePos,
                        std::list<NodeGui*> & usedNodes)
{
    QSize nodeSize = currentNode->getSize();
    boost::shared_ptr<Natron::Node> internalNode = currentNode->getNode();
    const std::vector<Edge*> & inputs = currentNode->getInputsArrows();
    NodeGuiPtr firstNonMaskInput;
    std::list<NodeGuiPtr> otherNonMaskInputs;
    std::list<NodeGuiPtr> maskInputs;

    for (U32 i = 0; i < inputs.size() ; ++i) {
        NodeGuiPtr source = inputs[i]->getSource();
        
        ///Check if the source is selected
        std::list<NodeGuiPtr>::const_iterator foundSelected = std::find(selectedNodes.begin(),selectedNodes.end(),source);
        if (foundSelected == selectedNodes.end()) {
            continue;
        }
        
        if (source) {
            bool isMask = internalNode->getLiveInstance()->isInputMask(i);
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

            ///and add it to the tree
            addNode(firstNonMaskInput, firstNonMaskInputPos);
        }

        ///Position all other non mask inputs
        int index = 0;
        for (std::list<NodeGuiPtr>::iterator it = otherNonMaskInputs.begin(); it != otherNonMaskInputs.end(); ++it,++index) {
            QPointF p = (*it)->mapToParent( (*it)->mapFromScene(currentNodeScenePos) );

            p.rx() -= ( (nodeSize.width() + (*it)->getSize().width() / 2.) ) * (index + 1);

            ///and add it to the tree
            addNode(*it, p);
            otherNonMaskInputsPos.push_back(p);
        }

        ///Position all mask inputs
        index = 0;
        for (std::list<NodeGuiPtr>::iterator it = maskInputs.begin(); it != maskInputs.end(); ++it,++index) {
            QPointF p = (*it)->mapToParent( (*it)->mapFromScene(currentNodeScenePos) );
            ///Note that here we subsctract nodeSize.width(): Actually we substract twice nodeSize.width() / 2: once to get to the left of the node
            ///and another time to add the space of half a node
            p.rx() += ( (nodeSize.width() + (*it)->getSize().width() / 2.) ) * (index + 1);

            ///and add it to the tree
            addNode(*it, p);
            maskInputsPos.push_back(p);
        }

        ///Now that we built the tree at this level, call this function again on the inputs that we just treated
        if (firstNonMaskInput) {
            buildTreeInternal(selectedNodes, firstNonMaskInput.get(),firstNonMaskInputPos, usedNodes);
        }

        std::list<QPointF>::iterator pointsIt = otherNonMaskInputsPos.begin();
        for (std::list<NodeGuiPtr>::iterator it = otherNonMaskInputs.begin(); it != otherNonMaskInputs.end(); ++it,++pointsIt) {
            buildTreeInternal(selectedNodes, it->get(),*pointsIt, usedNodes);
        }

        pointsIt = maskInputsPos.begin();
        for (std::list<NodeGuiPtr>::iterator it = maskInputs.begin(); it != maskInputs.end(); ++it,++pointsIt) {
            buildTreeInternal(selectedNodes, it->get(),*pointsIt, usedNodes);
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
    
    
    static bool hasNodeOutputsInList(const std::list<boost::shared_ptr<NodeGui> >& nodes,const boost::shared_ptr<NodeGui>& node)
    {
        const std::list<Natron::Node*>& outputs = node->getNode()->getOutputs();
        
        bool foundOutput = false;
        for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin(); it!=nodes.end(); ++it) {
            if (*it != node) {
                boost::shared_ptr<Natron::Node> n = (*it)->getNode();
                
                std::list<Natron::Node*>::const_iterator found = std::find(outputs.begin(),outputs.end(),n.get());
                if (found != outputs.end()) {
                    foundOutput = true;
                    break;
                }
            }
        }
        return foundOutput;
    }
    
    static bool hasNodeInputsInList(const std::list<boost::shared_ptr<NodeGui> >& nodes,const boost::shared_ptr<NodeGui>& node)
    {
        const std::vector<boost::shared_ptr<Natron::Node> >& inputs = node->getNode()->getInputs_mt_safe();
        
        bool foundInput = false;
        for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin(); it!=nodes.end(); ++it) {
            if (*it != node) {
                boost::shared_ptr<Natron::Node> n = (*it)->getNode();
                std::vector<boost::shared_ptr<Natron::Node> >::const_iterator found = std::find(inputs.begin(),inputs.end(),n);
                if (found != inputs.end()) {
                    foundInput = true;
                    break;
                }
            }
        }
        return foundInput;
    }
}
RearrangeNodesCommand::RearrangeNodesCommand(const std::list<boost::shared_ptr<NodeGui> > & nodes,
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

    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if (!hasNodeOutputsInList(nodes, (*it))) {
            boost::shared_ptr<Tree> newTree(new Tree);
            newTree->buildTree(*it, nodes, usedNodes);
            trees.push_back(newTree);
        }
    }

    ///For all trees find out which one has the top most level node
    QPointF topLevelPos(0,INT_MAX);
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
        QPointF delta( 0,topLevelPos.y() - treeTop.y() );
        if ( (delta.x() != 0) || (delta.y() != 0) ) {
            (*it)->moveAllTree(delta);
        }

        ///and insert the final result into the _nodes list
        const std::list<TreeNode> & treeNodes = (*it)->getNodes();
        for (std::list<TreeNode>::const_iterator it2 = treeNodes.begin(); it2 != treeNodes.end(); ++it2) {
            NodeToRearrange n;
            n.node = it2->first;
            QSize size = n.node->getSize();
            n.newPos = it2->second - QPointF(size.width() / 2.,size.height() / 2.);
            n.oldPos = n.node->pos();
            _nodes.push_back(n);
        }
    }
}

void
RearrangeNodesCommand::undo()
{
    for (std::list<NodeToRearrange>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->node->refreshPosition(it->oldPos.x(), it->oldPos.y(),true);
    }
    setText( QObject::tr("Rearrange nodes") );
}

void
RearrangeNodesCommand::redo()
{
    for (std::list<NodeToRearrange>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->node->refreshPosition(it->newPos.x(), it->newPos.y(),true);
    }
    setText( QObject::tr("Rearrange nodes") );
}

DisableNodesCommand::DisableNodesCommand(const std::list<boost::shared_ptr<NodeGui> > & nodes,
                                         QUndoCommand *parent)
    : QUndoCommand(parent)
      , _nodes()
{
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        _nodes.push_back(*it);
    }
}

void
DisableNodesCommand::undo()
{
    for (std::list<boost::weak_ptr<NodeGui> >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->lock()->getNode()->setNodeDisabled(false);
    }
    setText( QObject::tr("Disable nodes") );
}

void
DisableNodesCommand::redo()
{
    for (std::list<boost::weak_ptr<NodeGui> >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->lock()->getNode()->setNodeDisabled(true);
    }
    setText( QObject::tr("Disable nodes") );
}

EnableNodesCommand::EnableNodesCommand(const std::list<boost::shared_ptr<NodeGui> > & nodes,
                                       QUndoCommand *parent)
    : QUndoCommand(parent)
      , _nodes()
{
    for (std::list<boost::shared_ptr<NodeGui> > ::const_iterator it = nodes.begin(); it !=nodes.end(); ++it) {
        _nodes.push_back(*it);
    }
}

void
EnableNodesCommand::undo()
{
    for (std::list<boost::weak_ptr<NodeGui> >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->lock()->getNode()->setNodeDisabled(true);
    }
    setText( QObject::tr("Enable nodes") );
}

void
EnableNodesCommand::redo()
{
    for (std::list<boost::weak_ptr<NodeGui> >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        it->lock()->getNode()->setNodeDisabled(false);
    }
    setText( QObject::tr("Enable nodes") );
}


LoadNodePresetsCommand::LoadNodePresetsCommand(const boost::shared_ptr<NodeGui> & node,
                                               const std::list<boost::shared_ptr<NodeSerialization> >& serialization,
                                               QUndoCommand *parent)
: QUndoCommand(parent)
, _firstRedoCalled(false)
, _isUndone(false)
, _node(node)
, _newSerializations(serialization)
{

}

void
LoadNodePresetsCommand::getListAsShared(const std::list< boost::weak_ptr<Natron::Node> >& original,
                     std::list< boost::shared_ptr<Natron::Node> >& shared) const
{
    for (std::list< boost::weak_ptr<Natron::Node> >::const_iterator it = original.begin(); it != original.end(); ++it) {
        shared.push_back(it->lock());
    }
}

LoadNodePresetsCommand::~LoadNodePresetsCommand()
{
//    if (_isUndone) {
//        for (std::list<boost::shared_ptr<Natron::Node> >::iterator it = _newChildren.begin(); it != _newChildren.end(); ++it) {
//            (*it)->getDagGui()->deleteNodepluginsly(*it);
//        }
//    } else {
//        for (std::list<boost::shared_ptr<Natron::Node> >::iterator it = _oldChildren.begin(); it != _oldChildren.end(); ++it) {
//            (*it)->getDagGui()->deleteNodepluginsly(*it);
//        }
//    }

}

void
LoadNodePresetsCommand::undo()
{
    
    _isUndone = true;
    
    NodeGuiPtr node = _node.lock();
    boost::shared_ptr<Natron::Node> internalNode = node->getNode();
    boost::shared_ptr<MultiInstancePanel> panel = node->getMultiInstancePanel();
    internalNode->loadKnobs(*_oldSerialization.front(),true);
    if (panel) {
        std::list< boost::shared_ptr<Natron::Node> > newChildren,oldChildren;
        getListAsShared(_newChildren, newChildren);
        getListAsShared(_oldChildren, oldChildren);
        panel->removeInstances(newChildren);
        panel->addInstances(oldChildren);
    }
    internalNode->getLiveInstance()->evaluate_public(NULL, true, Natron::eValueChangedReasonUserEdited);
    internalNode->getApp()->triggerAutoSave();
    setText(QObject::tr("Load presets"));
}

void
LoadNodePresetsCommand::redo()
{
    
    NodeGuiPtr node = _node.lock();

    boost::shared_ptr<Natron::Node> internalNode = node->getNode();
    boost::shared_ptr<MultiInstancePanel> panel = node->getMultiInstancePanel();

    if (!_firstRedoCalled) {
       
        ///Serialize the current state of the node
        node->serializeInternal(_oldSerialization,true);
        
        if (panel) {
            const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >& children = panel->getInstances();
            for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = children.begin();
                 it != children.end(); ++it) {
                _oldChildren.push_back(it->first.lock());
            }
        }
        
        int k = 0;
        
        for (std::list<boost::shared_ptr<NodeSerialization> >::const_iterator it = _newSerializations.begin();
             it != _newSerializations.end(); ++it,++k) {
            
            if (k > 0)  { /// this is a multi-instance child, create it
               boost::shared_ptr<Natron::Node> newNode = panel->createNewInstance(false);
                newNode->loadKnobs(**it);
                std::list<SequenceTime> keys;
                newNode->getAllKnobsKeyframes(&keys);
                newNode->getApp()->getTimeLine()->addMultipleKeyframeIndicatorsAdded(keys, true);
                _newChildren.push_back(newNode);
            }
        }
    }
    
    internalNode->loadKnobs(*_newSerializations.front(),true);
    if (panel) {
        std::list< boost::shared_ptr<Natron::Node> > oldChildren,newChildren;
        getListAsShared(_oldChildren, oldChildren);
        getListAsShared(_newChildren, newChildren);
        panel->removeInstances(oldChildren);
        if (_firstRedoCalled) {
            panel->addInstances(newChildren);
        }
    }
    internalNode->getLiveInstance()->evaluate_public(NULL, true, Natron::eValueChangedReasonUserEdited);
    internalNode->getApp()->triggerAutoSave();
    _firstRedoCalled = true;

    setText(QObject::tr("Load presets"));
}



RenameNodeUndoRedoCommand::RenameNodeUndoRedoCommand(const boost::shared_ptr<NodeGui> & node,
                                                     NodeBackDrop* bd,
                                                     const QString& newName)
: QUndoCommand()
, _node(node)
, _bd(bd)
, _newName(newName)
{
    assert(node || bd);
    if (node) {
        _oldName = node->getNode()->getName().c_str();
    } else if (bd) {
        _oldName = bd->getName();
    }
}

RenameNodeUndoRedoCommand::~RenameNodeUndoRedoCommand()
{
    
}

void
RenameNodeUndoRedoCommand::undo()
{
    NodeGuiPtr node = _node.lock();
    if (node) {
        node->trySetName(_oldName);
    } else if (_bd) {
        _bd->trySetName(_oldName);
    }
    setText(QObject::tr("Rename node"));
}

void RenameNodeUndoRedoCommand::redo()
{
    NodeGuiPtr node = _node.lock();
    if (node) {
        node->trySetName(_newName);
    } else if (_bd) {
        _bd->trySetName(_newName);
    }
    setText(QObject::tr("Rename node"));
}

static void
sharedToWeak(const std::vector<boost::shared_ptr<Natron::Node> >& shared,
                  std::vector<boost::weak_ptr<Natron::Node> >& weak)
{
    for (std::vector<boost::shared_ptr<Natron::Node> >::const_iterator it = shared.begin(); it != shared.end(); ++it) {
        weak.push_back(*it);
    }
}


static void addTreeInputs(const std::list<boost::shared_ptr<NodeGui> >& nodes,const boost::shared_ptr<NodeGui>& node,ExtractNodeUndoRedoCommand::ExtractedTree& tree,
                          std::list<boost::shared_ptr<NodeGui> >& markedNodes)
{
    if (std::find(markedNodes.begin(), markedNodes.end(), node) != markedNodes.end()) {
        return;
    }
    
    if (std::find(nodes.begin(), nodes.end(), node) == nodes.end()) {
        return;
    }
    
    if (!hasNodeInputsInList(nodes,node)) {
        ExtractNodeUndoRedoCommand::ExtractedInput input;
        input.node = node;
        sharedToWeak(node->getNode()->getInputs_mt_safe(),input.inputs);
        tree.inputs.push_back(input);
        markedNodes.push_back(node);
    } else {
        tree.inbetweenNodes.push_back(node);
        markedNodes.push_back(node);
        const std::vector<Edge*>& inputs = node->getInputsArrows();
        for (std::vector<Edge*>::const_iterator it2 = inputs.begin() ; it2!=inputs.end(); ++it2) {
            boost::shared_ptr<NodeGui> input = (*it2)->getSource();
            if (input) {
                addTreeInputs(nodes, input, tree, markedNodes);
            }
        }
    }
}


///////////////

ExtractNodeUndoRedoCommand::ExtractNodeUndoRedoCommand(NodeGraph* graph,const std::list<boost::shared_ptr<NodeGui> >& nodes)
: QUndoCommand()
, _graph(graph)
, _trees()
{
    std::list<boost::shared_ptr<NodeGui> > markedNodes;

    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin(); it!=nodes.end(); ++it) {
        bool isOutput = !hasNodeOutputsInList(nodes, *it);
        if (isOutput) {
            ExtractedTree tree;
            tree.output.node = *it;
            boost::shared_ptr<Natron::Node> n = (*it)->getNode();
            const std::list<Natron::Node* >& outputs = n->getOutputs();
            for (std::list<Natron::Node*>::const_iterator it2 = outputs.begin();it2!=outputs.end();++it2) {
                int idx = (*it2)->inputIndex(n.get());
                tree.output.outputs.push_back(std::make_pair(idx,*it2));
            }
            
            const std::vector<Edge*>& inputs = (*it)->getInputsArrows();
            for (U32 i = 0; i < inputs.size(); ++i) {
                boost::shared_ptr<NodeGui> input = inputs[i]->getSource();
                if (input) {
                    addTreeInputs(nodes, input, tree, markedNodes);
                }
            }
            
            if (tree.inputs.empty()) {
                ExtractedInput input;
                input.node = *it;
                sharedToWeak(n->getInputs_mt_safe(),input.inputs);
                tree.inputs.push_back(input);
            }
            
            _trees.push_back(tree);
        }
    }

    
}

ExtractNodeUndoRedoCommand::~ExtractNodeUndoRedoCommand()
{
    
}

void
ExtractNodeUndoRedoCommand::undo()
{
    std::set<ViewerInstance* > viewers;
    
    for (std::list<ExtractedTree>::iterator it = _trees.begin(); it!=_trees.end(); ++it) {
        
        NodeGuiPtr output = it->output.node.lock();
        ///Connect and move output
        for (std::list<std::pair<int,Natron::Node*> >::iterator it2 = it->output.outputs.begin(); it2 != it->output.outputs.end(); ++it2) {
            it2->second->disconnectInput(it2->first);
            it2->second->connectInput(output->getNode(),it2->first);
        }
        
        QPointF curPos = output->getPos_mt_safe();
        output->refreshPosition(curPos.x() - 200, curPos.y(),true);
        
        ///Connect and move inputs
        for (std::list<ExtractedInput>::iterator it2 = it->inputs.begin(); it2 != it->inputs.end(); ++it2) {
            
            NodeGuiPtr input = it2->node.lock();
            for (U32 i  =  0; i < it2->inputs.size(); ++i) {
                if (it2->inputs[i].lock()) {
                    input->getNode()->connectInput(it2->inputs[i].lock(),i);
                }
            }
            
            if (input != output) {
                QPointF curPos = input->getPos_mt_safe();
                input->refreshPosition(curPos.x() - 200, curPos.y(),true);
            }
        }
        
        ///Move all other nodes
        
        for (std::list<boost::weak_ptr<NodeGui> >::iterator it2 = it->inbetweenNodes.begin() ; it2 != it->inbetweenNodes.end(); ++it2) {
            NodeGuiPtr node = it2->lock();
            QPointF curPos = node->getPos_mt_safe();
            node->refreshPosition(curPos.x() - 200, curPos.y(),true);
        }
        
        std::list<ViewerInstance* > tmp;
        output->getNode()->hasViewersConnected(&tmp);
        
        for (std::list<ViewerInstance* >::iterator it2 = tmp.begin() ; it2 != tmp.end(); ++it2) {
            viewers.insert(*it2);
        }

    }
    
    for (std::set<ViewerInstance* >::iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }
    
    _graph->getGui()->getApp()->triggerAutoSave();
    setText(QObject::tr("Extract node"));
}

void
ExtractNodeUndoRedoCommand::redo()
{
    std::set<ViewerInstance* > viewers;
    
    for (std::list<ExtractedTree>::iterator it = _trees.begin(); it!=_trees.end(); ++it) {
        std::list<ViewerInstance* > tmp;
        
        NodeGuiPtr output = it->output.node.lock();
        output->getNode()->hasViewersConnected(&tmp);
        
        for (std::list<ViewerInstance* >::iterator it2 = tmp.begin() ; it2 != tmp.end(); ++it2) {
            viewers.insert(*it2);
        }
        
        bool outputsAlreadyDisconnected = false;

        ///Reconnect outputs to the input of the input of the ExtractedInputs if inputs.size() == 1
        if (it->output.outputs.size() == 1 && it->inputs.size() == 1) {
            const ExtractedInput& selectedInput = it->inputs.front();
            
            const std::vector<boost::weak_ptr<Natron::Node> > &inputs = selectedInput.inputs;
            NodeGuiPtr selectedInputNode = selectedInput.node.lock();
            
            boost::shared_ptr<Natron::Node> inputToConnectTo ;
            for (U32 i = 0; i < inputs.size() ;++i) {
                if (inputs[i].lock() && !selectedInputNode->getNode()->getLiveInstance()->isInputOptional(i) &&
                    !selectedInputNode->getNode()->getLiveInstance()->isInputRotoBrush(i)) {
                    inputToConnectTo = inputs[i].lock();
                    break;
                }
            }
            
            if (inputToConnectTo) {
                for (std::list<std::pair<int,Natron::Node*> >::iterator it2 = it->output.outputs.begin(); it2 != it->output.outputs.end(); ++it2) {
                    it2->second->disconnectInput(it2->first);
                    it2->second->connectInput(inputToConnectTo, it2->first);
                }
                outputsAlreadyDisconnected = true;
            }
        }
        
        ///Disconnect and move output
        if (!outputsAlreadyDisconnected) {
            for (std::list<std::pair<int,Natron::Node*> >::iterator it2 = it->output.outputs.begin(); it2 != it->output.outputs.end(); ++it2) {
                it2->second->disconnectInput(it2->first);
            }
        }
        
        QPointF curPos = output->getPos_mt_safe();
        output->refreshPosition(curPos.x() + 200, curPos.y(),true);
        
       
        
        ///Disconnect and move inputs
        for (std::list<ExtractedInput>::iterator it2 = it->inputs.begin(); it2 != it->inputs.end(); ++it2) {
            NodeGuiPtr node = it2->node.lock();
            for (U32 i  =  0; i < it2->inputs.size(); ++i) {
                if (it2->inputs[i].lock()) {
                    node->getNode()->disconnectInput(i);
                }
            }
            if (node != output) {
                QPointF curPos = node->getPos_mt_safe();
                node->refreshPosition(curPos.x() + 200, curPos.y(),true);
            }
        }
        
        ///Move all other nodes
        
        for (std::list<boost::weak_ptr<NodeGui> >::iterator it2 = it->inbetweenNodes.begin() ; it2 != it->inbetweenNodes.end(); ++it2) {
            NodeGuiPtr node = it2->lock();
            QPointF curPos = node->getPos_mt_safe();
            node->refreshPosition(curPos.x() + 200, curPos.y(),true);
        }
    }
    
    for (std::set<ViewerInstance* >::iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }

    _graph->getGui()->getApp()->triggerAutoSave();
    

    setText(QObject::tr("Extract node"));
}
