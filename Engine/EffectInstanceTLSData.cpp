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
    // We use a list here because actions may be recursive.
    // The last element in this list is always the arguments of
    // the current action.
    std::list<GenericActionTLSArgsPtr> actionsArgsStack;


    EffectInstanceTLSDataPrivate()
    : actionsArgsStack()
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

#ifdef DEBUG
    if (!canBeCalledRecursively && !_imp->actionsArgsStack.empty()) {
        qDebug() << "A non-recursive action was called recursively";
    }
#endif
    _imp->actionsArgsStack.push_back(args);
}


void
EffectInstanceTLSData::pushRenderActionArgs(const FrameViewRequestPtr& requestData, const std::map<ImagePlaneDesc, ImagePtr>& outputPlanes)
{
    RenderActionTLSDataPtr args(new RenderActionTLSData);
    args->time = requestData->getTime();
    args->view = requestData->getView();

    args->scale = requestData->getRenderClone()->getCurrentRender()->getProxyScale();
    double mipMapScale = Image::getScaleFromMipMapLevel(requestData->getRenderMappedMipMapLevel());
    args->scale.x *= mipMapScale;
    args->scale.y *= mipMapScale;
    args->requestData = requestData;
    args->outputPlanes = outputPlanes;
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
    if (_imp->actionsArgsStack.empty()) {
        return;
    }
    _imp->actionsArgsStack.pop_back();
}


#ifdef DEBUG
bool
EffectInstanceTLSData::isDuringActionThatCannotSetValue() const
{
    if (_imp->actionsArgsStack.empty()) {
        return false;
    }
    return !_imp->actionsArgsStack.back()->canSetValue;
}
#endif


void
EffectInstanceTLSData::ensureLastActionInStackIsNotRender()
{
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


bool
EffectInstanceTLSData::getCurrentRenderActionArgs(FrameViewRequestPtr* requestData, std::map<ImagePlaneDesc, ImagePtr>* outputPlanes) const
{
    if (_imp->actionsArgsStack.empty()) {
        return false;
    }
    const RenderActionTLSData* args = dynamic_cast<const RenderActionTLSData*>(_imp->actionsArgsStack.back().get());
    if (!args) {
        return false;
    }

    if (requestData) {
        *requestData = args->requestData;
    }

    if (outputPlanes) {
        *outputPlanes = args->outputPlanes;
    }
 
    return true;
}

NATRON_NAMESPACE_EXIT
