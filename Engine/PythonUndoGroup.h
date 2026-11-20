/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#ifndef Engine_PythonUndoGroup_h
#define Engine_PythonUndoGroup_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <string>
#include <vector>

#include "Engine/EngineFwd.h"
#include "Engine/UndoCommand.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief The undo command of an undo group (App.beginUndoGroup()/endUndoGroup(), "with app.undoGroup(text):"):
 * the edits made from Python while the group is open, which are otherwise not undoable. The Python API records
 * each change before making it (AppInstance::recordKnobForUndoGroup() and the others): the state of the knobs,
 * the inputs of the nodes, and the nodes created or deleted. When the group ends, the command records the new
 * state, and the GUI pushes it on the undo stack of the node graph: one undo step for the whole group.
 *
 * The edits are already made when the command is pushed: the first redo() does nothing.
 * A node deleted in the group is only deactivated, so that undo() can restore it; it is destroyed with the
 * command, if it is still deleted then. Likewise, a node created in the group and undone is destroyed with the
 * command.
 **/
class PythonUndoGroupCommand
    : public UndoCommand
{
public:

    PythonUndoGroupCommand(const std::string& text);

    virtual ~PythonUndoGroupCommand();

    /// before the first change of knob in the group: records its state
    void recordKnob(const KnobIPtr& knob);

    /// before the first change of the inputs of node in the group: records them
    void recordInputs(const NodePtr& node);

    /// node was created in the group
    void recordNodeCreated(const NodePtr& node);

    /// node is deleted in the group: it is deactivated, so that undo() can restore it
    void deleteNode(const NodePtr& node, bool autoReconnect);

    /// whether the group changed something
    bool isEmpty() const;

    /// at the end of the group: records the new state of the knobs and inputs
    void finish();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    struct KnobRecord
    {
        KnobIWPtr knob;
        KnobIPtr before, after; // copies (createKnobCopy())
    };

    struct InputsRecord
    {
        NodeWPtr node;
        std::vector<NodeWPtr> before, after;
    };

    struct NodeRecord
    {
        NodeWPtr node;
        bool created; // else deleted
        bool autoReconnect; // of the deletion
    };

    void restoreKnobs(bool after);
    void restoreInputs(bool after);
    void setNodesActivated(bool done);

    std::list<KnobRecord> _knobs;
    std::list<InputsRecord> _inputs;
    std::list<NodeRecord> _nodes; // in the order of the changes
    bool _done; // the state after the group (initially, and after redo())
    bool _firstRedo;
};

NATRON_NAMESPACE_EXIT

#endif // Engine_PythonUndoGroup_h
