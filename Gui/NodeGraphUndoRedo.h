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

#ifndef NODEGRAPHUNDOREDO_H
#define NODEGRAPHUNDOREDO_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <vector>
#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include <QtCore/QPointF>
#include <QtCore/QCoreApplication>
#include <QUndoCommand>

#include "Global/GlobalDefines.h"

#include "Engine/EngineFwd.h"

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * This file gathers undo/redo command associated to the node graph. Each of them triggers an autosave when redone/undone
 * except the move command.
 **/


class MoveMultipleNodesCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(MoveMultipleNodesCommand)

public:

    MoveMultipleNodesCommand(const NodesGuiList & nodes,
                             double dx,
                             double dy,
                             QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();

private:

    void move(double dx, double dy);

    bool _firstRedoCalled;
    NodesGuiList _nodes;
    double _dx, _dy;
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
    Q_DECLARE_TR_FUNCTIONS(AddMultipleNodesCommand)

public:

    AddMultipleNodesCommand(NodeGraph* graph,
                            const NodesGuiList & nodes,
                            QUndoCommand *parent = 0);

    AddMultipleNodesCommand(NodeGraph* graph,
                            const NodeGuiPtr & node,
                            QUndoCommand* parent = 0);


    virtual ~AddMultipleNodesCommand();

    virtual void undo();
    virtual void redo();

private:

    std::list<NodeGuiWPtr> _nodes;
    NodeGraph* _graph;
    bool _firstRedoCalled;
    bool _isUndone;
};


class RemoveMultipleNodesCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(RemoveMultipleNodesCommand)

public:

    RemoveMultipleNodesCommand(NodeGraph* graph,
                               const NodesGuiList & nodes,
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
        NodesWList outputsToRestore;
        NodeGuiWPtr node;
    };

    std::list<NodeToRemove> _nodes;
    NodeGraph* _graph;
    bool _isRedone;
};

class ConnectCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(ConnectCommand)

public:
    ConnectCommand(NodeGraph* graph,
                   Edge* edge,
                   const NodeGuiPtr &oldSrc,
                   const NodeGuiPtr & newSrc,
                   QUndoCommand *parent = 0);

    virtual void undo();
    virtual void redo();

protected:

    static void doConnect(const NodeGuiPtr &oldSrc,
                          const NodeGuiPtr & newSrc,
                          const NodeGuiPtr& dst,
                          int inputNb);
    NodeGuiWPtr _oldSrc, _newSrc;
    NodeGuiWPtr _dst;
    NodeGraph* _graph;
    int _inputNb;
};

/*
 * @brief Inserts an existing node in a stream
 */
class InsertNodeCommand
    : public ConnectCommand
{
    Q_DECLARE_TR_FUNCTIONS(InsertNodeCommand)

public:

    InsertNodeCommand(NodeGraph* graph,
                      Edge* edge,
                      const NodeGuiPtr & newSrc,
                      QUndoCommand *parent = 0);

    virtual void undo();
    virtual void redo();

private:

    Edge* _inputEdge;
};


class ResizeBackdropCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(ResizeBackdropCommand)

public:

    ResizeBackdropCommand(const NodeGuiPtr& bd,
                          int w,
                          int h,
                          QUndoCommand *parent = 0);
    virtual ~ResizeBackdropCommand();

    virtual void undo();
    virtual void redo();
    virtual int id() const
    {
        return kNodeGraphResizeNodeBackdropCommandCompressionID;
    }

    virtual bool mergeWith(const QUndoCommand *command);

private:

    NodeGuiPtr _bd;
    int _w, _h;
    int _oldW, _oldH;
};


class DecloneMultipleNodesCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(DecloneMultipleNodesCommand)

public:

    DecloneMultipleNodesCommand(NodeGraph* graph,
                                const NodesGuiList & nodes,
                                QUndoCommand *parent = 0);


    virtual ~DecloneMultipleNodesCommand();

    virtual void undo();
    virtual void redo();

private:

    struct NodeToDeclone
    {
        NodeGuiWPtr node;
        NodeWPtr master;
    };

    std::list<NodeToDeclone> _nodes;
    NodeGraph* _graph;
};


class RearrangeNodesCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(RearrangeNodesCommand)

public:

    struct NodeToRearrange
    {
        NodeGuiPtr node;
        QPointF oldPos, newPos;
    };

    RearrangeNodesCommand(const NodesGuiList & nodes,
                          QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();

private:

    std::list<NodeToRearrange> _nodes;
};

class DisableNodesCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(DisableNodesCommand)

public:

    DisableNodesCommand(const NodesGuiList & nodes,
                        QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();

private:

    std::list<NodeGuiWPtr>_nodes;
};

class EnableNodesCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(EnableNodesCommand)

public:

    EnableNodesCommand(const NodesGuiList & nodes,
                       QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();

private:

    std::list<NodeGuiWPtr>_nodes;
};


class LoadNodePresetsCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(LoadNodePresetsCommand)

public:

    LoadNodePresetsCommand(const NodeGuiPtr & node,
                           const std::list<NodeSerializationPtr>& serialization,
                           QUndoCommand *parent = 0);

    virtual ~LoadNodePresetsCommand();
    virtual void undo();
    virtual void redo();

private:

    void getListAsShared(const std::list<NodeWPtr>& original,
                         std::list<NodePtr>& shared) const;

    bool _firstRedoCalled;
    bool _isUndone;
    NodeGuiWPtr _node;
    std::list<NodeWPtr> _oldChildren, _newChildren; //< children if multi-instance
    std::list<NodeSerializationPtr> _newSerializations, _oldSerialization;
};

class RenameNodeUndoRedoCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(RenameNodeUndoRedoCommand)

public:

    RenameNodeUndoRedoCommand(const NodeGuiPtr & node,
                              const QString& oldName,
                              const QString& newName);

    virtual ~RenameNodeUndoRedoCommand();
    virtual void undo();
    virtual void redo();

private:

    NodeGuiWPtr _node;
    QString _oldName, _newName;
};

struct ExtractedOutput
{
    NodeGuiWPtr node;
    std::list<std::pair<int, NodeWPtr> > outputs;
};

struct ExtractedInput
{
    NodeGuiWPtr node;
    std::vector<NodeWPtr> inputs;
};

struct ExtractedTree
{
    ExtractedOutput output;
    std::list<ExtractedInput> inputs;
    std::list<NodeGuiWPtr> inbetweenNodes;
};


class ExtractNodeUndoRedoCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(ExtractNodeUndoRedoCommand)

public:

    ExtractNodeUndoRedoCommand(NodeGraph* graph,
                               const NodesGuiList & nodes);

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
    Q_DECLARE_TR_FUNCTIONS(GroupFromSelectionCommand)

public:

    GroupFromSelectionCommand(NodeGraph* graph,
                              const NodesGuiList & nodes);

    virtual ~GroupFromSelectionCommand();

    virtual void undo();
    virtual void redo();

private:

    NodeGraph* _graph;
    std::list<NodeGuiWPtr> _originalNodes;
    struct OutputLink
    {
        int inputIdx;
        NodeWPtr inputNode;
    };
    typedef std::map<NodeWPtr, OutputLink> OutputLinksMap;
    OutputLinksMap _outputLinks;
    NodeGuiWPtr _group;
    bool _firstRedoCalled;
    bool _isRedone;
};

class InlineGroupCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(InlineGroupCommand)

public:

    InlineGroupCommand(NodeGraph* graph,
                       const NodesGuiList & nodes);

    virtual ~InlineGroupCommand();

    virtual void undo();
    virtual void redo();

private:

    struct NodeToConnect
    {
        NodeGuiWPtr input;
        std::map<NodeGuiWPtr, int> outputs;
    };

    struct InlinedGroup
    {
        NodeGuiWPtr group;
        std::list<NodeGuiWPtr> inlinedNodes;
        std::map<int, NodeToConnect> connections;
    };

    NodeGraph* _graph;
    std::list<InlinedGroup> _groupNodes;
    bool _firstRedoCalled;
};


NATRON_NAMESPACE_EXIT

#endif // NODEGRAPHUNDOREDO_H
