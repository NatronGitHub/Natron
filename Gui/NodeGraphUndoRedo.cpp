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

MoveCommand::MoveCommand(const boost::shared_ptr<NodeGui>& node, const QPointF &oldPos,
                         QUndoCommand *parent):QUndoCommand(parent),
_node(node),
_oldPos(oldPos),
_newPos(node->pos()){
    assert(node);
    
}
void MoveCommand::undo(){
    
    _node->refreshPosition(_oldPos.x(),_oldPos.y());
    _node->refreshEdges();
    
    if(_node->scene())
        _node->scene()->update();
    setText(QObject::tr("Move %1")
            .arg(_node->getNode()->getName().c_str()));
}
void MoveCommand::redo(){
    
    _node->refreshPosition(_newPos.x(),_newPos.y());
    _node->refreshEdges();
    setText(QObject::tr("Move %1")
            .arg(_node->getNode()->getName().c_str()));
}
bool MoveCommand::mergeWith(const QUndoCommand *command){
    
    const MoveCommand *moveCommand = static_cast<const MoveCommand *>(command);
    
    const boost::shared_ptr<NodeGui>& node = moveCommand->_node;
    if(_node != node)
        return false;
    _newPos = node->pos();
    setText(QObject::tr("Move %1")
            .arg(node->getNode()->getName().c_str()));
    return true;
}


AddCommand::AddCommand(NodeGraph* graph,const boost::shared_ptr<NodeGui>& node,QUndoCommand *parent):QUndoCommand(parent),
_node(node),_graph(graph),_undoWasCalled(false), _isUndone(false){
    
}

AddCommand::~AddCommand()
{
    if (_isUndone) {
        _graph->deleteNodePermanantly(_node);
    }
}

void AddCommand::undo(){
    
    _isUndone = true;
    _undoWasCalled = true;
    
    
    
    _inputs = _node->getNode()->getInputs_mt_safe();
    _outputs = _node->getNode()->getOutputs();
    
    _node->getNode()->deactivate();
    _graph->getGui()->getApp()->triggerAutoSave();
    _graph->getGui()->getApp()->redrawAllViewers();
    
    
    _graph->scene()->update();
    setText(QObject::tr("Add %1")
            .arg(_node->getNode()->getName().c_str()));
    
}
void AddCommand::redo(){
    
    _isUndone = false;
    if(_undoWasCalled){
        ///activate will trigger an autosave
        _node->getNode()->activate();
    }
    _graph->scene()->update();
    setText(QObject::tr("Add %1")
            .arg(_node->getNode()->getName().c_str()));
    
    
}

RemoveCommand::RemoveCommand(NodeGraph* graph,const boost::shared_ptr<NodeGui>& node,QUndoCommand *parent):QUndoCommand(parent),
_node(node),_graph(graph) , _isRedone(false){
    assert(node);
}

RemoveCommand::~RemoveCommand()
{
    if (_isRedone) {
        _graph->deleteNodePermanantly(_node);
    }
}

void RemoveCommand::undo() {
    
    _node->getNode()->activate();
    _isRedone = false;
    _graph->scene()->update();
    setText(QObject::tr("Remove %1")
            .arg(_node->getNode()->getName().c_str()));
    
    
    
}
void RemoveCommand::redo() {
    
    _isRedone = true;
    _inputs = _node->getNode()->getInputs_mt_safe();
    _outputs = _node->getNode()->getOutputs();
    
    _node->getNode()->deactivate();
    _graph->getGui()->getApp()->triggerAutoSave();
    _graph->getGui()->getApp()->redrawAllViewers();
    
    
    for (std::list<boost::shared_ptr<Natron::Node> >::iterator it = _outputs.begin(); it!=_outputs.end(); ++it) {
        assert(*it);
        boost::shared_ptr<InspectorNode> inspector = boost::dynamic_pointer_cast<InspectorNode>(*it);
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
    
    std::list<SequenceTime> keyframes;
    _node->getNode()->getAllKnobsKeyframes(&keyframes);
    _graph->getGui()->getApp()->getTimeLine()->removeMultipleKeyframeIndicator(keyframes);
    
    _graph->scene()->update();
    setText(QObject::tr("Remove %1")
            .arg(_node->getNode()->getName().c_str()));
    
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



//////// Implementation of Backdrops undo/redo commands
AddBackDropCommand::AddBackDropCommand(NodeGraph* graph,NodeBackDrop* bd,QUndoCommand *parent)
: QUndoCommand(parent)
, _graph(graph)
, _bd(bd)
, _isUndone(false)
{
    
}

AddBackDropCommand::~AddBackDropCommand()
{
    if (_isUndone) {
        _bd->setParentItem(NULL);
        _graph->removeBackDrop(_bd);
        delete _bd;
    }
}

void AddBackDropCommand::undo()
{
    _bd->setActive(false);
    _bd->setVisible(false);
    _bd->setSettingsPanelClosed(true);
    _isUndone = true;
    _graph->getGui()->getApp()->triggerAutoSave();
    setText(QObject::tr("Add %1").arg(_bd->getName()));
}

void AddBackDropCommand::redo()
{
    _bd->setActive(true);
    _bd->setVisible(true);
    _isUndone = false;
    _graph->getGui()->getApp()->triggerAutoSave();
    setText(QObject::tr("Add %1").arg(_bd->getName()));
}


RemoveBackDropCommand::RemoveBackDropCommand(NodeGraph* graph,NodeBackDrop* bd,const std::list<boost::shared_ptr<NodeGui> >& nodes,
                                             QUndoCommand *parent)
: QUndoCommand(parent)
, _graph(graph)
, _bd(bd)
, _nodes(nodes)
, _isRedone(false)
{
    
}

RemoveBackDropCommand::~RemoveBackDropCommand()
{
    if (_isRedone) {
        _bd->setParentItem(NULL);
        _graph->removeBackDrop(_bd);
        delete _bd;
    }
}


void RemoveBackDropCommand::undo()
{
    _bd->activate();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it!= _nodes.end();++it) {
        (*it)->getNode()->activate();
    }
    _graph->getGui()->getApp()->triggerAutoSave();
    _isRedone = false;
    setText(QObject::tr("Remove %1").arg(_bd->getName()));
}

void RemoveBackDropCommand::redo()
{
    _bd->deactivate();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it!= _nodes.end();++it) {
        (*it)->getNode()->deactivate();
    }
    _graph->getGui()->getApp()->triggerAutoSave();
    _isRedone = true;
    setText(QObject::tr("Remove %1").arg(_bd->getName()));
}

