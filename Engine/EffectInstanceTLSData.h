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

#ifndef EFFECTINSTANCETLSDATA_H
#define EFFECTINSTANCETLSDATA_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>
#include <map>
#include "Engine/EngineFwd.h"
#include "Engine/ViewIdx.h"
#include "Engine/RectI.h"
#include "Engine/EffectInstance.h"
#include "Global/GlobalDefines.h"


#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

NATRON_NAMESPACE_ENTER;


// Generic arguments passed to the OpenFX actions which may be sometimes not passed-back
// to the A.P.I and that would result in loss of context.
// This is vital at least for these suite functions:
// clipGetRegionOfDefinition: it does not pass-us back the render-scale
// paramGetValue: it does not pass-us back the current action time and view


class GenericActionTLSArgs
{
public:

    GenericActionTLSArgs()
    : time(0)
    , view()
    , scale()
#ifdef DEBUG
    , canSetValue(true)
#endif
    {

    }

    virtual ~GenericActionTLSArgs() {}


    // The time parameter passed to an OpenFX action
    double time;

    // the view parameter passed to an OpenFX action
    ViewIdx view;

    // the scale parameter passed to an OpenFX action
    RenderScale scale;

#ifdef DEBUG
    // whether or not this action can set value
    bool canSetValue;
#endif

    virtual GenericActionTLSArgsPtr createCopy()
    {
        GenericActionTLSArgsPtr ret(new GenericActionTLSArgs);
        ret->time = time;
        ret->scale = scale;
        ret->view = view;
#ifdef DEBUG
        ret->canSetValue = canSetValue;
#endif
        return ret;
    }
};


// The render action requires extra data that are valid throughout a single call
// of a render action
class RenderActionTLSData : public GenericActionTLSArgs
{
public:

    RenderActionTLSData()
    : GenericActionTLSArgs()
    , renderWindowPixel()
    , inputImages()
    , outputPlanes()
    , compsNeeded()
    {

    }

    virtual ~RenderActionTLSData() OVERRIDE {}


    // The render window passed to the render action in pixel coordinates.
    // This is useful in the following cases:
    // - the clipGetImage function on the output clip should return an OfxImage with bounds
    // that are clipped to this render window so that if the plug-in attempts to write outside
    // the bounds, it may be caught easily
    // - When calling clipGetImage on an input clip: if the call at the given time/view was not
    // advertised properly in getFramesNeeded, we don't have an image pointer to the input image,
    // thus we have to re-call renderRoI: but we need to make sure we don't ask for more than what
    // getRegionsOfInterest on the renderwindow would compute.
    RectI renderWindowPixel;

    // Input images that were pre-fetched in renderRoI so that they can
    // be accessed from getImage(): this ensure that input images are not de-allocated until
    // we don't need them anymore.
    // As a general note, all expensive actions that may not be cached should always have a shared pointer
    // on the TLS throughout the action to ensure at least we don't cycle through the graph with exponential cost.
    InputImagesMap inputImages;

    // For each plane to render, the pointers to the internal images used during render: this is used when calling
    // clipGetImage on the output clip.
    std::map<ImageComponents, PlaneToRender> outputPlanes;

    // FIXME: this should be removed and the result of the action getComponentsNeeded should be cached much like any
    // other action
    ComponentsNeededMapPtr compsNeeded;


    virtual GenericActionTLSArgsPtr createCopy() OVERRIDE FINAL
    {
        RenderActionTLSDataPtr ret(new RenderActionTLSData);
        ret->time = time;
        ret->scale = scale;
        ret->view = view;
#ifdef DEBUG
        ret->canSetValue = canSetValue;
#endif
        ret->renderWindowPixel = renderWindowPixel;
        ret->inputImages = inputImages;
        ret->outputPlanes  = outputPlanes;
        ret->compsNeeded = compsNeeded;
        return ret;
    }



};


//these are per-node thread-local data
struct EffectTLSDataPrivate;
class EffectTLSData
{
public:

    EffectTLSData();

    EffectTLSData(const EffectTLSData& other);

    /**
     * @brief Push TLS for an action that is not the render action. Mainly this will be needed by OfxClipInstance::getRegionOfDefinition and OfxClipInstance::getImage
     * This should only be needed for OpenFX plug-ins, as the Natron A.P.I already pass all required arguments in parameter.
     **/
    void pushActionArgs(double time, ViewIdx view, const RenderScale& scale
#ifdef DEBUG
                        , bool canSetValue
                        , bool canBeCalledRecursively
#endif
                        );

