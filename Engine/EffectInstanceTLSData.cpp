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

#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/TreeRender.h"
#include "Engine/FrameViewRequest.h"

NATRON_NAMESPACE_ENTER

struct EffectInstanceTLSDataPrivate
{
    // protects all data members since the multi-thread suite threads may access TLS of another thread
    // using the getTLSObjectForThread() function
    mutable QMutex lock;

    // We use a list here because actions may be recursive.
    // The last element in this list is always the arguments of
    // the current action.
    std::list<GenericActionTLSArgsPtr> actionsArgsStack;

    // Used to implement suite functions that return strings
    // to ensure that the string lives as long as the action runs
    std::vector<std::string> userPlaneStrings;

    EffectInstanceTLSDataPrivate()
    : lock()
    , actionsArgsStack()
    {

    }

};

EffectInstanceTLSData::EffectInstanceTLSData()
: _imp(new EffectInstanceTLSDataPrivate())
{
}


EffectInstanceTLSData::~EffectInstanceTLSData()
{

}

void
EffectInstanceTLSData::pushActionArgs(const std::string& actionName, TimeValue time, ViewIdx view, const RenderScale& scale
#ifdef DEBUG
                              , bool canSetValue
                              , bool canBeCalledRecursively
#endif
                              )
{
    GenericActionTLSArgsPtr args = boost::make_shared<GenericActionTLSArgs>();
    args->time = time;
    args->view = view;
    args->scale = scale;
    args->actionName = actionName;
#ifdef DEBUG
    args->canSetValue = canSetValue;
#endif

#ifdef DEBUG
    if (!canBeCalledRecursively && !_imp->actionsArgsStack.empty()) {
        qDebug() << "A non-recursive action was called recursively";
    }
#endif
    _imp->actionsArgsStack.push_back(args);
}

void
EffectInstanceTLSData::pushRenderActionArgs(TimeValue time, ViewIdx view, const RenderScale& proxyScale, unsigned int mipMapLevel,
                                            const RectI& renderWindow,
                                            const std::map<ImagePlaneDesc, ImagePtr>& outputPlanes)
{
    RenderActionTLSDataPtr args = boost::make_shared<RenderActionTLSData>();
    args->time = time;
    args->view = view;
    args->scale = proxyScale;
    args->mipMapLevel = mipMapLevel;
    args->renderWindow = renderWindow;
    args->outputPlanes = outputPlanes;
    args->actionName = "Render";
    QMutexLocker k(&_imp->lock);
#ifdef DEBUG
    args->canSetValue = false;
    if (!_imp->actionsArgsStack.empty()) {
        qDebug() << "The render action cannot be caller recursively, this is a bug!";
    }
#endif
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

bool
EffectInstanceTLSData::isCurrentAction(const std::string& actionName) const
{
    QMutexLocker k(&_imp->lock);
    if (_imp->actionsArgsStack.empty()) {
        return false;
    }
    const GenericActionTLSArgsPtr& args = _imp->actionsArgsStack.back();
    return actionName == args->actionName;
}

bool
EffectInstanceTLSData::hasActionInStack(const std::string& actionName) const
{
    QMutexLocker k(&_imp->lock);
    for (std::list<GenericActionTLSArgsPtr>::const_iterator it = _imp->actionsArgsStack.begin(); it != _imp->actionsArgsStack.end(); ++it) {
        if ((*it)->actionName == actionName) {
            return true;
        }
    }
    return false;
}

bool
EffectInstanceTLSData::getCurrentActionArgs(TimeValue* time, ViewIdx* view, RenderScale* scale, std::string* actionName) const
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
    if (actionName) {
        *actionName = args->actionName;
    }
    return true;
}


bool
EffectInstanceTLSData::getCurrentRenderActionArgs(TimeValue* time, ViewIdx* view, RenderScale* proxyScale, unsigned int* mipMapLevel,
                                                  RectI* renderWindow,
                                                  std::map<ImagePlaneDesc, ImagePtr>* outputPlanes) const
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
    if (proxyScale) {
        *proxyScale = args->scale;
    }
    if (mipMapLevel) {
        *mipMapLevel = args->mipMapLevel;
    }
    if (renderWindow) {
        *renderWindow = args->renderWindow;
    }

    if (outputPlanes) {
        *outputPlanes = args->outputPlanes;
    }

    return true;
}

void
EffectInstanceTLSData::clearActionStack()
{
    QMutexLocker k(&_imp->lock);
    _imp->actionsArgsStack.clear();
}

std::vector<std::string>&
EffectInstanceTLSData::getUserPlanesVector()
{
    return _imp->userPlaneStrings;
}

NATRON_NAMESPACE_EXIT
