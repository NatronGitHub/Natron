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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "PythonUndoGroup.h"

#include <set>

#include "Engine/EffectInstance.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"

NATRON_NAMESPACE_ENTER

PythonUndoGroupCommand::PythonUndoGroupCommand(const std::string& text)
    : UndoCommand(text)
    , _knobs()
    , _inputs()
    , _nodes()
    , _done(true)
    , _firstRedo(true)
{
}

PythonUndoGroupCommand::~PythonUndoGroupCommand()
{
    // The nodes that the current state does not have were only deactivated: destroy them, as
    // RemoveMultipleNodesCommand and AddMultipleNodesCommand do.
    for (std::list<NodeRecord>::const_iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        NodePtr node = it->node.lock();
        if ( node && !node->isActivated() && (it->created != _done) ) {
            node->destroyNode(false, false);
        }
    }
}

void
PythonUndoGroupCommand::recordKnob(const KnobIPtr& knob)
{
    if (!knob) {
        return;
    }
    for (std::list<KnobRecord>::const_iterator it = _knobs.begin(); it != _knobs.end(); ++it) {
        if (it->knob.lock() == knob) {
            return; // the state before the group is already recorded
        }
    }
    KnobRecord r;
    r.knob = knob;
    r.before = createKnobCopy(knob);
    if (!r.before) {
        return; // a knob without values (button, separator, page...)
    }
    _knobs.push_back(r);
}

void
PythonUndoGroupCommand::recordInputs(const NodePtr& node)
{
    if (!node) {
        return;
    }
    for (std::list<InputsRecord>::const_iterator it = _inputs.begin(); it != _inputs.end(); ++it) {
        if (it->node.lock() == node) {
            return;
        }
    }
    InputsRecord r;
    r.node = node;
    r.before = node->getInputs_copy();
    _inputs.push_back(r);
}

void
PythonUndoGroupCommand::recordNodeCreated(const NodePtr& node)
{
    if (!node) {
        return;
    }
    NodeRecord r;
    r.node = node;
    r.created = true;
    r.autoReconnect = false;
    _nodes.push_back(r);
}

void
PythonUndoGroupCommand::deleteNode(const NodePtr& node,
                                   bool autoReconnect)
{
    if (!node) {
        return;
    }
    NodeRecord r;
    r.node = node;
    r.created = false;
    r.autoReconnect = autoReconnect;
    _nodes.push_back(r);
    // as the node graph deletes a node (RemoveMultipleNodesCommand::redo()): kept for undo
    node->deactivate(std::list<NodePtr>(), // outputs to disconnect
                     true, // disconnect all nodes
                     autoReconnect, // reconnect the outputs to the input of this node
                     true, // hide the node GUI
                     true); // trigger a render
}

bool
PythonUndoGroupCommand::isEmpty() const
{
    return _knobs.empty() && _inputs.empty() && _nodes.empty();
}

void
PythonUndoGroupCommand::finish()
{
    for (std::list<KnobRecord>::iterator it = _knobs.begin(); it != _knobs.end(); ++it) {
        KnobIPtr knob = it->knob.lock();
        if (knob) {
            it->after = createKnobCopy(knob);
        }
    }
    for (std::list<InputsRecord>::iterator it = _inputs.begin(); it != _inputs.end(); ++it) {
        NodePtr node = it->node.lock();
        if (node) {
            it->after = node->getInputs_copy();
        }
    }
}

void
PythonUndoGroupCommand::restoreKnobs(bool after)
{
    std::set<KnobHolder*> holders;

    for (std::list<KnobRecord>::reverse_iterator it = _knobs.rbegin(); it != _knobs.rend(); ++it) {
        KnobIPtr knob = it->knob.lock();
        const KnobIPtr& state = after ? it->after : it->before;
        if (!knob || !state) {
            continue;
        }
        knob->cloneAndUpdateGui( state.get() );
        if ( knob->getHolder() ) {
            holders.insert( knob->getHolder() );
        }
    }
    for (std::set<KnobHolder*>::const_iterator it = holders.begin(); it != holders.end(); ++it) {
        (*it)->incrHashAndEvaluate(true, true);
    }
}

void
PythonUndoGroupCommand::restoreInputs(bool after)
{
    for (std::list<InputsRecord>::reverse_iterator it = _inputs.rbegin(); it != _inputs.rend(); ++it) {
        NodePtr node = it->node.lock();
        if (!node) {
            continue;
        }
        const std::vector<NodeWPtr>& inputs = after ? it->after : it->before;
        for (std::size_t i = 0; i < inputs.size(); ++i) {
            NodePtr input = inputs[i].lock();
            NodePtr current = node->getInput( (int)i );
            if (input == current) {
                continue;
            }
            if (input) {
                node->replaceInput(input, (int)i);
            } else {
                node->disconnectInput( (int)i );
            }
        }
    }
}

void
PythonUndoGroupCommand::setNodesActivated(bool done)
{
    // undo: the most recent change first
    if (done) {
        for (std::list<NodeRecord>::const_iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
            NodePtr node = it->node.lock();
            if (!node) {
                continue;
            }
            if ( it->created && !node->isActivated() ) {
                node->activate();
            } else if ( !it->created && node->isActivated() ) {
                node->deactivate(std::list<NodePtr>(), true, it->autoReconnect, true, true);
            }
        }
    } else {
        for (std::list<NodeRecord>::const_reverse_iterator it = _nodes.rbegin(); it != _nodes.rend(); ++it) {
            NodePtr node = it->node.lock();
            if (!node) {
                continue;
            }
            if ( it->created && node->isActivated() ) {
                node->deactivate(std::list<NodePtr>(), true, true, true, true);
            } else if ( !it->created && !node->isActivated() ) {
                node->activate();
            }
        }
    }
}

void
PythonUndoGroupCommand::undo()
{
    // the reverse of the group: the values, the connections, then the nodes
    restoreKnobs(false);
    restoreInputs(false);
    setNodesActivated(false);
    _done = false;
}

void
PythonUndoGroupCommand::redo()
{
    if (_firstRedo) {
        // pushed after the edits were made
        _firstRedo = false;

        return;
    }
    setNodesActivated(true);
    restoreInputs(true);
    restoreKnobs(true);
    _done = true;
}

NATRON_NAMESPACE_EXIT
