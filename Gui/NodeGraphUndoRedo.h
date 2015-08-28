/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NODEGRAPHUNDOREDO_H
#define NODEGRAPHUNDOREDO_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/GlobalDefines.h"
#include <list>
#include <vector>
#include <QPointF>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif
#include <QUndoCommand>

/**
 * This file gathers undo/redo command associated to the node graph. Each of them triggers an autosave when redone/undone
 * except the move command.
 **/

class Edge;
class NodeGui;
class NodeGraph;
class NodeSerialization;

typedef boost::shared_ptr<NodeGui> NodeGuiPtr;
typedef std::list<NodeGuiPtr> NodeGuiList;

namespace Natron {
class Node;
}


class MoveMultipleNodesCommand
    : public QUndoCommand
{
public:

    MoveMultipleNodesCommand(const NodeGuiList & nodes,
                             double dx,
                             double dy,
                             QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();


private:

    void move(double dx,double dy);

    bool _firstRedoCalled;
    NodeGuiList _nodes;
    double _dx,_dy;
};


/**
 * @class The first redo() call will do nothing, it assumes the node has already been added.
 * undo() just call deactivate() on all nodes, and following calls to redo() call activate()
 *
 * This class is used for many similar action such as add/duplicate/paste,etc...
 **/
class AddMultipleNodesCommand
    : public QUndoCommand
{
public:

    AddMultipleNodesCommand(NodeGraph* graph,
                            const std::list<boost::shared_ptr<NodeGui> > & nodes,
                            QUndoCommand *parent = 0);

    AddMultipleNodesCommand(NodeGraph* graph,
                            const boost::shared_ptr<NodeGui> & node,
                            QUndoCommand* parent = 0);


    virtual ~AddMultipleNodesCommand();

    virtual void undo();
    virtual void redo();

private:

    std::list<boost::weak_ptr<NodeGui> > _nodes;
    NodeGraph* _graph;
    bool _firstRedoCalled;
    bool _isUndone;
};


class RemoveMultipleNodesCommand
    : public QUndoCommand
{
public:

    RemoveMultipleNodesCommand(NodeGraph* graph,
                               const std::list<boost::shared_ptr<NodeGui> > & nodes,
                               QUndoCommand *parent = 0);

    virtual ~RemoveMultipleNodesCommand();

    virtual void undo();
    virtual void redo();

private:
    struct NodeToRemove
    {
        ///This contains the output nodes whose input should be reconnected to this node afterwards.
        ///This list contains only nodes that are not part of the selection: we restore only the
        ///inputs of the outputs nodes of the graph that were not removed
        std::list<Natron::Node* > outputsToRestore;
        boost::weak_ptr<NodeGui> node;
    };

    std::list<NodeToRemove> _nodes;
    NodeGraph* _graph;
    bool _isRedone;
};

class ConnectCommand
    : public QUndoCommand
{
public:
    ConnectCommand(NodeGraph* graph,
                   Edge* edge,
                   const boost::shared_ptr<NodeGui> &oldSrc,
                   const boost::shared_ptr<NodeGui> & newSrc,
                   QUndoCommand *parent = 0);

    virtual void undo();
    virtual void redo();

private:
    
    void doConnect(const boost::shared_ptr<Natron::Node> &oldSrc,
                   const boost::shared_ptr<Natron::Node> & newSrc);
    
    boost::weak_ptr<NodeGui> _oldSrc,_newSrc;
    boost::weak_ptr<NodeGui> _dst;
    NodeGraph* _graph;
    int _inputNb;
};


class ResizeBackDropCommand
    : public QUndoCommand
{
public:

    ResizeBackDropCommand(const NodeGuiPtr& bd,
                          int w,
                          int h,
                          QUndoCommand *parent = 0);
    virtual ~ResizeBackDropCommand();

    virtual void undo();
    virtual void redo();
    virtual int id() const
    {
        return kNodeGraphResizeNodeBackDropCommandCompressionID;
    }

    virtual bool mergeWith(const QUndoCommand *command);

private:

    NodeGuiPtr _bd;
    int _w,_h;
    int _oldW,_oldH;
};


class DecloneMultipleNodesCommand
    : public QUndoCommand
{
public:

    DecloneMultipleNodesCommand(NodeGraph* graph,
                                const std::list<boost::shared_ptr<NodeGui> > & nodes,
                                QUndoCommand *parent = 0);


    virtual ~DecloneMultipleNodesCommand();

    virtual void undo();
    virtual void redo();

private:

    struct NodeToDeclone
    {
        boost::weak_ptr<NodeGui> node;
        boost::weak_ptr<Natron::Node> master;
    };

  

    std::list<NodeToDeclone> _nodes;
    NodeGraph* _graph;
};


class RearrangeNodesCommand
    : public QUndoCommand
{
public:

    struct NodeToRearrange
    {
        boost::shared_ptr<NodeGui> node;
        QPointF oldPos,newPos;
    };

    RearrangeNodesCommand(const std::list<boost::shared_ptr<NodeGui> > & nodes,
                          QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();

private:

    std::list<NodeToRearrange> _nodes;
};

class DisableNodesCommand
    : public QUndoCommand
{
public:

    DisableNodesCommand(const std::list<boost::shared_ptr<NodeGui> > & nodes,
                        QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();

private:

    std::list<boost::weak_ptr<NodeGui> >_nodes;
};

class EnableNodesCommand
    : public QUndoCommand
{
public:

    EnableNodesCommand(const std::list<boost::shared_ptr<NodeGui> > & nodes,
                       QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();

private:

    std::list<boost::weak_ptr<NodeGui> >_nodes;
};


class LoadNodePresetsCommand
: public QUndoCommand
{
    
public:
    
    LoadNodePresetsCommand(const boost::shared_ptr<NodeGui> & node,
                           const std::list<boost::shared_ptr<NodeSerialization> >& serialization,
                       QUndoCommand *parent = 0);
    
    virtual ~LoadNodePresetsCommand();
    virtual void undo();
    virtual void redo();

private:
    
    void getListAsShared(const std::list< boost::weak_ptr<Natron::Node> >& original,
                         std::list< boost::shared_ptr<Natron::Node> >& shared) const;
    
    bool _firstRedoCalled;
    bool _isUndone;
    boost::weak_ptr<NodeGui> _node;
    std::list< boost::weak_ptr<Natron::Node> > _oldChildren,_newChildren; //< children if multi-instance
    std::list<boost::shared_ptr<NodeSerialization> > _newSerializations,_oldSerialization;
};

class RenameNodeUndoRedoCommand
: public QUndoCommand
{
public:
    
    RenameNodeUndoRedoCommand(const boost::shared_ptr<NodeGui> & node,
                              const QString& oldName,
                              const QString& newName);
    
    virtual ~RenameNodeUndoRedoCommand();
    virtual void undo();
    virtual void redo();
    
private:
    
    boost::weak_ptr<NodeGui> _node;
    QString _oldName,_newName;
};

struct ExtractedOutput
{
    boost::weak_ptr<NodeGui> node;
    std::list<std::pair<int,Natron::Node*> > outputs;
};

struct ExtractedInput
{
    boost::weak_ptr<NodeGui> node;
    std::vector<boost::weak_ptr<Natron::Node> > inputs;
};

struct ExtractedTree
{
    ExtractedOutput output;
    std::list<ExtractedInput> inputs;
    std::list<boost::weak_ptr<NodeGui> > inbetweenNodes;
};


class ExtractNodeUndoRedoCommand
: public QUndoCommand
{
public:
    
    ExtractNodeUndoRedoCommand(NodeGraph* graph,const std::list<boost::shared_ptr<NodeGui> > & nodes);
    
    virtual ~ExtractNodeUndoRedoCommand();
    virtual void undo();
    virtual void redo();
    
    
    private:
    
  
    NodeGraph* _graph;
    std::list<ExtractedTree> _trees;
};

class GroupFromSelectionCommand
: public QUndoCommand
{
    
public:
    
    GroupFromSelectionCommand(NodeGraph* graph,const std::list<boost::shared_ptr<NodeGui> > & nodes);
    
    virtual ~GroupFromSelectionCommand();
    
    virtual void undo();
    virtual void redo();
    
private:
    
    NodeGraph* _graph;
    std::list<boost::weak_ptr<NodeGui> > _originalNodes;
    boost::weak_ptr<NodeGui> _group;
    bool _firstRedoCalled;
    bool _isRedone;
};

class InlineGroupCommand
: public QUndoCommand
{
    
public:
    
    InlineGroupCommand(NodeGraph* graph,const std::list<boost::shared_ptr<NodeGui> > & nodes);
    
    virtual ~InlineGroupCommand();
    
    virtual void undo();
    virtual void redo();
    
private:
    
    struct NodeToConnect {
        boost::weak_ptr<NodeGui> input;
        std::map<boost::weak_ptr<NodeGui>,int> outputs;
    };
    
    
    struct InlinedGroup
    {
        boost::weak_ptr<NodeGui> group;
        std::list<boost::weak_ptr<NodeGui> > inlinedNodes;
        std::map<int,NodeToConnect> connections;
    };
    
    NodeGraph* _graph;
    
    std::list<InlinedGroup> _groupNodes;
    bool _firstRedoCalled;
};


#endif // NODEGRAPHUNDOREDO_H
