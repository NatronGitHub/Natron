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

#include <QDebug>
#include <QMutex>

NATRON_NAMESPACE_ENTER

struct EffectInstanceTLSDataPrivate
{
    // We use a list here because actions may be recursive.
    // The last element in this list is always the arguments of
    // the current action.
    std::list<GenericActionTLSArgsPtr> actionsArgsStack;

    // These are arguments global to the render of frame.
    // In each render of a frame multiple subsequent render on the effect may occur but these data should remain the same.
    // Multiple threads may share the same pointer as these datas remain the same.
    TreeRenderNodeArgsWPtr frameArgs;

    // Even though these data are unique to the holder thread, we need a Mutex when copying one thread data
    // over another one.
    // Each data member must be protected by this mutex in getters/setters
    mutable QMutex lock;

    EffectInstanceTLSDataPrivate()
    : actionsArgsStack()
    , frameArgs()
    , lock()
    {

    }

    EffectInstanceTLSDataPrivate(const EffectInstanceTLSDataPrivate& other)
    {
        // Lock the other thread TLS (mainly for the current action args)
        QMutexLocker k(&other.lock);
        for (std::list<GenericActionTLSArgsPtr>::const_iterator it = other.actionsArgsStack.begin(); it!=other.actionsArgsStack.end(); ++it) {
            actionsArgsStack.push_back((*it)->createCopy());
        }
        
        // Parallel render args do not need to be copied because they are not modified by any thread throughout a render
        frameArgs = other.frameArgs;
    }

};

EffectInstanceTLSData::EffectInstanceTLSData()
: _imp(new EffectInstanceTLSDataPrivate())
{
}

EffectInstanceTLSData::EffectInstanceTLSData(const EffectInstanceTLSData& other)
: _imp(new EffectInstanceTLSDataPrivate(*other._imp))
{
}

EffectInstanceTLSData::~EffectInstanceTLSData()
{

}

void
EffectInstanceTLSData::pushActionArgs(TimeValue time, ViewIdx view, const RenderScale& scale
#ifdef DEBUG
                              , bool canSetValue
                              , bool canBeCalledRecursively
#endif
                              )
{
    GenericActionTLSArgsPtr args(new GenericActionTLSArgs);
    args->time = time;
    args->view = view;
    args->scale = scale;
#ifdef DEBUG
    args->canSetValue = canSetValue;
#endif

    QMutexLocker k(&_imp->lock);
#ifdef DEBUG
    if (!canBeCalledRecursively && !_imp->actionsArgsStack.empty()) {
        qDebug() << "A non-recursive action was called recursively";
    }
#endif
    _imp->actionsArgsStack.push_back(args);
}


void
EffectInstanceTLSData::pushRenderActionArgs(TimeValue time, ViewIdx view, RenderScale& scale,
                          const RectI& renderWindowPixel,
                          const std::map<ImageComponents, PlaneToRender>& outputPlanes)
{
    RenderActionTLSDataPtr args(new RenderActionTLSData);
    args->time = time;
    args->view = view;
    args->scale = scale;
    args->renderWindowPixel = renderWindowPixel;
    args->outputPlanes = outputPlanes;
#ifdef DEBUG
    args->canSetValue = false;
    if (!_imp->actionsArgsStack.empty()) {
        qDebug() << "The render action cannot be caller recursively, this is a bug!";
    }
#endif
    QMutexLocker k(&_imp->lock);
    _imp->actionsArgsStack.push_back(args);

}

void
EffectInstanceTLSData::popArgs()
{
    QMutexLocker k(&_imp->lock);
    if (_imp->actionsArgsStack.empty()) {
        return;
    }
    _imp->actionsArgsStack.pop_back();
}


#ifdef DEBUG
bool
EffectInstanceTLSData::isDuringActionThatCannotSetValue() const
{
    QMutexLocker k(&_imp->lock);
    if (_imp->actionsArgsStack.empty()) {
        return false;
    }
    return !_imp->actionsArgsStack.back()->canSetValue;
}
#endif


TreeRenderNodeArgsPtr
EffectInstanceTLSData::getRenderArgs() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->frameArgs.lock();
}

void
EffectInstanceTLSData::setRenderArgs(const TreeRenderNodeArgsPtr& renderArgs)
{
    QMutexLocker k(&_imp->lock);
    _imp->frameArgs = renderArgs;
}


void
EffectInstanceTLSData::ensureLastActionInStackIsNotRender()
{
    QMutexLocker k(&_imp->lock);
    if (_imp->actionsArgsStack.empty()) {
        return;
    }
    const GenericActionTLSArgsPtr& curAction = _imp->actionsArgsStack.back();
    const RenderActionTLSData* args = dynamic_cast<const RenderActionTLSData*>(curAction.get());
    if (!args) {
        return;
    }
    _imp->actionsArgsStack.pop_back();
}

bool
EffectInstanceTLSData::getCurrentActionArgs(TimeValue* time, ViewIdx* view, RenderScale* scale) const
{
    QMutexLocker k(&_imp->lock);
    if (_imp->actionsArgsStack.empty()) {
        return false;
    }
    const GenericActionTLSArgsPtr& args = _imp->actionsArgsStack.back();

    if (time) {
        *time = args->time;
    }
    if (view) {
        *view = args->view;
    }
    if (scale) {
        *scale = args->scale;
    }
    return true;
}

void
EffectInstanceTLSData::updateCurrentRenderActionOutputPlanes(const std::map<ImageComponents, PlaneToRender>& outputPlanes)
{
    QMutexLocker k(&_imp->lock);
    if (_imp->actionsArgsStack.empty()) {
        return;
    }
    const GenericActionTLSArgsPtr& curAction = _imp->actionsArgsStack.back();
    RenderActionTLSData* args = dynamic_cast<RenderActionTLSData*>(curAction.get());
    if (!args) {
        return;
    }
    args->outputPlanes = outputPlanes;
}


bool
EffectInstanceTLSData::getCurrentRenderActionArgs(TimeValue* time, ViewIdx* view, RenderScale* scale,
                                                  RectI* renderWindowPixel,
                                                  std::map<ImageComponents, PlaneToRender>* outputPlanes) const
{
    QMutexLocker k(&_imp->lock);
    if (_imp->actionsArgsStack.empty()) {
        return false;
    }
    const RenderActionTLSData* args = dynamic_cast<const RenderActionTLSData*>(_imp->actionsArgsStack.back().get());
    if (!args) {
        return false;
    }

    if (time) {
        *time = args->time;
    }
    if (view) {
        *view = args->view;
    }
    if (scale) {
        *scale = args->scale;
    }
    if (renderWindowPixel) {
        *renderWindowPixel = args->renderWindowPixel;
    }

    if (outputPlanes) {
        *outputPlanes = args->outputPlanes;
    }
 
    return true;
}

NATRON_NAMESPACE_EXIT
