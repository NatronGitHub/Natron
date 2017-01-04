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

#include "EffectInstance.h"
#include "EffectInstancePrivate.h"

#include <QDebug>
#include <QThread>
#include <QCoreApplication>

#include "Engine/AppInstance.h"
#include "Engine/Cache.h"
#include "Engine/EffectInstanceActionResults.h"
#include "Engine/EffectOpenGLContextData.h"
#include "Engine/Format.h"
#include "Engine/KnobTypes.h"
#include "Engine/Hash64.h"
#include "Engine/Node.h"
#include "Engine/NodeMetadata.h"
#include "Engine/Project.h"
#include "Engine/TreeRenderNodeArgs.h"
#include "Engine/ThreadPool.h"

NATRON_NAMESPACE_ENTER;



ActionRetCodeEnum
EffectInstance::getComponentsAction(TimeValue time,
                                               ViewIdx view,
                                               const TreeRenderNodeArgsPtr& render,
                                               std::map<int, std::list<ImageComponents> >* inputLayersNeeded,
                                               std::list<ImageComponents>* layersProduced,
                                               TimeValue* passThroughTime,
                                               ViewIdx* passThroughView,
                                               int* passThroughInputNb)
{
    bool processAllRequested;
    std::bitset<4> processChannels;
    getDefaultComponentsNeededAndProduced(time, view, render, inputLayersNeeded, layersProduced, passThroughTime, passThroughView, passThroughInputNb, &processAllRequested, &processChannels);
    return eActionStatusReplyDefault;
} // getComponentsNeededAndProduced

void
EffectInstance::getDefaultComponentsNeededAndProduced(TimeValue time,
                                                      ViewIdx view,
                                                      const TreeRenderNodeArgsPtr& render,
                                                      std::map<int, std::list<ImageComponents> >* inputLayersNeeded,
                                                      std::list<ImageComponents>* layersProduced,
                                                      TimeValue* passThroughTime,
                                                      ViewIdx* passThroughView,
                                                      int* passThroughInputNb,
                                                      bool* processAllRequested,
                                                      std::bitset<4>* processChannels)
{
    *passThroughTime = time;
    *passThroughView = view;
    *passThroughInputNb = getNode()->getPreferredInput();
    *processAllRequested = false;


    // Get the output needed components
    {
        // Check if the node has a OutputLayer choice
        ImageComponents layer;

        bool ok = getNode()->getSelectedLayer(-1, processChannels, processAllRequested, &layer);

        // If the user did not select any components or the layer is the color-plane, fallback on
        // meta-data color plane
        if (layer.getNumComponents() == 0 || layer.isColorPlane()) {
            ok = false;
        }

        std::vector<ImageComponents> clipPrefsAllComps;
        ImageComponents clipPrefsComps = getColorPlaneComponents(render, -1);
        {
            if ( clipPrefsComps.isPairedComponents() ) {
                ImageComponents first, second;
                clipPrefsComps.getPlanesPair(&first, &second);
                clipPrefsAllComps.push_back(first);
                clipPrefsAllComps.push_back(second);
            } else {
                clipPrefsAllComps.push_back(clipPrefsComps);
            }
        }

        if (ok) {
            layersProduced->push_back(layer);

            if ( !clipPrefsComps.isColorPlane() ) {
                layersProduced->insert( layersProduced->end(), clipPrefsAllComps.begin(), clipPrefsAllComps.end() );
            }
        } else {
            layersProduced->insert( layersProduced->end(), clipPrefsAllComps.begin(), clipPrefsAllComps.end() );
        }

    }

    // For each input get their needed components
    int maxInput = getMaxInputCount();
    for (int i = 0; i < maxInput; ++i) {

        EffectInstancePtr input = getInput(i);
        if (!input) {
            continue;
        }

        std::list<ImageComponents> &componentsSet = (*inputLayersNeeded)[i];

        // Get the selected layer from the source channels menu
        std::bitset<4> inputProcChannels;
        ImageComponents layer;
        bool isAll;
        bool ok = getNode()->getSelectedLayer(i, &inputProcChannels, &isAll, &layer);

        // When color plane or all choice then request the default metadata components
        if (isAll || layer.isColorPlane()) {
            ok = false;
        }

        // For a mask get its selected channel
        ImageComponents maskComp;
        int channelMask = getNode()->getMaskChannel(i, &maskComp);


        std::vector<ImageComponents> clipPrefsAllComps;
        {
            ImageComponents clipPrefsComps = getColorPlaneComponents(render, i);
            if ( clipPrefsComps.isPairedComponents() ) {
                ImageComponents first, second;
                clipPrefsComps.getPlanesPair(&first, &second);
                clipPrefsAllComps.push_back(first);
                clipPrefsAllComps.push_back(second);
            } else {
                clipPrefsAllComps.push_back(clipPrefsComps);
            }
        }

        if ( (channelMask != -1) && (maskComp.getNumComponents() > 0) ) {

            // If this is a mask, ask for the selected mask layer
            componentsSet.push_back(maskComp);

        } else if (ok) {
            componentsSet.push_back(layer);
        } else {
            //Use regular clip preferences
            componentsSet.insert( componentsSet.end(), clipPrefsAllComps.begin(), clipPrefsAllComps.end() );
        }

    } // for each input
} // getDefaultComponentsNeededAndProduced

ActionRetCodeEnum
EffectInstance::getComponentsNeededInternal(TimeValue time,
                                            ViewIdx view,
                                            const TreeRenderNodeArgsPtr& render,
                                            std::map<int, std::list<ImageComponents> >* inputLayersNeeded,
                                            std::list<ImageComponents>* layersProduced,
                                            TimeValue* passThroughTime,
                                            ViewIdx* passThroughView,
                                            int* passThroughInputNb,
                                            bool* processAllRequested,
                                            std::bitset<4>* processChannels)
{

    if ( !isMultiPlanar() ) {
        getDefaultComponentsNeededAndProduced(time, view, render, inputLayersNeeded, layersProduced, passThroughTime, passThroughView, passThroughInputNb, processAllRequested, processChannels);
    } else {

        // call the getClipComponents action

        ActionRetCodeEnum stat = getComponentsAction(time, view, render, inputLayersNeeded, layersProduced, passThroughTime, passThroughView, passThroughInputNb);
        if (isFailureRetCode(stat)) {
            return stat;
        }

        for (int i = 0; i < 4; ++i) {
            (*processChannels)[i] = getNode()->getProcessChannel(i);
        }

        *processAllRequested = false;
    }
    return eActionStatusOK;

}



/**
 * @brief Add the layers from the inputList to the toList if they do not already exist in the list.
 * For the color plane, if it already existed in toList it is replaced by the value in inputList
 **/
static void mergeLayersList(const std::list<ImageComponents>& inputList,
                                std::list<ImageComponents>* toList)
{
    for (std::list<ImageComponents>::const_iterator it = inputList.begin(); it != inputList.end(); ++it) {

        std::list<ImageComponents>::iterator foundMatch = ImageComponents::findEquivalentLayer(*it, toList->begin(), toList->end());

        // If we found the color plane, replace it by this color plane which may have changed (e.g: input was Color.RGB but this node Color.RGBA)
        if (foundMatch != toList->end()) {
            toList->erase(foundMatch);
        }
        toList->push_back(*it);

    } // for each input components
} // mergeLayersList

/**
 * @brief Remove any layer from the toRemove list from toList.
 **/
static void removeFromLayersList(const std::list<ImageComponents>& toRemove,
                                 std::list<ImageComponents>* toList)
{
    for (std::list<ImageComponents>::const_iterator it = toRemove.begin(); it != toRemove.end(); ++it) {
        std::list<ImageComponents>::iterator foundMatch = ImageComponents::findEquivalentLayer(*it, toList->begin(), toList->end());
        if (foundMatch != toList->end()) {
            toList->erase(foundMatch);
        }
    } // for each input components

} // removeFromLayersList

ActionRetCodeEnum
EffectInstance::getComponents_public(TimeValue inArgsTime, ViewIdx view, const TreeRenderNodeArgsPtr& render, GetComponentsResultsPtr* results)

{
    // Round time for non continuous effects
    TimeValue time = inArgsTime;
    {
        int roundedTime = std::floor(time + 0.5);
        if (roundedTime != time && !canRenderContinuously(render)) {
            time = TimeValue(roundedTime);
        }
    }


    // Get the render local args
    FrameViewRequestPtr fvRequest;
    if (render) {

        // Ensure the render object corresponds to this node.
        assert(render->getNode() == getNode());

        // We may not have created a request object for this frame/view yet. Create it.
        bool created = render->getOrCreateFrameViewRequest(time, view, &fvRequest);
        (void)created;
    }

    U64 hash = 0;

    if (fvRequest) {
        GetComponentsResultsPtr cached = fvRequest->getComponentsResults();
        if (cached) {
            *results = cached;
            return eActionStatusOK;
        }
        fvRequest->getHash(&hash);
    }


    // Get a hash to cache the results
    if (hash == 0) {
        ComputeHashArgs hashArgs;
        hashArgs.render = render;
        hashArgs.time = time;
        hashArgs.view = view;
        hashArgs.hashType = HashableObject::eComputeHashTypeTimeViewVariant;
        hash = computeHash(hashArgs);
    }

    assert(hash != 0);

    GetComponentsKeyPtr cacheKey(new GetComponentsKey(hash, getNode()->getPluginID()));

    // Ensure the cache fetcher lives as long as we compute the action
    CacheFetcher cacheAccess(cacheKey);

    GetComponentsResultsPtr ret = toGetComponentsResults(cacheAccess.isCached());
    if (ret) {
        *results = ret;
        return eActionStatusOK;
    }
    ret = GetComponentsResults::create(cacheKey);


    // For each input index what layers are required
    std::map<int, std::list<ImageComponents> > inputLayersNeeded;

    // The layers that are produced by this effect
    std::list<ImageComponents> outputLayersProduced;

    // The layers that this effect can fetch from the pass-through input but does not produce itself
    std::list<ImageComponents> passThroughPlanes;

    int passThroughInputNb = -1;
    TimeValue passThroughTime(0);
    ViewIdx passThroughView(0);
    std::bitset<4> processChannels;
    bool processAllRequested = false;
    {

        EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
        EffectActionArgsSetter_RAII actionArgsTls(tls, time, view, RenderScale(1.)
#ifdef DEBUG
                                                  , /*canSetValue*/ false
                                                  , /*canBeCalledRecursively*/ false
#endif
                                                  );

        ActionRetCodeEnum stat = getComponentsNeededInternal(time, view, render, &inputLayersNeeded, &outputLayersProduced, &passThroughTime, &passThroughView, &passThroughInputNb, &processAllRequested, &processChannels);
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }


    // If the plug-in does not block upstream planes, recurse up-stream on the pass-through input to get available components.
    PassThroughEnum passThrough = isPassThroughForNonRenderedPlanes();
    if ( (passThrough == ePassThroughPassThroughNonRenderedPlanes) ||
        ( passThrough == ePassThroughRenderAllRequestedPlanes) ) {

        assert(passThroughInputNb != -1);

        EffectInstancePtr passThroughInput = getInput(passThroughInputNb);
        if (passThroughInput) {
            TreeRenderNodeArgsPtr inputRenderArgs;
            if (render) {
                inputRenderArgs = render->getInputRenderArgs(passThroughInputNb);
            }
            GetComponentsResultsPtr upstreamResults;
            ActionRetCodeEnum stat = getComponents_public(time, view, inputRenderArgs, &upstreamResults);
            if (isFailureRetCode(stat)) {
                return stat;
            }

            std::map<int, std::list<ImageComponents> > upstreamInputLayersNeeded;
            std::list<ImageComponents> upstreamOutputLayersProduced;
            std::list<ImageComponents> upstreamAvailableLayers;
            TimeValue upstreamPassThroughTime;
            ViewIdx upstreamPassThroughView;
            int upstreamPassThroughInputNb;
            std::bitset<4> upstreamProcessChannels;
            bool upstreamProcessAll;
            upstreamResults->getResults(&upstreamInputLayersNeeded, &upstreamOutputLayersProduced, &upstreamAvailableLayers, &upstreamPassThroughInputNb, &upstreamPassThroughTime, &upstreamPassThroughView, &upstreamProcessChannels, &upstreamProcessAll);

            // Merge pass-through planes produced + pass-through available planes and make it as the pass-through planes for this node
            // if they are not produced by this node
            mergeLayersList(upstreamOutputLayersProduced, &upstreamAvailableLayers);

            // upstreamAvailableLayers now contain all available planes in input of this node
            // Remove from this list all layers produced from this node to get the pass-through planes list
            removeFromLayersList(outputLayersProduced, &upstreamAvailableLayers);
            
            passThroughPlanes = upstreamAvailableLayers;
        }
    } // if pass-through for planes

    ret->setResults(inputLayersNeeded, outputLayersProduced, passThroughPlanes, passThroughInputNb, passThroughTime, passThroughView, processChannels, processAllRequested);

    if (fvRequest) {
        fvRequest->setComponentsNeededResults(ret);
    }
    cacheAccess.setEntry(ret);

    return eActionStatusOK;
    
} // getComponents_public


