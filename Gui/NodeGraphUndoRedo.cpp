//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "NodeGraphUndoRedo.h"

#include <QDebug>

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

MoveMultipleNodesCommand::MoveMultipleNodesCommand(const std::list<NodeToMove>& nodes,
                                                   const std::list<NodeBackDrop*>& bds,
                                                   double dx,double dy,
                                                   const QPointF& mouseScenePos,
                                                   QUndoCommand *parent)
: QUndoCommand(parent)
, _nodes(nodes)
, _bds(bds)
, _mouseScenePos(mouseScenePos)
, _dx(dx)
, _dy(dy)
{
    assert(!nodes.empty() || !bds.empty());
}

void MoveMultipleNodesCommand::move(double dx,double dy)
{
    for (std::list<NodeToMove>::iterator it = _nodes.begin();it!=_nodes.end();++it) {
        QPointF pos = it->node->getPos_mt_safe();
        it->node->refreshPosition(pos.x() + dx , pos.y() + dy,it->isWithinBD || _nodes.size() > 1,_mouseScenePos);
    }
    for (std::list<NodeBackDrop*>::iterator it = _bds.begin();it!=_bds.end();++it) {
        QPointF pos = (*it)->getPos_mt_safe();
        pos += QPointF(dx,dy);
        (*it)->setPos_mt_safe(pos);
    }
}

void MoveMultipleNodesCommand::undo(){
    
    move(-_dx, -_dy);
    setText(QObject::tr("Move nodes"));
}
void MoveMultipleNodesCommand::redo(){
    
    move(_dx, _dy);
    setText(QObject::tr("Move nodes"));
}
bool MoveMultipleNodesCommand::mergeWith(const QUndoCommand *command){
    
    const MoveMultipleNodesCommand *mvCmd = static_cast<const MoveMultipleNodesCommand *>(command);
    
    if (!mvCmd) {
        return false;
    }
    if (mvCmd->_bds.size() != _bds.size() || mvCmd->_nodes.size() != _nodes.size()) {
        return false;
    }
    {
        std::list<NodeToMove >::const_iterator itOther = mvCmd->_nodes.begin();
        for (std::list<NodeToMove>::const_iterator it = _nodes.begin();it!=_nodes.end();++it,++itOther) {
            if (it->node != itOther->node) {
                return false;
            }
        }
        
    }
    {
        std::list<NodeBackDrop*>::const_iterator itOther = mvCmd->_bds.begin();
        for (std::list<NodeBackDrop*>::const_iterator it = _bds.begin();it!=_bds.end();++it,++itOther) {
            if (*itOther != *it) {
                return false;
            }
        }
    }
    _dx += mvCmd->_dx;
    _dy += mvCmd->_dy;
    return true;
}


AddMultipleNodesCommand::AddMultipleNodesCommand(NodeGraph* graph,const std::list<boost::shared_ptr<NodeGui> >& nodes,
                                                 const std::list<NodeBackDrop*>& bds,
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
                        const boost::shared_ptr<NodeGui>& node,
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
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _nodes.begin(); it!=_nodes.end(); ++it) {
            _graph->deleteNodePermanantly(*it);
        }
        
        for (std::list<NodeBackDrop*>::iterator it = _bds.begin(); it!= _bds.end(); ++it) {
            (*it)->setParentItem(NULL);
            _graph->removeBackDrop(*it);
            delete *it;
        }
    }
}

void AddMultipleNodesCommand::undo() {
    
    _isUndone = true;
    
    for (std::list<NodeBackDrop*>::iterator it = _bds.begin(); it!= _bds.end(); ++it) {
        (*it)->deactivate();
    }
    
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it!=_nodes.end(); ++it) {
        (*it)->getNode()->deactivate();
    }
    _graph->getGui()->getApp()->triggerAutoSave();
    _graph->getGui()->getApp()->checkViewersConnection();
    
    
    setText(QObject::tr("Add node"));
    
}
void AddMultipleNodesCommand::redo() {
    
    _isUndone = false;
    if (_firstRedoCalled) {
        
        for (std::list<NodeBackDrop*>::iterator it = _bds.begin(); it!= _bds.end(); ++it) {
            (*it)->activate();
        }
        
        for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it!=_nodes.end(); ++it) {
            (*it)->getNode()->activate();
        }
    }
    
    _graph->getGui()->getApp()->triggerAutoSave();
    _graph->getGui()->getApp()->checkViewersConnection();

    
    _firstRedoCalled = true;
    setText(QObject::tr("Add node"));
    
    
}