    /**
     * @brief Push TLS for the render action for any plug-in (not only OpenFX). This will be needed in EffectInstance::getImage
     **/
    void pushRenderActionArgs(double time, ViewIdx view, RenderScale& scale,
                              const RectI& renderWindowPixel,
                              const InputImagesMap& preRenderedInputImages,
                              const std::map<ImageComponents, PlaneToRender>& outputPlanes,
                              const ComponentsNeededMapPtr& compsNeeded);

    /**
     * @brief Pop the current action TLS. This call must match a call to one of the push functions above.
     * The push/pop functions should be encapsulated in a RAII style class to ensure the TLS is correctly popped
     * in all cases.
     **/
    void popArgs();

    /**
     * @brief Get the recursion level in plug-in actions. 1 indicates that we are currently in a single action.
     **/
    int getActionsRecursionLevel() const;

#ifdef DEBUG
    /**
     * @brief Returns true if the action cannot have its knobs calling setValue
     **/
    bool isDuringActionThatCannotSetValue() const;
#endif

    /**
     * @brief Retrieve the current thread action args. This will return true on success, false if we are not
     * between a push/pop bracket.
     **/
    bool getCurrentActionArgs(double* time, ViewIdx* view, RenderScale* scale) const;

    /**
     * @brief Same as above execpt for the render action. Any field can be set to NULL if you do not need to retrieve it
     **/
    bool getCurrentRenderActionArgs(double* time, ViewIdx* view, RenderScale* scale,
                                    RectI* renderWindowPixel,
                                    InputImagesMap* preRenderedInputImages,
                                    std::map<ImageComponents, PlaneToRender>* outputPlanes,
                                    ComponentsNeededMapPtr* compsNeeded) const;

    /**
     * @brief If the current action is the render action, this sets the output planes.
     **/
    void updateCurrentRenderActionOutputPlanes(const std::map<ImageComponents, PlaneToRender>& outputPlanes);

    /**
     * @brief This function pops the last action arguments of the stack if it is the render action.
     * The reason for this function is when we do host frame threading:
     * The original thread might already have TLS set for the render action when we copy TLS onto the new thread.
     * But the new thread does not want the render action TLS, so we pop it.
     **/
    void ensureLastActionInStackIsNotRender();

    /**
     * @brief Returns the parallel render args TLS
     **/
    ParallelRenderArgsPtr getParallelRenderArgs() const;

    /**
     * @brief Creates the parallel render args TLS
     **/
    ParallelRenderArgsPtr getOrCreateParallelRenderArgs();

    /**
     * @brief Clear the parallel render args pointer
     **/
    void invalidateParallelRenderArgs();
    
private:
    
    boost::scoped_ptr<EffectTLSDataPrivate> _imp;

        
};

class EffectActionArgsSetter_RAII
{
    EffectTLSDataPtr tls;
    EffectInstancePtr effect;
public:

    EffectActionArgsSetter_RAII(const EffectTLSDataPtr& tls,
                                const EffectInstancePtr& effect,
                                double time, ViewIdx view, const RenderScale& scale
#ifdef DEBUG
                                , bool canSetValue
                                , bool canBeCalledRecursively
#endif
    )
    : tls(tls)
    , effect(effect)
    {
        tls->pushActionArgs(time, view, scale
#ifdef DEBUG
                            , canSetValue
                            , canBeCalledRecursively
#endif
                            );
    }

    ~EffectActionArgsSetter_RAII()
    {
        tls->popArgs();

        // An action might have requested overlay interacts refresh, check it now at the end of the action.
        effect->checkAndRedrawOverlayInteractsIfNeeded();
    }

};

class RenderActionArgsSetter_RAII
{
    EffectTLSDataPtr tls;
public:

    RenderActionArgsSetter_RAII(const EffectTLSDataPtr& tls,
                                double time, ViewIdx view, RenderScale& scale,
                                const RectI& renderWindowPixel,
                                const InputImagesMap& preRenderedInputImages,
                                const std::map<ImageComponents, PlaneToRender>& outputPlanes,
                                const ComponentsNeededMapPtr& compsNeeded)
    : tls(tls)
    {
        tls->pushRenderActionArgs(time, view, scale, renderWindowPixel, preRenderedInputImages, outputPlanes, compsNeeded);
    }

    ~RenderActionArgsSetter_RAII()
    {
        tls->popArgs();
    }
    
};


NATRON_NAMESPACE_EXIT;

#endif // EFFECTINSTANCETLSDATA_H