ActionRetCodeEnum
EffectInstance::attachOpenGLContext_public(TimeValue time, ViewIdx view, const RenderScale& scale,
                                           const TreeRenderNodeArgsPtr& renderArgs,
                                           const OSGLContextPtr& glContext,
                                           EffectOpenGLContextDataPtr* data)
{

    // Does this effect support concurrent OpenGL renders ?
    // If so, we just have to lock the attachedContexts map.
    // If not, we have to take the lock now and release it in dettachOpenGLContext_public
    bool concurrentGLRender = supportsConcurrentOpenGLRenders();
    boost::scoped_ptr<QMutexLocker> locker;
    if (concurrentGLRender) {
        locker.reset( new QMutexLocker(&_imp->attachedContextsMutex) );
    } else {
        _imp->attachedContextsMutex.lock();
    }

    std::map<OSGLContextWPtr, EffectOpenGLContextDataPtr>::iterator found = _imp->attachedContexts.find(glContext);
    if ( found != _imp->attachedContexts.end() ) {
        // The context is already attached
        *data = found->second;

        return eActionStatusOK;
    }

    EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
    EffectActionArgsSetter_RAII actionArgsTls(tls,time, view, scale
#ifdef DEBUG
                                              , /*canSetValue*/ false
                                              , /*canBeCalledRecursively*/ false
#endif
                                              );

    ActionRetCodeEnum stat = attachOpenGLContext(time, view, scale, renderArgs, glContext, data);

    if (!isFailureRetCode(stat)) {
        if (!concurrentGLRender) {
            (*data)->setHasTakenLock(true);
        }
        _imp->attachedContexts.insert( std::make_pair(glContext, *data) );
    } else {
        _imp->attachedContextsMutex.unlock();
    }

    // Take the lock until dettach is called for plug-ins that do not support concurrent GL renders
    return stat;

} // attachOpenGLContext_public

void
EffectInstance::dettachAllOpenGLContexts()
{



    QMutexLocker locker(&_imp->attachedContextsMutex);

    for (std::map<OSGLContextWPtr, EffectOpenGLContextDataPtr>::iterator it = _imp->attachedContexts.begin(); it != _imp->attachedContexts.end(); ++it) {
        OSGLContextPtr context = it->first.lock();
        if (!context) {
            continue;
        }
        OSGLContextAttacherPtr attacher = OSGLContextAttacher::create(context);
        attacher->attach();

        if (it->second.use_count() == 1) {
            // If no render is using it, dettach the context
            dettachOpenGLContext(TreeRenderNodeArgsPtr(), context, it->second);
        }
    }
    if ( !_imp->attachedContexts.empty() ) {
        OSGLContext::unsetCurrentContextNoRenderInternal(true, 0);
    }
    _imp->attachedContexts.clear();

} // dettachAllOpenGLContexts

ActionRetCodeEnum
EffectInstance::attachOpenGLContext(TimeValue /*time*/, ViewIdx /*view*/, const RenderScale& /*scale*/, const TreeRenderNodeArgsPtr& /*renderArgs*/, const OSGLContextPtr& /*glContext*/, EffectOpenGLContextDataPtr* /*data*/)
{
    return eActionStatusReplyDefault;
}


ActionRetCodeEnum
EffectInstance::dettachOpenGLContext(const TreeRenderNodeArgsPtr& /*renderArgs*/, const OSGLContextPtr& /*glContext*/, const EffectOpenGLContextDataPtr& /*data*/)
{
    return eActionStatusReplyDefault;
}


ActionRetCodeEnum
EffectInstance::dettachOpenGLContext_public(const TreeRenderNodeArgsPtr& renderArgs, const OSGLContextPtr& glContext, const EffectOpenGLContextDataPtr& data)
{

    bool concurrentGLRender = supportsConcurrentOpenGLRenders();
    boost::scoped_ptr<QMutexLocker> locker;
    if (concurrentGLRender) {
        locker.reset( new QMutexLocker(&_imp->attachedContextsMutex) );
    }


    bool mustUnlock = data->getHasTakenLock();
    std::map<OSGLContextWPtr, EffectOpenGLContextDataPtr>::iterator found = _imp->attachedContexts.find(glContext);
    if ( found != _imp->attachedContexts.end() ) {
        _imp->attachedContexts.erase(found);
    }

    EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
    EffectActionArgsSetter_RAII actionArgsTls(tls, TimeValue(0), ViewIdx(0), RenderScale(1.)
#ifdef DEBUG
                                              , /*canSetValue*/ false
                                              , /*canBeCalledRecursively*/ false
#endif
                                              );

    ActionRetCodeEnum ret = dettachOpenGLContext(renderArgs, glContext, data);
    if (mustUnlock) {
        _imp->attachedContextsMutex.unlock();
    }
    
    return ret;

} // dettachOpenGLContext_public


ActionRetCodeEnum
EffectInstance::beginSequenceRender(double /*first*/,
                                       double /*last*/,
                                       double /*step*/,
                                       bool /*interactive*/,
                                       const RenderScale & /*scale*/,
                                       bool /*isSequentialRender*/,
                                       bool /*isRenderResponseToUserInteraction*/,
                                       bool /*draftMode*/,
                                       ViewIdx /*view*/,
                                       RenderBackendTypeEnum /*backendType*/,
                                       const EffectOpenGLContextDataPtr& /*glContextData*/,
                                       const TreeRenderNodeArgsPtr& /*render*/)
{
    return eActionStatusReplyDefault;
}

ActionRetCodeEnum
EffectInstance::endSequenceRender(double /*first*/,
                                     double /*last*/,
                                     double /*step*/,
                                     bool /*interactive*/,
                                     const RenderScale & /*scale*/,
                                     bool /*isSequentialRender*/,
                                     bool /*isRenderResponseToUserInteraction*/,
                                     bool /*draftMode*/,
                                     ViewIdx /*view*/,
                                     RenderBackendTypeEnum /*backendType*/,
                                     const EffectOpenGLContextDataPtr& /*glContextData*/,
                                     const TreeRenderNodeArgsPtr& /*render*/)
{
    return eActionStatusReplyDefault;
}


ActionRetCodeEnum
EffectInstance::render(const RenderActionArgs & /*args*/)
{
    return eActionStatusReplyDefault;
}


ActionRetCodeEnum
EffectInstance::render_public(const RenderActionArgs & args)
{

    REPORT_CURRENT_THREAD_ACTION( "kOfxImageEffectActionRender", getNode() );
    return render(args);

} // render_public


ActionRetCodeEnum
EffectInstance::beginSequenceRender_public(double first,
                                           double last,
                                           double step,
                                           bool interactive,
                                           const RenderScale & scale,
                                           bool isSequentialRender,
                                           bool isRenderResponseToUserInteraction,
                                           bool draftMode,
                                           ViewIdx view,
                                           RenderBackendTypeEnum backendType,
                                           const EffectOpenGLContextDataPtr& glContextData,
                                           const TreeRenderNodeArgsPtr& render)
{

    REPORT_CURRENT_THREAD_ACTION( "kOfxImageEffectActionBeginSequenceRender", getNode() );



    EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
    EffectActionArgsSetter_RAII actionArgsTls(tls, TimeValue(first), view, scale
#ifdef DEBUG
                                              , /*canSetValue*/ false
                                              , /*canBeCalledRecursively*/ false
#endif
                                              );


    return beginSequenceRender(first, last, step, interactive, scale,
                               isSequentialRender, isRenderResponseToUserInteraction, draftMode, view, backendType, glContextData, render);
} // beginSequenceRender_public

ActionRetCodeEnum
EffectInstance::endSequenceRender_public(double first,
                                         double last,
                                         double step,
                                         bool interactive,
                                         const RenderScale & scale,
                                         bool isSequentialRender,
                                         bool isRenderResponseToUserInteraction,
                                         bool draftMode,
                                         ViewIdx view,
                                         RenderBackendTypeEnum backendType,
                                         const EffectOpenGLContextDataPtr& glContextData,
                                         const TreeRenderNodeArgsPtr& render)
{


    REPORT_CURRENT_THREAD_ACTION( "kOfxImageEffectActionEndSequenceRender", getNode() );

    EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
    EffectActionArgsSetter_RAII actionArgsTls(tls, TimeValue(first), view, scale
#ifdef DEBUG
                                              , /*canSetValue*/ false
                                              , /*canBeCalledRecursively*/ false
#endif
                                              );


    return endSequenceRender(first, last, step, interactive, scale, isSequentialRender, isRenderResponseToUserInteraction, draftMode, view, backendType, glContextData, render);

} // endSequenceRender_public

ActionRetCodeEnum
EffectInstance::getDistorsion(TimeValue /*time*/,
                                 const RenderScale & /*renderScale*/,
                                 ViewIdx /*view*/,
                                 const TreeRenderNodeArgsPtr& /*render*/,
                                 DistorsionFunction2D* /*distorsion*/)
{
    return eActionStatusReplyDefault;
}