RemoveMultipleNodesCommand::RemoveMultipleNodesCommand(NodeGraph* graph,
                             const std::list<boost::shared_ptr<NodeGui> >& nodes,
                             const std::list<NodeBackDrop*>& bds,
                             QUndoCommand *parent)
: QUndoCommand(parent)
, _nodes()
, _bds(bds)
, _graph(graph)
, _isRedone(false)
{
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin(); it!=nodes.end(); ++it) {
        NodeToRemove n;
        n.node = *it;
        
        ///find all outputs to restore
        const std::list<boost::shared_ptr<Natron::Node> >& outputs = (*it)->getNode()->getOutputs();
        for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it2 = outputs.begin(); it2!=outputs.end(); ++it2) {
            bool restore = true;
            for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it3 = nodes.begin();it3!=nodes.end();++it3) {
                if ((*it3)->getNode() == *it2) {
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
        for (std::list<NodeToRemove>::iterator it = _nodes.begin(); it!=_nodes.end(); ++it) {
            _graph->deleteNodePermanantly(it->node);
        }
        
        for (std::list<NodeBackDrop*>::iterator it = _bds.begin(); it!= _bds.end(); ++it) {
            (*it)->setParentItem(NULL);
            _graph->removeBackDrop(*it);
            delete *it;
        }
    }
}

void RemoveMultipleNodesCommand::undo() {
    
    std::list<SequenceTime> allKeysToAdd;
    std::list<NodeToRemove>::iterator next = _nodes.begin();
    ++next;
    for (std::list<NodeToRemove>::iterator it = _nodes.begin(); it!=_nodes.end(); ++it,++next) {
        it->node->getNode()->activate(it->outputsToRestore,false);
        if (it->node->isSettingsPanelVisible()) {
            it->node->getNode()->showKeyframesOnTimeline(next == _nodes.end());
        }
    }
    for (std::list<NodeBackDrop*>::iterator it = _bds.begin(); it!= _bds.end(); ++it) {
        (*it)->activate();
    }
    
    _isRedone = false;
    _graph->scene()->update();
    setText(QObject::tr("Remove node"));
    
    
    
}
void RemoveMultipleNodesCommand::redo() {
    
    _isRedone = true;
    
    std::list<NodeToRemove>::iterator next = _nodes.begin();
    ++next;
    for (std::list<NodeToRemove>::iterator it = _nodes.begin(); it!=_nodes.end(); ++it,++next) {
        
        
        ///Make a copy before calling deactivate which will modify the list
        std::list<boost::shared_ptr<Natron::Node> > outputs = it->node->getNode()->getOutputs();

        it->node->getNode()->deactivate(it->outputsToRestore,false,_nodes.size() == 1);
        
        if (_nodes.size() == 1) {
            ///If we're deleting a single node and there's a viewer in output,reconnect the viewer to another connected input it has
            for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it2 = outputs.begin(); it2!=outputs.end(); ++it2) {
                assert(*it2);
                
                ///the output must be in the outputs to restore
                std::list<boost::shared_ptr<Natron::Node> >::const_iterator found =
                std::find(it->outputsToRestore.begin(),it->outputsToRestore.end(),*it2);
                
                if (found != it->outputsToRestore.end()) {
                    InspectorNode* inspector = dynamic_cast<InspectorNode*>(it2->get());
                    ///if the node is an inspector, when disconnecting the active input just activate another input instead
                    if (inspector) {
                        const std::vector<boost::shared_ptr<Natron::Node> >& inputs = inspector->getInputs_mt_safe();
                        ///set as active input the first non null input
                        for (U32 i = 0; i < inputs.size() ;++i) {
                            if (inputs[i]) {
                                inspector->setActiveInputAndRefresh(i);
                                break;
                            }
                        }
                    }
                }
            }

        }
        if (it->node->isSettingsPanelVisible()) {
            it->node->getNode()->hideKeyframesFromTimeline(next == _nodes.end());
        }

    }
    for (std::list<NodeBackDrop*>::iterator it = _bds.begin(); it!= _bds.end(); ++it) {
        (*it)->deactivate();
    }
    
    _graph->getGui()->getApp()->triggerAutoSave();
    _graph->getGui()->getApp()->redrawAllViewers();
    
    _graph->scene()->update();
    setText(QObject::tr("Remove node"));
    
}


