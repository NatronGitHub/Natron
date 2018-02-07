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
#include <QtCore/QThread>
#include <QCoreApplication>

#include "Engine/AppInstance.h"
#include "Engine/Cache.h"
#include "Engine/EffectInstanceActionResults.h"
#include "Engine/EffectOpenGLContextData.h"
#include "Engine/Format.h"
#include "Engine/OverlayInteractBase.h"
#include "Engine/KnobTypes.h"
#include "Engine/Hash64.h"
#include "Engine/Node.h"
#include "Engine/NodeMetadata.h"
#include "Engine/Project.h"
#include "Engine/ThreadPool.h"


NATRON_NAMESPACE_ENTER

/**
 * @brief Add the layers from the inputList to the toList if they do not already exist in the list.
 * For the color plane, if it already existed in toList it is replaced by the value in inputList
 **/
void
EffectInstance::mergeLayersList(const std::list<ImagePlaneDesc>& inputList,
                            std::list<ImagePlaneDesc>* toList)
{
    for (std::list<ImagePlaneDesc>::const_iterator it = inputList.begin(); it != inputList.end(); ++it) {

        std::list<ImagePlaneDesc>::iterator foundMatch = ImagePlaneDesc::findEquivalentLayer(*it, toList->begin(), toList->end());

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
static void removeFromLayersList(const std::list<ImagePlaneDesc>& toRemove,
                                 std::list<ImagePlaneDesc>* toList)
{
    for (std::list<ImagePlaneDesc>::const_iterator it = toRemove.begin(); it != toRemove.end(); ++it) {
        std::list<ImagePlaneDesc>::iterator foundMatch = ImagePlaneDesc::findEquivalentLayer<std::list<ImagePlaneDesc>::iterator>(*it, toList->begin(), toList->end());
        if (foundMatch != toList->end()) {
            toList->erase(foundMatch);
        }
    } // for each input components

} // removeFromLayersList

ActionRetCodeEnum
EffectInstance::getLayersProducedAndNeeded(TimeValue time,
                                               ViewIdx view,
                                               std::map<int, std::list<ImagePlaneDesc> >* inputLayersNeeded,
                                               std::list<ImagePlaneDesc>* layersProduced,
                                               TimeValue* passThroughTime,
                                               ViewIdx* passThroughView,
                                               int* passThroughInputNb)
{
    bool processAllRequested;
    std::bitset<4> processChannels;
    std::list<ImagePlaneDesc> passThroughPlanes;
    getLayersProducedAndNeeded_default(time, view, inputLayersNeeded, layersProduced, &passThroughPlanes, passThroughTime, passThroughView, passThroughInputNb, &processAllRequested, &processChannels);
    return eActionStatusReplyDefault;
} // getComponentsNeededAndProduced

ActionRetCodeEnum
EffectInstance::getDefaultLayersNeededInInput(int i,
                                                TimeValue time,
                                                ViewIdx view,
                                                std::map<int, std::list<ImagePlaneDesc> >* inputLayersNeeded)
{

    std::list<ImagePlaneDesc> upstreamAvailableLayers;
    ActionRetCodeEnum stat = getAvailableLayers(time, view, i, &upstreamAvailableLayers);
    (void)stat;



    std::list<ImagePlaneDesc> &componentsSet = (*inputLayersNeeded)[i];

    // Get the selected layer from the source channels menu
    std::bitset<4> inputProcChannels;
    ImagePlaneDesc layer;
    bool isAll;
    bool ok = getSelectedLayer(i, upstreamAvailableLayers, &inputProcChannels, &isAll, &layer);

    // When color plane or all choice then request the default metadata components
    if (isAll || layer.isColorPlane()) {
        ok = false;
    }

    // For a mask get its selected channel
    ImagePlaneDesc maskComp;
    int channelMask = getMaskChannel(i, upstreamAvailableLayers, &maskComp);


    std::vector<ImagePlaneDesc> clipPrefsAllComps;
    {
        ImagePlaneDesc metadataPlane, metadataPairedPlane;
        getMetadataComponents(i, &metadataPlane, &metadataPairedPlane);

        // Some plug-ins, such as The Foundry Furnace set the meta-data to disparity/motion vector, requiring
        // both planes to be computed at once (Forward/Backard for motion vector) (Left/Right for Disparity)
        if (metadataPlane.getNumComponents() > 0) {
            clipPrefsAllComps.push_back(metadataPlane);
        }
        if (metadataPairedPlane.getNumComponents() > 0) {
            clipPrefsAllComps.push_back(metadataPairedPlane);
        }
        if (clipPrefsAllComps.empty()) {
            // If metada are not set yet, at least append RGBA
            clipPrefsAllComps.push_back(ImagePlaneDesc::getRGBAComponents());
        }
    }

    if ( (channelMask != -1) && (maskComp.getNumComponents() > 0) ) {

        // If this is a mask, ask for the selected mask layer
        componentsSet.push_back(maskComp);

    } else if (ok && layer.getNumComponents() > 0) {
        componentsSet.push_back(layer);
    } else {
        //Use regular clip preferences
        componentsSet.insert( componentsSet.end(), clipPrefsAllComps.begin(), clipPrefsAllComps.end() );
    }
    return eActionStatusOK;
} // getDefaultLayersNeededInInput


ActionRetCodeEnum
EffectInstance::getLayersProducedAndNeeded_default(TimeValue time,
                                                   ViewIdx view,
                                                   std::map<int, std::list<ImagePlaneDesc> >* inputLayersNeeded,
                                                   std::list<ImagePlaneDesc>* layersProduced,
                                                   std::list<ImagePlaneDesc>* passThroughPlanes,
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

    {
        std::list<ImagePlaneDesc> upstreamAvailableLayers;
        ActionRetCodeEnum stat = eActionStatusOK;
        if (*passThroughInputNb != -1) {
            stat = getAvailableLayers(time, view, *passThroughInputNb, &upstreamAvailableLayers);
        }
        if (isFailureRetCode(stat)) {
            return stat;
        }
        // upstreamAvailableLayers now contain all available planes in input of this node
        // Remove from this list all layers produced from this node to get the pass-through planes list
        removeFromLayersList(*layersProduced, &upstreamAvailableLayers);

        *passThroughPlanes = upstreamAvailableLayers;

    }

    // Get the output needed components
    {

        std::vector<ImagePlaneDesc> clipPrefsAllComps;

        // The clipPrefsComps is the number of components desired by the plug-in in the
        // getTimeInvariantMetadata action (getClipPreferences for OpenFX) mapped to the
        // color-plane.
        //
        // There's a special case for a plug-in that requests a 2 component image:
        // OpenFX does not support 2-component images by default. 2 types of plug-in
        // may request such images:
        // - non multi-planar effect that supports 2 component images, added with the Natron OpenFX extensions
        // - multi-planar effect that supports The Foundry Furnace plug-in suite: the value returned is either
        // disparity components or a motion vector components.
        //
        ImagePlaneDesc metadataPlane, metadataPairedPlane;
        getMetadataComponents(-1, &metadataPlane, &metadataPairedPlane);
        // Some plug-ins, such as The Foundry Furnace set the meta-data to disparity/motion vector, requiring
        // both planes to be computed at once (Forward/Backard for motion vector) (Left/Right for Disparity)
        if (metadataPlane.getNumComponents() > 0) {
            clipPrefsAllComps.push_back(metadataPlane);
        }
        if (metadataPairedPlane.getNumComponents() > 0) {
            clipPrefsAllComps.push_back(metadataPairedPlane);
        }
        if (clipPrefsAllComps.empty()) {
            // If metada are not set yet, at least append RGBA
            clipPrefsAllComps.push_back(ImagePlaneDesc::getRGBAComponents());
        }

        // Natron adds for all non multi-planar effects a default layer selector to emulate
        // multi-plane even if the plug-in is not aware of it. When calling getImagePlane(), the
        // plug-in will receive this user-selected plane, mapped to the number of components indicated
        // by the plug-in in getTimeInvariantMetadata
        ImagePlaneDesc layer;
        bool gotUserSelectedPlane;
        {
            // In output, the available layers are those pass-through the input + project layers +
            // layers produced by this node
            std::list<ImagePlaneDesc> availableLayersInOutput = *passThroughPlanes;
            availableLayersInOutput.insert(availableLayersInOutput.end(), clipPrefsAllComps.begin(), clipPrefsAllComps.end());

            {
                std::list<ImagePlaneDesc> projectLayers = getApp()->getProject()->getProjectDefaultLayers();
                mergeLayersList(projectLayers, &availableLayersInOutput);
            }

            KnobLayersPtr userPlanesKnob = _imp->defKnobs->createPlanesKnob.lock();
            if (userPlanesKnob) {
                std::list<ImagePlaneDesc> userCreatedLayers = userPlanesKnob->decodePlanesList();
                mergeLayersList(userCreatedLayers, &availableLayersInOutput);
            }
            
            gotUserSelectedPlane = getSelectedLayer(-1, availableLayersInOutput, processChannels, processAllRequested, &layer);
        }

        // If the user did not select any components or the layer is the color-plane, fallback on
        // meta-data color plane
        if (layer.getNumComponents() == 0 || layer.isColorPlane()) {
            gotUserSelectedPlane = false;
        }


        if (gotUserSelectedPlane) {
            layersProduced->push_back(layer);
        } else {
            layersProduced->insert( layersProduced->end(), clipPrefsAllComps.begin(), clipPrefsAllComps.end() );
        }

    }

    // For each input get their needed components
    int maxInput = getNInputs();
    for (int i = 0; i < maxInput; ++i) {
        ActionRetCodeEnum stat = getDefaultLayersNeededInInput(i, time, view, inputLayersNeeded);
        if (isFailureRetCode(stat)) {
            return stat;
        }
    } // for each input
    return eActionStatusOK;
} // getLayersProducedAndNeeded_default

ActionRetCodeEnum
EffectInstance::getComponentsNeededInternal(TimeValue time,
                                            ViewIdx view,
                                            std::map<int, std::list<ImagePlaneDesc> >* inputLayersNeeded,
                                            std::list<ImagePlaneDesc>* layersProduced,
                                            std::list<ImagePlaneDesc>* passThroughPlanes,
                                            TimeValue* passThroughTime,
                                            ViewIdx* passThroughView,
                                            int* passThroughInputNb,
                                            bool* processAllRequested,
                                            std::bitset<4>* processChannels)
{

    if ( !isMultiPlanar() ) {
        return getLayersProducedAndNeeded_default(time, view, inputLayersNeeded, layersProduced, passThroughPlanes, passThroughTime, passThroughView, passThroughInputNb, processAllRequested, processChannels);
    }
    
    
    // call the getClipComponents action
    

    ActionRetCodeEnum stat = getLayersProducedAndNeeded(time, view, inputLayersNeeded, layersProduced, passThroughTime, passThroughView, passThroughInputNb);
    if (isFailureRetCode(stat)) {
        return stat;
    } else if (stat == eActionStatusReplyDefault) {
        return getLayersProducedAndNeeded_default(time, view, inputLayersNeeded, layersProduced, passThroughPlanes, passThroughTime, passThroughView, passThroughInputNb, processAllRequested, processChannels);
    } else {
        int maxInputs = getNInputs();
        for (int i = 0; i < maxInputs; ++i) {
            if (getNode()->isInputHostDescribed(i)) {
                ActionRetCodeEnum stat = getDefaultLayersNeededInInput(i, time, view, inputLayersNeeded);
                (void)stat;
            }
        }
    }

    // Ensure the plug-in made the metadata plane available at least
    if (layersProduced->empty()) {
        std::list<ImagePlaneDesc> metadataPlanes;
        ImagePlaneDesc metadataPlane, metadataPairedPlane;
        getMetadataComponents(-1, &metadataPlane, &metadataPairedPlane);
        if (metadataPairedPlane.getNumComponents() > 0) {
            metadataPlanes.push_back(metadataPairedPlane);
        }
        if (metadataPlane.getNumComponents() > 0) {
            metadataPlanes.push_back(metadataPlane);
        }
        mergeLayersList(metadataPlanes, layersProduced);
    }


    // If the plug-in does not block upstream planes, recurse up-stream on the pass-through input to get available components.
    std::list<ImagePlaneDesc> upstreamAvailableLayers;
    PlanePassThroughEnum passThrough = getPlanePassThrough();
    if ( (passThrough == ePassThroughPassThroughNonRenderedPlanes) || (passThrough == ePassThroughRenderAllRequestedPlanes) ) {

        if ((*passThroughInputNb != -1)) {

            ActionRetCodeEnum stat = getAvailableLayers(time, view, *passThroughInputNb, &upstreamAvailableLayers);
            if (!isFailureRetCode(stat)) {

                // upstreamAvailableLayers now contain all available planes in input of this node
                // Remove from this list all layers produced from this node to get the pass-through planes list
                removeFromLayersList(*layersProduced, &upstreamAvailableLayers);

                *passThroughPlanes = upstreamAvailableLayers;
            }
        }
    } // if pass-through for planes

    // For masks, if the plug-in defaults to the color plane, use the user selected plane instead
    {
        int maxInputs = getNInputs();
        for (int i = 0; i < maxInputs; ++i) {
            if (!getNode()->isInputMask(i)) {
                continue;
            }
            std::list<ImagePlaneDesc>& maskPlanesNeeded = (*inputLayersNeeded)[i];
            if (!maskPlanesNeeded.empty()) {
                if (!maskPlanesNeeded.front().isColorPlane()) {
                    continue;
                }
            }
            ImagePlaneDesc maskComp;
            int channelMask = getMaskChannel(i, upstreamAvailableLayers, &maskComp);
            if (channelMask >= 0 && maskComp.getNumComponents() > 0) {
                maskPlanesNeeded.clear();
                maskPlanesNeeded.push_back(maskComp);
            }
        }
    }


    for (int i = 0; i < 4; ++i) {
        (*processChannels)[i] = getProcessChannel(i);
    }

    KnobBoolPtr processAllKnob = getProcessAllLayersKnob();
    if (processAllKnob) {
        *processAllRequested = processAllKnob->getValue();
    } else {
        *processAllRequested = false;
    }

    return eActionStatusOK;

} // getComponentsNeededInternal


ActionRetCodeEnum
EffectInstance::getAvailableLayers(TimeValue time, ViewIdx view, int inputNb,  std::list<ImagePlaneDesc>* availableLayers)
{

    EffectInstancePtr effect;
    if (inputNb >= 0) {
        effect = getInputRenderEffect(inputNb, time, view);
    } else {
        effect = shared_from_this();
    }
    if (!effect) {
        // If input is diconnected, at least return RGBA by default
        availableLayers->push_back(ImagePlaneDesc::getRGBAComponents());
        return eActionStatusInputDisconnected;
    }

    std::list<ImagePlaneDesc> passThroughLayers;
    {
        GetComponentsResultsPtr actionResults;
        ActionRetCodeEnum stat = effect->getLayersProducedAndNeeded_public(time, view, &actionResults);
        if (!isFailureRetCode(stat)) {

            std::map<int, std::list<ImagePlaneDesc> > inputLayersNeeded;
            std::list<ImagePlaneDesc> layersProduced;
            TimeValue passThroughTime;
            ViewIdx passThroughView;
            int passThroughInputNb;
            std::bitset<4> processChannels;
            bool processAll;
            actionResults->getResults(&inputLayersNeeded, &layersProduced, &passThroughLayers, &passThroughInputNb, &passThroughTime, &passThroughView, &processChannels, &processAll);

            // Merge pass-through planes produced + pass-through available planes and make it as the pass-through planes for this node
            // if they are not produced by this node
            mergeLayersList(layersProduced, &passThroughLayers);
        }
    }

    // Ensure the color layer is always the first one available in the list
    for (std::list<ImagePlaneDesc>::iterator it = passThroughLayers.begin(); it != passThroughLayers.end(); ++it) {
        if (it->isColorPlane()) {
            availableLayers->push_front(*it);
            passThroughLayers.erase(it);
            break;
        }
    }

    // In output, also make available the default project layers and the user created components
    if (inputNb == -1) {

        std::list<ImagePlaneDesc> projectLayers = getApp()->getProject()->getProjectDefaultLayers();
        mergeLayersList(projectLayers, availableLayers);
    }

    mergeLayersList(passThroughLayers, availableLayers);

    {
        KnobLayersPtr userPlanesKnob = effect->getUserPlanesKnob();
        if (userPlanesKnob) {
            std::list<ImagePlaneDesc> userCreatedLayers = userPlanesKnob->decodePlanesList();
            mergeLayersList(userCreatedLayers, availableLayers);
        }

    }
    return eActionStatusOK;
} // getAvailableLayers

ActionRetCodeEnum
EffectInstance::getLayersProducedAndNeeded_public(TimeValue inArgsTime, ViewIdx view,  GetComponentsResultsPtr* results)

{
    // Round time for non continuous effects
    TimeValue time = inArgsTime;
    {
        int roundedTime = std::floor(time + 0.5);
        if (roundedTime != time && !canRenderContinuously()) {
            time = TimeValue(roundedTime);
        }
    }

    U64 hash = 0;
    // Get a hash to cache the results
    {
        ComputeHashArgs hashArgs;
        hashArgs.time = time;
        hashArgs.view = view;
        hashArgs.hashType = HashableObject::eComputeHashTypeTimeViewVariant;
        hash = computeHash(hashArgs);
    }


    assert(hash != 0);


    GetComponentsKeyPtr cacheKey;
    cacheKey = boost::make_shared<GetComponentsKey>(hash,  getNode()->getPluginID());


    *results = GetComponentsResults::create(cacheKey);

    // Ensure the cache fetcher lives as long as we compute the action
    CacheEntryLockerBasePtr cacheAccess = (*results)->getFromCache();

    CacheEntryLockerBase::CacheEntryStatusEnum cacheStatus = cacheAccess->getStatus();
    while (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusComputationPending) {
        cacheStatus = cacheAccess->waitForPendingEntry();
    }

    if (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusCached) {
        if (!cacheAccess->isPersistent()) {
            *results = toGetComponentsResults(cacheAccess->getProcessLocalEntry());
        }
        return eActionStatusOK;
    }
    assert(cacheStatus == CacheEntryLockerBase::eCacheEntryStatusMustCompute);


    // For each input index what layers are required
    std::map<int, std::list<ImagePlaneDesc> > inputLayersNeeded;

    // The layers that are produced by this effect
    std::list<ImagePlaneDesc> outputLayersProduced;

    // The layers that this effect can fetch from the pass-through input but does not produce itself
    std::list<ImagePlaneDesc> passThroughPlanes;

    int passThroughInputNb = -1;
    TimeValue passThroughTime(0);
    ViewIdx passThroughView(0);
    std::bitset<4> processChannels;
    bool processAllRequested = false;
    {
        ActionRetCodeEnum stat = getComponentsNeededInternal(time, view, &inputLayersNeeded, &outputLayersProduced, &passThroughPlanes, &passThroughTime, &passThroughView, &passThroughInputNb, &processAllRequested, &processChannels);
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }


    (*results)->setResults(inputLayersNeeded, outputLayersProduced, passThroughPlanes, passThroughInputNb, passThroughTime, passThroughView, processChannels, processAllRequested);

    cacheAccess->insertInCache();

    return eActionStatusOK;
    
} // getLayersProducedAndNeeded_public


ActionRetCodeEnum
EffectInstance::attachOpenGLContext_public(TimeValue time, ViewIdx view, const RenderScale& scale,
                                           const OSGLContextPtr& glContext,
                                           EffectOpenGLContextDataPtr* data)
{

    // Does this effect support concurrent OpenGL renders ?
    // If so, we just have to lock the attachedContexts map.
    // If not, we have to take the lock now and release it in dettachOpenGLContext_public
    bool concurrentGLRender = supportsConcurrentOpenGLRenders();
    boost::scoped_ptr<QMutexLocker> locker;
    if (concurrentGLRender) {
        locker.reset( new QMutexLocker(&_imp->common->attachedContextsMutex) );
    } else {
        _imp->common->attachedContextsMutex.lock();
    }

    std::map<OSGLContextWPtr, EffectOpenGLContextDataPtr>::iterator found = _imp->common->attachedContexts.find(glContext);
    if ( found != _imp->common->attachedContexts.end() ) {
        // The context is already attached
        *data = found->second;

        return eActionStatusOK;
    }

    const bool renderScaleSupported = supportsRenderScale();
    const RenderScale mappedScale = renderScaleSupported ? scale : RenderScale(1.);


    ActionRetCodeEnum stat = attachOpenGLContext(time, view, mappedScale, glContext, data);

    if (!isFailureRetCode(stat)) {
        if (!concurrentGLRender) {
            (*data)->setHasTakenLock(true);
        }
        _imp->common->attachedContexts.insert( std::make_pair(glContext, *data) );
    } else {
        if (!concurrentGLRender) {
            _imp->common->attachedContextsMutex.unlock();
        }
    }

    // Take the lock until dettach is called for plug-ins that do not support concurrent GL renders
    return stat;

} // attachOpenGLContext_public

void
EffectInstance::dettachAllOpenGLContexts()
{



    QMutexLocker locker(&_imp->common->attachedContextsMutex);

    for (std::map<OSGLContextWPtr, EffectOpenGLContextDataPtr>::iterator it = _imp->common->attachedContexts.begin(); it != _imp->common->attachedContexts.end(); ++it) {
        OSGLContextPtr context = it->first.lock();
        if (!context) {
            continue;
        }
        OSGLContextAttacherPtr attacher = OSGLContextAttacher::create(context);
        attacher->attach();

        if (it->second.use_count() == 1) {
            // If no render is using it, dettach the context
            dettachOpenGLContext(context, it->second);
        }
    }
    if ( !_imp->common->attachedContexts.empty() ) {
        OSGLContext::unsetCurrentContextNoRenderInternal(true, 0);
    }
    _imp->common->attachedContexts.clear();

} // dettachAllOpenGLContexts

ActionRetCodeEnum
EffectInstance::attachOpenGLContext(TimeValue /*time*/, ViewIdx /*view*/, const RenderScale& /*scale*/, const OSGLContextPtr& /*glContext*/, EffectOpenGLContextDataPtr* /*data*/)
{
    return eActionStatusReplyDefault;
}


ActionRetCodeEnum
EffectInstance::dettachOpenGLContext(const OSGLContextPtr& /*glContext*/, const EffectOpenGLContextDataPtr& /*data*/)
{
    return eActionStatusReplyDefault;
}


ActionRetCodeEnum
EffectInstance::dettachOpenGLContext_public(const OSGLContextPtr& glContext, const EffectOpenGLContextDataPtr& data)
{

    bool concurrentGLRender = supportsConcurrentOpenGLRenders();
    boost::scoped_ptr<QMutexLocker> locker;
    if (concurrentGLRender) {
        locker.reset( new QMutexLocker(&_imp->common->attachedContextsMutex) );
    }


    bool mustUnlock = data->getHasTakenLock();
    std::map<OSGLContextWPtr, EffectOpenGLContextDataPtr>::iterator found = _imp->common->attachedContexts.find(glContext);
    if ( found != _imp->common->attachedContexts.end() ) {
        _imp->common->attachedContexts.erase(found);
    }

    ActionRetCodeEnum ret = dettachOpenGLContext(glContext, data);
    if (mustUnlock) {
        _imp->common->attachedContextsMutex.unlock();
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
                                       const EffectOpenGLContextDataPtr& /*glContextData*/)
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
                                     const EffectOpenGLContextDataPtr& /*glContextData*/)
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

    REPORT_CURRENT_THREAD_ACTION( kOfxImageEffectActionRender, getNode() );

    // Each render clone should hold the time and view passed to the render action
    assert(args.time == getCurrentRenderTime());
    assert(args.view == getCurrentRenderView());
    
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
                                           const EffectOpenGLContextDataPtr& glContextData)
{

    REPORT_CURRENT_THREAD_ACTION( kOfxImageEffectActionBeginSequenceRender, getNode() );

    return beginSequenceRender(first, last, step, interactive, scale,
                               isSequentialRender, isRenderResponseToUserInteraction, draftMode, view, backendType, glContextData);
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
                                         const EffectOpenGLContextDataPtr& glContextData)
{


    REPORT_CURRENT_THREAD_ACTION( kOfxImageEffectActionEndSequenceRender, getNode() );

    return endSequenceRender(first, last, step, interactive, scale, isSequentialRender, isRenderResponseToUserInteraction, draftMode, view, backendType, glContextData);

} // endSequenceRender_public

ActionRetCodeEnum
EffectInstance::getInverseDistortion(TimeValue /*time*/,
                              const RenderScale & /*renderScale*/,
                              bool /*draftRender*/,
                              ViewIdx /*view*/,
                              DistortionFunction2D* /*distortion*/)
{
    return eActionStatusReplyDefault;
}

ActionRetCodeEnum
EffectInstance::getInverseDistortion_public(TimeValue inArgsTime,
                                            const RenderScale & renderScale,
                                            bool draftRender,
                                            ViewIdx view,
                                            GetDistortionResultsPtr* results)
{
    assert(results);
    
    ActionRetCodeEnum stat = eActionStatusOK;
    TimeValue time = inArgsTime;
    {
        int roundedTime = std::floor(time + 0.5);
        if (roundedTime != time && !canRenderContinuously()) {
            time = TimeValue(roundedTime);
        }
    }

    const bool renderScaleSupported = supportsRenderScale();
    const RenderScale mappedScale = renderScaleSupported ? renderScale : RenderScale(1.);


    bool isDeprecatedTransformSupportEnabled = getCanTransform3x3();
    bool distortSupported = getCanDistort();


    // If the effect is identity, do not call the getDistortion action, instead just return an identity matrix
    int identityInputNb = -1;
    {
        // If the effect is identity on the format, that means its bound to be identity anywhere and does not depend on the render window.
        RectI identityWindow;
        identityWindow.x1 = INT_MIN;
        identityWindow.y1 = INT_MIN;
        identityWindow.x2 = INT_MAX;
        identityWindow.y2 = INT_MAX;
        RenderScale scale(1.);
        IsIdentityResultsPtr identityResults;
        stat = isIdentity_public(true, time, scale, identityWindow, view, 0, &identityResults);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        identityResults->getIdentityData(&identityInputNb, 0, 0, 0);
    }

    if (!distortSupported && !isDeprecatedTransformSupportEnabled && identityInputNb < 0) {
        return eActionStatusReplyDefault;
    }

    if (identityInputNb >= 0) {
        DistortionFunction2DPtr disto = boost::make_shared<DistortionFunction2D>();
        disto->transformMatrix = boost::make_shared<Transform::Matrix3x3>();
        disto->transformMatrix->setIdentity();

        // Do not cache the results
        (*results) = GetDistortionResults::create(GetDistortionKeyPtr());
        (*results)->setResults(disto);
    } else {

        U64 hash;
        // Get a hash to cache the results
        {
            ComputeHashArgs hashArgs;
            hashArgs.time = time;
            hashArgs.view = view;
            hashArgs.hashType = HashableObject::eComputeHashTypeTimeViewVariant;
            hash = computeHash(hashArgs);
        }


        GetDistortionKeyPtr cacheKey;
        {
            cacheKey = boost::make_shared<GetDistortionKey>(hash, renderScale, getNode()->getPluginID());
        }


        *results = GetDistortionResults::create(cacheKey);

        CacheEntryLockerBasePtr cacheAccess;
        {

            // Ensure the cache fetcher lives as long as we compute the action
            cacheAccess = (*results)->getFromCache();

            CacheEntryLockerBase::CacheEntryStatusEnum cacheStatus = cacheAccess->getStatus();
            while (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusComputationPending) {
                cacheStatus = cacheAccess->waitForPendingEntry();
            }

            if (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusCached) {
                if (!cacheAccess->isPersistent()) {
                    *results = toGetDistortionResults(cacheAccess->getProcessLocalEntry());
                }
                return eActionStatusOK;
            }

            assert(cacheStatus == CacheEntryLockerBase::eCacheEntryStatusMustCompute);
        }

        DistortionFunction2DPtr disto = boost::make_shared<DistortionFunction2D>();
        // Call the action
        stat = getInverseDistortion(time, mappedScale, draftRender, view, disto.get());
        if (isFailureRetCode(stat)) {
            return stat;
        }

        if (stat != eActionStatusReplyDefault) {

            // Either the matrix or the distortion functor should be set
            assert(disto->transformMatrix || disto->func);

            (*results)->setResults(disto);
        }

        cacheAccess->insertInCache();
    }
    
    return stat;
} // getDistortion_public

ActionRetCodeEnum
EffectInstance::isIdentity(TimeValue /*time*/,
                           const RenderScale & /*scale*/,
                           const RectI & /*roi*/,
                           ViewIdx /*view*/,
                           const ImagePlaneDesc& /*plane*/,
                           TimeValue* /*inputTime*/,
                           ViewIdx* /*inputView*/,
                           int* inputNb,
                           ImagePlaneDesc* /*inputPlane*/)
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
                                  const ImagePlaneDesc* plane,
                                  IsIdentityResultsPtr* results)
{
    

    {
        int roundedTime = std::floor(time + 0.5);

        // A continuous effect is identity on itself on nearest integer time
        if (roundedTime != time && !canRenderContinuously()) {
            // We do not cache it because for non continuous effects we only cache stuff at
            // valid frame times
            *results = IsIdentityResults::create(IsIdentityKeyPtr());
            (*results)->setIdentityData(-2, TimeValue(roundedTime), view, plane ? *plane : ImagePlaneDesc::getNoneComponents());
            return eActionStatusOK;
        }
    }

    const bool renderScaleSupported = supportsRenderScale();
    const RenderScale mappedScale = renderScaleSupported ? scale : RenderScale(1.);


    U64 hash = 0;
    // Get a hash to cache the results
    if (useIdentityCache) {
        ComputeHashArgs hashArgs;
        hashArgs.time = time;
        hashArgs.view = view;
        hashArgs.hashType = HashableObject::eComputeHashTypeTimeViewVariant;
        hash = computeHash(hashArgs);
    }


    IsIdentityKeyPtr cacheKey;
    {
        cacheKey = boost::make_shared<IsIdentityKey>(hash, time, plane ? *plane : ImagePlaneDesc::getNoneComponents(), getNode()->getPluginID());
    }


    *results = IsIdentityResults::create(cacheKey);

    CacheEntryLockerBasePtr cacheAccess;
    if (useIdentityCache) {

        // Ensure the cache fetcher lives as long as we compute the action
        cacheAccess = (*results)->getFromCache();

        CacheEntryLockerBase::CacheEntryStatusEnum cacheStatus = cacheAccess->getStatus();
        while (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusComputationPending) {
            cacheStatus = cacheAccess->waitForPendingEntry();
        }

        if (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusCached) {
            if (!cacheAccess->isPersistent()) {
                *results = toIsIdentityResults(cacheAccess->getProcessLocalEntry());
            }
            return eActionStatusOK;
        }
        
        assert(cacheStatus == CacheEntryLockerBase::eCacheEntryStatusMustCompute);
    }

    // Figure out the identity plane if needed.
    bool handleIdentityPlane = false;
    int identityInputNb = -1;
    TimeValue identityTime = time;
    ViewIdx identityView = view;
    ImagePlaneDesc inputPlane = plane ? *plane : ImagePlaneDesc::getNoneComponents();
    ImagePlaneDesc identityPlane;
    if ((isNodeDisabledForFrame(time, view) || !hasAtLeastOneChannelToProcess() ) || (isHostMixEnabled() && getHostMixingValue(time, view) == 0.)) {
        // Node is disabled or doesn't have any channel to process, be identity on the main input
        identityInputNb = getNode()->getPreferredInput();
        handleIdentityPlane = true;
    } else {
        // Call the isIdentity plug-in action
        RectI mappedRenderWindow = renderWindow;

        // The plug-in isn't multiplanar, handle the plane ourselves
        handleIdentityPlane = !isMultiPlanar();

        if (mappedScale.x != scale.x || mappedScale.y != scale.y) {
            // map the render window to the appropriate scale
            RectD canonicalRenderWindow;
            double par = getAspectRatio(-1);
            renderWindow.toCanonical_noClipping(scale, par, &canonicalRenderWindow);
            canonicalRenderWindow.toPixelEnclosing(mappedScale, par, &mappedRenderWindow);
        }

        ActionRetCodeEnum stat = isIdentity(time, mappedScale, mappedRenderWindow, view, inputPlane, &identityTime, &identityView, &identityInputNb, &identityPlane);
        if (isFailureRetCode(stat)) {
            return stat;
        }

        // -2 means identity on itself.
        // A sequential effect cannot be identity on itself
        if (identityInputNb == -2) {
            SequentialPreferenceEnum sequential = getSequentialRenderSupport();
            assert(identityTime != time || view != identityView || inputPlane != identityPlane);
            if (sequential == eSequentialPreferenceOnlySequential || (identityTime == time && view == identityView && inputPlane == identityPlane)) {
                identityInputNb = -1;
            }
        }

    }

    if (handleIdentityPlane && identityInputNb >= 0 && plane) {
        // Use the get layers needed action to figure out the identity plane by default
        GetComponentsResultsPtr results;
        ActionRetCodeEnum stat = getLayersProducedAndNeeded_public(time, view, &results);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        const std::map<int, std::list<ImagePlaneDesc> >& neededInputsMap = results->getNeededInputPlanes();
        std::map<int, std::list<ImagePlaneDesc> >::const_iterator foundInput = neededInputsMap.find(identityInputNb);
        if (foundInput != neededInputsMap.end() && !foundInput->second.empty()) {


            // If the effect is identity, we most likely want to pass-through to the color plane and not something else.
            // For example, a Premult node set with alpha premult channel set to come from plane RotoMask.Alpha and RGB to come from the Color plane
            // will request 2 planes in input.
            const std::list<ImagePlaneDesc>& identityPlanes = foundInput->second;
            std::list<ImagePlaneDesc>::const_iterator foundEquivalent = ImagePlaneDesc::findEquivalentLayer(inputPlane, identityPlanes.begin(), identityPlanes.end());
            if (foundEquivalent != identityPlanes.end()) {
                identityPlane = *foundEquivalent;
            } else {
                identityPlane = identityPlanes.front();
            }
        }
    }


    (*results)->setIdentityData(identityInputNb, identityTime, identityView, identityPlane);


    if (cacheAccess) {
        cacheAccess->insertInCache();
    }
    return eActionStatusOK;
} // isIdentity_public