ActionRetCodeEnum
EffectInstance::getDistorsion_public(TimeValue inArgsTime,
                                     const RenderScale & renderScale,
                                     ViewIdx view,
                                     const TreeRenderNodeArgsPtr& render,
                                     GetDistorsionResultsPtr* outDisto) {
    assert(outDisto);

    TimeValue time = inArgsTime;
    {
        int roundedTime = std::floor(time + 0.5);
        if (roundedTime != time && !canRenderContinuously(render)) {
            time = TimeValue(roundedTime);
        }
    }

    // Get the render local args
    FrameViewRequestPtr fvRequest;
    if (render) {

        assert(render->getCurrentDistortSupport());

        // Ensure the render object corresponds to this node.
        assert(render->getNode() == getNode());

        // We may not have created a request object for this frame/view yet. Create it.
        bool created = render->getOrCreateFrameViewRequest(time, view, &fvRequest);
        (void)created;
    } else {
        assert(getNode()->getCurrentCanDistort());
    }

    U64 hash = 0;

    if (fvRequest) {
        GetDistorsionResultsPtr cachedDisto = fvRequest->getDistorsionResults();
        if (cachedDisto) {
            *outDisto = cachedDisto;
            return eActionStatusOK;
        }
        fvRequest->getHash(&hash);
    }



    // Get a hash to cache the results
    if (hash == 0) {
        ComputeHashArgs hashArgs;
        hashArgs.render = render;
        hashArgs.time = time;
        hashArgs.view = view;
        hashArgs.hashType = HashableObject::eComputeHashTypeTimeViewVariant;
        hash = computeHash(hashArgs);
    }

    assert(hash != 0);

    GetDistorsionKeyPtr cacheKey(new GetDistorsionKey(hash, renderScale, getNode()->getPluginID()));

    // Ensure the cache fetcher lives as long as we compute the action
    CacheFetcher cacheAccess(cacheKey);

    GetDistorsionResultsPtr ret = toGetDistorsionResults(cacheAccess.isCached());
    if (ret) {
        *outDisto = ret;
        return eActionStatusOK;
    }
    ret = GetDistorsionResults::create(cacheKey);

    // If the effect is identity, do not call the getDistorsion action, instead just return an identity
    // identity time and view.

    DistorsionFunction2DPtr distorsion(new DistorsionFunction2D);
    bool isIdentity;
    {
        // If the effect is identity on the format, that means its bound to be identity anywhere and does not depend on the render window.
        RectI format = getOutputFormat(render);
        RenderScale scale(1.);
        IsIdentityResultsPtr identityResults;
        isIdentity = isIdentity_public(true, time, scale, format, view, render, &identityResults);
    }

    if (isIdentity) {
        distorsion->transformMatrix.reset(new Transform::Matrix3x3);
        distorsion->transformMatrix->setIdentity();
    } else {

        EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
        EffectActionArgsSetter_RAII actionArgsTls(tls, time, view, renderScale
#ifdef DEBUG
                                                  , /*canSetValue*/ false
                                                  , /*canBeCalledRecursively*/ true
#endif

                                                  );

        // Call the action
        ActionRetCodeEnum stat = getDistorsion(time, renderScale, view, render, distorsion.get());
        if (isFailureRetCode(stat)) {
            return stat;
        }

    }

    ret->setDistorsionResults(distorsion);

    if (fvRequest) {
        fvRequest->setDistorsionResults(ret);
    }
    
    *outDisto = ret;
    cacheAccess.setEntry(ret);
    return eActionStatusOK;

} // getDistorsion_public

ActionRetCodeEnum
EffectInstance::isIdentity(TimeValue /*time*/,
                      const RenderScale & /*scale*/,
                      const RectI & /*roi*/,
                      ViewIdx /*view*/,
                      const TreeRenderNodeArgsPtr& /*render*/,
                      TimeValue* /*inputTime*/,
                      ViewIdx* /*inputView*/,
                      int* inputNb)
{
    *inputNb = -1;
    return eActionStatusReplyDefault;
}


ActionRetCodeEnum
EffectInstance::isIdentity_public(bool useIdentityCache, // only set to true when calling for the whole image (not for a subrect)
                                  TimeValue time,
                                  const RenderScale & scale,
                                  const RectI & renderWindow,
                                  ViewIdx view,
                                  const TreeRenderNodeArgsPtr& render,
                                  IsIdentityResultsPtr* results)
{
    

    {
        int roundedTime = std::floor(time + 0.5);

        // A continuous effect is identity on itself on nearest integer time
        if (roundedTime != time && !canRenderContinuously(render)) {
            // We do not cache it because for non continuous effects we only cache stuff at
            // valid frame times
            *results = IsIdentityResults::create(IsIdentityKeyPtr());
            (*results)->setIdentityData(-2, TimeValue(roundedTime), view);
            return eActionStatusOK;
        }
    }

    // Non view aware effect is identity on the main view
    if (isViewInvariant() == eViewInvarianceAllViewsInvariant && view != 0) {
        // We do not cache it because for we only cache stuff for
        // valid views
        *results = IsIdentityResults::create(IsIdentityKeyPtr());
        (*results)->setIdentityData(-2, time, ViewIdx(0));
        return eActionStatusOK;
    }


    // Get the render local args
    FrameViewRequestPtr fvRequest;
    if (render) {

        // Ensure the render object corresponds to this node.
        assert(render->getNode() == getNode());

        // We may not have created a request object for this frame/view yet. Create it.
        bool created = render->getOrCreateFrameViewRequest(time, view, &fvRequest);
        (void)created;
    }

    U64 hash = 0;
    if (fvRequest) {
        *results = fvRequest->getIdentityResults();
        if (*results) {
            return eActionStatusOK;
        }
        fvRequest->getHash(&hash);
    }

    // Get a hash to cache the results
    if (useIdentityCache && hash == 0) {
        ComputeHashArgs hashArgs;
        hashArgs.render = render;
        hashArgs.time = time;
        hashArgs.view = view;
        hashArgs.hashType = HashableObject::eComputeHashTypeTimeViewVariant;
        hash = computeHash(hashArgs);
    }

    IsIdentityKeyPtr cacheKey;
    boost::scoped_ptr<CacheFetcher> cacheAccess;
    if (useIdentityCache) {
        cacheKey.reset(new IsIdentityKey(hash, getNode()->getPluginID()));
        cacheAccess.reset(new CacheFetcher(cacheKey));
        *results = toIsIdentityResults(cacheAccess->isCached());
        if (*results) {
            return eActionStatusOK;
        }
    }

    IsIdentityResultsPtr identityData = IsIdentityResults::create(cacheKey);
    bool caught = false;

    // Node is disabled or doesn't have any channel to process, be identity on the main input
    if ((getNode()->isNodeDisabledForFrame(time, view) || !getNode()->hasAtLeastOneChannelToProcess() )) {
        identityData->setIdentityData(getNode()->getPreferredInput(), time, view);
        caught = true;
    }

    // Call the isIdentity plug-in action
    if (!caught) {


        EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
        EffectActionArgsSetter_RAII actionArgsTls(tls, time, view, scale
#ifdef DEBUG
                                                  , /*canSetValue*/ false
                                                  , /*canBeCalledRecursively*/ true
#endif
                                                  );

        TimeValue identityTime;
        ViewIdx identityView;
        int identityInputNb;
        ActionRetCodeEnum stat = isIdentity(time, scale, renderWindow, view, render, &identityTime, &identityView, &identityInputNb);
        if (isFailureRetCode(stat)) {
            return stat;
        }

        // -2 means identity on itself.
        // A sequential effect cannot be identity on itself
        if (identityInputNb == -2) {
            SequentialPreferenceEnum sequential;
            if (render) {
                sequential = render->getCurrentRenderSequentialPreference();
            } else {
                sequential = getNode()->getCurrentSequentialRenderSupport();
            }

            if (sequential == eSequentialPreferenceOnlySequential) {
                identityInputNb = -1;
            }
        }

        identityData->setIdentityData(identityInputNb, identityTime, identityView);

        caught = true;

    }
    if (!caught) {
        identityData->setIdentityData(-1, time, view);
    }

    if (fvRequest) {
        fvRequest->setIdentityResults(identityData);
    }
    if (cacheAccess) {
        cacheAccess->setEntry(identityData);
    }
    *results = identityData;
    return eActionStatusOK;
} // isIdentity_public

ActionRetCodeEnum
EffectInstance::getRegionOfDefinitionFromCache(TimeValue inArgsTime,
                                               const RenderScale & scale,
                                               ViewIdx view,
                                               RectD* results)
{
    TimeValue time = inArgsTime;

    // Get the hash of the node
    U64 hash = 0;
    {
        FindHashArgs findArgs;
        findArgs.time = time;
        findArgs.view = view;
        findArgs.hashType = HashableObject::eComputeHashTypeTimeViewVariant;
        bool gotHash = findCachedHash(findArgs, &hash);

        if (!gotHash) {
            return eActionStatusFailed;
        }
    }
    assert(hash != 0);

    GetRegionOfDefinitionKeyPtr cacheKey(new GetRegionOfDefinitionKey(hash, scale, getNode()->getPluginID()));

    CacheFetcher cacheAccess(cacheKey);


    GetRegionOfDefinitionResultsPtr ret = toGetRegionOfDefinitionResults(cacheAccess.isCached());
    if (!ret) {
        return eActionStatusFailed;
    }

    *results = ret->getRoD();
    
    return eActionStatusOK;
} // getRegionOfDefinitionFromCache

ActionRetCodeEnum
EffectInstance::getRegionOfDefinition_public(TimeValue inArgsTime,
                                             const RenderScale & scale,
                                             ViewIdx view,
                                             const TreeRenderNodeArgsPtr& render,
                                             GetRegionOfDefinitionResultsPtr* results)
{
    TimeValue time = inArgsTime;
    {
        int roundedTime = std::floor(time + 0.5);
        if (roundedTime != time && !canRenderContinuously(render)) {
            time = TimeValue(roundedTime);
        }
    }


    // Get the render local args
    FrameViewRequestPtr fvRequest;
    if (render) {

        // Ensure the render object corresponds to this node.
        assert(render->getNode() == getNode());

        // We may not have created a request object for this frame/view yet. Create it.
        bool created = render->getOrCreateFrameViewRequest(time, view, &fvRequest);
        (void)created;
    }
    U64 hash = 0;

    if (fvRequest) {
        *results = fvRequest->getRegionOfDefinitionResults();
        if (*results) {
            return eActionStatusOK;
        }
        fvRequest->getHash(&hash);
    }

    // When drawing a paint-stroke, never use the getRegionOfDefinition cache because the RoD changes at each render step
    // but the hash does not (so that each draw step can re-use the same image.)
    bool useCache = !getNode()->isDuringPaintStrokeCreation();

    // Get a hash to cache the results
    if (useCache && hash == 0) {
        ComputeHashArgs hashArgs;
        hashArgs.render = render;
        hashArgs.time = time;
        hashArgs.view = view;
        hashArgs.hashType = HashableObject::eComputeHashTypeTimeViewVariant;
        hash = computeHash(hashArgs);
    }

    GetRegionOfDefinitionKeyPtr cacheKey(new GetRegionOfDefinitionKey(hash, scale, getNode()->getPluginID()));
    boost::scoped_ptr<CacheFetcher> cacheAccess;
    if (useCache) {
        assert(hash != 0);
        cacheKey.reset(new GetRegionOfDefinitionKey(hash, scale, getNode()->getPluginID()));
        cacheAccess.reset(new CacheFetcher(cacheKey));
        *results = toGetRegionOfDefinitionResults(cacheAccess->isCached());
        if (*results) {
            return eActionStatusOK;
        }
    }

    *results = GetRegionOfDefinitionResults::create(cacheKey);

    // If the effect is identity, do not call the getRegionOfDefinition action, instead just return the input identity at the
    // identity time and view.

    {
        // If the effect is identity on the format, that means its bound to be identity anywhere and does not depend on the render window.
        RectI format = getOutputFormat(render);
        RenderScale scale(1.);
        IsIdentityResultsPtr identityResults;
        ActionRetCodeEnum stat = isIdentity_public(true, time, scale, format, view, render, &identityResults);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        assert(identityResults);

        int inputIdentityNb;
        TimeValue identityTime;
        ViewIdx identityView;
        identityResults->getIdentityData(&inputIdentityNb, &identityTime, &identityView);

        if (inputIdentityNb == -1) {
            // This effect is identity
            EffectInstancePtr identityInputNode = getInput(inputIdentityNb);
            if (!identityInputNode) {
                return eActionStatusInputDisconnected;
            }

            // Get the render args for the input
            TreeRenderNodeArgsPtr inputRenderArgs;
            if (render) {
                inputRenderArgs = render->getInputRenderArgs(inputIdentityNb);
            }

            GetRegionOfDefinitionResultsPtr inputResults;
            ActionRetCodeEnum stat = identityInputNode->getRegionOfDefinition_public(identityTime, scale, identityView, inputRenderArgs, &inputResults);
            if (isFailureRetCode(stat)) {
                return stat;
            }
            (*results)->setRoD(inputResults->getRoD());
        } else {
            // Not identity

            // Check if all mandatory inputs are connected, otherwise return an error
            int nInputs = getMaxInputCount();
            for (int i = 0; i < nInputs; ++i) {
                if (isInputOptional(i)) {
                    continue;
                }
                if (!getInput(i)) {
                    return eActionStatusInputDisconnected;
                }
            }

            EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();

            EffectActionArgsSetter_RAII actionArgsTls(tls,time, view, scale
#ifdef DEBUG
                                                      , /*canSetValue*/ false
                                                      , /*canBeCalledRecursively*/ true
#endif
                                                      );


            RectD rod;
            ActionRetCodeEnum stat = getRegionOfDefinition(time, scale, view, render, &rod);

            if (isFailureRetCode(stat)) {
                return stat;
            }
            ifInfiniteApplyHeuristic(time, scale, view, render, &rod);

            (*results)->setRoD(rod);
            
            
        } // inputIdentityNb == -1
    }

    if (fvRequest) {
        fvRequest->setRegionOfDefinitionResults(*results);
    }
    if (cacheAccess) {
        cacheAccess->setEntry(*results);
    }
    
    return eActionStatusOK;
    
} // getRegionOfDefinition_public


