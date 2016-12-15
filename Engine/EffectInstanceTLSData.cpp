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

struct EffectTLSDataPrivate
{
    // We use a list here because actions may be recursive.
    // The last element in this list is always the arguments of
    // the current action.
    std::list<GenericActionTLSArgsPtr> actionsArgsStack;

    // These are arguments global to the render of frame.
    // In each render of a frame multiple subsequent render on the effect may occur but these data should remain the same.
    // Multiple threads may share the same pointer as these datas remain the same.
    ParallelRenderArgsPtr frameArgs;

    // Even though these data are unique to the holder thread, we need a Mutex when copying one thread data
    // over another one.
    // Each data member must be protected by this mutex in getters/setters
    mutable QMutex lock;

    EffectTLSDataPrivate()
    : actionsArgsStack()
    , frameArgs()
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
        
        // Parallel render args do not need to be copied because they are not modified by any thread throughout a render
        frameArgs = other.frameArgs;
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

void
EffectTLSData::pushActionArgs(double time, ViewIdx view, const RenderScale& scale
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
EffectTLSData::pushRenderActionArgs(double time, ViewIdx view, RenderScale& scale,
                          const RectI& renderWindowPixel,
                          const InputImagesMap& preRenderedInputImages,
                          const std::map<ImageComponents, PlaneToRender>& outputPlanes,
                          const ComponentsNeededMapPtr& compsNeeded)
{
    RenderActionTLSDataPtr args(new RenderActionTLSData);
    args->time = time;
    args->view = view;
    args->scale = scale;
    args->renderWindowPixel = renderWindowPixel;
    args->inputImages = preRenderedInputImages;
    args->outputPlanes = outputPlanes;
    args->compsNeeded = compsNeeded;
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
EffectTLSData::popArgs()
{
    QMutexLocker k(&_imp->lock);
    if (_imp->actionsArgsStack.empty()) {
        return;
    }
    _imp->actionsArgsStack.pop_back();
}

int
EffectTLSData::getActionsRecursionLevel() const
{
    QMutexLocker k(&_imp->lock);
    return (int)_imp->actionsArgsStack.size();
}

#ifdef DEBUG
bool
EffectTLSData::isDuringActionThatCannotSetValue() const
{
    QMutexLocker k(&_imp->lock);
    if (_imp->actionsArgsStack.empty()) {
        return false;
    }
    return !_imp->actionsArgsStack.back()->canSetValue;
}
#endif


ParallelRenderArgsPtr
EffectTLSData::getParallelRenderArgs() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->frameArgs;
}


ParallelRenderArgsPtr
EffectTLSData::getOrCreateParallelRenderArgs()
{
    QMutexLocker k(&_imp->lock);
    if (_imp->frameArgs) {
        return _imp->frameArgs;
    }
    _imp->frameArgs.reset(new ParallelRenderArgs);
    return _imp->frameArgs;
}

void
EffectTLSData::invalidateParallelRenderArgs()
{
    QMutexLocker k(&_imp->lock);
    _imp->frameArgs.reset();
}

void
EffectTLSData::ensureLastActionInStackIsNotRender()
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
EffectTLSData::getCurrentActionArgs(double* time, ViewIdx* view, RenderScale* scale) const
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
EffectTLSData::updateCurrentRenderActionOutputPlanes(const std::map<ImageComponents, PlaneToRender>& outputPlanes)
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
EffectTLSData::getCurrentRenderActionArgs(double* time, ViewIdx* view, RenderScale* scale,
                                RectI* renderWindowPixel,
                                InputImagesMap* preRenderedInputImages,
                                std::map<ImageComponents, PlaneToRender>* outputPlanes,
                                ComponentsNeededMapPtr* compsNeeded) const
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
    if (preRenderedInputImages) {
        *preRenderedInputImages = args->inputImages;
    }
    if (outputPlanes) {
        *outputPlanes = args->outputPlanes;
    }
    if (compsNeeded) {
        *compsNeeded = args->compsNeeded;
    }
    return true;
}

NATRON_NAMESPACE_EXIT