MoveBackDropCommand::MoveBackDropCommand(NodeBackDrop* bd,double dx,double dy,
                                         const std::list<boost::shared_ptr<NodeGui> >& nodes,
                                         QUndoCommand *parent )
: QUndoCommand(parent)
, _bd(bd)
, _nodes(nodes)
, _dx(dx)
, _dy(dy)
{
    
}

MoveBackDropCommand::~MoveBackDropCommand()
{
    
}


void MoveBackDropCommand::undo()
{
    move(-_dx,-_dy);
    setText(QObject::tr("Move %1").arg(_bd->getName()));
}

void MoveBackDropCommand::move(double dx,double dy)
{
    QPointF delta(dx,dy);
    QPointF bdPos = _bd->pos();
    _bd->setPos_mt_safe(bdPos + delta);
    
    ///Also move all the nodes
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it!= _nodes.end();++it) {
        QPointF nodePos = (*it)->pos();
        nodePos += delta;
        (*it)->refreshPosition(nodePos.x(), nodePos.y(),true);
    }
    
    
}

void MoveBackDropCommand::redo()
{
    move(_dx,_dy);
    setText(QObject::tr("Move %1").arg(_bd->getName()));
}

bool MoveBackDropCommand::mergeWith(const QUndoCommand *command)
{
    const MoveBackDropCommand* mvCmd = dynamic_cast<const MoveBackDropCommand*>(command);
    if (!mvCmd) {
        return false;
    }
    if (mvCmd->_bd != _bd || mvCmd->_nodes.size() != _nodes.size()) {
        return false;
    }
    
    ///check nodes
    std::list<boost::shared_ptr<NodeGui> >::const_iterator itOther = mvCmd->_nodes.begin();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it!= _nodes.end();++it,++itOther) {
        if (*itOther != *it) {
            return false;
        }
    }
    _dx += mvCmd->_dx;
    _dy += mvCmd->_dy;
    return true;
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


DuplicateBackDropCommand::DuplicateBackDropCommand(NodeGraph* graph,NodeBackDrop* bd,
                                                   const std::list<boost::shared_ptr<NodeGui> >& nodes,QUndoCommand *parent)
: QUndoCommand(parent)
, _graph(graph)
, _bd(bd)
, _nodes(nodes)
, _isUndone(false)
, _firstRedoCalled(false)
{
    
}

DuplicateBackDropCommand::~DuplicateBackDropCommand()
{
    if (_isUndone) {
        _bd->setParentItem(NULL);
        _graph->removeBackDrop(_bd);
        delete _bd;
    }
}

void DuplicateBackDropCommand::undo()
{
    _bd->deactivate();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it!= _nodes.end();++it) {
        (*it)->getNode()->deactivate();
    }
    _isUndone = true;
    _graph->getGui()->getApp()->triggerAutoSave();
    setText(QObject::tr("Duplicate %1").arg(_bd->getName()));
}

void DuplicateBackDropCommand::redo()
{
    if (_firstRedoCalled) {
        for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it!= _nodes.end();++it) {
            (*it)->getNode()->activate();
        }
        _bd->activate();
    }
    _isUndone = false;
    _graph->getGui()->getApp()->triggerAutoSave();
    setText(QObject::tr("Duplicate %1").arg(_bd->getName()));
}