void
EffectInstance::calcDefaultRegionOfDefinition_public(TimeValue time, const RenderScale & scale, ViewIdx view,const TreeRenderNodeArgsPtr& render, RectD *rod)
{
    EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
    EffectActionArgsSetter_RAII actionArgsTls(tls, time, view, scale
#ifdef DEBUG
                                              , /*canSetValue*/ false
                                              , /*canBeCalledRecursively*/ true
#endif
                                              );


    return calcDefaultRegionOfDefinition(time, scale, view, render, rod);
}

void
EffectInstance::calcDefaultRegionOfDefinition(TimeValue /*time*/,
                                              const RenderScale & scale,
                                              ViewIdx /*view*/,
                                              const TreeRenderNodeArgsPtr& render,
                                              RectD *rod)
{

    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    RectI format = getOutputFormat(render);
    double par = getAspectRatio(render, -1);
    format.toCanonical_noClipping(mipMapLevel, par, rod);
} // calcDefaultRegionOfDefinition

ActionRetCodeEnum
EffectInstance::getRegionOfDefinition(TimeValue time,
                                      const RenderScale & scale,
                                      ViewIdx view,
                                      const TreeRenderNodeArgsPtr& render,
                                      RectD* rod) //!< rod is in canonical coordinates
{
    bool firstInput = true;
    RenderScale renderMappedScale = scale;

    // By default, union the region of definition of all non mask inputs.
    int nInputs = getMaxInputCount();
    for (int i = 0; i < nInputs; ++i) {
        if ( isInputMask(i) ) {
            continue;
        }
        EffectInstancePtr input = getInput(i);
        if (input) {
            GetRegionOfDefinitionResultsPtr inputResults;
            TreeRenderNodeArgsPtr inputRenderArgs;
            if (render) {
                inputRenderArgs = render->getInputRenderArgs(i);
                assert(inputRenderArgs);
            }

            ActionRetCodeEnum stat = input->getRegionOfDefinition_public(time, renderMappedScale, view, inputRenderArgs, &inputResults);
            if (isFailureRetCode(stat)) {
                return stat;
            }
            assert(inputResults);

            const RectD& inputRoD =  inputResults->getRoD();

            assert(inputRoD.x2 >= inputRoD.x1 && inputRoD.y2 >= inputRoD.y1);


            if (firstInput) {
                *rod = inputRoD;
                firstInput = false;
            } else {
                rod->merge(inputRoD);
            }
            assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);
        }
    }

    // if rod was not set, return default, else return OK
    return firstInput ? eActionStatusReplyDefault : eActionStatusOK;
} // getRegionOfDefinition

void
EffectInstance::ifInfiniteApplyHeuristic(TimeValue time,
                                         const RenderScale & scale,
                                         ViewIdx view,
                                         const TreeRenderNodeArgsPtr& render,
                                         RectD* rod) //!< input/output
{
    // If the rod is infinite clip it to the format

    assert(rod);
    if ( rod->isNull() ) {
        // if the RoD is empty, set it to a "standard" empty RoD (0,0,0,0)
        rod->clear();
    }
    assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);
    bool x1Infinite = rod->x1 <= kOfxFlagInfiniteMin;
    bool y1Infinite = rod->y1 <= kOfxFlagInfiniteMin;
    bool x2Infinite = rod->x2 >= kOfxFlagInfiniteMax;
    bool y2Infinite = rod->y2 >= kOfxFlagInfiniteMax;

    // Get the union of the inputs.
    RectD inputsUnion;

    // Do the following only if one coordinate is infinite otherwise we wont need the RoD of the input
    if (x1Infinite || y1Infinite || x2Infinite || y2Infinite) {

        // initialize with the effect's default RoD, because inputs may not be connected to other effects (e.g. Roto)
        calcDefaultRegionOfDefinition_public(time, scale, view, render, &inputsUnion);
    }

    // If infinite : clip to inputsUnion if not null, otherwise to project default
    RectD canonicalFormat;

    if (x1Infinite || y1Infinite || x2Infinite || y2Infinite) {
        RectI format = getOutputFormat(render);
        assert(!format.isNull());
        double par = getAspectRatio(render, -1);
        unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
        format.toCanonical_noClipping(mipMapLevel, par, &canonicalFormat);
    }

    // BE CAREFUL:
    // std::numeric_limits<int>::infinity() does not exist (check std::numeric_limits<int>::has_infinity)
    if (x1Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->x1 = std::min(inputsUnion.x1, canonicalFormat.x1);
        } else {
            rod->x1 = canonicalFormat.x1;
        }
        rod->x2 = std::max(rod->x1, rod->x2);
    }
    if (y1Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->y1 = std::min(inputsUnion.y1, canonicalFormat.y1);
        } else {
            rod->y1 = canonicalFormat.y1;
        }
        rod->y2 = std::max(rod->y1, rod->y2);
    }
    if (x2Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->x2 = std::max(inputsUnion.x2, canonicalFormat.x2);
        } else {
            rod->x2 = canonicalFormat.x2;
        }
        rod->x1 = std::min(rod->x1, rod->x2);
    }
    if (y2Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->y2 = std::max(inputsUnion.y2, canonicalFormat.y2);
        } else {
            rod->y2 = canonicalFormat.y2;
        }
        rod->y1 = std::min(rod->y1, rod->y2);
    }

    assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);
    
} // ifInfiniteApplyHeuristic


ActionRetCodeEnum
EffectInstance::getRegionsOfInterest_public(TimeValue inArgsTime,
                                            const RenderScale & scale,
                                            const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                            ViewIdx view,
                                            const TreeRenderNodeArgsPtr& render,
                                            RoIMap* ret)
{
    TimeValue time = inArgsTime;
    {
        int roundedTime = std::floor(time + 0.5);
        if (roundedTime != time && !canRenderContinuously(render)) {
            time = TimeValue(roundedTime);
        }
    }

    assert(renderWindow.x2 >= renderWindow.x1 && renderWindow.y2 >= renderWindow.y1);


    EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
    EffectActionArgsSetter_RAII actionArgsTls(tls,time, view, scale
#ifdef DEBUG
                                              , /*canSetValue*/ false
                                              , /*canBeCalledRecursively*/ false
#endif
                                              );

    return getRegionsOfInterest(time, scale, renderWindow, view, render, ret);

} // getRegionsOfInterest_public

ActionRetCodeEnum
EffectInstance::getRegionsOfInterest(TimeValue time,
                                     const RenderScale & scale,
                                     const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                     ViewIdx view,
                                     const TreeRenderNodeArgsPtr& render,
                                     RoIMap* ret)
{
    bool tilesSupported;

    if (render) {
        tilesSupported = render->getCurrentTilesSupport();
    } else {
        tilesSupported = getNode()->getCurrentSupportTiles();
    }

    int nInputs = getMaxInputCount();
    for (int i = 0; i < nInputs; ++i) {
        EffectInstancePtr input = getInput(i);
        if (!input) {
            continue;
        }
        if (tilesSupported) {
            ret->insert( std::make_pair(i, renderWindow) );
        } else {
            // Tiles not supported: get the RoD as RoI
            GetRegionOfDefinitionResultsPtr inputResults;
            // Get the render args for the input
            TreeRenderNodeArgsPtr inputRenderArgs;
            if (render) {
                inputRenderArgs = render->getInputRenderArgs(i);
            }

            ActionRetCodeEnum stat = input->getRegionOfDefinition_public(time, scale, view, inputRenderArgs, &inputResults);
            if (isFailureRetCode(stat)) {
                return stat;
            }
            const RectD& inputRoD = inputResults->getRoD();

            ret->insert( std::make_pair(i, inputRoD) );
        }

    }
    return eActionStatusReplyDefault;
} // getRegionsOfInterest


ActionRetCodeEnum
EffectInstance::getFramesNeeded(TimeValue time,
                                ViewIdx view,
                                const TreeRenderNodeArgsPtr& /*render*/,
                                FramesNeededMap* ret)
{
    RangeD defaultRange = {time, time};

    std::vector<RangeD> ranges(1);
    ranges[0] = defaultRange;

    FrameRangesMap defViewRange;
    defViewRange.insert( std::make_pair(view, ranges) );

    int nInputs = getMaxInputCount();
    for (int i = 0; i < nInputs; ++i) {

        EffectInstancePtr input = getInput(i);
        if (input) {
            ret->insert( std::make_pair(i, defViewRange) );
        }

    }
    
    return eActionStatusReplyDefault;
} // getFramesNeeded