ActionRetCodeEnum
EffectInstance::getRegionOfDefinition_public(TimeValue inArgsTime,
                                             const RenderScale & scale,
                                             ViewIdx view,
                                             GetRegionOfDefinitionResultsPtr* results)
{
    TimeValue time = inArgsTime;
    {
        int roundedTime = std::floor(time + 0.5);
        if (roundedTime != time && !canRenderContinuously()) {
            time = TimeValue(roundedTime);
        }
    }

    const bool renderScaleSupported = supportsRenderScale();
    const RenderScale mappedScale = renderScaleSupported ? scale : RenderScale(1.);



    U64 hash = 0;
    // Get a hash to cache the results
    {
        ComputeHashArgs hashArgs;
        hashArgs.time = time;
        hashArgs.view = view;
        hashArgs.hashType = HashableObject::eComputeHashTypeTimeViewVariant;
        hash = computeHash(hashArgs);
    }

    GetRegionOfDefinitionKeyPtr cacheKey;
    cacheKey = boost::make_shared<GetRegionOfDefinitionKey>(hash, mappedScale, getNode()->getPluginID());


    *results = GetRegionOfDefinitionResults::create(cacheKey);

    CacheEntryLockerBasePtr cacheAccess;
    {

        cacheAccess = (*results)->getFromCache();

        CacheEntryLockerBase::CacheEntryStatusEnum cacheStatus = cacheAccess->getStatus();
        while (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusComputationPending) {
            cacheStatus = cacheAccess->waitForPendingEntry();
        }

        if (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusCached) {
            if (!cacheAccess->isPersistent()) {
                *results = toGetRegionOfDefinitionResults(cacheAccess->getProcessLocalEntry());
            }
            return eActionStatusOK;
        }
        assert(cacheStatus == CacheEntryLockerBase::eCacheEntryStatusMustCompute);

    }



    // If the effect is identity, do not call the getRegionOfDefinition action, instead just return the input identity at the
    // identity time and view.

    {
        // If the effect is identity and infinite window, that means its bound to be identity anywhere and does not depend on the render window.
        RectI identityWindow;
        identityWindow.x1 = INT_MIN;
        identityWindow.y1 = INT_MIN;
        identityWindow.x2 = INT_MAX;
        identityWindow.y2 = INT_MAX;
        RenderScale scale(1.);
        IsIdentityResultsPtr identityResults;
        ActionRetCodeEnum stat = isIdentity_public(true, time, mappedScale, identityWindow, view, 0, &identityResults);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        assert(identityResults);

        int inputIdentityNb;
        TimeValue identityTime;
        ViewIdx identityView;
        identityResults->getIdentityData(&inputIdentityNb, &identityTime, &identityView, 0);

        if (inputIdentityNb >= 0) {
            // This effect is identity
            EffectInstancePtr identityInputNode = getInputRenderEffect(inputIdentityNb, identityTime, identityView);
            if (!identityInputNode) {
                return eActionStatusInputDisconnected;
            }


            GetRegionOfDefinitionResultsPtr inputResults;
            ActionRetCodeEnum stat = identityInputNode->getRegionOfDefinition_public(identityTime, mappedScale, identityView, &inputResults);
            if (isFailureRetCode(stat)) {
                return stat;
            }
            (*results)->setRoD(inputResults->getRoD());
        } else {
            // Not identity

            // Check if all mandatory inputs are connected, otherwise return an error
            int nInputs = getNInputs();
            for (int i = 0; i < nInputs; ++i) {
                if (getNode()->isInputOptional(i)) {
                    continue;
                }
                if (!getInputMainInstance(i)) {
                    return eActionStatusInputDisconnected;
                }
            }


            RectD rod;
            ActionRetCodeEnum stat = getRegionOfDefinition(time, mappedScale, view, &rod);

            if (isFailureRetCode(stat)) {
                return stat;
            }
            ifInfiniteApplyHeuristic(time, mappedScale, view, &rod);

            (*results)->setRoD(rod);
            
            
        } // inputIdentityNb == -1
    }

    if (cacheAccess) {
        cacheAccess->insertInCache();
    }
    
    return eActionStatusOK;
    
} // getRegionOfDefinition_public


