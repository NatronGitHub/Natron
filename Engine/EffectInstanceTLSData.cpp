/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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


#include "EffectInstanceTLSData.h"

#include <QMutex>

NATRON_NAMESPACE_ENTER

struct EffectTLSDataPrivate
{
    // We use a list here because actions may be recursive.
    // The last element in this list is always the arguments of
    // the current action.
    std::list<GenericActionTLSArgsPtr> actionsArgsStack;

    // Used to count the begin/endRenderAction recursion
    int beginEndRenderCount;

    // Used to count the recursion in the plug-in function calls
    int actionRecursionLevel;

#ifdef DEBUG
    // Used to track actions that attempt to call setValue on knobs but are not allowed (only in debug mode)
    std::list<bool> canSetValue;
#endif

    // These are arguments global to the render of frame.
    // In each render of a frame multiple subsequent render on the effect may occur but these data should remain the same.
    // Multiple threads may share the same pointer as these datas remain the same.
    ParallelRenderArgsPtr frameArgs;

    // When rendering with the viewer, to compute the frame/view hash we need to call getFramesNeeded.
    // But for the viewer the frames needed depend on the index we are rendering (i.e: A or B).
    int viewerTextureIndex;

    // Even though these data are unique to the holder thread, we need a Mutex when copying one thread data
    // over another one.
    // Each data member must be protected by this mutex in getters/setters
    mutable QMutex lock;

    EffectTLSDataPrivate()
    : actionsArgsStack()
    , beginEndRenderCount(0)
    , actionRecursionLevel(0)
#ifdef DEBUG
    , canSetValue()
#endif
    , frameArgs()
    , viewerTextureIndex(0)
    , lock()
    {

    }

    EffectTLSDataPrivate(const EffectTLSDataPrivate& other)
    {
        // Lock the other thread TLS (mainly for the current action args)
        QMutexLocker k(&other.lock);
        for (std::list<GenericActionTLSArgsPtr>::const_iterator it = other.actionsArgsStack.begin(); it!=other.actionsArgsStack.end(); ++it) {
            actionsArgsStack.push_back((*it)->createCopy());
        }
        beginEndRenderCount = other.beginEndRenderCount;
        actionRecursionLevel = other.actionRecursionLevel;
#ifdef DEBUG
        canSetValue = other.canSetValue;
#endif
        frameArgs = other.frameArgs;
        viewerTextureIndex = other.viewerTextureIndex;
    }

};

EffectTLSData::EffectTLSData()
: _imp(new EffectTLSDataPrivate())
{
}

EffectTLSData::EffectTLSData(const EffectTLSData& other)
: _imp(new EffectTLSDataPrivate(*other._imp))
{
}


NATRON_NAMESPACE_EXIT