ActionRetCodeEnum
EffectInstance::getFramesNeeded_public(TimeValue inArgsTime,
                                       ViewIdx view,
                                       const TreeRenderNodeArgsPtr& render,
                                       GetFramesNeededResultsPtr* results)
{

    // Round time for non continuous effects
    TimeValue time = inArgsTime;
    {
        int roundedTime = std::floor(time + 0.5);
        if (roundedTime != time && !canRenderContinuously(render)) {
            time = TimeValue(roundedTime);
        }
    }

    // Get the render local args
    FrameViewRequestPtr fvRequest;
    if (render) {

        // Ensure the render object corresponds to this node.
        assert(render->getNode() == getNode());

        // We may not have created a request object for this frame/view yet. Create it.
        bool created = render->getOrCreateFrameViewRequest(time, view, &fvRequest);
        (void)created;
    }
    U64 hashValue = 0;
    if (fvRequest) {
        *results = fvRequest->getFramesNeededResults();
        fvRequest->getHash(&hashValue);
        if (*results) {
            return eActionStatusOK;
        }
        fvRequest->getHash(&hashValue);
    }

    NodePtr thisNode = getNode();

    GetFramesNeededKeyPtr cacheKey;
    boost::scoped_ptr<CacheFetcher> cacheAccess;

    // Only use the cache if we got a hash.
    // We cannot compute the hash here because the hash itself requires the result of this function.
    // The results of this function is cached externally instead
    bool isHashCached = hashValue != 0;
    if (isHashCached) {

        cacheKey.reset(new GetFramesNeededKey(hashValue, getNode()->getPluginID()));
        cacheAccess.reset(new CacheFetcher(cacheKey));
        *results = toGetFramesNeededResults(cacheAccess->isCached());
        if (*results) {
            return eActionStatusOK;
        }
    }


    // Call the action
    FramesNeededMap framesNeeded;
    {

        // Check if all mandatory inputs are connected, otherwise return an error
        int nInputs = getMaxInputCount();
        for (int i = 0; i < nInputs; ++i) {
            if (isInputOptional(i)) {
                continue;
            }
            if (!getInput(i)) {
                return eActionStatusInputDisconnected;
            }
        }

        EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
        EffectActionArgsSetter_RAII actionArgsTls(tls,time, view, RenderScale(1.)
#ifdef DEBUG
                                                  , /*canSetValue*/ false
                                                  , /*canBeCalledRecursively*/ false
#endif
                                                  );

        ActionRetCodeEnum stat = getFramesNeeded(time, view, render, &framesNeeded);
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }


    // If the effect is identity, do not follow the result of the getFramesNeeded action, instead just add the input identity at the
    // identity time and view.
    TimeValue inputIdentityTime;
    ViewIdx inputIdentityView;
    int inputIdentityNb;
    IsIdentityResultsPtr identityResults;
    {
        // If the effect is identity on the format, that means its bound to be identity anywhere and does not depend on the render window.
        RectI format = getOutputFormat(render);
        RenderScale scale(1.);
        ActionRetCodeEnum stat = isIdentity_public(true, time, scale, format, view, render, &identityResults);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        identityResults->getIdentityData(&inputIdentityNb, &inputIdentityTime, &inputIdentityView);
    }

    if (inputIdentityNb != -1) {

        // If the node is identity, get the preferred input at the identity time

        framesNeeded.clear();

        RangeD defaultRange = {inputIdentityTime, inputIdentityTime};

        std::vector<RangeD> ranges;
        ranges.push_back(defaultRange);

        FrameRangesMap defViewRange;
        defViewRange[inputIdentityView] = ranges;
        if (inputIdentityNb != -1) {
            framesNeeded[inputIdentityNb] = defViewRange;
        }

    }


    if (!cacheAccess) {
        cacheKey.reset(new GetFramesNeededKey(hashValue, getNode()->getPluginID()));
        cacheAccess.reset(new CacheFetcher(cacheKey));
    }

    *results = GetFramesNeededResults::create(cacheKey);
    (*results)->setFramesNeeded(framesNeeded);
    
    if (fvRequest) {
        fvRequest->setFramesNeededResults(*results);
    }
    
    cacheAccess->setEntry(*results);

    return eActionStatusOK;

} // getFramesNeeded_public




ActionRetCodeEnum
EffectInstance::getFrameRange(const TreeRenderNodeArgsPtr& render,
                              double *first,
                              double *last)
{
    // default is infinite if there are no non optional input clips
    *first = INT_MIN;
    *last = INT_MAX;
    int nInputs = getMaxInputCount();

    // Default to the union of all the frame ranges of the non optional input clips.

    // Edit: Uncommented the isInputOptional call which introduces a bugs with Genarts Monster plug-ins when 2 generators
    // are connected in the pipeline. They must rely on the time domain to maintain an internal state and apparantly
    // not taking optional inputs into accounts messes it up.

    for (int i = 0; i < nInputs; ++i) {
        EffectInstancePtr input = getInput(i);
        if (input) {
            //if (!isInputOptional(i))
            GetFrameRangeResultsPtr inputResults;
            ActionRetCodeEnum stat = input->getFrameRange_public(render, &inputResults);
            if (isFailureRetCode(stat)) {
                return stat;
            }
            assert(inputResults);
            RangeD inputRange;
            inputResults->getFrameRangeResults(&inputRange);
            if (i == 0) {
                *first = inputRange.min;
                *last = inputRange.max;
            } else {
                *first = std::min(*first, inputRange.min);
                *last = std::max(*last, inputRange.max);
            }
        }
    }
    return eActionStatusReplyDefault;
} // getFrameRange


ActionRetCodeEnum
EffectInstance::getFrameRange_public(const TreeRenderNodeArgsPtr& render, GetFrameRangeResultsPtr* results)
{

    // Get a hash to cache the results
    U64 hash = 0;

    if (render) {
        *results = render->getFrameRangeResults();
        if (*results) {
            return eActionStatusOK;
        }
        render->getTimeViewInvariantHash(&hash);
    }


    if (hash == 0) {

        ComputeHashArgs hashArgs;
        hashArgs.render = render;
        hashArgs.hashType = HashableObject::eComputeHashTypeTimeViewInvariant;
        hash = computeHash(hashArgs);
    }

    GetFrameRangeKeyPtr cacheKey(new GetFrameRangeKey(hash, getNode()->getPluginID()));
    CacheFetcher cacheAccess(cacheKey);

    *results = toGetFrameRangeResults(cacheAccess.isCached());
    if (*results) {
        return eActionStatusOK;
    }


    // Call the action
    RangeD range;
    {


        EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
        EffectActionArgsSetter_RAII actionArgsTls(tls,TimeValue(0), ViewIdx(0), RenderScale(1.)
#ifdef DEBUG
                                                  , /*canSetValue*/ false
                                                  , /*canBeCalledRecursively*/ false
#endif
                                                  );

        ActionRetCodeEnum stat = getFrameRange(render, &range.min, &range.max);
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }

    *results = GetFrameRangeResults::create(cacheKey);
    (*results)->setFrameRangeResults(range);
    cacheAccess.setEntry(*results);

    if (render) {
        render->setFrameRangeResults(*results);
    }
    return eActionStatusOK;
    
} // getFrameRange_public


void
EffectInstance::beginKnobsValuesChanged_public(ValueChangedReasonEnum reason)
{
    beginKnobsValuesChanged(reason);
}

void
EffectInstance::endKnobsValuesChanged_public(ValueChangedReasonEnum reason)
{
    endKnobsValuesChanged(reason);
}


void
EffectInstance::beginEditKnobs_public()
{

    beginEditKnobs();
}

bool
EffectInstance::knobChanged(const KnobIPtr& /*k*/,
                         ValueChangedReasonEnum /*reason*/,
                         ViewSetSpec /*view*/,
                         TimeValue /*time*/)
{
    return false;
}

bool
EffectInstance::onKnobValueChanged_public(const KnobIPtr& k,
                                          ValueChangedReasonEnum reason,
                                          TimeValue time,
                                          ViewSetSpec view)
{
    NodePtr node = getNode();
    if (!node->isNodeCreated()) {
        return false;
    }

    // If the param changed is a button and the node is disabled don't do anything which might
    // trigger an analysis
    if ( (reason == eValueChangedReasonUserEdited) && toKnobButton(k) && node->getDisabledKnobValue() ) {
        return false;
    }

    if ( (reason != eValueChangedReasonTimeChanged) && ( isReader() || isWriter() ) && (k->getName() == kOfxImageEffectFileParamName) ) {
        node->onFileNameParameterChanged(k);
    }

    bool ret = false;

    // assert(!(view.isAll() || view.isCurrent())); // not yet implemented
    const ViewIdx viewIdx( (view.isAll()) ? 0 : view );
    bool wasFormatKnobCaught = node->handleFormatKnob(k);
    KnobHelperPtr kh = boost::dynamic_pointer_cast<KnobHelper>(k);
    assert(kh);
    if (kh && kh->isDeclaredByPlugin() && !wasFormatKnobCaught) {
        {


            REPORT_CURRENT_THREAD_ACTION( "kOfxActionInstanceChanged", getNode() );
            // Map to a plug-in known reason
            if (reason == eValueChangedReasonUserEdited) {
                reason = eValueChangedReasonUserEdited;
            }
            ret |= knobChanged(k, reason, view, time);
        }
    }

    if ( kh && ( reason != eValueChangedReasonTimeChanged) ) {
        ///Run the following only in the main-thread
        if ( hasOverlay() && node->shouldDrawOverlay(time, ViewIdx(0)) && !node->hasHostOverlayForParam(k) ) {

            // Some plugins (e.g. by digital film tools) forget to set kOfxInteractPropSlaveToParam.
            // Most hosts trigger a redraw if the plugin has an active overlay.
            requestOverlayInteractRefresh();
        }
        if (isOverlaySlaveParam(kh)) {
            kh->redraw();
        }
    }

    ret |= node->onEffectKnobValueChanged(k, reason);

    //Don't call the python callback if the reason is time changed
    if (reason == eValueChangedReasonTimeChanged) {
        return false;
    }

    ///If there's a knobChanged Python callback, run it
    {
        bool userEdited = reason == eValueChangedReasonUserEdited ||
        reason == eValueChangedReasonUserEdited;
        getNode()->runChangedParamCallback(k, userEdited);
    }

    ///Refresh the dynamic properties that can be changed during the instanceChanged action
    node->refreshDynamicProperties();

    // If there are any render clones, kill them as the plug-in might have changed internally
    clearRenderInstances();
    
    return ret;
} // onKnobValueChanged_public


void
EffectInstance::onInputChanged(int /*inputNo*/, const NodePtr& /*oldNode*/, const NodePtr& /*newNode*/)
{
}

void
EffectInstance::onInputChanged_public(int inputNo, const NodePtr& oldNode, const NodePtr& newNode)
{

    REPORT_CURRENT_THREAD_ACTION( "kOfxActionInstanceChanged", getNode() );


    EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
    EffectActionArgsSetter_RAII actionArgsTls(tls, TimeValue(getApp()->getTimeLine()->currentFrame()), ViewIdx(0), RenderScale(1.)
#ifdef DEBUG
                                              , /*canSetValue*/ true
                                              , /*canBeCalledRecursively*/ true
#endif
                                              );

    onInputChanged(inputNo, oldNode, newNode);
} // onInputChanged_public

