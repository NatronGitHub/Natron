//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef NODEGRAPHUNDOREDO_H
#define NODEGRAPHUNDOREDO_H

#include "Global/GlobalDefines.h"

#include <vector>
#include <boost/shared_ptr.hpp>

#include <QUndoCommand>
#include <QPointF>

class Edge;
class NodeGui;
class NodeGraph;
class NodeBackDrop;
namespace Natron
{
class Node;
}

class MoveCommand : public QUndoCommand
{
public:
    MoveCommand(const boost::shared_ptr<NodeGui>& node, const QPointF &oldPos,
                QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();
    virtual int id() const { return kNodeGraphMoveNodeCommandCompressionID; }
    virtual bool mergeWith(const QUndoCommand *command);
    
    
private:
    boost::shared_ptr<NodeGui> _node;
    QPointF _oldPos;
    QPointF _newPos;
};


class AddCommand : public QUndoCommand
{
public:
    
    AddCommand(NodeGraph* graph,const boost::shared_ptr<NodeGui>& node,QUndoCommand *parent = 0);
    
    virtual ~AddCommand();
    
    virtual void undo();
    virtual void redo();
    
private:
    std::list<boost::shared_ptr<Natron::Node> > _outputs;
    std::vector<boost::shared_ptr<Natron::Node> > _inputs;
    boost::shared_ptr<NodeGui> _node;
    NodeGraph* _graph;
    bool _undoWasCalled;
    bool _isUndone;
};

class RemoveCommand : public QUndoCommand{
public:
    
    RemoveCommand(NodeGraph* graph,const boost::shared_ptr<NodeGui>& node,QUndoCommand *parent = 0);
    
    virtual ~RemoveCommand();
    
    virtual void undo();
    virtual void redo();
    
private:
    std::list<boost::shared_ptr<Natron::Node> > _outputs;
    std::vector<boost::shared_ptr<Natron::Node> > _inputs;
    boost::shared_ptr<NodeGui> _node; //< mutable because we need to modify it externally
    NodeGraph* _graph;
    bool _isRedone;
};

class ConnectCommand : public QUndoCommand{
public:
    ConnectCommand(NodeGraph* graph,Edge* edge,const boost::shared_ptr<NodeGui>&oldSrc,
                   const boost::shared_ptr<NodeGui>& newSrc,QUndoCommand *parent = 0);
    
    virtual void undo();
    virtual void redo();
    
private:
    Edge* _edge;
    boost::shared_ptr<NodeGui> _oldSrc,_newSrc;
    boost::shared_ptr<NodeGui> _dst;
    NodeGraph* _graph;
    int _inputNb;
};

class AddBackDropCommand : public QUndoCommand {
    
public:
    
    AddBackDropCommand(NodeGraph* graph,NodeBackDrop* bd,QUndoCommand *parent = 0);
    virtual ~AddBackDropCommand();
    
    virtual void undo();
    virtual void redo();
    
private:
    
    NodeGraph* _graph;
    NodeBackDrop* _bd;
    bool _isUndone;
};


class RemoveBackDropCommand : public QUndoCommand {
    
public:
    
    RemoveBackDropCommand(NodeGraph* graph,NodeBackDrop* bd,const std::list<boost::shared_ptr<NodeGui> >& nodes,QUndoCommand *parent = 0);
    virtual ~RemoveBackDropCommand();
    
    virtual void undo();
    virtual void redo();
    
private:
    
    NodeGraph* _graph;
    NodeBackDrop* _bd;
    std::list<boost::shared_ptr<NodeGui> > _nodes;
    bool _isRedone;
};


class MoveBackDropCommand : public QUndoCommand {
    
public:
    
    MoveBackDropCommand(NodeBackDrop* bd,double dx,double dy,
                        const std::list<boost::shared_ptr<NodeGui> >& nodes,
                        QUndoCommand *parent = 0);
    virtual ~MoveBackDropCommand();
    
    virtual void undo();
    virtual void redo();
    virtual int id() const { return kNodeGraphMoveNodeBackDropCommandCompressionID; }
    virtual bool mergeWith(const QUndoCommand *command);
    
    
private:
    
    void move(double dx,double dy);
    
    NodeBackDrop* _bd;
    std::list<boost::shared_ptr<NodeGui> > _nodes;
    double _dx,_dy;
    
};

class ResizeBackDropCommand : public QUndoCommand {
    
public:
    
    ResizeBackDropCommand(NodeBackDrop* bd,int w,int h,QUndoCommand *parent = 0);
    virtual ~ResizeBackDropCommand();
    
    virtual void undo();
    virtual void redo();
    virtual int id() const { return kNodeGraphResizeNodeBackDropCommandCompressionID; }
    virtual bool mergeWith(const QUndoCommand *command);
    
    
private:
    
    NodeBackDrop* _bd;
    int _w,_h;
    int _oldW,_oldH;
    
};

class DuplicateBackDropCommand : public QUndoCommand {
    
public:
    
    DuplicateBackDropCommand(NodeGraph* graph,NodeBackDrop* bd,const std::list<boost::shared_ptr<NodeGui> >& nodes,QUndoCommand *parent = 0);
    virtual ~DuplicateBackDropCommand();
    
    virtual void undo();
    virtual void redo();
    
private:
    
    NodeGraph* _graph;
    NodeBackDrop* _bd;
    std::list<boost::shared_ptr<NodeGui> > _nodes;
    bool _isUndone;
    bool _firstRedoCalled;
};

#endif // NODEGRAPHUNDOREDO_H
