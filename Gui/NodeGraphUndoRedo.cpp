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

#include "Gui/NodeGui.h"
#include "Gui/NodeGraph.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/Edge.h"
#include "Gui/NodeBackDrop.h"

#define MINIMUM_VERTICAL_SPACE_BETWEEN_NODES 10

typedef boost::shared_ptr<NodeGui> NodeGuiPtr;

MoveMultipleNodesCommand::MoveMultipleNodesCommand(const std::list<NodeToMove> & nodes,
                                                   const std::list<NodeBackDrop*> & bds,
                                                   double dx,
                                                   double dy,
                                                   const QPointF & mouseScenePos,
                                                   QUndoCommand *parent)
    : QUndoCommand(parent)
      , _firstRedoCalled(false)
      , _nodes(nodes)
      , _bds(bds)
      , _mouseScenePos(mouseScenePos)
      , _dx(dx)
      , _dy(dy)
{
    assert( !nodes.empty() || !bds.empty() );
}

void
MoveMultipleNodesCommand::move(bool skipMagnet,
                               double dx,
                               double dy)
{
    for (std::list<NodeToMove>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        QPointF pos = it->node->getPos_mt_safe();
        it->node->refreshPosition(pos.x() + dx, pos.y() + dy,it->isWithinBD || _nodes.size() > 1 || skipMagnet,_mouseScenePos);
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
    const MoveMultipleNodesCommand *mvCmd = static_cast<const MoveMultipleNodesCommand *>(command);

    if (!mvCmd) {
        return false;
    }
    if ( ( mvCmd->_bds.size() != _bds.size() ) || ( mvCmd->_nodes.size() != _nodes.size() ) ) {
        return false;
    }
    {
        std::list<NodeToMove >::const_iterator itOther = mvCmd->_nodes.begin();
        for (std::list<NodeToMove>::const_iterator it = _nodes.begin(); it != _nodes.end(); ++it,++itOther) {
            if (it->node != itOther->node) {
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
      , _nodes(nodes)
      , _bds(bds)
      , _graph(graph)
      , _firstRedoCalled(false)
      , _isUndone(false)
{
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
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
            _graph->deleteNodePermanantly(*it);
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

    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        (*it)->getNode()->deactivate(std::list< boost::shared_ptr<Natron::Node> >(), //outputs to disconnect
                                     true, //disconnect all nodes, disregarding the first parameter.
                                     true, //reconnect outputs to inputs of this node?
                                     true, //hide nodeGui?
                                     false); // triggerRender
        std::list<ViewerInstance* > viewers;
        (*it)->getNode()->hasViewersConnected(&viewers);
        for (std::list<ViewerInstance* >::iterator it2 = viewers.begin(); it2 != viewers.end(); ++it2) {
            std::list<ViewerInstance*>::iterator foundViewer = std::find(viewersToRefresh.begin(), viewersToRefresh.end(), *it2);
            if ( foundViewer == viewersToRefresh.end() ) {
                viewersToRefresh.push_back(*it2);
            }
        }
    }
    _graph->getGui()->getApp()->triggerAutoSave();

    for (std::list<ViewerInstance* >::iterator it = viewersToRefresh.begin(); it != viewersToRefresh.end(); ++it) {
        (*it)->updateTreeAndRender();
    }


    setText( QObject::tr("Add node") );
}

void
AddMultipleNodesCommand::redo()
{
    _isUndone = false;
    std::list<ViewerInstance*> viewersToRefresh;
    if (_firstRedoCalled) {
        for (std::list<NodeBackDrop*>::iterator it = _bds.begin(); it != _bds.end(); ++it) {
            (*it)->activate();
        }

        for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
            (*it)->getNode()->activate(std::list< boost::shared_ptr<Natron::Node> >(), //inputs to restore
                                       true, //restore all inputs ?
                                       false); //triggerRender
        }
    }
    _graph->getGui()->getApp()->triggerAutoSave();

    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
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
        (*it)->updateTreeAndRender();
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
        const std::list<boost::shared_ptr<Natron::Node> > & outputs = (*it)->getNode()->getOutputs();
        for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it2 = outputs.begin(); it2 != outputs.end(); ++it2) {
            bool restore = true;
            for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it3 = nodes.begin(); it3 != nodes.end(); ++it3) {
                if ( (*it3)->getNode() == *it2 ) {
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
            _graph->deleteNodePermanantly(it->node);
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
        it->node->getNode()->activate(it->outputsToRestore,false,false);
        if ( it->node->isSettingsPanelVisible() ) {
            it->node->getNode()->showKeyframesOnTimeline( next == _nodes.end() );
        }
        std::list<ViewerInstance* > viewers;
        it->node->getNode()->hasViewersConnected(&viewers);
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
        (*it)->updateTreeAndRender();
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
        ///Make a copy before calling deactivate which will modify the list
        std::list<boost::shared_ptr<Natron::Node> > outputs = it->node->getNode()->getOutputs();

        it->node->getNode()->deactivate(it->outputsToRestore,false,_nodes.size() == 1,true,false);

        std::list<ViewerInstance* > viewers;
        it->node->getNode()->hasViewersConnected(&viewers);
        for (std::list<ViewerInstance* >::iterator it2 = viewers.begin(); it2 != viewers.end(); ++it2) {
            std::list<ViewerInstance*>::iterator foundViewer = std::find(viewersToRefresh.begin(), viewersToRefresh.end(), *it2);
            if ( foundViewer == viewersToRefresh.end() ) {
                viewersToRefresh.push_back(*it2);
            }
        }


        if (_nodes.size() == 1) {
            ///If we're deleting a single node and there's a viewer in output,reconnect the viewer to another connected input it has
            for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it2 = outputs.begin(); it2 != outputs.end(); ++it2) {
                assert(*it2);

                ///the output must be in the outputs to restore
                std::list<boost::shared_ptr<Natron::Node> >::const_iterator found =
                    std::find(it->outputsToRestore.begin(),it->outputsToRestore.end(),*it2);

                if ( found != it->outputsToRestore.end() ) {
                    InspectorNode* inspector = dynamic_cast<InspectorNode*>( it2->get() );
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
        if ( it->node->isSettingsPanelVisible() ) {
            it->node->getNode()->hideKeyframesFromTimeline( next == _nodes.end() );
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
        (*it)->updateTreeAndRender();
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
    assert(_dst);
}

void
ConnectCommand::undo()
{
    boost::shared_ptr<InspectorNode> inspector = boost::dynamic_pointer_cast<InspectorNode>( _dst->getNode() );

    if (inspector) {
        ///if the node is an inspector, the redo() action might have disconnect the dst and src nodes
        ///hence the _edge ptr might have been invalidated, recreate it
        NodeGui::InputEdgesMap::const_iterator it = _dst->getInputsArrows().find(_inputNb);
        while ( it == _dst->getInputsArrows().end() ) {
            inspector->addEmptyInput();
            it = _dst->getInputsArrows().find(_inputNb);
        }
        _edge = it->second;
    }

    if (_oldSrc) {
        _graph->getGui()->getApp()->getProject()->connectNodes( _edge->getInputNumber(), _oldSrc->getNode(), _edge->getDest()->getNode() );
        _oldSrc->refreshOutputEdgeVisibility();
    }
    if (_newSrc) {
        _graph->getGui()->getApp()->getProject()->disconnectNodes( _newSrc->getNode(), _edge->getDest()->getNode() );
        _newSrc->refreshOutputEdgeVisibility();
        ///if the node is an inspector, when disconnecting the active input just activate another input instead
        if (inspector) {
            const std::vector<boost::shared_ptr<Natron::Node> > & inputs = inspector->getInputs_mt_safe();
            ///set as active input the first non null input
            for (U32 i = 0; i < inputs.size(); ++i) {
                if (inputs[i]) {
                    inspector->setActiveInputAndRefresh(i);
                    break;
                }
            }
        }
    }

    if (_oldSrc) {
        setText( QObject::tr("Connect %1 to %2")
                 .arg( _edge->getDest()->getNode()->getName().c_str() ).arg( _oldSrc->getNode()->getName().c_str() ) );
    } else {
        setText( QObject::tr("Disconnect %1")
                 .arg( _edge->getDest()->getNode()->getName().c_str() ) );
    }

    _graph->getGui()->getApp()->triggerAutoSave();
    std::list<ViewerInstance* > viewers;
    _edge->getDest()->getNode()->hasViewersConnected(&viewers);
    for (std::list<ViewerInstance* >::iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->updateTreeAndRender();
    }
    ///if there are no viewers, at least update the render inputs
    if ( viewers.empty() ) {
        _edge->getDest()->getNode()->updateRenderInputs();
        if ( _edge->getSource() ) {
            _edge->getSource()->getNode()->updateRenderInputs();
        }
    }
} // undo

void
ConnectCommand::redo()
{
    boost::shared_ptr<InspectorNode> inspector = boost::dynamic_pointer_cast<InspectorNode>( _dst->getNode() );

    _inputNb = _edge->getInputNumber();
    if (inspector) {
        ///if the node is an inspector we have to do things differently

        if (!_newSrc) {
            if (_oldSrc) {
                ///we want to connect to nothing, hence disconnect
                _graph->getGui()->getApp()->getProject()->disconnectNodes(_oldSrc->getNode(),inspector);
                _oldSrc->refreshOutputEdgeVisibility();
            }
        } else {
            ///disconnect any connection already existing with the _oldSrc
            if (_oldSrc) {
                _graph->getGui()->getApp()->getProject()->disconnectNodes(_oldSrc->getNode(),inspector);
                _oldSrc->refreshOutputEdgeVisibility();
            }
            ///also disconnect any current connection between the inspector and the _newSrc
            _graph->getGui()->getApp()->getProject()->disconnectNodes(_newSrc->getNode(),inspector);


            ///after disconnect calls the _edge pointer might be invalid since the edges might have been destroyed.
            NodeGui::InputEdgesMap::const_iterator it = _dst->getInputsArrows().find(_inputNb);
            while ( it == _dst->getInputsArrows().end() ) {
                inspector->addEmptyInput();
                it = _dst->getInputsArrows().find(_inputNb);
            }
            _edge = it->second;

            ///and connect the inspector to the _newSrc
            _graph->getGui()->getApp()->getProject()->connectNodes(_inputNb, _newSrc->getNode(), inspector);
            _newSrc->refreshOutputEdgeVisibility();
        }
    } else {
        _edge->setSource(_newSrc);
        if (_oldSrc) {
            if ( !_graph->getGui()->getApp()->getProject()->disconnectNodes( _oldSrc->getNode(), _dst->getNode() ) ) {
                qDebug() << "Failed to disconnect (input) " << _oldSrc->getNode()->getName().c_str()
                         << " to (output) " << _dst->getNode()->getName().c_str();
            }
            _oldSrc->refreshOutputEdgeVisibility();
        }
        if (_newSrc) {
            if ( !_graph->getGui()->getApp()->getProject()->connectNodes( _inputNb, _newSrc->getNode(), _dst->getNode() ) ) {
                qDebug() << "Failed to connect (input) " << _newSrc->getNode()->getName().c_str()
                         << " to (output) " << _dst->getNode()->getName().c_str();
            }
            _newSrc->refreshOutputEdgeVisibility();
        }
    }

    assert(_dst);
    _dst->refreshEdges();

    if (_newSrc) {
        setText( QObject::tr("Connect %1 to %2")
                 .arg( _edge->getDest()->getNode()->getName().c_str() ).arg( _newSrc->getNode()->getName().c_str() ) );
    } else {
        setText( QObject::tr("Disconnect %1")
                 .arg( _edge->getDest()->getNode()->getName().c_str() ) );
    }

    ///if the node has no inputs, all the viewers attached to that node should get disconnected. This will be done
    ///in VideoEngine::startEngine
    std::list<ViewerInstance* > viewers;
    _edge->getDest()->getNode()->hasViewersConnected(&viewers);
    for (std::list<ViewerInstance* >::iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->updateTreeAndRender();
    }

    ///if there are no viewers, at least update the render inputs
    if ( viewers.empty() ) {
        _edge->getDest()->getNode()->updateRenderInputs();
        if ( _edge->getSource() ) {
            _edge->getSource()->getNode()->updateRenderInputs();
        }
    }

    ViewerInstance* isDstAViewer = dynamic_cast<ViewerInstance*>( _edge->getDest()->getNode()->getLiveInstance() );
    if (!isDstAViewer) {
        _graph->getGui()->getApp()->triggerAutoSave();
    }
} // redo

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
        assert(n.master);
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
        it->node->getNode()->getLiveInstance()->slaveAllKnobs( it->master->getLiveInstance() );
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
        it->node->getNode()->getLiveInstance()->unslaveAllKnobs();
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
                   std::list<NodeGui*> & usedNodes)
    {
        QPointF outputPos = output->pos();
        QSize nodeSize = output->getSize();

        outputPos += QPointF(nodeSize.width() / 2.,nodeSize.height() / 2.);
        addNode(output, outputPos);

        buildTreeInternal(output.get(),output->mapToScene( output->mapFromParent(outputPos) ), usedNodes);
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

    void buildTreeInternal(NodeGui* currentNode,const QPointF & currentNodeScenePos,std::list<NodeGui*> & usedNodes);
};

typedef std::list< boost::shared_ptr<Tree> > TreeList;

void
Tree::buildTreeInternal(NodeGui* currentNode,
                        const QPointF & currentNodeScenePos,
                        std::list<NodeGui*> & usedNodes)
{
    QSize nodeSize = currentNode->getSize();
    boost::shared_ptr<Natron::Node> internalNode = currentNode->getNode();
    const std::map<int,Edge*> & inputs = currentNode->getInputsArrows();
    NodeGuiPtr firstNonMaskInput;
    std::list<NodeGuiPtr> otherNonMaskInputs;
    std::list<NodeGuiPtr> maskInputs;

    for (std::map<int,Edge*>::const_iterator it = inputs.begin(); it != inputs.end(); ++it) {
        NodeGuiPtr source = it->second->getSource();
        if (source) {
            bool isMask = internalNode->getLiveInstance()->isInputMask(it->first);
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
            buildTreeInternal(firstNonMaskInput.get(),firstNonMaskInputPos, usedNodes);
        }

        std::list<QPointF>::iterator pointsIt = otherNonMaskInputsPos.begin();
        for (std::list<NodeGuiPtr>::iterator it = otherNonMaskInputs.begin(); it != otherNonMaskInputs.end(); ++it,++pointsIt) {
            buildTreeInternal(it->get(),*pointsIt, usedNodes);
        }

        pointsIt = maskInputsPos.begin();
        for (std::list<NodeGuiPtr>::iterator it = maskInputs.begin(); it != maskInputs.end(); ++it,++pointsIt) {
            buildTreeInternal(it->get(),*pointsIt, usedNodes);
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
        const std::list<boost::shared_ptr<Natron::Node> > & outputs = (*it)->getNode()->getOutputs();
        if ( outputs.empty() ) {
            boost::shared_ptr<Tree> newTree(new Tree);
            newTree->buildTree(*it, usedNodes);
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
      , _nodes(nodes)
{
}

void
DisableNodesCommand::undo()
{
    for (std::list<NodeGuiPtr>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        (*it)->getNode()->setNodeDisabled(false);
    }
    setText( QObject::tr("Disable nodes") );
}

void
DisableNodesCommand::redo()
{
    for (std::list<NodeGuiPtr>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        (*it)->getNode()->setNodeDisabled(true);
    }
    setText( QObject::tr("Disable nodes") );
}

EnableNodesCommand::EnableNodesCommand(const std::list<boost::shared_ptr<NodeGui> > & nodes,
                                       QUndoCommand *parent)
    : QUndoCommand(parent)
      , _nodes(nodes)
{
}

void
EnableNodesCommand::undo()
{
    for (std::list<NodeGuiPtr>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        (*it)->getNode()->setNodeDisabled(true);
    }
    setText( QObject::tr("Enable nodes") );
}

void
EnableNodesCommand::redo()
{
    for (std::list<NodeGuiPtr>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        (*it)->getNode()->setNodeDisabled(false);
    }
    setText( QObject::tr("Enable nodes") );
}