ActionRetCodeEnum
EffectInstance::getTimeInvariantMetaDatas_public(const TreeRenderNodeArgsPtr& render, GetTimeInvariantMetaDatasResultsPtr* results)
{
    // Get a hash to cache the results
    U64 hash = 0;

    if (render) {
        *results = render->getTimeInvariantMetadataResults();
        if (*results) {
            return eActionStatusOK;
        }
        render->getTimeInvariantMetadataHash(&hash);
    }


    if (hash == 0) {

        ComputeHashArgs hashArgs;
        hashArgs.render = render;
        hashArgs.hashType = HashableObject::eComputeHashTypeOnlyMetadataSlaves;
        hash = computeHash(hashArgs);
    }

    GetTimeInvariantMetaDatasKeyPtr cacheKey(new GetTimeInvariantMetaDatasKey(hash, getNode()->getPluginID()));
    CacheFetcher cacheAccess(cacheKey);

    *results = toGetTimeInvariantMetaDatasResults(cacheAccess.isCached());
    if (*results) {
        return eActionStatusOK;
    }

    // If the node is disabled return the meta-datas of the main input.
    // Don't do that for an identity node: a render may be identity but not the metadatas (e.g: NoOp)
    // A disabled generator still has to return some meta-datas, so let it return the default meta-datas.
    bool isDisabled = getNode()->getDisabledKnobValue();
    if (isDisabled) {
        int mainInput = getNode()->getPreferredInput();
        if (mainInput != -1) {
            EffectInstancePtr input = getInput(mainInput);
            if (input) {
                TreeRenderNodeArgsPtr inputRenderArgs;
                if (render) {
                    inputRenderArgs = render->getInputRenderArgs(mainInput);
                }
                GetTimeInvariantMetaDatasResultsPtr inputResults;
                ActionRetCodeEnum stat = input->getTimeInvariantMetaDatas_public(inputRenderArgs, &inputResults);
                if (isFailureRetCode(stat)) {
                    return stat;
                }
                *results = inputResults;
                return eActionStatusOK;
            }
        }
    }

    NodeMetadata metadata;
    ActionRetCodeEnum stat = getDefaultMetadata(render, metadata);

    if (isFailureRetCode(stat)) {
        return stat;
    }

    *results = GetTimeInvariantMetaDatasResults::create(cacheKey);

    if (!isDisabled) {


        EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
        EffectActionArgsSetter_RAII actionArgsTls(tls, TimeValue(0), ViewIdx(0), RenderScale(1.)
#ifdef DEBUG
                                                  , /*canSetValue*/ true
                                                  , /*canBeCalledRecursively*/ true
#endif
                                                  );

        // If the node is disabled, don't call getClipPreferences on the plug-in:
        // we don't want it to change output Format or other metadatas
        ActionRetCodeEnum stat = getTimeInvariantMetaDatas(metadata);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        _imp->checkMetadata(metadata);
    }


    // For a Reader, try to add the output format to the project formats.
    if (isReader()) {
        Format format;
        format.set(metadata.getOutputFormat());
        format.setPixelAspectRatio(metadata.getPixelAspectRatio(-1));
        getApp()->getProject()->setOrAddProjectFormat(format, true);
    }

    cacheAccess.setEntry(*results);
    return eActionStatusOK;
} // getTimeInvariantMetaDatas_public


static ImageComponents
getUnmappedComponentsForInput(const EffectInstancePtr& self,
                              int inputNb,
                              const std::vector<NodeMetadataPtr>& inputs,
                              const ImageComponents& firstNonOptionalConnectedInputComps)
{
    ImageComponents rawComps;

    if (inputs[inputNb]) {
        rawComps = inputs[inputNb]->getImageComponents(-1);
    } else {
        ///The node is not connected but optional, return the closest supported components
        ///of the first connected non optional input.
        rawComps = firstNonOptionalConnectedInputComps;
    }
    if (rawComps) {
        if (!rawComps) {
            //None comps
            return rawComps;
        } else {
            rawComps = self->findClosestSupportedComponents(inputNb, rawComps); //turn that into a comp the plugin expects on that clip
        }
    }
    if (!rawComps) {
        rawComps = ImageComponents::getRGBAComponents(); // default to RGBA
    }

    return rawComps;
} // getUnmappedComponentsForInput

ActionRetCodeEnum
EffectInstance::getDefaultMetadata(const TreeRenderNodeArgsPtr& render, NodeMetadata &metadata)
{
    NodePtr node = getNode();

    if (!node) {
        return eActionStatusInputDisconnected;
    }

    const bool multiBitDepth = supportsMultipleClipDepths();
    int nInputs = getMaxInputCount();
    metadata.clearAndResize(nInputs);

    // OK find the deepest chromatic component on our input clips and the one with the
    // most components
    bool hasSetCompsAndDepth = false;
    ImageBitDepthEnum deepestBitDepth = eImageBitDepthNone;
    ImageComponents mostComponents;

    //Default to the project frame rate
    double frameRate = getApp()->getProjectFrameRate();

    double inputPar = 1.;
    bool inputParSet = false;
    ImagePremultiplicationEnum premult = eImagePremultiplicationOpaque;
    bool premultSet = false;

    bool hasOneInputContinuous = false;
    bool hasOneInputFrameVarying = false;

    // Find the components of the first non optional connected input
    // They will be used for disconnected inpu
    ImageComponents firstNonOptionalConnectedInputComps;


    std::vector<NodeMetadataPtr> inputMetadatas(nInputs);
    for (int i = 0; i < nInputs; ++i) {
        const EffectInstancePtr& input = getInput(i);
        if (input) {
            TreeRenderNodeArgsPtr inputArgs;
            if (render) {
                inputArgs = render->getInputRenderArgs(i);
            }
            GetTimeInvariantMetaDatasResultsPtr results;
            ActionRetCodeEnum stat = input->getTimeInvariantMetaDatas_public(inputArgs, &results);
            if (!isFailureRetCode(stat)) {
                inputMetadatas[i] = results->getMetadatasResults();

                if ( !firstNonOptionalConnectedInputComps && !isInputOptional(i) ) {
                    firstNonOptionalConnectedInputComps = inputMetadatas[i]->getImageComponents(-1);
                }
            }


        }
    }

    for (int i = 0; i < nInputs; ++i) {

        const NodeMetadataPtr& input = inputMetadatas[i];
        if (input) {
            frameRate = std::max( frameRate, input->getOutputFrameRate() );
        }


        if (input) {
            if (!inputParSet) {
                inputPar = input->getPixelAspectRatio(-1);
                inputParSet = true;
            }

            if (!hasOneInputContinuous) {
                hasOneInputContinuous |= input->getIsContinuous();
            }
            if (!hasOneInputFrameVarying) {
                hasOneInputFrameVarying |= input->getIsFrameVarying();
            }
        }

        ImageComponents rawComp = getUnmappedComponentsForInput(shared_from_this(), i, inputMetadatas, firstNonOptionalConnectedInputComps);
        ImageBitDepthEnum rawDepth = input ? input->getBitDepth(-1) : eImageBitDepthFloat;
        ImagePremultiplicationEnum rawPreMult = input ? input->getOutputPremult() : eImagePremultiplicationPremultiplied;

        if ( rawComp.isColorPlane() ) {
            // Note: first chromatic input gives the default output premult too, even if not connected
            // (else the output of generators may be opaque even if the host default is premultiplied)
            if ( ( rawComp == ImageComponents::getRGBAComponents() ) && (input || !premultSet) ) {
                if (rawPreMult == eImagePremultiplicationPremultiplied) {
                    premult = eImagePremultiplicationPremultiplied;
                    premultSet = true;
                } else if ( (rawPreMult == eImagePremultiplicationUnPremultiplied) && ( !premultSet || (premult != eImagePremultiplicationPremultiplied) ) ) {
                    premult = eImagePremultiplicationUnPremultiplied;
                    premultSet = true;
                }
            }

            if (input) {
                //Update deepest bitdepth and most components only if the infos are relevant, i.e: only if the clip is connected
                hasSetCompsAndDepth = true;
                if ( getSizeOfForBitDepth(deepestBitDepth) < getSizeOfForBitDepth(rawDepth) ) {
                    deepestBitDepth = rawDepth;
                }

                if ( rawComp.getNumComponents() > mostComponents.getNumComponents() ) {
                    mostComponents = rawComp;
                }
            }
        }
    } // for each input


    if (!hasSetCompsAndDepth) {
        mostComponents = ImageComponents::getRGBAComponents();
        deepestBitDepth = eImageBitDepthFloat;
    }

    // set some stuff up
    metadata.setOutputFrameRate(frameRate);
    metadata.setOutputFielding(eImageFieldingOrderNone);

    bool hasAnimation = getHasAnimation();

    // An effect is frame varying if one of its inputs is varying or it has animation
    metadata.setIsFrameVarying(hasOneInputFrameVarying || hasAnimation);

    // An effect is continuous if at least one of its inputs is continuous or if one of its knobs
    // is animated
    metadata.setIsContinuous(hasOneInputContinuous || hasAnimation);

    // now find the best depth that the plugin supports
    deepestBitDepth = node->getClosestSupportedBitDepth(deepestBitDepth);

    bool multipleClipsPAR = supportsMultipleClipPARs();


    Format projectFormat;
    getApp()->getProject()->getProjectDefaultFormat(&projectFormat);
    double projectPAR = projectFormat.getPixelAspectRatio();

    RectI firstOptionalInputFormat, firstNonOptionalInputFormat;

    // Format: Take format from the first non optional input if any. Otherwise from the first optional input.
    // Otherwise fallback on project format
    bool firstOptionalInputFormatSet = false, firstNonOptionalInputFormatSet = false;

    // now add the input gubbins to the per inputs metadatas
    for (int i = -1; i < nInputs; ++i) {

        double par;
        if (!multipleClipsPAR) {
            par = inputParSet ? inputPar : projectPAR;
        } else {
            if (inputParSet) {
                par = inputPar;
            } else {
                if (i >= 0 && inputMetadatas[i]) {
                    par = inputMetadatas[i]->getPixelAspectRatio(-1);
                } else {
                    par = projectPAR;
                }
            }
        }
        metadata.setPixelAspectRatio(i, par);

        bool isOptional = i >= 0 && isInputOptional(i);
        if (i >= 0) {
            if (isOptional) {
                if (!firstOptionalInputFormatSet && inputMetadatas[i]) {
                    firstOptionalInputFormat = inputMetadatas[i]->getOutputFormat();
                    firstOptionalInputFormatSet = true;
                }
            } else {
                if (!firstNonOptionalInputFormatSet && inputMetadatas[i]) {
                    firstNonOptionalInputFormat = inputMetadatas[i]->getOutputFormat();
                    firstNonOptionalInputFormatSet = true;
                }
            }
        }

        if ( (i == -1) || isOptional ) {
            // "Optional input clips can always have their component types remapped"
            // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#id482755
            ImageBitDepthEnum depth = deepestBitDepth;
            ImageComponents remappedComps;
            if ( !mostComponents.isColorPlane() ) {
                // hmm custom component type, don't touch it and pass it through
                metadata.setImageComponents(i, mostComponents);
            } else {
                remappedComps = mostComponents;
                remappedComps = findClosestSupportedComponents(i, remappedComps);
                metadata.setImageComponents(i, remappedComps);
                if ( (i == -1) && !premultSet &&
                    ( ( remappedComps == ImageComponents::getRGBAComponents() ) || ( remappedComps == ImageComponents::getAlphaComponents() ) ) ) {
                    premult = eImagePremultiplicationPremultiplied;
                    premultSet = true;
                }
            }

            metadata.setBitDepth(i, depth);
        } else {

            ImageComponents rawComps = getUnmappedComponentsForInput(shared_from_this(), i, inputMetadatas, firstNonOptionalConnectedInputComps);
            ImageBitDepthEnum rawDepth;
            if (i >= 0 && inputMetadatas[i]) {
                rawDepth = inputMetadatas[i]->getBitDepth(-1);
            } else {
                rawDepth = eImageBitDepthFloat;
            }

            if ( rawComps.isColorPlane() ) {
                ImageBitDepthEnum depth = multiBitDepth ? node->getClosestSupportedBitDepth(rawDepth) : deepestBitDepth;
                metadata.setBitDepth(i, depth);
            } else {
                metadata.setBitDepth(i, rawDepth);
            }
            metadata.setImageComponents(i, rawComps);
        }
    }

    // default to a reasonable value if there is no input
    if (!premultSet || !inputParSet) {
        premult = eImagePremultiplicationOpaque;
    }
    // set output premultiplication
    metadata.setOutputPremult(premult);

    RectI outputFormat;

    if (firstNonOptionalInputFormatSet) {
        outputFormat = firstNonOptionalInputFormat;
    } else if (firstOptionalInputFormatSet) {
        outputFormat = firstOptionalInputFormat;
    } else {
        outputFormat = projectFormat;
    }
    
    
    metadata.setOutputFormat(outputFormat);
    
    return eActionStatusOK;
} // getDefaultMetadata