ConnectCommand::ConnectCommand(NodeGraph* graph,Edge* edge,const boost::shared_ptr<NodeGui>& oldSrc,
                               const boost::shared_ptr<NodeGui>& newSrc,QUndoCommand *parent):QUndoCommand(parent),
_edge(edge),
_oldSrc(oldSrc),
_newSrc(newSrc),
_dst(edge->getDest()),
_graph(graph),
_inputNb(edge->getInputNumber())
{
    assert(_dst);
}

void ConnectCommand::undo() {
    
    boost::shared_ptr<InspectorNode> inspector = boost::dynamic_pointer_cast<InspectorNode>(_dst->getNode());
    
    if (inspector) {
        ///if the node is an inspector, the redo() action might have disconnect the dst and src nodes
        ///hence the _edge ptr might have been invalidated, recreate it
        NodeGui::InputEdgesMap::const_iterator it = _dst->getInputsArrows().find(_inputNb);
        while (it == _dst->getInputsArrows().end()) {
            inspector->addEmptyInput();
            it = _dst->getInputsArrows().find(_inputNb);
        }
        _edge = it->second;
    }
    
    if(_oldSrc){
        _graph->getGui()->getApp()->getProject()->connectNodes(_edge->getInputNumber(), _oldSrc->getNode(), _edge->getDest()->getNode());
        _oldSrc->refreshOutputEdgeVisibility();
    }
    if(_newSrc){
        _graph->getGui()->getApp()->getProject()->disconnectNodes(_newSrc->getNode(), _edge->getDest()->getNode());
        _newSrc->refreshOutputEdgeVisibility();
        ///if the node is an inspector, when disconnecting the active input just activate another input instead
        if (inspector) {
            const std::vector<boost::shared_ptr<Natron::Node> >& inputs = inspector->getInputs_mt_safe();
            ///set as active input the first non null input
            for (U32 i = 0; i < inputs.size() ;++i) {
                if (inputs[i]) {
                    inspector->setActiveInputAndRefresh(i);
                    break;
                }
            }
        }
    }
    
    if(_oldSrc){
        setText(QObject::tr("Connect %1 to %2")
                .arg(_edge->getDest()->getNode()->getName().c_str()).arg(_oldSrc->getNode()->getName().c_str()));
    }else{
        setText(QObject::tr("Disconnect %1")
                .arg(_edge->getDest()->getNode()->getName().c_str()));
    }
    
    _graph->getGui()->getApp()->triggerAutoSave();
    std::list<ViewerInstance* > viewers;
    _edge->getDest()->getNode()->hasViewersConnected(&viewers);
    for(std::list<ViewerInstance* >::iterator it = viewers.begin();it!=viewers.end();++it){
        (*it)->updateTreeAndRender();
    }
    ///if there are no viewers, at least update the render inputs
    if (viewers.empty()) {
        _edge->getDest()->getNode()->updateRenderInputs();
        if (_edge->getSource()) {
            _edge->getSource()->getNode()->updateRenderInputs();
        }
    }
}
void ConnectCommand::redo() {
    
    boost::shared_ptr<InspectorNode> inspector = boost::dynamic_pointer_cast<InspectorNode>(_dst->getNode());
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
            while (it == _dst->getInputsArrows().end()) {
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
            if(!_graph->getGui()->getApp()->getProject()->disconnectNodes(_oldSrc->getNode(), _dst->getNode())){
                qDebug() << "Failed to disconnect (input) " << _oldSrc->getNode()->getName().c_str()
                << " to (output) " << _dst->getNode()->getName().c_str();
            }
            _oldSrc->refreshOutputEdgeVisibility();
        }
        if (_newSrc) {
            if(!_graph->getGui()->getApp()->getProject()->connectNodes(_inputNb, _newSrc->getNode(), _dst->getNode())){
                qDebug() << "Failed to connect (input) " << _newSrc->getNode()->getName().c_str()
                << " to (output) " << _dst->getNode()->getName().c_str();
                
            }
            _newSrc->refreshOutputEdgeVisibility();
        }
        
    }
    
    assert(_dst);
    _dst->refreshEdges();
    
    if (_newSrc) {
        setText(QObject::tr("Connect %1 to %2")
                .arg(_edge->getDest()->getNode()->getName().c_str()).arg(_newSrc->getNode()->getName().c_str()));
        
        
    } else {
        setText(QObject::tr("Disconnect %1")
                .arg(_edge->getDest()->getNode()->getName().c_str()));
    }
    
    ///if the node has no inputs, all the viewers attached to that node should get disconnected. This will be done
    ///in VideoEngine::startEngine
    std::list<ViewerInstance* > viewers;
    _edge->getDest()->getNode()->hasViewersConnected(&viewers);
    for(std::list<ViewerInstance* >::iterator it = viewers.begin();it!=viewers.end();++it){
        (*it)->updateTreeAndRender();
    }
    
    ///if there are no viewers, at least update the render inputs
    if (viewers.empty()) {
        _edge->getDest()->getNode()->updateRenderInputs();
        if (_edge->getSource()) {
            _edge->getSource()->getNode()->updateRenderInputs();
        }
    }
    
    ViewerInstance* isDstAViewer = dynamic_cast<ViewerInstance*>(_edge->getDest()->getNode()->getLiveInstance());
    if (!isDstAViewer) {
        _graph->getGui()->getApp()->triggerAutoSave();
    }
    
    
    
}