void
EffectInstance::calcDefaultRegionOfDefinition_public(TimeValue time, const RenderScale & scale, ViewIdx view, RectD *rod)
{

    return calcDefaultRegionOfDefinition(time, scale, view, rod);
}

void
EffectInstance::calcDefaultRegionOfDefinition(TimeValue /*time*/,
                                              const RenderScale & scale,
                                              ViewIdx /*view*/,
                                              RectD *rod)
{

    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    RectI format = getOutputFormat();
    double par = getAspectRatio(-1);
    format.toCanonical_noClipping(mipMapLevel, par, rod);
} // calcDefaultRegionOfDefinition

ActionRetCodeEnum
EffectInstance::getRegionOfDefinition(TimeValue time,
                                      const RenderScale & scale,
                                      ViewIdx view,
                                      RectD* rod) //!< rod is in canonical coordinates
{
    bool firstInput = true;
    RenderScale renderMappedScale = scale;

    // By default, union the region of definition of all non mask inputs.
    int nInputs = getNInputs();
    for (int i = 0; i < nInputs; ++i) {
        if ( getNode()->isInputMask(i) ) {
            continue;
        }
        EffectInstancePtr input = getInputRenderEffect(i, time, view);
        if (input) {
            GetRegionOfDefinitionResultsPtr inputResults;

            ActionRetCodeEnum stat = input->getRegionOfDefinition_public(time, renderMappedScale, view, &inputResults);
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
        calcDefaultRegionOfDefinition_public(time, scale, view, &inputsUnion);
    }

    // If infinite : clip to inputsUnion if not null, otherwise to project default
    RectD canonicalFormat;

    if (x1Infinite || y1Infinite || x2Infinite || y2Infinite) {
        RectI format = getOutputFormat();
        assert(!format.isNull());
        double par = getAspectRatio(-1);
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
EffectInstance::getDefaultRegionOfInterestForInput(const EffectInstancePtr& input,
                                                   int inputNb,
                                                   TimeValue time,
                                                   const RenderScale & scale,
                                                   const RectD & renderWindow,
                                                   ViewIdx view,
                                                   RectD* roi)
{


    if (supportsTiles() && getNode()->isTilesSupportedByInput(inputNb)) {
        *roi = renderWindow;
    } else {
        // Tiles not supported: get the RoD as RoI
        GetRegionOfDefinitionResultsPtr inputResults;

        ActionRetCodeEnum stat = input->getRegionOfDefinition_public(time, scale, view, &inputResults);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        *roi = inputResults->getRoD();
    }

    return eActionStatusOK;
}


ActionRetCodeEnum
EffectInstance::getRegionsOfInterest_public(TimeValue inArgsTime,
                                            const RenderScale & scale,
                                            const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                            ViewIdx view,
                                            RoIMap* ret)
{
    TimeValue time = inArgsTime;
    {
        int roundedTime = std::floor(time + 0.5);
        if (roundedTime != time && !canRenderContinuously()) {
            time = TimeValue(roundedTime);
        }
    }

    const bool renderScaleSupported = supportsRenderScale();
    const RenderScale mappedScale = renderScaleSupported ? scale : RenderScale(1.);

    assert(renderWindow.x2 >= renderWindow.x1 && renderWindow.y2 >= renderWindow.y1);

    int nInputs = getNInputs();
    for (int i = 0; i < nInputs; ++i) {
        if (!getNode()->isInputHostDescribed(i)) {
            continue;
        }
        EffectInstancePtr input = getInputMainInstance(i);
        if (input) {
            RectD roi;
            ActionRetCodeEnum stat = getDefaultRegionOfInterestForInput(input, i, time, scale, renderWindow, view, &roi);
            if (isFailureRetCode(stat)) {
                return stat;
            }
            ret->insert(std::make_pair(i, roi));
        }
    }

    return getRegionsOfInterest(time, mappedScale, renderWindow, view, ret);

} // getRegionsOfInterest_public

ActionRetCodeEnum
EffectInstance::getRegionsOfInterest(TimeValue time,
                                     const RenderScale & scale,
                                     const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                     ViewIdx view,
                                     RoIMap* ret)
{
    int nInputs = getNInputs();
    for (int i = 0; i < nInputs; ++i) {
        EffectInstancePtr input = getInputRenderEffect(i, time, view);
        if (!input) {
            continue;
        }
        RectD roi;
        ActionRetCodeEnum stat = getDefaultRegionOfInterestForInput(input, i, time, scale, renderWindow, view, &roi);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        ret->insert(std::make_pair(i, roi));

    }
    return eActionStatusReplyDefault;
} // getRegionsOfInterest


ActionRetCodeEnum
EffectInstance::getFramesNeeded(TimeValue time,
                                ViewIdx view,
                                FramesNeededMap* ret)
{
    RangeD defaultRange = {time, time};

    std::vector<RangeD> ranges(1);
    ranges[0] = defaultRange;

    FrameRangesMap defViewRange;
    defViewRange.insert( std::make_pair(view, ranges) );

    int nInputs = getNInputs();
    for (int i = 0; i < nInputs; ++i) {

        EffectInstancePtr input = getInputRenderEffect(i, time, view);
        if (input) {
            ret->insert( std::make_pair(i, defViewRange) );
        }

    }
    
    return eActionStatusReplyDefault;
} // getFramesNeeded

ActionRetCodeEnum
EffectInstance::getFramesNeeded_public(TimeValue inArgsTime,
                                       ViewIdx view,
                                       GetFramesNeededResultsPtr* results)
{

    // Round time for non continuous effects
    TimeValue time = inArgsTime;
    {
        int roundedTime = std::floor(time + 0.5);
        if (roundedTime != time && !canRenderContinuously()) {
            time = TimeValue(roundedTime);
        }
    }


    // Since the frames needed results are part of the computation of the hash itself, only cache it internally if we got a hash.
    // Otherwise this is cached in the hash computation itself.
    U64 hash = 0;
    bool isHashCached;
    // Get a hash to cache the results
    {
        HashableObject::FindHashArgs findArgs;
        findArgs.hashType = HashableObject::eComputeHashTypeTimeViewVariant;
        findArgs.time = time;
        findArgs.view = view;
        isHashCached = findCachedHash(findArgs, &hash);
    }
    NodePtr thisNode = getNode();

    GetFramesNeededKeyPtr cacheKey;
    cacheKey = boost::make_shared<GetFramesNeededKey>(hash, getNode()->getPluginID());

    *results = GetFramesNeededResults::create(cacheKey);

    CacheEntryLockerBasePtr cacheAccess;

    // Only use the cache if we got a hash.
    // We cannot compute the hash here because the hash itself requires the result of this function.
    // The results of this function is cached externally instead
    if (isHashCached) {


        cacheAccess = (*results)->getFromCache();

        CacheEntryLockerBase::CacheEntryStatusEnum cacheStatus = cacheAccess->getStatus();
        while (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusComputationPending) {
            cacheStatus = cacheAccess->waitForPendingEntry();
        }

        if (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusCached) {
            if (!cacheAccess->isPersistent()) {
                *results = toGetFramesNeededResults(cacheAccess->getProcessLocalEntry());
            }
            return eActionStatusOK;
        }
        assert(cacheStatus == CacheEntryLockerBase::eCacheEntryStatusMustCompute);
    }


    // Call the action
    FramesNeededMap framesNeeded;
    {

        // Check if all mandatory inputs are connected, otherwise return an error
        int nInputs = getNInputs();
        for (int i = 0; i < nInputs; ++i) {
            if (getNode()->isInputOptional(i)) {
                continue;
            }
            if (!getInputMainInstance(i)) {
                return eActionStatusInputDisconnected;
            }
        }

        for (int i = 0; i < nInputs; ++i) {
            if (getNode()->isInputHostDescribed(i) && getInputMainInstance(i)) {
                // Add host handled inputs frame needed
                RangeD defaultRange = {time, time};
                std::vector<RangeD> ranges(1);
                ranges[0] = defaultRange;
                FrameRangesMap defViewRange;
                defViewRange.insert( std::make_pair(view, ranges) );
                framesNeeded.insert(std::make_pair(i, defViewRange));
            }
        }


        ActionRetCodeEnum stat = getFramesNeeded(time, view, &framesNeeded);
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
        RectI identityWindow;
        identityWindow.x1 = INT_MIN;
        identityWindow.y1 = INT_MIN;
        identityWindow.x2 = INT_MAX;
        identityWindow.y2 = INT_MAX;
        RenderScale scale(1.);
        ActionRetCodeEnum stat = isIdentity_public(isHashCached, time, scale, identityWindow, view, 0, &identityResults);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        identityResults->getIdentityData(&inputIdentityNb, &inputIdentityTime, &inputIdentityView, 0);
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


    if (!cacheKey) {
        cacheKey = boost::make_shared<GetFramesNeededKey>(0,  getNode()->getPluginID());

    }
    

    (*results)->setFramesNeeded(framesNeeded);

    if (cacheAccess) {
        cacheAccess->insertInCache();
    }

    return eActionStatusOK;

} // getFramesNeeded_public




ActionRetCodeEnum
EffectInstance::getFrameRange(double *first,
                              double *last)
{
    // default is infinite if there are no non optional input clips
    *first = INT_MIN;
    *last = INT_MAX;
    int nInputs = getNInputs();

    // Default to the union of all the frame ranges of the non optional input clips.

    // Edit: Uncommented the isInputOptional call which introduces a bugs with Genarts Monster plug-ins when 2 generators
    // are connected in the pipeline. They must rely on the time domain to maintain an internal state and apparantly
    // not taking optional inputs into accounts messes it up.

    for (int i = 0; i < nInputs; ++i) {
        EffectInstancePtr input = getInputRenderEffectAtAnyTimeView(i);
        if (input) {
            //if (!isInputOptional(i))
            GetFrameRangeResultsPtr inputResults;
            ActionRetCodeEnum stat = input->getFrameRange_public(&inputResults);
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
EffectInstance::getFrameRange_public(GetFrameRangeResultsPtr* results)
{

    // Get a hash to cache the results
    U64 hash = 0;

    *results = _imp->getFrameRangeResults();

    if (*results) {
        return eActionStatusOK;
    }

    {
        ComputeHashArgs hashArgs;
        hashArgs.hashType = HashableObject::eComputeHashTypeTimeViewInvariant;
        hash = computeHash(hashArgs);
    }

    GetFrameRangeKeyPtr cacheKey = boost::make_shared<GetFrameRangeKey>(hash, getNode()->getPluginID());
    *results = GetFrameRangeResults::create(cacheKey);

    CacheEntryLockerBasePtr cacheAccess = (*results)->getFromCache();

    CacheEntryLockerBase::CacheEntryStatusEnum cacheStatus = cacheAccess->getStatus();
    while (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusComputationPending) {
        cacheStatus = cacheAccess->waitForPendingEntry();
    }

    if (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusCached) {
        if (!cacheAccess->isPersistent()) {
            *results = toGetFrameRangeResults(cacheAccess->getProcessLocalEntry());
        }
        return eActionStatusOK;
    }
    assert(cacheStatus == CacheEntryLockerBase::eCacheEntryStatusMustCompute);

    // Call the action
    RangeD range;
    {


        ActionRetCodeEnum stat = getFrameRange(&range.min, &range.max);
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }

    (*results)->setFrameRangeResults(range);

    if (_imp->renderData) {
        _imp->setFrameRangeResults(*results);
    }
    cacheAccess->insertInCache();

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
    if ( (reason == eValueChangedReasonUserEdited) && toKnobButton(k) && getDisabledKnobValue() ) {
        return false;
    }

    // for image readers, image writers, and video writers, frame range must be updated before kOfxActionInstanceChanged is called on kOfxImageEffectFileParamName
    bool mustCallOnFileNameParameterChanged = false;
    if ( (reason != eValueChangedReasonTimeChanged) && ( isReader() || isWriter() ) && (k->getName() == kOfxImageEffectFileParamName) ) {
        if ( isVideoReader() ) {
            mustCallOnFileNameParameterChanged = true;
        } else {
            onFileNameParameterChanged(k);
        }
    }

    bool ret = false;

    // assert(!(view.isAll() || view.isCurrent())); // not yet implemented
    const ViewIdx viewIdx( (view.isAll()) ? 0 : view );
    bool wasFormatKnobCaught = handleFormatKnob(k);
    KnobHelperPtr kh = boost::dynamic_pointer_cast<KnobHelper>(k);
    assert(kh);
    if (kh && kh->getKnobDeclarationType() == KnobI::eKnobDeclarationTypePlugin && !wasFormatKnobCaught) {
        {


            REPORT_CURRENT_THREAD_ACTION( kOfxActionInstanceChanged, getNode() );
            // Map to a plug-in known reason
            if (reason == eValueChangedReasonUserEdited) {
                reason = eValueChangedReasonUserEdited;
            }
            ret |= knobChanged(k, reason, view, time);
        }
    }

    // for video readers, frame range must be updated after kOfxActionInstanceChanged is called on kOfxImageEffectFileParamName
    if (mustCallOnFileNameParameterChanged) {
        onFileNameParameterChanged(k);
    }

    if ( kh && ( reason != eValueChangedReasonTimeChanged) ) {

        // If we have an overlay, redraw the interact
        if (node->shouldDrawOverlay(time, ViewIdx(0), eOverlayViewportTypeViewer)) {
            // Some plugins (e.g. by digital film tools) forget to set kOfxInteractPropSlaveToParam.
            // Most hosts trigger a redraw if the plugin has an active overlay.
            OverlayInteractBasePtr knobInteract = kh->getCustomInteract();
            if (knobInteract) {
                knobInteract->redraw();
            } else {
                getApp()->redrawAllViewers();
            }
        }

        // If the param is also slave of a timeline interact, redraw timelines
        if (isOverlaySlaveParam(kh, eOverlaySlaveTimeline)) {
            getApp()->redrawAllTimelines();
        }

    }
    

    ret |= handleDefaultKnobChanged(k, reason);

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


    return ret;
} // onKnobValueChanged_public

void
EffectInstance::onMetadataChanged(const NodeMetadata& metadata)
{
    assert(QThread::currentThread() == qApp->thread());

    // Refresh warnings
    refreshMetadaWarnings(metadata);

    // Refresh identity state
    getNode()->refreshIdentityState();

    // Refresh default layer & mask channel selectors
    refreshChannelSelectors();

    // Premult might have changed, check for warnings
    checkForPremultWarningAndCheckboxes();

    // Refresh channel checkbox and layer selectors visibility
    refreshLayersSelectorsVisibility();
}

bool
EffectInstance::onMetadataChanged_nonRecursive()
{
    // Call the onMetadataChanged action
    NodePtr node = getNode();

    
    GetTimeInvariantMetadataResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetadata_public(&results);
    if (!isFailureRetCode(stat)) {
        NodeMetadataPtr metadata = results->getMetadataResults();
        assert(metadata);
        U64 currentHash = results->getHashKey();
        if (currentHash == node->getLastTimeInvariantMetadataHash()) {
            return false;
        }
        node->setLastTimeInvariantMetadataHash(currentHash);
        onMetadataChanged(*metadata);
        return true;
    }
    return false;
} // onMetadataChanged_nonRecursive

void
EffectInstance::onMetadataChanged_recursive(std::set<NodePtr>* markedNodes)
{
    // Can only be called on the main thread
    assert(QThread::currentThread() == qApp->thread());

    if (getApp()->isCreatingNode() || getApp()->getProject()->isLoadingProject()) {
        // Never do a recursive downstream pass to refresh metadata when creating a node or loading a project.
        // Let the NodeCollection::createNodesFromSerialization function do it once for each nodes.
        return;
    }

    // Check if we already recursed on this node
    NodePtr node = getNode();
    if (markedNodes->find(node) != markedNodes->end()) {
        return;
    }


    // mark this node
    markedNodes->insert(node);

    if (!dynamic_cast<NodeGroup*>(this)) {
        // For a group, we don't really have any metadata going on, its not really part of the graph
        if (!onMetadataChanged_nonRecursive()) {
            return;
        }
    }

    // Recurse downstream
    NodesList outputs;
    node->getOutputsWithGroupRedirection(outputs);
    for (NodesList::const_iterator it = outputs.begin(); it!=outputs.end(); ++it) {
        (*it)->getEffectInstance()->onMetadataChanged_recursive(markedNodes);
    }
} // onMetadataChanged_recursive

void
EffectInstance::onMetadataChanged_recursive_public()
{
    std::set<NodePtr> markedNodes;
    onMetadataChanged_recursive(&markedNodes);

}

void
EffectInstance::onMetadataChanged_nonRecursive_public()
{
    onMetadataChanged_nonRecursive();
}

void
EffectInstance::onInputChanged(int /*inputNo*/)
{
}

void
EffectInstance::onInputChanged_public(int inputNo)
{

    REPORT_CURRENT_THREAD_ACTION( kOfxActionInstanceChanged, getNode() );
    {
        std::map<int, MaskSelector >::iterator it = _imp->defKnobs->maskSelectors.find(inputNo);
        if (it != _imp->defKnobs->maskSelectors.end()) {
            _imp->onMaskSelectorChanged(it->first, it->second);
        }
    }

    // Don't call the input changed action if the input is unknown by the plug-in
    if (getNode()->isInputHostDescribed(inputNo)) {
        return;
    }

    onInputChanged(inputNo);
} // onInputChanged_public

ActionRetCodeEnum
EffectInstance::getTimeInvariantMetadata_public(GetTimeInvariantMetadataResultsPtr* results)
{
    // Get a hash to cache the results
    U64 hash = 0;

    *results = _imp->getTimeInvariantMetadataResults();
    if (*results) {
        return eActionStatusOK;
    }

    {
        ComputeHashArgs hashArgs;
        hashArgs.hashType = HashableObject::eComputeHashTypeOnlyMetadataSlaves;
        hash = computeHash(hashArgs);
    }

    GetTimeInvariantMetadataKeyPtr cacheKey = boost::make_shared<GetTimeInvariantMetadataKey>(hash, getNode()->getPluginID());
    *results = GetTimeInvariantMetadataResults::create(cacheKey);
    NodeMetadataPtr metadata = boost::make_shared<NodeMetadata>();
    (*results)->setMetadataResults(metadata);


    CacheEntryLockerBasePtr cacheAccess = (*results)->getFromCache();

    CacheEntryLockerBase::CacheEntryStatusEnum cacheStatus = cacheAccess->getStatus();
    while (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusComputationPending) {
        cacheStatus = cacheAccess->waitForPendingEntry();
    }

    if (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusCached) {
        if (!cacheAccess->isPersistent()) {
            *results = toGetTimeInvariantMetadataResults(cacheAccess->getProcessLocalEntry());
        }
        return eActionStatusOK;
    }
    assert(cacheStatus == CacheEntryLockerBase::eCacheEntryStatusMustCompute);


    // If the node is disabled return the meta-datas of the main input.
    // Don't do that for an identity node: a render may be identity but not the metadata (e.g: NoOp)
    // A disabled generator still has to return some meta-datas, so let it return the default meta-datas.
    bool isDisabled = getDisabledKnobValue();
    if (isDisabled) {
        int mainInput = getNode()->getPreferredInput();
        if (mainInput != -1) {
            EffectInstancePtr input = getInputRenderEffectAtAnyTimeView(mainInput);
            if (input) {

                GetTimeInvariantMetadataResultsPtr inputResults;
                ActionRetCodeEnum stat = input->getTimeInvariantMetadata_public(&inputResults);
                if (isFailureRetCode(stat)) {
                    return stat;
                }
                *results = inputResults;

                return eActionStatusOK;
            }
        }
    }

    ActionRetCodeEnum stat = getDefaultMetadata(*metadata);

    if (isFailureRetCode(stat)) {
        return stat;
    }


    if (!isDisabled) {

        // If the node is disabled, don't call getClipPreferences on the plug-in:
        // we don't want it to change output Format or other metadata
        ActionRetCodeEnum stat = getTimeInvariantMetadata(*metadata);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        _imp->checkMetadata(*metadata);
    }

    _imp->setTimeInvariantMetadataResults(*results);
    cacheAccess->insertInCache();


    // For a Reader, try to add the output format to the project formats.
    if (isReader()) {
        Format format;
        format.set(metadata->getOutputFormat());
        format.setPixelAspectRatio(metadata->getPixelAspectRatio(-1));
        getApp()->getProject()->setOrAddProjectFormat(format, true);
    }


    return eActionStatusOK;
} // getTimeInvariantMetadata_public


int
EffectInstance::getUnmappedNumberOfCompsForColorPlane(int inputNb, const std::vector<NodeMetadataPtr>& inputs, int firstNonOptionalConnectedInputComps) const
{
    int rawComps;

    if (inputs[inputNb]) {
        rawComps = inputs[inputNb]->getColorPlaneNComps(-1);
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
            rawComps = getNode()->findClosestSupportedNumberOfComponents(inputNb, rawComps);
        }
    }
    if (!rawComps) {
        rawComps = 4; // default to RGBA
    }

    return rawComps;
} // getUnmappedNumberOfCompsForColorPlane

ActionRetCodeEnum
EffectInstance::getDefaultMetadata(NodeMetadata &metadata)
{

    const bool multiBitDepth = isMultipleInputsWithDifferentBitDepthsSupported();
    int nInputs = getNInputs();

    // OK find the deepest chromatic component on our input clips and the one with the
    // most components
    bool hasSetCompsAndDepth = false;
    ImageBitDepthEnum deepestBitDepth = eImageBitDepthNone;
    int mostComponents = 0;

    //Default to the project frame rate
    double frameRate = getApp()->getProjectFrameRate();

    double inputPar = 1.;
    bool inputParSet = false;
    ImagePremultiplicationEnum premult = eImagePremultiplicationOpaque;
    bool premultSet = false;

    bool hasOneInputContinuous = false;
    bool hasOneInputFrameVarying = false;

    // Find the components of the first non optional connected input
    // They will be used for disconnected input
    int firstNonOptionalConnectedInputComps = 0;


    std::vector<NodeMetadataPtr> inputMetadata(nInputs);
    for (int i = 0; i < nInputs; ++i) {
        const EffectInstancePtr& input = getInputRenderEffectAtAnyTimeView(i);
        if (input) {
            GetTimeInvariantMetadataResultsPtr results;
            ActionRetCodeEnum stat = input->getTimeInvariantMetadata_public(&results);
            if (!isFailureRetCode(stat)) {
                inputMetadata[i] = results->getMetadataResults();

                if ( !firstNonOptionalConnectedInputComps && !getNode()->isInputOptional(i) ) {
                    firstNonOptionalConnectedInputComps = inputMetadata[i]->getColorPlaneNComps(-1);
                }
            }


        }
    }

    for (int i = 0; i < nInputs; ++i) {

        const NodeMetadataPtr& input = inputMetadata[i];
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

        int rawComp = getUnmappedNumberOfCompsForColorPlane(i, inputMetadata, firstNonOptionalConnectedInputComps);
        ImageBitDepthEnum rawDepth = input ? input->getBitDepth(-1) : eImageBitDepthFloat;
        ImagePremultiplicationEnum rawPreMult = input ? input->getOutputPremult() : eImagePremultiplicationPremultiplied;

        // Note: first chromatic input gives the default output premult too, even if not connected
        // (else the output of generators may be opaque even if the host default is premultiplied)
        if ( ( rawComp == 4 ) && (input || !premultSet) ) {
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

            if (mostComponents == 1 && mostComponents != rawComp) {
                // one is alpha, the other is anything else than just alpha: the union is RGBA
                mostComponents = 4;
            }

            if ( rawComp > mostComponents ) {
                mostComponents = rawComp;
            }
        }

    } // for each input


    if (!hasSetCompsAndDepth) {
        mostComponents = 4;
        deepestBitDepth = eImageBitDepthFloat;
    }

    // set some stuff up
    metadata.setOutputFrameRate(frameRate);
    metadata.setOutputFielding(eImageFieldingOrderNone);

    // From commit https://github.com/MrKepzie/Natron/commit/1747275505e297c0c03f5b472c921f29540402f2
    // an effect is not considered frame varying or continuous if one of its knobs is animated:
    // Since all knobs encode their values at the current render time/view in the hash, we don't need to
    // modify this flag: if the animation of the knobs happen to be the same at some point in the curve (e.g:
    // there's a constant portion) then the results for that frame will be cached! If we were to set this flag
    // it would force caching of a new image for each frame, which may not be desirable.

    // Note that the function EffectInstance::canRenderContinuously() also checks if the effect has an animation

    //const bool hasAnimation = getHasAnimation();

    // An effect is frame varying if one of its inputs is varying
    metadata.setIsFrameVarying(hasOneInputFrameVarying /* || hasAnimation*/);

    // An effect is continuous if at least one of its inputs is continuous
    metadata.setIsContinuous(hasOneInputContinuous /* || hasAnimation*/);

    // now find the best depth that the plugin supports
    deepestBitDepth = getNode()->getClosestSupportedBitDepth(deepestBitDepth);

    bool multipleClipsPAR = isMultipleInputsWithDifferentPARSupported();


    Format projectFormat;
    getApp()->getProject()->getProjectDefaultFormat(&projectFormat);
    double projectPAR = projectFormat.getPixelAspectRatio();

    RectI firstOptionalInputFormat, firstNonOptionalInputFormat;

    // Format: Take format from the first non optional input if any. Otherwise from the first optional input.
    // Otherwise fallback on project format
    bool firstOptionalInputFormatSet = false, firstNonOptionalInputFormatSet = false;

    // now add the input gubbins to the per inputs metadata
    for (int i = -1; i < nInputs; ++i) {

        if (i >= 0 && !inputMetadata[i]) {
            continue;
        }
        double par;
        if (!multipleClipsPAR) {
            par = inputParSet ? inputPar : projectPAR;
        } else {
            if (inputParSet) {
                par = inputPar;
            } else {
                if (i >= 0 && inputMetadata[i]) {
                    par = inputMetadata[i]->getPixelAspectRatio(-1);
                } else {
                    par = projectPAR;
                }
            }
        }
        metadata.setPixelAspectRatio(i, par);

        bool isOptional = i >= 0 && getNode()->isInputOptional(i);
        if (i >= 0) {
            if (isOptional) {
                if (!firstOptionalInputFormatSet && inputMetadata[i]) {
                    firstOptionalInputFormat = inputMetadata[i]->getOutputFormat();
                    firstOptionalInputFormatSet = true;
                }
            } else {
                if (!firstNonOptionalInputFormatSet && inputMetadata[i]) {
                    firstNonOptionalInputFormat = inputMetadata[i]->getOutputFormat();
                    firstNonOptionalInputFormatSet = true;
                }
            }
        }

        if ( (i == -1) || isOptional ) {
            // "Optional input clips can always have their component types remapped"
            // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#id482755
            ImageBitDepthEnum depth = deepestBitDepth;
            int remappedComps = getNode()->findClosestSupportedNumberOfComponents(i, mostComponents);
            metadata.setColorPlaneNComps(i, remappedComps);
            if ( (i == -1) && !premultSet &&
                ( ( remappedComps == 4 ) || ( remappedComps == 1 ) ) ) {
                premult = eImagePremultiplicationPremultiplied;
                premultSet = true;
            }


            metadata.setBitDepth(i, depth);
        } else {

            int rawComps = getUnmappedNumberOfCompsForColorPlane(i, inputMetadata, firstNonOptionalConnectedInputComps);
            ImageBitDepthEnum rawDepth;
            if (i >= 0 && inputMetadata[i]) {
                rawDepth = inputMetadata[i]->getBitDepth(-1);
            } else {
                rawDepth = eImageBitDepthFloat;
            }


            ImageBitDepthEnum depth = multiBitDepth ? getNode()->getClosestSupportedBitDepth(rawDepth) : deepestBitDepth;
            metadata.setBitDepth(i, depth);

            metadata.setColorPlaneNComps(i, rawComps);
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
EffectInstance::Implementation::checkMetadata(NodeMetadata &md)
{

    const bool supportsMultipleClipDepths = _publicInterface->isMultipleInputsWithDifferentBitDepthsSupported();
    const bool supportsMultipleClipPARs = _publicInterface->isMultipleInputsWithDifferentPARSupported();

    int nInputs = _publicInterface->getNInputs();

    double outputPAR = md.getPixelAspectRatio(-1);

    ImageBitDepthEnum outputDepth = md.getBitDepth(-1);

    // First fix incorrect metadata that could have been set by the plug-in
    for (int i = -1; i < nInputs; ++i) {

        if (i >= 0) {
            EffectInstancePtr input = _publicInterface->getInputRenderEffectAtAnyTimeView(i);
            if (!input) {
                continue;
            }
        }
        ImageBitDepthEnum depth = md.getBitDepth(i);
        md.setBitDepth( i, _publicInterface->getNode()->getClosestSupportedBitDepth(depth));

        int nComps = md.getColorPlaneNComps(i);
        bool isAlpha = false;
        bool isRGB = false;
        if (i == -1) {
            if ( nComps == 3 ) {
                isRGB = true;
            } else if ( nComps == 1 ) {
                isAlpha = true;
            }
        }

        nComps = _publicInterface->getNode()->findClosestSupportedNumberOfComponents(i, nComps);


        md.setColorPlaneNComps(i, nComps);

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

    createInstanceAction();

}

NATRON_NAMESPACE_EXIT