void
EffectInstance::Implementation::refreshMetadaWarnings(const NodeMetadata &metadata)
{
    assert(QThread::currentThread() == qApp->thread());

    NodePtr node = _publicInterface->getNode();

    if (!node) {
        return;
    }

    int nInputs = node->getMaxInputCount();

    QString bitDepthWarning = tr("This nodes converts higher bit depths images from its inputs to a lower bitdepth image. As "
                                 "a result of this process, the quality of the images is degraded. The following conversions are done:\n");
    bool setBitDepthWarning = false;
    const bool supportsMultipleClipDepths = _publicInterface->supportsMultipleClipDepths();
    const bool supportsMultipleClipPARs = _publicInterface->supportsMultipleClipPARs();
    const bool supportsMultipleClipFPSs = _publicInterface->supportsMultipleClipFPSs();
    std::vector<EffectInstancePtr> inputs(nInputs);
    for (int i = 0; i < nInputs; ++i) {
        inputs[i] = _publicInterface->getInput(i);
    }


    ImageBitDepthEnum outputDepth = metadata.getBitDepth(-1);
    double outputPAR = metadata.getPixelAspectRatio(-1);
    bool outputFrameRateSet = false;
    double outputFrameRate = metadata.getOutputFrameRate();
    bool mustWarnFPS = false;
    bool mustWarnPAR = false;

    int nbConnectedInputs = 0;
    for (int i = 0; i < nInputs; ++i) {
        //Check that the bitdepths are all the same if the plug-in doesn't support multiple depths
        if ( !supportsMultipleClipDepths && (metadata.getBitDepth(i) != outputDepth) ) {
        }

        const double pixelAspect = metadata.getPixelAspectRatio(i);

        if (!supportsMultipleClipPARs) {
            if (pixelAspect != outputPAR) {
                mustWarnPAR = true;
            }
        }

        if (!inputs[i]) {
            continue;
        }

        ++nbConnectedInputs;

        const double fps = inputs[i]->getFrameRate(TreeRenderNodeArgsPtr());



        if (!supportsMultipleClipFPSs) {
            if (!outputFrameRateSet) {
                outputFrameRate = fps;
                outputFrameRateSet = true;
            } else if (std::abs(outputFrameRate - fps) > 0.01) {
                // We have several inputs with different frame rates
                mustWarnFPS = true;
            }
        }


        ImageBitDepthEnum inputOutputDepth = inputs[i]->getBitDepth(TreeRenderNodeArgsPtr(), -1);

        //If the bit-depth conversion will be lossy, warn the user
        if ( Image::isBitDepthConversionLossy( inputOutputDepth, metadata.getBitDepth(i) ) ) {
            bitDepthWarning.append( QString::fromUtf8( inputs[i]->getNode()->getLabel_mt_safe().c_str() ) );
            bitDepthWarning.append( QString::fromUtf8(" (") + QString::fromUtf8( Image::getDepthString(inputOutputDepth).c_str() ) + QChar::fromLatin1(')') );
            bitDepthWarning.append( QString::fromUtf8(" ----> ") );
            bitDepthWarning.append( QString::fromUtf8( node->getLabel_mt_safe().c_str() ) );
            bitDepthWarning.append( QString::fromUtf8(" (") + QString::fromUtf8( Image::getDepthString( metadata.getBitDepth(i) ).c_str() ) + QChar::fromLatin1(')') );
            bitDepthWarning.append( QChar::fromLatin1('\n') );
            setBitDepthWarning = true;
        }


        if ( !supportsMultipleClipPARs && (pixelAspect != outputPAR) ) {
            qDebug() << node->getScriptName_mt_safe().c_str() << ": The input " << inputs[i]->getNode()->getScriptName_mt_safe().c_str()
            << ") has a pixel aspect ratio (" << metadata.getPixelAspectRatio(i)
            << ") different than the output clip (" << outputPAR << ") but it doesn't support multiple clips PAR. "
            << "This should have been handled earlier before connecting the nodes, @see Node::canConnectInput.";
        }
    }

    std::map<Node::StreamWarningEnum, QString> warnings;
    if (setBitDepthWarning) {
        warnings[Node::eStreamWarningBitdepth] = bitDepthWarning;
    } else {
        warnings[Node::eStreamWarningBitdepth] = QString();
    }

    if (mustWarnFPS && nbConnectedInputs > 1) {
        QString fpsWarning = tr("One or multiple inputs have a frame rate different of the output. "
                                "It is not handled correctly by this node. To remove this warning make sure all inputs have "
                                "the same frame-rate, either by adjusting project settings or the upstream Read node.");
        warnings[Node::eStreamWarningFrameRate] = fpsWarning;
    } else {
        warnings[Node::eStreamWarningFrameRate] = QString();
    }

    if (mustWarnPAR && nbConnectedInputs > 1) {
        QString parWarnings = tr("One or multiple input have a pixel aspect ratio different of the output. It is not "
                                 "handled correctly by this node and may yield unwanted results. Please adjust the "
                                 "pixel aspect ratios of the inputs so that they match by using a Reformat node.");
        warnings[Node::eStreamWarningPixelAspectRatio] = parWarnings;
    } else {
        warnings[Node::eStreamWarningPixelAspectRatio] = QString();
    }
    
    
    node->setStreamWarnings(warnings);
} // refreshMetadaWarnings

void
EffectInstance::Implementation::checkMetadata(NodeMetadata &md)
{
    NodePtr node = _publicInterface->getNode();

    if (!node) {
        return;
    }

    const bool supportsMultipleClipDepths = _publicInterface->supportsMultipleClipDepths();
    const bool supportsMultipleClipPARs = _publicInterface->supportsMultipleClipPARs();

    int nInputs = node->getMaxInputCount();

    double outputPAR = md.getPixelAspectRatio(-1);

    ImageBitDepthEnum outputDepth = md.getBitDepth(-1);

    // First fix incorrect metadatas that could have been set by the plug-in
    for (int i = -1; i < nInputs; ++i) {

        ImageBitDepthEnum depth = md.getBitDepth(i);
        md.setBitDepth( i, node->getClosestSupportedBitDepth(depth));

        ImageComponents comps = md.getImageComponents(i);
        bool isAlpha = false;
        bool isRGB = false;
        if (i == -1) {
            if ( comps == ImageComponents::getRGBComponents() ) {
                isRGB = true;
            } else if ( comps == ImageComponents::getAlphaComponents() ) {
                isAlpha = true;
            }
        }
        if ( comps.isColorPlane() ) {
            comps = node->findClosestSupportedComponents(i, comps);
        }

        md.setImageComponents(i, comps);

        if (i == -1) {
            // Force opaque for RGB and unpremult for alpha
            if (isRGB) {
                md.setOutputPremult(eImagePremultiplicationOpaque);
            } else if (isAlpha) {
                md.setOutputPremult(eImagePremultiplicationUnPremultiplied);
            }
        }

        if (i >= 0) {
            //Check that the bitdepths are all the same if the plug-in doesn't support multiple depths
            if ( !supportsMultipleClipDepths && (md.getBitDepth(i) != outputDepth) ) {
                md.setBitDepth(i, outputDepth);
            }

            const double pixelAspect = md.getPixelAspectRatio(i);

            if (!supportsMultipleClipPARs) {
                if (pixelAspect != outputPAR) {
                    md.setPixelAspectRatio(i, outputPAR);
                }
            }
        }
    }

} //checkMetadata

void
EffectInstance::purgeCaches_public()
{

    purgeCaches();
}

void
EffectInstance::createInstanceAction_public()
{
    EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
    EffectActionArgsSetter_RAII actionArgsTls(tls,  TimeValue(getApp()->getTimeLine()->currentFrame()), ViewIdx(0), RenderScale(1.)
#ifdef DEBUG
                                              , /*canSetValue*/ true
                                              , /*canBeCalledRecursively*/ false
#endif
                                              );


    createInstanceAction();

    // Check if there is any overlay
    initializeOverlayInteract();
}



void
EffectInstance::drawOverlay_public(TimeValue time,
                                   const RenderScale & renderScale,
                                   ViewIdx view)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() && !getNode()->hasHostOverlay() ) {
        return;
    }


    RenderScale actualScale;
    if ( !canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }


    EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
    EffectActionArgsSetter_RAII actionArgsTls(tls,  time, view, renderScale
#ifdef DEBUG
                                              , /*canSetValue*/ false
                                              , /*canBeCalledRecursively*/ false
#endif
                                              );

    DuringInteractActionSetter_RAII _setter(getNode());
    bool drawHostOverlay = shouldDrawHostOverlay();
    drawOverlay(time, actualScale, view);
    if (drawHostOverlay) {
        getNode()->drawHostOverlay(time, actualScale, view);
    }
} // drawOverlay_public

bool
EffectInstance::onOverlayPenDown_public(TimeValue time,
                                        const RenderScale & renderScale,
                                        ViewIdx view,
                                        const QPointF & viewportPos,
                                        const QPointF & pos,
                                        double pressure,
                                        TimeValue timestamp,
                                        PenType pen)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasHostOverlay() ) {
        return false;
    }

    RenderScale actualScale;
    if ( !canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    bool ret;
    {
        EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
        EffectActionArgsSetter_RAII actionArgsTls(tls,  time, view, renderScale
#ifdef DEBUG
                                                  , /*canSetValue*/ true
                                                  , /*canBeCalledRecursively*/ true
#endif
                                                  );

        DuringInteractActionSetter_RAII _setter(getNode());
        bool drawHostOverlay = shouldDrawHostOverlay();
        if (!shouldPreferPluginOverlayOverHostOverlay()) {
            ret = drawHostOverlay ? getNode()->onOverlayPenDownDefault(time, actualScale, view, viewportPos, pos, pressure) : false;
            if (!ret) {
                ret |= onOverlayPenDown(time, actualScale, view, viewportPos, pos, pressure, timestamp, pen);
            }
        } else {
            ret = onOverlayPenDown(time, actualScale, view, viewportPos, pos, pressure, timestamp, pen);
            if (!ret && drawHostOverlay) {
                ret |= getNode()->onOverlayPenDownDefault(time, actualScale, view, viewportPos, pos, pressure);
            }
        }

    }

    return ret;
} // onOverlayPenDown_public