ResizeBackDropCommand::ResizeBackDropCommand(NodeBackDrop* bd,int w,int h,QUndoCommand *parent)
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

void ResizeBackDropCommand::undo()
{
    _bd->resize(_oldW, _oldH);
    setText(QObject::tr("Resize %1").arg(_bd->getName()));
}

void ResizeBackDropCommand::redo()
{
    _bd->resize(_w, _h);
    setText(QObject::tr("Resize %1").arg(_bd->getName()));
}

bool ResizeBackDropCommand::mergeWith(const QUndoCommand *command)
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
                                                     const std::list<boost::shared_ptr<NodeGui> >& nodes,
                                                     const std::list<NodeBackDrop*>& bds,
                                                     QUndoCommand *parent)
: QUndoCommand(parent)
, _nodes()
, _bds()
, _graph(graph)
{
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin(); it!=nodes.end(); ++it) {
        NodeToDeclone n;
        n.node = *it;
        n.master = (*it)->getNode()->getMasterNode();
        assert(n.master);
        _nodes.push_back(n);
    }
    
    for (std::list<NodeBackDrop*>::const_iterator it = bds.begin(); it!=bds.end(); ++it) {
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

void DecloneMultipleNodesCommand::undo()
{
    for (std::list<NodeToDeclone>::iterator it = _nodes.begin(); it!=_nodes.end(); ++it) {
        it->node->getNode()->getLiveInstance()->slaveAllKnobs(it->master->getLiveInstance());
    }
    for (std::list<BDToDeclone>::iterator it = _bds.begin(); it!=_bds.end(); ++it) {
        it->bd->slaveTo(it->master);
    }
    _graph->getGui()->getApp()->triggerAutoSave();
    setText("Declone node");
}

void DecloneMultipleNodesCommand::redo()
{
    for (std::list<NodeToDeclone>::iterator it = _nodes.begin(); it!=_nodes.end(); ++it) {
        it->node->getNode()->getLiveInstance()->unslaveAllKnobs();
    }
    for (std::list<BDToDeclone>::iterator it = _bds.begin(); it!=_bds.end(); ++it) {
        it->bd->unslave();
    }
    _graph->getGui()->getApp()->triggerAutoSave();
    setText("Declone node");
}
