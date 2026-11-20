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

#include <algorithm>
#include <set>

#include <QDebug>

#include "Engine/EffectInstance.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/Curve.h"
#include "Engine/Node.h"
#include "Engine/ViewIdx.h"

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

bool
PythonUndoGroupCommand::getKnobState(const KnobIPtr& knob,
                                     KnobState* state)
{
    state->copy = createKnobCopy(knob);
    if (!state->copy) {
        return false;
    }
    const int nDims = knob->getDimension();
    state->expressions.resize(nDims);
    state->expressionsHaveRet.resize(nDims);
    state->masters.resize(nDims);
    for (int i = 0; i < nDims; ++i) {
        state->expressions[i] = knob->getExpression(i);
        state->expressionsHaveRet[i] = knob->isExpressionUsingRetVariable(i);
        std::pair<int, KnobIPtr> master = knob->getMaster(i);
        state->masters[i] = std::make_pair( master.first, KnobIWPtr(master.second) );
    }

    return true;
}

namespace {
// whether the values and the animation of a and b are the same (a and b have the same type)
template <typename T>
bool
sameValues(const KnobIPtr& a,
           const KnobIPtr& b)
{
    Knob<T>* ka = dynamic_cast<Knob<T>*>( a.get() );
    Knob<T>* kb = dynamic_cast<Knob<T>*>( b.get() );
    if (!ka || !kb) {
        return true; // not this type
    }
    for (int i = 0; i < a->getDimension(); ++i) {
        CurvePtr ca = a->getCurve(ViewIdx(0), i);
        CurvePtr cb = b->getCurve(ViewIdx(0), i);
        const KeyFrameSet ka_ = ca ? ca->getKeyFrames_mt_safe() : KeyFrameSet();
        const KeyFrameSet kb_ = cb ? cb->getKeyFrames_mt_safe() : KeyFrameSet();
        if ( (ka_.size() != kb_.size()) || !std::equal( ka_.begin(), ka_.end(), kb_.begin() ) ) {
            return false;
        }
        if ( ka_.empty() && ( ka->getValue(i) != kb->getValue(i) ) ) {
            return false;
        }
    }

    return true;
}
}

bool
PythonUndoGroupCommand::knobHasState(const KnobIPtr& knob,
                                     const KnobState& state)
{
    KnobState current;

    if ( !getKnobState(knob, &current) ) {
        return true;
    }
    for (std::size_t i = 0; i < state.expressions.size() && i < current.expressions.size(); ++i) {
        if ( (state.expressions[i] != current.expressions[i]) || (state.masters[i].first != current.masters[i].first) ||
             (state.masters[i].second.lock() != current.masters[i].second.lock()) ) {
            return false;
        }
    }
    // the parametric and bytes knobs are not compared
    return sameValues<int>(current.copy, state.copy) && sameValues<bool>(current.copy, state.copy) &&
           sameValues<double>(current.copy, state.copy) && sameValues<std::string>(current.copy, state.copy);
}

void
PythonUndoGroupCommand::setKnobState(const KnobIPtr& knob,
                                     const KnobState& state)
{
    // the links first: a slave gets its master's values
    const int nDims = std::min( knob->getDimension(), (int)state.masters.size() );
    for (int i = 0; i < nDims; ++i) {
        std::pair<int, KnobIPtr> current = knob->getMaster(i);
        KnobIPtr master = state.masters[i].second.lock();
        if ( (current.second == master) && (current.first == state.masters[i].first) ) {
            continue;
        }
        if (current.second) {
            knob->unSlave(i, false);
        }
        if (master) {
            knob->slaveTo(i, master, state.masters[i].first);
        }
    }
    // values and animation (cloneAndUpdateGui() does not clear an expression: they are set below)
    knob->cloneAndUpdateGui( state.copy.get() );
    for (int i = 0; i < nDims; ++i) {
        if ( state.expressions[i].empty() ) {
            if ( !knob->getExpression(i).empty() ) {
                knob->clearExpression(i, true);
            }
        } else if ( (knob->getExpression(i) != state.expressions[i]) ||
                    (knob->isExpressionUsingRetVariable(i) != state.expressionsHaveRet[i]) ) {
            try {
                knob->setExpression(i, state.expressions[i], state.expressionsHaveRet[i], false);
            } catch (const std::exception& e) {
                qDebug() << "Undo group: could not restore an expression:" << e.what();
            }
        }
    }
}

void
PythonUndoGroupCommand::recordKnob(const KnobIPtr& knob)
{
    if ( !knob || !_recordedKnobs.insert( knob.get() ).second ) {
        return; // the state before the group is already recorded
    }
    KnobRecord r;
    r.knob = knob;
    if ( !getKnobState(knob, &r.before) ) {
        // a knob without values (button, separator, page...), or a type that cannot be copied
        qDebug() << "Undo group: the changes of" << knob->getName().c_str() << "(" << knob->typeName().c_str() << ") are not recorded";

        return;
    }
    _knobs.push_back(r);
}

void
PythonUndoGroupCommand::recordInputs(const NodePtr& node)
{
    if ( !node || !_recordedInputs.insert( node.get() ).second ) {
        return;
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
            getKnobState(knob, &it->after);
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
        const KnobState& state = after ? it->after : it->before;
        const KnobState& expected = after ? it->before : it->after;
        if (!knob || !state.copy) {
            continue;
        }
        // A knob changed since (e.g. in its settings panel, whose undo stack is another one) keeps its
        // value: restoring the state of the group would silently lose that change.
        if ( !knobHasState(knob, expected) ) {
            qDebug() << "Undo group" << getText().c_str() << ":" << knob->getName().c_str()
                     << "changed since, it is not restored";
            continue;
        }
        setKnobState(knob, state);
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