bool
EffectInstance::onOverlayPenDoubleClicked_public(TimeValue time,
                                                 const RenderScale & renderScale,
                                                 ViewIdx view,
                                                 const QPointF & viewportPos,
                                                 const QPointF & pos)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasHostOverlay() ) {
        return false;
    }

    RenderScale actualScale;
    if ( !canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    bool ret;
    {
        EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
        EffectActionArgsSetter_RAII actionArgsTls(tls,  time, view, renderScale
#ifdef DEBUG
                                                  , /*canSetValue*/ true
                                                  , /*canBeCalledRecursively*/ true
#endif
                                                  );

        DuringInteractActionSetter_RAII _setter(getNode());
        bool drawHostOverlay = shouldDrawHostOverlay();
        if (!shouldPreferPluginOverlayOverHostOverlay()) {
            ret = drawHostOverlay ? getNode()->onOverlayPenDoubleClickedDefault(time, actualScale, view, viewportPos, pos) : false;
            if (!ret) {
                ret |= onOverlayPenDoubleClicked(time, actualScale, view, viewportPos, pos);
            }
        } else {
            ret = onOverlayPenDoubleClicked(time, actualScale, view, viewportPos, pos);
            if (!ret && drawHostOverlay) {
                ret |= getNode()->onOverlayPenDoubleClickedDefault(time, actualScale, view, viewportPos, pos);
            }
        }

    }

    return ret;
} // onOverlayPenDoubleClicked_public

bool
EffectInstance::onOverlayPenMotion_public(TimeValue time,
                                          const RenderScale & renderScale,
                                          ViewIdx view,
                                          const QPointF & viewportPos,
                                          const QPointF & pos,
                                          double pressure,
                                          TimeValue timestamp)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasHostOverlay() ) {
        return false;
    }

    RenderScale actualScale;
    if ( !canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
    EffectActionArgsSetter_RAII actionArgsTls(tls,  time, view, renderScale
#ifdef DEBUG
                                              , /*canSetValue*/ true
                                              , /*canBeCalledRecursively*/ true
#endif
                                              );

    DuringInteractActionSetter_RAII _setter(getNode());
    bool ret;
    bool drawHostOverlay = shouldDrawHostOverlay();
    if (!shouldPreferPluginOverlayOverHostOverlay()) {
        ret = drawHostOverlay ? getNode()->onOverlayPenMotionDefault(time, actualScale, view, viewportPos, pos, pressure) : false;
        if (!ret) {
            ret |= onOverlayPenMotion(time, actualScale, view, viewportPos, pos, pressure, timestamp);
        }
    } else {
        ret = onOverlayPenMotion(time, actualScale, view, viewportPos, pos, pressure, timestamp);
        if (!ret && drawHostOverlay) {
            ret |= getNode()->onOverlayPenMotionDefault(time, actualScale, view, viewportPos, pos, pressure);
        }
    }


    return ret;
} // onOverlayPenMotion_public

bool
EffectInstance::onOverlayPenUp_public(TimeValue time,
                                      const RenderScale & renderScale,
                                      ViewIdx view,
                                      const QPointF & viewportPos,
                                      const QPointF & pos,
                                      double pressure,
                                      TimeValue timestamp)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasHostOverlay() ) {
        return false;
    }

    RenderScale actualScale;
    if ( !canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    bool ret;
    {

        EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
        EffectActionArgsSetter_RAII actionArgsTls(tls,  time, view, renderScale
#ifdef DEBUG
                                                  , /*canSetValue*/ true
                                                  , /*canBeCalledRecursively*/ true
#endif
                                                  );
        DuringInteractActionSetter_RAII _setter(getNode());
        bool drawHostOverlay = shouldDrawHostOverlay();
        if (!shouldPreferPluginOverlayOverHostOverlay()) {
            ret = drawHostOverlay ? getNode()->onOverlayPenUpDefault(time, actualScale, view, viewportPos, pos, pressure) : false;
            if (!ret) {
                ret |= onOverlayPenUp(time, actualScale, view, viewportPos, pos, pressure, timestamp);
            }
        } else {
            ret = onOverlayPenUp(time, actualScale, view, viewportPos, pos, pressure, timestamp);
            if (!ret && drawHostOverlay) {
                ret |= getNode()->onOverlayPenUpDefault(time, actualScale, view, viewportPos, pos, pressure);
            }
        }

    }

    return ret;
} // onOverlayPenUp_public

bool
EffectInstance::onOverlayKeyDown_public(TimeValue time,
                                        const RenderScale & renderScale,
                                        ViewIdx view,
                                        Key key,
                                        KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasHostOverlay() ) {
        return false;
    }

    RenderScale actualScale;
    if ( !canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }


    bool ret;
    {
        EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
        EffectActionArgsSetter_RAII actionArgsTls(tls,  time, view, renderScale
#ifdef DEBUG
                                                  , /*canSetValue*/ true
                                                  , /*canBeCalledRecursively*/ true
#endif
                                                  );
        DuringInteractActionSetter_RAII _setter(getNode());
        ret = onOverlayKeyDown(time, actualScale, view, key, modifiers);
        if (!ret && shouldDrawHostOverlay()) {
            ret |= getNode()->onOverlayKeyDownDefault(time, actualScale, view, key, modifiers);
        }
    }

    return ret;
} // onOverlayKeyDown_public

bool
EffectInstance::onOverlayKeyUp_public(TimeValue time,
                                      const RenderScale & renderScale,
                                      ViewIdx view,
                                      Key key,
                                      KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasHostOverlay() ) {
        return false;
    }

    RenderScale actualScale;
    if ( !canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    bool ret;
    {
        EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
        EffectActionArgsSetter_RAII actionArgsTls(tls,  time, view, renderScale
#ifdef DEBUG
                                                  , /*canSetValue*/ true
                                                  , /*canBeCalledRecursively*/ true
#endif

                                                  );
        DuringInteractActionSetter_RAII _setter(getNode());
        ret = onOverlayKeyUp(time, actualScale, view, key, modifiers);
        if (!ret && shouldDrawHostOverlay()) {
            ret |= getNode()->onOverlayKeyUpDefault(time, actualScale, view, key, modifiers);
        }
    }

    return ret;
} // onOverlayKeyUp_public

bool
EffectInstance::onOverlayKeyRepeat_public(TimeValue time,
                                          const RenderScale & renderScale,
                                          ViewIdx view,
                                          Key key,
                                          KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasHostOverlay() ) {
        return false;
    }

    RenderScale actualScale;
    if ( !canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    bool ret;
    {
        EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
        EffectActionArgsSetter_RAII actionArgsTls(tls,  time, view, renderScale
#ifdef DEBUG
                                                  , /*canSetValue*/ true
                                                  , /*canBeCalledRecursively*/ true
#endif

                                                  );
        DuringInteractActionSetter_RAII _setter(getNode());
        ret = onOverlayKeyRepeat(time, actualScale, view, key, modifiers);
        if (!ret && shouldDrawHostOverlay()) {
            ret |= getNode()->onOverlayKeyRepeatDefault(time, actualScale, view, key, modifiers);
        }
    }

    return ret;
} // onOverlayKeyRepeat_public

bool
EffectInstance::onOverlayFocusGained_public(TimeValue time,
                                            const RenderScale & renderScale,
                                            ViewIdx view)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() && !getNode()->hasHostOverlay() ) {
        return false;
    }

    RenderScale actualScale;
    if ( !canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    bool ret;
    {

        EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
        EffectActionArgsSetter_RAII actionArgsTls(tls,  time, view, renderScale
#ifdef DEBUG
                                                  , /*canSetValue*/ true
                                                  , /*canBeCalledRecursively*/ true
#endif

                                                  );
        DuringInteractActionSetter_RAII _setter(getNode());
        ret = onOverlayFocusGained(time, actualScale, view);
        if (shouldDrawHostOverlay()) {
            ret |= getNode()->onOverlayFocusGainedDefault(time, actualScale, view);
        }

    }

    return ret;
} // onOverlayFocusGained_public

bool
EffectInstance::onOverlayFocusLost_public(TimeValue time,
                                          const RenderScale & renderScale,
                                          ViewIdx view)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() && !getNode()->hasHostOverlay() ) {
        return false;
    }

    RenderScale actualScale;
    if ( !canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }


    bool ret;
    {

        EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
        EffectActionArgsSetter_RAII actionArgsTls(tls,  time, view, renderScale
#ifdef DEBUG
                                                  , /*canSetValue*/ true
                                                  , /*canBeCalledRecursively*/ true
#endif

                                                  );
        DuringInteractActionSetter_RAII _setter(getNode());
        ret = onOverlayFocusLost(time, actualScale, view);
        if (shouldDrawHostOverlay()) {
            ret |= getNode()->onOverlayFocusLostDefault(time, actualScale, view);
        }
    }
    
    return ret;
} // onOverlayFocusLost_public

void
EffectInstance::drawOverlay(TimeValue /*time*/,
                         const RenderScale & /*renderScale*/,
                         ViewIdx /*view*/)
{
}

bool
EffectInstance::onOverlayPenDown(TimeValue /*time*/,
                              const RenderScale & /*renderScale*/,
                              ViewIdx /*view*/,
                              const QPointF & /*viewportPos*/,
                              const QPointF & /*pos*/,
                              double /*pressure*/,
                              TimeValue /*timestamp*/,
                              PenType /*pen*/) 
{
    return false;
}

bool
EffectInstance::onOverlayPenDoubleClicked(TimeValue /*time*/,
                                       const RenderScale & /*renderScale*/,
                                       ViewIdx /*view*/,
                                       const QPointF & /*viewportPos*/,
                                       const QPointF & /*pos*/)
{
    return false;
}

bool
EffectInstance::onOverlayPenMotion(TimeValue /*time*/,
                                const RenderScale & /*renderScale*/,
                                ViewIdx /*view*/,
                                const QPointF & /*viewportPos*/,
                                const QPointF & /*pos*/,
                                double /*pressure*/,
                                TimeValue /*timestamp*/)
{
    return false;
}

bool
EffectInstance::onOverlayPenUp(TimeValue /*time*/,
                            const RenderScale & /*renderScale*/,
                            ViewIdx /*view*/,
                            const QPointF & /*viewportPos*/,
                            const QPointF & /*pos*/,
                            double /*pressure*/,
                            TimeValue /*timestamp*/)
{
    return false;
}

bool
EffectInstance::onOverlayKeyDown(TimeValue /*time*/,
                              const RenderScale & /*renderScale*/,
                              ViewIdx /*view*/,
                              Key /*key*/,
                              KeyboardModifiers /*modifiers*/)
{
    return false;
}

bool
EffectInstance::onOverlayKeyUp(TimeValue /*time*/,
                            const RenderScale & /*renderScale*/,
                            ViewIdx /*view*/,
                            Key /*key*/,
                            KeyboardModifiers /*modifiers*/)
{
    return false;
}

bool
EffectInstance::onOverlayKeyRepeat(TimeValue /*time*/,
                                const RenderScale & /*renderScale*/,
                                ViewIdx /*view*/,
                                Key /*key*/,
                                KeyboardModifiers /*modifiers*/)
{
    return false;
}

bool
EffectInstance::onOverlayFocusGained(TimeValue /*time*/,
                                  const RenderScale & /*renderScale*/,
                                  ViewIdx /*view*/)
{
    return false;
}

bool
EffectInstance::onOverlayFocusLost(TimeValue /*time*/,
                                const RenderScale & /*renderScale*/,
                                ViewIdx /*view*/)
{
    return false;
}

NATRON_NAMESPACE_EXIT;