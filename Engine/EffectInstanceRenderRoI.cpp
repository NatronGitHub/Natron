/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include <map>
#include <sstream>
#include <algorithm> // min, max
#include <fstream>
#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include <QtCore/QThreadPool>
#include <QtCore/QReadWriteLock>
#include <QtCore/QCoreApplication>
#include <QtConcurrentMap> // QtCore on Qt4, QtConcurrent on Qt5
#include <QtConcurrentRun> // QtCore on Qt4, QtConcurrent on Qt5

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include "Global/QtCompat.h"

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/DiskCacheNode.h"
#include "Engine/EffectInstanceTLSData.h"
#include "Engine/EffectOpenGLContextData.h"
#include "Engine/Distortion2D.h"
#include "Engine/Cache.h"
#include "Engine/CacheEntryBase.h"
#include "Engine/CacheEntryKeyBase.h"
#include "Engine/Image.h"
#include "Engine/ImageCacheEntry.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/Log.h"
#include "Engine/MultiThread.h"
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/OSGLContext.h"
#include "Engine/GPUContextPool.h"
#include "Engine/PluginMemory.h"
#include "Engine/Project.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/RotoShapeRenderNode.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/RotoPaint.h"
#include "Engine/Settings.h"
#include "Engine/TreeRenderQueueManager.h"
#include "Engine/Timer.h"
#include "Engine/Transform.h"
#include "Engine/TreeRender.h"
#include "Engine/ThreadPool.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

#define kNatronPersistentErrorInfiniteRoI "NatronPersistentErrorInfiniteRoI"
#define kNatronPersistentErrorProxyUnsupported "NatronPersistentErrorProxyUnsupported"

// This controls how many frames a plug-in can pre-fetch per input
// This is to avoid cases where the user would for example use the FrameBlend node with a huge amount of frames so that they
// do not all stick altogether in memory
#define NATRON_MAX_FRAMES_NEEDED_PRE_FETCHING 3

NATRON_NAMESPACE_ENTER


/**
 * @brief This function determines the planes to render and calls recursively on upstream nodes unavailable planes
 **/
ActionRetCodeEnum
EffectInstance::Implementation::handlePassThroughPlanes(const FrameViewRequestPtr& requestData,
                                                        const TreeRenderExecutionDataPtr& requestPassSharedData,
                                                        const RectD& roiCanonical,
                                                        AcceptedRequestConcatenationFlags concatenationFlags,
                                                        std::map<int, std::list<ImagePlaneDesc> >* inputLayersNeeded,
                                                        bool *isPassThrough)
{

    *isPassThrough = false;

    std::list<ImagePlaneDesc> layersProduced, passThroughLayers;
    int passThroughInputNb;
    TimeValue passThroughTime;
    ViewIdx passThroughView;
    bool processAllLayers;
    std::bitset<4> processChannels;
    {


        GetComponentsResultsPtr results = requestData->getComponentsResults();
        if (!results) {
            ActionRetCodeEnum stat = _publicInterface->getLayersProducedAndNeeded_public(_publicInterface->getCurrentRenderTime(), _publicInterface->getCurrentRenderView(), &results);
            if (isFailureRetCode(stat)) {
                return stat;
            }
            requestData->setComponentsNeededResults(results);
        }


        results->getResults(inputLayersNeeded, &layersProduced, &passThroughLayers, &passThroughInputNb, &passThroughTime, &passThroughView, &processChannels, &processAllLayers);
    }

    std::list<ImagePlaneDesc> componentsToFetchUpstream;
    /*
     * For all requested planes, check which components can be produced in output by this node.
     * If the components are from the color plane, if another set of components of the color plane is present
     * we try to render with those instead.
     */
    {
        const ImagePlaneDesc& plane = requestData->getPlaneDesc();
        if (plane.getNumComponents() == 0) {
            return eActionStatusFailed;
        }

        std::list<ImagePlaneDesc>::const_iterator foundProducedLayer = ImagePlaneDesc::findEquivalentLayer(plane, layersProduced.begin(), layersProduced.end());
        if (foundProducedLayer != layersProduced.end()) {

            if (plane != *foundProducedLayer) {
                // Make sure that we ask to render what is produced by this effect rather than the original requested component.
                // e.g: The color.RGB might have been requested but this effect might produce color.RGBA
                requestData->setPlaneDesc(*foundProducedLayer);
            }
        } else {
            // The plane does not exist on this effect
            // If the effect is not set to "All plane" and the pass-through input nb is not set then fail
            if (!processAllLayers) {
                if (passThroughInputNb == -1) {
                    //_publicInterface->getNode()->setPersistentMessage(eMessageTypeError, kNatronPersistentErrorGenericRenderMessage, _publicInterface->tr("Could not fetch plane \"%1\" because it is not available on this node").arg(QString::fromUtf8(plane.getPlaneLabel().c_str())).toStdString());
                    return eActionStatusFailed;
                } else {
                    // Fetch the plane on the pass-through input
                    
                    EffectInstancePtr ptInput = _publicInterface->getInputMainInstance(passThroughInputNb);
                    if (!ptInput) {
                        return eActionStatusInputDisconnected;
                    }
                        
                    *isPassThrough = true;

                    FrameViewRequestPtr createdRequest;
                    return ptInput->requestRender(passThroughTime, passThroughView, requestData->getProxyScale(), requestData->getMipMapLevel(), plane, roiCanonical, passThroughInputNb, concatenationFlags,requestData, requestPassSharedData, &createdRequest, 0);
                }
            }
        }
    }

    return eActionStatusOK;

} // handlePassThroughPlanes


ActionRetCodeEnum
EffectInstance::Implementation::handleIdentityEffect(double par,
                                                     const RectD& rod,
                                                     const RenderScale& combinedScale,
                                                     const RectD& canonicalRoi,
                                                     const FrameViewRequestPtr& requestData,
                                                     const TreeRenderExecutionDataPtr& requestPassSharedData,
                                                     AcceptedRequestConcatenationFlags concatenationFlags,
                                                     bool *isIdentity)
{

    TimeValue inputTimeIdentity;
    int inputNbIdentity;
    ViewIdx inputIdentityView;
    ImagePlaneDesc identityPlane;

    {
        // If the effect is identity over the whole RoD then we can forward the render completly to the identity node
        RectI pixelRod;
        rod.toPixelEnclosing(combinedScale, par, &pixelRod);

        IsIdentityResultsPtr results;
        {
            ActionRetCodeEnum stat = _publicInterface->isIdentity_public(true, _publicInterface->getCurrentRenderTime(), combinedScale, pixelRod, _publicInterface->getCurrentRenderView(), &requestData->getPlaneDesc(),  &results);
            if (isFailureRetCode(stat)) {
                return stat;
            }
        }

        results->getIdentityData(&inputNbIdentity, &inputTimeIdentity, &inputIdentityView, &identityPlane);

    }
    *isIdentity = inputNbIdentity >= 0 || inputNbIdentity == -2;
    if (!*isIdentity) {
        return eActionStatusOK;
    }

    // If effect is identity on itself, call renderRoI again at different time and view.
    if (inputNbIdentity == -2) {

        // Be safe: we may hit an infinite recursion without this check
        assert(inputTimeIdentity != _publicInterface->getCurrentRenderTime() || inputIdentityView != _publicInterface->getCurrentRenderView() || identityPlane != requestData->getPlaneDesc());
        if ( inputTimeIdentity == _publicInterface->getCurrentRenderTime()  && inputIdentityView == _publicInterface->getCurrentRenderView() && identityPlane == requestData->getPlaneDesc()) {
            return eActionStatusFailed;
        }

        FrameViewRequestPtr createdRequest;
        return _publicInterface->requestRender(inputTimeIdentity, inputIdentityView, requestData->getProxyScale(), requestData->getMipMapLevel(), identityPlane, canonicalRoi, -1, concatenationFlags, requestData, requestPassSharedData, &createdRequest, 0);

    } else {
        assert(inputNbIdentity != -1);
        EffectInstancePtr identityInput = _publicInterface->getInputMainInstance(inputNbIdentity);
        if (!identityInput) {
            return eActionStatusInputDisconnected;
        }

        FrameViewRequestPtr createdRequest;
        return identityInput->requestRender(inputTimeIdentity, inputIdentityView, requestData->getProxyScale(), requestData->getMipMapLevel(), identityPlane, canonicalRoi, inputNbIdentity, concatenationFlags,requestData, requestPassSharedData, &createdRequest, 0);

    }
} // EffectInstance::Implementation::handleIdentityEffect

EffectInstance::AcceptedRequestConcatenationFlags
EffectInstance::Implementation::getConcatenationFlagsForInput(int inputNb) const
{
    AcceptedRequestConcatenationFlags ret = eAcceptedRequestConcatenationNone;
    if (_publicInterface->getNode()->canInputReceiveTransform3x3(inputNb)) {
        ret |= eAcceptedRequestConcatenationDeprecatedTransformMatrix;
    }
    if (_publicInterface->getNode()->canInputReceiveDistortion(inputNb)) {
        ret |= eAcceptedRequestConcatenationDistortionFunc;
    }
    return ret;
}

ActionRetCodeEnum
EffectInstance::Implementation::handleConcatenation(const TreeRenderExecutionDataPtr& requestPassSharedData,
                                                    const FrameViewRequestPtr& requestData,
                                                    AcceptedRequestConcatenationFlags downstreamConcatFlags,
                                                    const RenderScale& renderScale,
                                                    const RectD& canonicalRoi,
                                                    bool draftRender,
                                                    bool *concatenated)


{
    *concatenated = false;
    if (!_publicInterface->getCurrentRender()->isConcatenationEnabled()) {
        return eActionStatusOK;
    }

    bool canDistort = _publicInterface->getCanDistort();
    bool canTransform = _publicInterface->getCanTransform3x3();

    if (!canDistort && !canTransform) {
        return eActionStatusOK;
    }

    const bool requesterCanReceiveDeprecatedTransform3x3 = downstreamConcatFlags & eAcceptedRequestConcatenationDeprecatedTransformMatrix;
    const bool requesterCanReceiveDistortionFunc = downstreamConcatFlags & eAcceptedRequestConcatenationDistortionFunc;

    // If the caller can apply a distortion, then check if this effect has a distortion
    // otherwise, don't bother
    if (!requesterCanReceiveDeprecatedTransform3x3 && !requesterCanReceiveDistortionFunc) {
        return eActionStatusOK;
    }
    assert((requesterCanReceiveDeprecatedTransform3x3 && !requesterCanReceiveDistortionFunc) || (!requesterCanReceiveDeprecatedTransform3x3 && requesterCanReceiveDistortionFunc));

    TimeValue curTime = _publicInterface->getCurrentRenderTime();
    ViewIdx curView = _publicInterface->getCurrentRenderView();


    // Call the getDistortion action
    DistortionFunction2DPtr disto;
    {
        GetDistortionResultsPtr results = requestData->getDistortionResults();
        if (!results) {
            ActionRetCodeEnum stat = _publicInterface->getInverseDistortion_public(curTime, renderScale, draftRender, curView, &results);
            if (isFailureRetCode(stat)) {
                return stat;
            }
        }
        if (results) {
            disto = results->getResults();
            requestData->setDistortionResults(results);
        }
    }

    // No distortion or invalid input
    if (!disto || disto->inputNbToDistort == -1) {
        return eActionStatusOK;
    }

    // We support backward compatibility for plug-ins that only support Transforms: if a function is returned we do not concatenate.
    if (disto->func && !requesterCanReceiveDistortionFunc) {
        return eActionStatusOK;
    }

    assert((disto->func && requesterCanReceiveDistortionFunc) || disto->transformMatrix);

    // Recurse on input given by plug-in
    EffectInstancePtr distoInput = _publicInterface->getInputMainInstance(disto->inputNbToDistort);
    if (!distoInput) {
        return eActionStatusInputDisconnected;
    }


    // Compute the regions of interest in input for this RoI.
    RoIMap inputsRoi;
    {
        ActionRetCodeEnum stat = _publicInterface->getRegionsOfInterest_public(curTime, renderScale, canonicalRoi, curView, &inputsRoi);
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }

    RectD inputRoI;
    {
        RoIMap::iterator foundRoI = inputsRoi.find(disto->inputNbToDistort);
        if (foundRoI == inputsRoi.end()) {
            // RoI not specified... use the same RoI as passed in argument
            inputRoI = canonicalRoi;
        } else {
            inputRoI = foundRoI->second;
        }
    }

    if (inputRoI.isNull()) {
        return eActionStatusOK;
    }


    AcceptedRequestConcatenationFlags thisNodeConcatFlags = getConcatenationFlagsForInput(disto->inputNbToDistort);

    FrameViewRequestPtr inputRequest;
    distoInput->requestRender(curTime, curView, requestData->getProxyScale(), requestData->getMipMapLevel(), requestData->getPlaneDesc(), inputRoI, disto->inputNbToDistort, thisNodeConcatFlags, requestData, requestPassSharedData, &inputRequest, 0);

    // Create a distorsion stack that will be applied by the effect downstream
    if (!requestData->getDistorsionStack()) {
        Distortion2DStackPtr distoStack(new Distortion2DStack);

        // Append the list of upstream distorsions (if any)
        Distortion2DStackPtr upstreamDistoStack = inputRequest->getDistorsionStack();
        if (upstreamDistoStack) {
            assert(upstreamDistoStack->getInputImageEffect());
            distoStack->setInputImageEffect(upstreamDistoStack->getInputImageEffect());
            distoStack->pushDistortionStack(*upstreamDistoStack);
        } else {
            distoStack->setInputImageEffect(distoInput);
        }
        distoStack->pushDistortionFunction(disto);



        // Set the stack on the frame view request
        requestData->setDistorsionStack(distoStack);
    }
    
    *concatenated = true;

    return eActionStatusOK;
} // handleConcatenation

ActionRetCodeEnum
EffectInstance::Implementation::lookupCachedImage(unsigned int mipMapLevel,
                                                  const RenderScale& proxyScale,
                                                  const ImagePlaneDesc& plane,
                                                  const std::vector<RectI>& perMipMapPixelRoD,
                                                  const RectI& pixelRoi,
                                                  CacheAccessModeEnum cachePolicy,
                                                  RenderBackendTypeEnum backend,
                                                  bool readOnly,
                                                  ImagePtr* image,
                                                  bool* hasPendingTiles,
                                                  bool* hasUnrenderedTiles)
{
    *hasPendingTiles = false;
    *hasUnrenderedTiles = true;
    if (!*image) {
        assert(!readOnly);
        *image = createCachedImage(pixelRoi, perMipMapPixelRoD, mipMapLevel, proxyScale, plane, backend, cachePolicy, true /*delayAllocation*/);
        if (!*image) {
            if (_publicInterface->isRenderAborted()) {
                return eActionStatusAborted;
            } else {
                return eActionStatusFailed;
            }
        }

    } else {
        if (readOnly && !(*image)->getBounds().contains(pixelRoi)) {
            // We are read-only: do not modify the image, instead just return that there are unrendered tiles.
            return eActionStatusOK;
        }
        ActionRetCodeEnum stat = (*image)->ensureBounds(pixelRoi, mipMapLevel, perMipMapPixelRoD, _publicInterface->shared_from_this());
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }

    ImageCacheEntryPtr cacheEntry = (*image)->getCacheEntry();
    assert(cacheEntry);
    ActionRetCodeEnum stat = cacheEntry->fetchCachedTilesAndUpdateStatus(readOnly, NULL, hasUnrenderedTiles, hasPendingTiles);
    if (isFailureRetCode(stat)) {
        return stat;
    }
    return eActionStatusOK;
} // lookupCachedImage

bool
EffectInstance::Implementation::canSplitRenderWindowWithIdentityRectangles(const RenderScale& renderMappedScale,
                                                                           RectD* inputRoDIntersectionCanonical)
{
    RectD inputsIntersection;
    bool inputsIntersectionSet = false;
    bool hasDifferentRods = false;
    int maxInput = _publicInterface->getNInputs();
    bool hasMask = false;

    TimeValue time = _publicInterface->getCurrentRenderTime();
    ViewIdx view = _publicInterface->getCurrentRenderView();

    RotoStrokeItemPtr attachedStroke = toRotoStrokeItem(_publicInterface->getAttachedRotoItem());


    for (int i = 0; i < maxInput; ++i) {

        RectD inputRod;

        EffectInstancePtr input = _publicInterface->getInputRenderEffect(i, time, view);
        if (!input) {
            continue;
        }

        hasMask |= _publicInterface->getNode()->isInputMask(i);


        if (attachedStroke) {
            inputRod = attachedStroke->getLastStrokeMovementBbox();
        } else {

            GetRegionOfDefinitionResultsPtr rodResults;
            ActionRetCodeEnum stat = input->getRegionOfDefinition_public(time, renderMappedScale, view, &rodResults);
            if (isFailureRetCode(stat)) {
                break;
            }
            inputRod = rodResults->getRoD();
            if (inputRod.isNull()) {
                continue;
            }
        }

        if (!inputsIntersectionSet) {
            inputsIntersection = inputRod;
            inputsIntersectionSet = true;
        } else {
            if (!hasDifferentRods) {
                if (inputRod != inputsIntersection) {
                    hasDifferentRods = true;
                }
            }
            inputsIntersection.intersect(inputRod, &inputsIntersection);
        }
    }

    /*
     If the effect has 1 or more inputs and:
     - An input is a mask OR
     - Several inputs have different region of definition
     Try to split the rectangles to render in smaller rectangles, we have great chances that these smaller rectangles
     are identity over one of the input effect, thus avoiding pixels to render.
     */
    if ( inputsIntersectionSet && (hasMask || hasDifferentRods) ) {
        *inputRoDIntersectionCanonical = inputsIntersection;
        return true;
    }

    return false;
} // canSplitRenderWindowWithIdentityRectangles

ActionRetCodeEnum
EffectInstance::Implementation::checkRestToRender(bool updateTilesStateFromCache,
                                                  const FrameViewRequestPtr& requestData,
                                                  const RectI& renderMappedRoI,
                                                  const RenderScale& renderMappedScale,
                                                  const std::map<ImagePlaneDesc, ImagePtr>& producedImagePlanes,
                                                  std::list<RectToRender>* renderRects,
                                                  bool* hasPendingTiles)
{
    renderRects->clear();

    
    // Compute the rectangle portion (renderWindow) left to render.
    TileStateHeader tilesState;
    bool hasUnRenderedTile;
    ImagePtr image = requestData->getFullscaleImagePlane();

    ImageCacheEntryPtr cacheEntry;
    if (image) {
        cacheEntry = image->getCacheEntry();
    }

    if (!cacheEntry) {
        // If the image is not cache, fill the state with empty tiles
        hasUnRenderedTile = true;
        *hasPendingTiles = false;
        int tileSizeX, tileSizeY;
        ImageBitDepthEnum outputBitDepth = _publicInterface->getBitDepth(-1);
        appPTR->getTileCache()->getTileSizePx(outputBitDepth, &tileSizeX, &tileSizeY);;
        tilesState.init(tileSizeX, tileSizeY, renderMappedRoI);
    } else {

        // Get the tiles states on the image plane requested, but also fetch from the cache other produced planes so we can
        // cache them.
        for (std::map<ImagePlaneDesc, ImagePtr>::const_iterator it = producedImagePlanes.begin(); it != producedImagePlanes.end(); ++it) {
            ImageCacheEntryPtr planeCacheEntry = it->second->getCacheEntry();
            ActionRetCodeEnum stat;
            if (updateTilesStateFromCache) {
                if (planeCacheEntry == cacheEntry) {
                    stat = planeCacheEntry->fetchCachedTilesAndUpdateStatus(false, &tilesState, &hasUnRenderedTile, hasPendingTiles);
                } else {
                    stat = planeCacheEntry->fetchCachedTilesAndUpdateStatus(false, NULL, NULL, NULL);
                }
            } else {
                stat = eActionStatusOK;
                if (planeCacheEntry == cacheEntry) {
                    planeCacheEntry->getStatus(&tilesState, &hasUnRenderedTile, hasPendingTiles);
                } else {
                    planeCacheEntry->getStatus(NULL, NULL, NULL);
                }

            }
            if (isFailureRetCode(stat)) {
                return stat;
            }
        }
        
    }

    // The image is already computed
    if (!hasUnRenderedTile) {
        return eActionStatusOK;
    }

    // If the effect does not support tiles, render everything again
    if (!_publicInterface->supportsTiles()) {
        // If not using the cache, render the full RoI
        // The RoI has already been set to the pixelRoD in this case
        RectToRender r;
        r.rect = renderMappedRoI;
        r.identityInputNumber = -1;
        renderRects->push_back(r);
        return eActionStatusOK;
    }

    //
    // If the effect has multiple inputs (such as Source + mask) (e.g: Merge),
    // if the inputs do not have the same RoD, the plug-in may be identity
    // outside the intersection of the input RoDs.
    // We try to call isIdentity on each tile outside the input intersection
    // so that we may not have to render uninteresting areas.
    //
    std::list<RectToRender> identityRects;
    {
        RectD inputRodIntersection;
        RectI inputRodIntersectionPixel;
        if (canSplitRenderWindowWithIdentityRectangles(renderMappedScale, &inputRodIntersection)) {

            double par = _publicInterface->getAspectRatio(-1);
            inputRodIntersection.toPixelEnclosing(renderMappedScale, par, &inputRodIntersectionPixel);

            TimeValue time = _publicInterface->getCurrentRenderTime();
            ViewIdx view = _publicInterface->getCurrentRenderView();

            // For each tile, if outside of the input intersections, check if it is identity.
            // If identity mark as rendered, and add to the RectToRender list.
            for (TileStateVector::iterator it = tilesState.state->tiles.begin(); it != tilesState.state->tiles.end(); ++it) {

                if (it->status != eTileStatusNotRendered) {
                    continue;
                }
                
                if ( !it->bounds.intersects(inputRodIntersectionPixel) ) {
                    TimeValue identityInputTime;
                    int identityInputNb;
                    ViewIdx inputIdentityView;
                    ImagePlaneDesc identityPlane;
                    {
                        IsIdentityResultsPtr results;
                        ActionRetCodeEnum stat = _publicInterface->isIdentity_public(false, time, renderMappedScale, it->bounds, view, &requestData->getPlaneDesc(), &results);
                        if (isFailureRetCode(stat)) {
                            continue;
                        } else {
                            results->getIdentityData(&identityInputNb, &identityInputTime, &inputIdentityView, &identityPlane);
                        }
                    }
                    if (identityInputNb >= 0) {

                        // Mark the tile rendered
                        it->status = requestData->getParentRender()->isDraftRender() ? eTileStatusRenderedLowQuality : eTileStatusRenderedHighestQuality;

                        // Add this rectangle to the rects to render list (it will just copy the source image and
                        // not actually call render on it)
                        RectToRender r;
                        r.rect = it->bounds;
                        r.identityInputNumber = identityInputNb;
                        r.identityTime = identityInputTime;
                        r.identityView = inputIdentityView;
                        r.identityPlane = identityPlane;
                        identityRects.push_back(r);
                    }
                } // if outside of inputs intersection
            } // for each tile to render
        } // canSplitRenderWindowWithIdentityRectangles
    }

    // Now we try to reduce the unrendered tiles in bigger rectangles so that there's a lot less calls to
    // the render action.
    std::list<RectI> reducedRects;
    ImageTilesState::getMinimalRectsToRenderFromTilesState(renderMappedRoI, tilesState, &reducedRects);

    if (reducedRects.empty()) {
        return eActionStatusOK;
    }

    // If there's an identity rect covered by a rectangle to render, remove it
    for (std::list<RectToRender>::const_iterator it = identityRects.begin(); it != identityRects.end(); ++it) {
        bool hasRectContainingIdentityRect = false;
        for (std::list<RectI>::const_iterator it2 = reducedRects.begin(); it2 != reducedRects.end(); ++it2) {
            if (it2->contains(it->rect)) {
                hasRectContainingIdentityRect = true;
                break;
            }
        }
        if (!hasRectContainingIdentityRect) {
            renderRects->push_back(*it);
        }
    }

    // If plug-in wants host frame threading and there is only 1 rect to render, split it
    if (reducedRects.size() == 1 && requestData->getRenderDevice() == eRenderBackendTypeCPU && _publicInterface->getRenderThreadSafety() == eRenderSafetyFullySafeFrame) {
        RectI mainRenderRect = reducedRects.front();

        // Estimate num cpus according to the rectangle to render.
        // the MultiThread suite will adjust the tasks to the actual number of cpus available, no need to do the clamping ourselves.
        unsigned int nCPUs = ( std::min(mainRenderRect.x2 - mainRenderRect.x1, 4096) * (mainRenderRect.y2 - mainRenderRect.y1) ) / 4096;
        if (nCPUs > 1) {
            reducedRects = mainRenderRect.splitIntoSmallerRects(nCPUs);
        }
        
    }
    for (std::list<RectI>::const_iterator it = reducedRects.begin(); it != reducedRects.end(); ++it) {
        if (!it->isNull()) {
            RectToRender r;
            r.rect = *it;
            renderRects->push_back(r);
        }
    }
    return eActionStatusOK;
} // checkRestToRender

RenderBackendTypeEnum
EffectInstance::Implementation::storageModeToBackendType(StorageModeEnum storage)
{
    switch (storage) {
        case eStorageModeRAM:
            return eRenderBackendTypeCPU;
        case eStorageModeGLTex:
            return eRenderBackendTypeOpenGL;
        default:
            return eRenderBackendTypeCPU;
    }
}

StorageModeEnum
EffectInstance::Implementation::storageModeFromBackendType(RenderBackendTypeEnum backend)
{
    switch (backend) {
        case eRenderBackendTypeOpenGL:
            return eStorageModeGLTex;
            break;
        case eRenderBackendTypeCPU:
        case eRenderBackendTypeOSMesa:
            return eStorageModeRAM;
    }
    assert(false);

    return eStorageModeRAM;
}

ImagePtr
EffectInstance::Implementation::createCachedImage(const RectI& roiPixels,
                                                  const std::vector<RectI>& perMipMapPixelRoD,
                                                  unsigned int mappedMipMapLevel,
                                                  const RenderScale& proxyScale,
                                                  const ImagePlaneDesc& plane,
                                                  RenderBackendTypeEnum backend,
                                                  CacheAccessModeEnum cachePolicy,
                                                  bool delayAllocation)
{

    // Mark the image as draft in the cache
    TreeRenderPtr render = _publicInterface->getCurrentRender();
    bool isDraftRender = render->isDraftRender();
    
    // The node frame/view hash to identify the image in the cache
    U64 nodeFrameViewHash;
    {
        HashableObject::ComputeHashArgs args;
        args.time = _publicInterface->getCurrentRenderTime();
        args.view = _publicInterface->getCurrentRenderView();
        args.hashType = HashableObject::eComputeHashTypeTimeViewVariant;
        nodeFrameViewHash = _publicInterface->computeHash(args);
    }

    bool supportsDraft = _publicInterface->isDraftRenderSupported();

    // The bitdepth of the image
    ImageBitDepthEnum outputBitDepth = _publicInterface->getBitDepth(-1);

    // Create the corresponding image plane
    Image::InitStorageArgs initArgs;
    {
        initArgs.bounds = roiPixels;
        initArgs.perMipMapPixelRoD = perMipMapPixelRoD;
        initArgs.cachePolicy = cachePolicy;
        initArgs.renderClone = _publicInterface->shared_from_this();
        initArgs.proxyScale = proxyScale;
        initArgs.mipMapLevel = mappedMipMapLevel;
        initArgs.isDraft = supportsDraft ? isDraftRender : false;
        initArgs.nodeTimeViewVariantHash = nodeFrameViewHash;
        initArgs.bufferFormat = _publicInterface->getPreferredBufferLayout();
        initArgs.bitdepth = outputBitDepth;
        initArgs.plane = plane;
        initArgs.storage = storageModeFromBackendType(backend);
        initArgs.createTilesMapEvenIfNoCaching = true;
        initArgs.glContext = render->getGPUOpenGLContext();
        initArgs.textureTarget = GL_TEXTURE_2D;
        
        // Do not allocate the image buffers yet, instead do it before rendering.
        // We need to create the image before because it does the cache look-up itself, and we don't want to got further if
        // there's something cached.
        initArgs.delayAllocation = delayAllocation;
    }


    // Image::create will lookup the cache (if asked for)
    // Since multiple threads may want to access to the same image in the cache concurrently,
    // the first thread that gets onto a tile to render will render it and lock-out other threads
    // until it is rendered entirely.
    return Image::create(initArgs);
} // createCachedImage


ActionRetCodeEnum
EffectInstance::Implementation::launchRenderForSafetyAndBackend(const FrameViewRequestPtr& requestData,
                                                                const RenderScale& combinedScale,
                                                                RenderBackendTypeEnum backendType,
                                                                const std::list<RectToRender>& renderRects,
                                                                const std::map<ImagePlaneDesc, ImagePtr>& cachedPlanes)
{

    // If we reach here, it can be either because the planes are cached or not, either way
    // the planes are NOT a total identity, and they may have some content left to render.
    ActionRetCodeEnum renderRetCode = eActionStatusOK;

    // There should always be at least 1 plane to render (The color plane)
    assert(!renderRects.empty());

    RenderSafetyEnum safety = _publicInterface->getRenderThreadSafety();
    // eRenderSafetyInstanceSafe means that there is at most one render per instance
    // NOTE: the per-instance lock should be shared between
    // all clones of the same instance, because an InstanceSafe plugin may assume it is the sole owner of the output image,
    // and read-write on it.
    // It is probably safer to assume that several clones may write to the same output image only in the eRenderSafetyFullySafe case.

    // eRenderSafetyFullySafe means that there is only one render per FRAME : the lock is per image

    boost::scoped_ptr<QMutexLocker> locker;

    // Since we may are going to sit and wait on this lock, to allow this thread to be re-used by another task of the thread pool we
    // temporarily release the thread to the threadpool and reserve it again once
    // we waited.
    boost::scoped_ptr<ReleaseTPThread_RAII> releaser;
    if (safety == eRenderSafetyInstanceSafe) {
        releaser.reset(new ReleaseTPThread_RAII);
        locker.reset( new QMutexLocker( &renderData->instanceSafeRenderMutex ) );

    } else if (safety == eRenderSafetyUnsafe) {
        PluginPtr p = _publicInterface->getNode()->getPlugin();
        assert(p);
        releaser.reset(new ReleaseTPThread_RAII);
        locker.reset( new QMutexLocker( p->getPluginLock().get() ) );

    } else {
        // no need to lock
        Q_UNUSED(locker);
    }
    releaser.reset();
    
    TreeRenderPtr render = _publicInterface->getCurrentRender();

    OSGLContextPtr glContext;
    switch (backendType) {
        case eRenderBackendTypeOpenGL:
            glContext = render->getGPUOpenGLContext();
            break;
        case eRenderBackendTypeOSMesa:
            glContext = render->getCPUOpenGLContext();
            break;
        default:
            break;
    }


    // Bind the OpenGL context if there's any
    OSGLContextAttacherPtr glContextAttacher;
    if (glContext) {
        glContextAttacher = OSGLContextAttacher::create(glContext);
        glContextAttacher->attach();
    }

    EffectOpenGLContextDataPtr glContextData;
    if (backendType == eRenderBackendTypeOpenGL ||
        backendType == eRenderBackendTypeOSMesa) {
        ActionRetCodeEnum stat = _publicInterface->attachOpenGLContext_public(_publicInterface->getCurrentRenderTime(), _publicInterface->getCurrentRenderView(), combinedScale, glContext, &glContextData);
        if (isFailureRetCode(stat)) {
            renderRetCode = stat;
        }
    }
    if (renderRetCode == eActionStatusOK) {

        renderRetCode = launchPluginRenderAndHostFrameThreading(requestData, glContext, glContextData, combinedScale, backendType, renderRects, cachedPlanes);

        if (backendType == eRenderBackendTypeOpenGL ||
            backendType == eRenderBackendTypeOSMesa) {
            // If the plug-in doesn't support concurrent OpenGL renders, release the lock that was taken in the call to attachOpenGLContext_public() above.
            // For safe plug-ins, we call dettachOpenGLContext_public when the effect is destroyed in Node::destroy() with the function EffectInstance::dettachAllOpenGLContexts().
            // If we were the last render to use this context, clear the data now

            if ( glContextData->getHasTakenLock() ||
                !_publicInterface->supportsConcurrentOpenGLRenders() ||
                glContextData.use_count() == 2) {

                _publicInterface->dettachOpenGLContext_public(glContext, glContextData);
            }
        }
    }
 
    return renderRetCode;
} // launchInternalRender




ActionRetCodeEnum
EffectInstance::Implementation::handleUpstreamFramesNeeded(const TreeRenderExecutionDataPtr& requestPassSharedData,
                                                           const FrameViewRequestPtr& requestPassData,
                                                           const RenderScale& proxyScale,
                                                           unsigned int mipMapLevel,
                                                           const RectD& roiCanonical,
                                                           const std::map<int, std::list<ImagePlaneDesc> >& neededInputLayers)
{
    // For all frames/views needed, recurse on inputs with the appropriate RoI

    // Get frames needed to recurse upstream
    TimeValue time = _publicInterface->getCurrentRenderTime();
    ViewIdx view = _publicInterface->getCurrentRenderView();

    FramesNeededMap framesNeeded;
    {
        GetFramesNeededResultsPtr results = requestPassData->getFramesNeededResults();
        if (!results) {
            ActionRetCodeEnum stat = _publicInterface->getFramesNeeded_public(time, view,  &results);
            if (isFailureRetCode(stat)) {
                return stat;
            }
            requestPassData->setFramesNeededResults(results);
        }
        results->getFramesNeeded(&framesNeeded);
    }

    RenderScale combinedScale = EffectInstance::getCombinedScale(mipMapLevel, proxyScale);

    // Compute the regions of interest in input for this RoI.
    // The regions of interest returned is only valid for this RoI, we don't cache it. Rather we cache on the input the bounding box
    // of all the calls of getRegionsOfInterest that were made down-stream so that the node gets rendered only once.
    RoIMap inputsRoi;
    {
        ActionRetCodeEnum stat = _publicInterface->getRegionsOfInterest_public(time, combinedScale, roiCanonical, view, &inputsRoi);
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }

    for (FramesNeededMap::const_iterator it = framesNeeded.begin(); it != framesNeeded.end(); ++it) {

        int inputNb = it->first;

        assert(inputNb != -1);

        EffectInstancePtr mainInstanceInput = _publicInterface->getInputMainInstance(inputNb);
        if (!mainInstanceInput) {
            continue;
        }

        const bool isOptional = _publicInterface->getNode()->isInputOptional(inputNb);

        ///There cannot be frames needed without components needed.
        const std::list<ImagePlaneDesc>* inputPlanesNeeded = 0;
        {
            std::map<int, std::list<ImagePlaneDesc> >::const_iterator foundCompsNeeded = neededInputLayers.find(inputNb);
            if ( foundCompsNeeded == neededInputLayers.end() ) {
                continue;
            }
            inputPlanesNeeded = &foundCompsNeeded->second;
        }
        
        if (inputPlanesNeeded->empty()) {
            continue;
        }

        RectD inputRoI;
        {
            RoIMap::iterator foundRoI = inputsRoi.find(inputNb);
            if (foundRoI == inputsRoi.end()) {
                // RoI not specified... use the same RoI as passed in argument
                inputRoI = roiCanonical;
            } else {
                inputRoI = foundRoI->second;
            }
        }

        if (inputRoI.isNull()) {
            continue;
        }

        AcceptedRequestConcatenationFlags concatenationFlags = getConcatenationFlagsForInput(inputNb);


        if ( inputRoI.isInfinite() ) {
            _publicInterface->getNode()->setPersistentMessage( eMessageTypeError, kNatronPersistentErrorInfiniteRoI, EffectInstance::tr("%1 asked for an infinite region of interest upstream.").arg( QString::fromUtf8( _publicInterface->getNode()->getScriptName_mt_safe().c_str() ) ).toStdString() );
            return eActionStatusFailed;
        } else {
            _publicInterface->getNode()->clearPersistentMessage(kNatronPersistentErrorInfiniteRoI);
        }
        bool inputIsContinuous = mainInstanceInput->canRenderContinuously();

        int nbRequestedFramesForInput = 0;
        {


            // For all views requested in input
            for (FrameRangesMap::const_iterator viewIt = it->second.begin(); viewIt != it->second.end(); ++viewIt) {

                // For all ranges in this view
                for (std::size_t range = 0; range < viewIt->second.size(); ++range) {


                    // If the range bounds are no integers and the range covers more than 1 frame (min != max),
                    // we have no clue of the interval we should use between the min and max.
                    if (viewIt->second[range].min != viewIt->second[range].max && viewIt->second[range].min != (int)viewIt->second[range].min) {
                        qDebug() << "WARNING:" <<  _publicInterface->getScriptName_mt_safe().c_str() << "is requesting a non integer frame range [" << viewIt->second[range].min << ","
                        << viewIt->second[range].max <<"], this is border-line and not specified if this is supported by OpenFX. Natron will render "
                        "this range assuming an interval of 1 between frame times.";

                    }

                    // For all frames in the range
                    for (double f = viewIt->second[range].min; f <= viewIt->second[range].max; f += 1.) {


                        TimeValue inputTime(f);
                        {
                            int roundedInputTime = std::floor(f + 0.5);
                            if (roundedInputTime != inputTime && !inputIsContinuous) {
                                inputTime = TimeValue(roundedInputTime);
                            }
                        }

                        EffectInstancePtr inputEffect = _publicInterface->getInputRenderEffect(inputNb, inputTime, viewIt->first);
                        if (!inputEffect) {
                            continue;
                        }


                        for (std::list<ImagePlaneDesc>::const_iterator planeIt = inputPlanesNeeded->begin(); planeIt != inputPlanesNeeded->end(); ++planeIt) {
                            FrameViewRequestPtr createdRequest;
                            ActionRetCodeEnum stat = inputEffect->requestRender(inputTime, viewIt->first, proxyScale, mipMapLevel, *planeIt, inputRoI, inputNb, concatenationFlags, requestPassData, requestPassSharedData, &createdRequest, 0);
                            if (isFailureRetCode(stat)) {
                                if (isOptional) {
                                    continue;
                                }
                                return stat;
                            }
                            ++nbRequestedFramesForInput;
                            if (nbRequestedFramesForInput >= NATRON_MAX_FRAMES_NEEDED_PRE_FETCHING) {
                                break;
                            }
                        } // for each plane needed
                        if (nbRequestedFramesForInput >= NATRON_MAX_FRAMES_NEEDED_PRE_FETCHING) {
                            break;
                        }
                    } // for all frames
                    if (nbRequestedFramesForInput >= NATRON_MAX_FRAMES_NEEDED_PRE_FETCHING) {
                        break;
                    }
                } // for all ranges
            } // for all views
        } // EffectInstance::NotifyInputNRenderingStarted_RAII
    } // for all inputs

    return eActionStatusOK;
} // handleUpstreamFramesNeeded


ActionRetCodeEnum
EffectInstance::requestRender(TimeValue timeInArgs,
                              ViewIdx view,
                              const RenderScale& proxyScale,
                              unsigned int mipMapLevel,
                              const ImagePlaneDesc& plane,
                              const RectD & roiCanonical,
                              int inputNbInRequester,
                              AcceptedRequestConcatenationFlags concatenationFlags,
                              const FrameViewRequestPtr& requesterFrameViewRequest,
                              const TreeRenderExecutionDataPtr& requestPassSharedData,
                              FrameViewRequestPtr* createdRequest,
                              EffectInstancePtr* createdRenderClone)
{
    // Requested time is rounded to an epsilon so we can be sure to find it again in getImage, accounting for precision
    TimeValue time =  roundImageTimeToEpsilon(timeInArgs);

    // Check that time and view can be rendered by this effect:
    // round the time to closest integer if the effect is not continuous
    {
        int roundedTime = std::floor(time + 0.5);


        // A non-continuous effect is identity on itself on nearest integer time
        if (roundedTime != time && !canRenderContinuously()) {
            // We do not cache it because for non continuous effects we only cache stuff at
            // valid frame times
            return requestRender(TimeValue(roundedTime), view, proxyScale, mipMapLevel, plane, roiCanonical, inputNbInRequester, concatenationFlags, requesterFrameViewRequest, requestPassSharedData, createdRequest, 0);
        }
    }

    // For each different time/view pairs in the TreeRender, we create a specific render clone. We need need to do so because most knob functions take a time/view in parameter that can only be
    // recovered locally on the effect. To avoid the use of thread local storage, we clone the minimal amount of data.
    //
    // The mipMapLevel, proxyScale and plane is an argument of the render action but we do not create a clone just for that, instead we create a FrameViewRequest object to identify the render request.
    // A render clone may concurrently render one or multiple FrameViewRequest.

    FrameViewRenderKey frameViewKey = {time, view, requestPassSharedData->getTreeRender()};
    EffectInstancePtr renderClone = toEffectInstance(createRenderClone(frameViewKey));
    if (!renderClone) {
        return eActionStatusFailed;
    }
    if (createdRenderClone) {
        *createdRenderClone = renderClone;
    }

    {


        // Set this clone as the input effect of the requester effect at the given time/view
        EffectInstancePtr requesterEffect;
        if (requesterFrameViewRequest) {
            requesterEffect = requesterFrameViewRequest->getEffect();
        }
        if (inputNbInRequester >= 0 && requesterEffect && requesterEffect != renderClone) {
            FrameViewPair p = {time, view};
            QMutexLocker k(&requesterEffect->_imp->renderData->lock);
            requesterEffect->_imp->renderData->renderInputs[inputNbInRequester].insert(std::make_pair(p, renderClone));
        }
    }

    {
        QMutexLocker k(&renderClone->_imp->renderData->lock);
        // Find a frame view request matching the mipmapLevel/proxyScale/plane
        FrameViewKey requestKey = {mipMapLevel, proxyScale, plane};
        FrameViewRequestMap::iterator foundMatchingRequest = renderClone->_imp->renderData->requests.find(requestKey);
        if (foundMatchingRequest != renderClone->_imp->renderData->requests.end()) {
            *createdRequest = foundMatchingRequest->second.lock();
            if (!*createdRequest) {
                renderClone->_imp->renderData->requests.erase(foundMatchingRequest);
            }
        }
        if (!*createdRequest) {
            // Create a request if it did not already exist
            createdRequest->reset(new FrameViewRequest(plane, mipMapLevel, proxyScale, renderClone, requestPassSharedData->getTreeRender()));
            renderClone->_imp->renderData->requests.insert(std::make_pair(requestKey, *createdRequest));
        }
    }
    if (requesterFrameViewRequest) {
        // Add the requester request as a listener of this request. This has to be done before the call to requestRenderInternal()
        // Since the function shouldCacheOutput() called inside depends on the number of listeners returned by getNumListeners()
        (*createdRequest)->addListener(requestPassSharedData, requesterFrameViewRequest);
    }


    // Lock the request so that multiple threads get a coherent status
    ActionRetCodeEnum stat;
    {
        stat = renderClone->requestRenderInternal(roiCanonical, concatenationFlags, *createdRequest, requesterFrameViewRequest, requestPassSharedData);
    }
    if (!isFailureRetCode(stat)) {
        // Add this frame/view as depdency of the requester
        if (requesterFrameViewRequest) {
            requesterFrameViewRequest->addDependency(requestPassSharedData, *createdRequest);
        }

        requestPassSharedData->addTaskToRender(*createdRequest);
    }
    return stat;
} // requestRender

ActionRetCodeEnum
EffectInstance::requestRenderInternal(const RectD & roiCanonical,
                                      AcceptedRequestConcatenationFlags concatenationFlags,
                                      const FrameViewRequestPtr& requestData,
                                      const FrameViewRequestPtr& requesterFrameViewRequest,
                                      const TreeRenderExecutionDataPtr& requestPassSharedData)
{

    boost::scoped_ptr<FrameViewRequestLocker> requestLocker(new FrameViewRequestLocker(requestData));

    TreeRenderPtr render = getCurrentRender();
    assert(render);


    // Some nodes do not support render-scale and can only render at scale 1.
    // If the render requested a mipmap level different than 0, we must render at mipmap level 0 then downscale to the requested
    // mipmap level.
    // If the render requested a proxy scale different than 1, we fail because we cannot render at scale 1 then resize at an arbitrary scale.

    const bool renderScaleSupport = supportsRenderScale();
    const bool renderFullScaleThenDownScale = !renderScaleSupport && requestData->getMipMapLevel() > 0;

    const RenderScale& proxyScale = requestData->getProxyScale();

    if (!renderScaleSupport && (proxyScale.x != 1. || proxyScale.y != 1.)) {
        getNode()->setPersistentMessage(eMessageTypeError, kNatronPersistentErrorProxyUnsupported, tr("This node does not support proxy scale. It can only render at full resolution").toStdString());
        return eActionStatusFailed;
    } else {
        getNode()->clearPersistentMessage(kNatronPersistentErrorProxyUnsupported);
    }

    const unsigned int mappedMipMapLevel = renderFullScaleThenDownScale ? 0 : requestData->getMipMapLevel();
    requestData->setRenderMappedMipMapLevel(mappedMipMapLevel);
    RenderScale originalCombinedScale = EffectInstance::getCombinedScale(requestData->getMipMapLevel(), proxyScale);
    const RenderScale mappedCombinedScale = renderFullScaleThenDownScale ? RenderScale(1.) : originalCombinedScale;

    const double par = getAspectRatio(-1);


    // Get the region of definition of the effect at this frame/view in canonical coordinates
    std::vector<RectD> perMipMapLevelRoDCanonical;
    // The RoD in pixel coordinates at the scale of mappedCombinedScale
    std::vector<RectI> perMipMapLevelRoDPixel;

    if (!requestData->getRoDAtEachMipMapLevel(&perMipMapLevelRoDCanonical, &perMipMapLevelRoDPixel)) {
        perMipMapLevelRoDCanonical.resize(requestData->getMipMapLevel() + 1);
        perMipMapLevelRoDPixel.resize(perMipMapLevelRoDCanonical.size());
        for (std::size_t m = 0; m < perMipMapLevelRoDCanonical.size(); ++m) {
            GetRegionOfDefinitionResultsPtr results;
            RenderScale levelCombinedScale = EffectInstance::getCombinedScale(m, proxyScale);
            {
                ActionRetCodeEnum stat = getRegionOfDefinition_public(getCurrentRenderTime(), levelCombinedScale, getCurrentRenderView(), &results);
                if (isFailureRetCode(stat)) {
                    return stat;
                }
            }
            perMipMapLevelRoDCanonical[m] = results->getRoD();

            // If the plug-in RoD is null, there's nothing to render.
            if (perMipMapLevelRoDCanonical[m].isNull()) {
                return eActionStatusInputDisconnected;
            }


            perMipMapLevelRoDCanonical[m].toPixelEnclosing(levelCombinedScale, par, &perMipMapLevelRoDPixel[m]);
        }
        requestData->setRoDAtEachMipMapLevel(perMipMapLevelRoDCanonical, perMipMapLevelRoDPixel);
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Handle pass-through planes /////////////////////////////////////////////////////////////
    std::map<int, std::list<ImagePlaneDesc> > inputLayersNeeded;
    {
        bool isPassThrough;
        ActionRetCodeEnum upstreamRetCode = _imp->handlePassThroughPlanes(requestData, requestPassSharedData, roiCanonical, concatenationFlags, &inputLayersNeeded, &isPassThrough);
        if (isFailureRetCode(upstreamRetCode)) {
            return upstreamRetCode;
        }

        // There might no plane produced by this node that were requested
        if (isPassThrough) {
            requestData->initStatus(FrameViewRequest::eFrameViewRequestStatusPassThrough);
            return eActionStatusOK;
        }
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Handle identity effects /////////////////////////////////////////////////////////////////
    {
        bool isIdentity;
        ActionRetCodeEnum upstreamRetCode = _imp->handleIdentityEffect(par, perMipMapLevelRoDCanonical[mappedMipMapLevel], mappedCombinedScale, roiCanonical, requestData, requestPassSharedData, concatenationFlags, &isIdentity);
        if (isFailureRetCode(upstreamRetCode)) {
            return upstreamRetCode;
        }
        if (isIdentity) {
            requestData->initStatus(FrameViewRequest::eFrameViewRequestStatusPassThrough);
            return eActionStatusOK;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Handle Concatenations //////////////////////////////////////////////////////////////////
    {

        bool concatenated;
        ActionRetCodeEnum upstreamRetCode = _imp->handleConcatenation(requestPassSharedData, requestData, concatenationFlags, mappedCombinedScale, roiCanonical, render->isDraftRender(), &concatenated);
        if (isFailureRetCode(upstreamRetCode)) {
            return upstreamRetCode;
        }
        if (concatenated) {
            requestData->initStatus(FrameViewRequest::eFrameViewRequestStatusPassThrough);
            return eActionStatusOK;

        }
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Compute RoI  ///////////////////////////////////////////////////

    // This is the region to render in pixel coordinates at the scale of renderMappedScale
    RectI renderMappedRoI;

    // Get the pixel RoD/RoI at the mipmap level requested
    RectI downscaledRoI;

    RenderScale downscaledCombinedScale = EffectInstance::getCombinedScale(requestData->getMipMapLevel(), requestData->getProxyScale());

    // Round the roi to the tile size if the render is cached
    ImageBitDepthEnum outputBitDepth = getBitDepth(-1);
    int tileWidth, tileHeight;
    CacheBase::getTileSizePx(outputBitDepth, &tileWidth, &tileHeight);


    if (!supportsTiles()) {
        // If tiles are not supported the RoI is the full image bounds
        renderMappedRoI = perMipMapLevelRoDPixel[mappedMipMapLevel];
        downscaledRoI = perMipMapLevelRoDPixel[requestData->getMipMapLevel()];
    } else {


        roiCanonical.toPixelEnclosing(downscaledCombinedScale, par, &downscaledRoI);

        // Round the downscaled roi to the tile size
        downscaledRoI.roundToTileSize(tileWidth, tileHeight);

        // Make sure the RoI falls within the image bounds
        if ( !downscaledRoI.intersect(perMipMapLevelRoDPixel[requestData->getMipMapLevel()], &downscaledRoI) ) {
            requestData->initStatus(FrameViewRequest::eFrameViewRequestStatusRendered);
            return eActionStatusOK;
        }


        if (renderScaleSupport) {
            renderMappedRoI = downscaledRoI;
        } else {
            renderMappedRoI = downscaledRoI.upscalePowerOfTwo(requestData->getMipMapLevel());
            renderMappedRoI.roundToTileSize(tileWidth, tileHeight);

            // Make sure the RoI falls within the image bounds
            if ( !renderMappedRoI.intersect(perMipMapLevelRoDPixel[mappedMipMapLevel], &renderMappedRoI) ) {
                requestData->initStatus(FrameViewRequest::eFrameViewRequestStatusRendered);
                return eActionStatusOK;
            }

        }
    } // supportsTiles

    // The RoI cannot be null here, we checked it before
    assert(!downscaledRoI.isNull());
    assert(!renderMappedRoI.isNull());

    // The requested portion to render in canonical coordinates
    RectD roundedCanonicalRoI;
    renderMappedRoI.toCanonical(mappedCombinedScale, par, perMipMapLevelRoDCanonical[mappedMipMapLevel], &roundedCanonicalRoI);


    // Check that the image rendered in output is always rounded to the tile size intersected to the RoD
    assert(renderMappedRoI.x1 % tileWidth == 0 || renderMappedRoI.x1 == perMipMapLevelRoDPixel[mappedMipMapLevel].x1);
    assert(renderMappedRoI.y1 % tileHeight == 0 || renderMappedRoI.y1 == perMipMapLevelRoDPixel[mappedMipMapLevel].y1);
    assert(renderMappedRoI.x2 % tileWidth == 0 || renderMappedRoI.x2 == perMipMapLevelRoDPixel[mappedMipMapLevel].x2);
    assert(renderMappedRoI.y2 % tileHeight == 0 || renderMappedRoI.y2 == perMipMapLevelRoDPixel[mappedMipMapLevel].y2);

    // Merge the roi requested onto the existing RoI requested for this frame/view

    {
        RectD curRoI = requestData->getCurrentRoI();

        FrameViewRequest::FrameViewRequestStatusEnum status = requestData->getStatus();

        if (status == FrameViewRequest::eFrameViewRequestStatusRendered && curRoI.contains(roiCanonical)) {
            // The request is already rendered and contains the requested roi, return
            return eActionStatusOK;
        }

        if (curRoI.isNull()) {
            curRoI = roundedCanonicalRoI;
        } else {
            curRoI.merge(roundedCanonicalRoI);
        }
        requestData->setCurrentRoI(curRoI);
    }




    // Check for abortion before checking cache
    if (isRenderAborted()) {
        return eActionStatusAborted;
    }


    const bool isAccumulating = isAccumulationEnabled();


    // Should the output of this render be cached ?
    CacheAccessModeEnum cachePolicy;
    if (isAccumulating) {
        // For now we only cache images that were rendered on the CPU
        // When accumulation is enabled we also disable caching as the image is anyway held as a member on the effect
        cachePolicy = eCacheAccessModeNone;
    } else {
        if (renderFullScaleThenDownScale) {
            // Always cache effects that do not support render scale
            bool cacheWriteOnly = requestData->checkIfByPassCacheEnabledAndTurnoff();
            if (cacheWriteOnly) {
                cachePolicy = eCacheAccessModeWriteOnly;
            } else {
                cachePolicy = eCacheAccessModeReadWrite;
            }
        } else {
            cachePolicy = _imp->shouldRenderUseCache(requestPassSharedData, requestData);
        }
    }

    // Get the render device
    RenderBackendTypeEnum backendType;
    if (requestData->isFallbackRenderDeviceEnabled()) {
        backendType = requestData->getFallbackRenderDevice();
    } else {
        if (!requestData->isRenderDeviceSet()) {
            _imp->resolveRenderBackend(requestPassSharedData, requestData, renderMappedRoI, &cachePolicy, &backendType);
            requestData->setRenderDevice(backendType);
        } else {
            backendType = requestData->getRenderDevice();
        }
    }


    requestData->setCachePolicy(cachePolicy);
    
    

    // Get the image on the FrameViewRequest
    // If this request was already rendered once in the tree,
    // the image already exists, but it doesn't mean the area rendered on the image or its render device matches
    // what was requested
    //
    // Generally if the image is valid, this means we are within a call to getImagePlane()

    FrameViewRequest::FrameViewRequestStatusEnum requestStatus = FrameViewRequest::eFrameViewRequestStatusNotRendered;


    ImagePtr requestedImageScale = requestData->getRequestedScaleImagePlane();
    ImagePtr fullScaleImage = requestData->getFullscaleImagePlane();

    // The image must have a cache entry object, even if the policy is eCacheAccessModeNone
    // so we can sync concurrent threads to render the same image.
    assert(!requestedImageScale || requestedImageScale->getCacheEntry());
    assert(!fullScaleImage || fullScaleImage->getCacheEntry());

    // If the image does not match the render device, wipe it and make a new one
    if (!requestedImageScale) {
        // If we are in a concurrent host frame threading render and we need to modify the output image,
        // just launch a new render to ensure thread safety.
        if (requestPassSharedData->isNewTreeRenderUponUnrenderedImageEnabled()) {
            return _imp->launchIsolatedRender(requestData);
        }
    } else {

        bool differentStorage = requestedImageScale->getStorageMode() != EffectInstance::Implementation::storageModeFromBackendType(backendType);

        if (differentStorage) {

            // If we are in a concurrent host frame threading render and we need to modify the output image,
            // just launch a new render to ensure thread safety.
            if (requestPassSharedData->isNewTreeRenderUponUnrenderedImageEnabled()) {
                return _imp->launchIsolatedRender(requestData);
            } else {
                requestedImageScale.reset();
                fullScaleImage.reset();

            }
        }
    }
    

    // When accumulating, re-use the same buffer of previous steps and resize it if needed.
    // Note that in this mode only a single plane can be rendered at once and the plug-in render safety must
    // allow only a single thread to run
    ImagePtr accumBuffer = getAccumBuffer(requestData->getPlaneDesc());
    if ((isAccumulating && accumBuffer && accumBuffer->getMipMapLevel() != mappedMipMapLevel)) {
        accumBuffer.reset();
    }

    if (isAccumulating && accumBuffer) {

        accumBuffer->updateRenderCloneAndImage(shared_from_this());
        // When drawing with a paint brush, we may only render the bounding box of the un-rendered points.
        RectD updateAreaCanonical;
        if (getAccumulationUpdateRoI(&updateAreaCanonical)) {

            RectI updateAreaPixel;
            updateAreaCanonical.toPixelEnclosing(mappedCombinedScale, par, &updateAreaPixel);
            updateAreaPixel.intersect(renderMappedRoI, &updateAreaPixel);

            // Notify the TreeRender of the update area so that in turn the OpenGL viewer can update only the required portion
            // of the texture
            //qDebug() << getScriptName_mt_safe().c_str() << "update area";
            //updateAreaCanonical.debug();
            if (updateAreaPixel.isNull()) {
                // Nothing to update, return quickly
                requestData->initStatus(FrameViewRequest::eFrameViewRequestStatusRendered);
                requestData->setRequestedScaleImagePlane(accumBuffer);
                return eActionStatusOK;
            }
            // If this is the first time we compute this frame view request, erase in the tiles state map the portion that was drawn
            // by the user,
            if (!requestedImageScale) {
                // Get the accum buffer on the node. Note that this is not concurrent renders safe.
                requestedImageScale = accumBuffer;
                requestedImageScale->getCacheEntry()->markCacheTilesInRegionAsNotRendered(updateAreaPixel);

            }
        }

    } // isAccumulating


    // Evaluate the tiles state map on the image to check what's left to render and fetch tiles from cache
    // Note that no memory allocation is done here, only existing tiles are fetched from the cache.

    // If this effect doesn't support render scale, look in the cache first if there's an image
    // at our desired mipmap level
    bool hasUnRenderedTile = true;
    bool hasPendingTiles = false;

    bool lookupReadOnly = requestPassSharedData->isNewTreeRenderUponUnrenderedImageEnabled();
    ActionRetCodeEnum stat = _imp->lookupCachedImage(requestData->getMipMapLevel(), requestData->getProxyScale(), requestData->getPlaneDesc(), perMipMapLevelRoDPixel, downscaledRoI, cachePolicy, backendType, lookupReadOnly, &requestedImageScale, &hasPendingTiles, &hasUnRenderedTile);
    if (isFailureRetCode(stat)) {
        return stat;
    }

    // If we are in a concurrent host frame threading render and we need to modify the output image,
    // just launch a new render to ensure thread safety.
    if (lookupReadOnly && (hasUnRenderedTile || hasPendingTiles)) {
        return _imp->launchIsolatedRender(requestData);
    }

    if (!hasPendingTiles && !hasUnRenderedTile) {
        requestStatus = FrameViewRequest::eFrameViewRequestStatusRendered;
    } else if (mappedMipMapLevel != requestData->getMipMapLevel()) {

        // The previous lookupCachedImage() call marked the tiles as pending, but we are not going to compute now
        // so unmark them, instead do it once we rendered the full scale image
        requestedImageScale->getCacheEntry()->markCacheTilesAsAborted();
        requestedImageScale.reset();

        stat = _imp->lookupCachedImage(mappedMipMapLevel, requestData->getProxyScale(), requestData->getPlaneDesc(), perMipMapLevelRoDPixel, renderMappedRoI, cachePolicy, backendType, false, &fullScaleImage, &hasPendingTiles, &hasUnRenderedTile);
        if (isFailureRetCode(stat)) {
            return stat;
        }

        // Do not set the renderStatus to FrameViewRequest::eFrameViewRequestStatusRendered because
        // we still need to downscale the image
    }


    // If the node supports render scale, make the fullScaleImage point to the requestedImageScale so that we don't
    // ruin the render code of if/else
    if (!fullScaleImage) {
        fullScaleImage = requestedImageScale;
    }
    assert(fullScaleImage);
    requestData->setRequestedScaleImagePlane(requestedImageScale);
    requestData->setFullscaleImagePlane(fullScaleImage);


    // Set the accumulation buffer if:
    // - The effect accumulates and it does not yet have a buffer
    // - The effect is no longer accumulating but has a buffer, in which case we update the accumulation buffer.
    if ((isAccumulating && !accumBuffer) || (!isAccumulating && accumBuffer)) {
        setAccumBuffer(requestData->getPlaneDesc(), requestedImageScale);
    }

    requestData->initStatus(requestStatus);

    // If there's nothing to render, do not even add the inputs as needed dependencies.
    if (requestStatus == FrameViewRequest::eFrameViewRequestStatusNotRendered) {

        ActionRetCodeEnum upstreamRetCode = _imp->handleUpstreamFramesNeeded(requestPassSharedData, requestData, proxyScale, mappedMipMapLevel, roundedCanonicalRoI, inputLayersNeeded);
        
        if (isFailureRetCode(upstreamRetCode)) {
            return upstreamRetCode;
        }
    }
    return eActionStatusOK;
} // requestRenderInternal



ActionRetCodeEnum
EffectInstance::launchNodeRender(const TreeRenderExecutionDataPtr& requestPassSharedData, const FrameViewRequestPtr& requestData)
{

    {
        FrameViewRequest::FrameViewRequestStatusEnum requestStatus = requestData->getStatus();
        switch (requestStatus) {
            case FrameViewRequest::eFrameViewRequestStatusRendered:
            case FrameViewRequest::eFrameViewRequestStatusPassThrough:
                return eActionStatusOK;
            case FrameViewRequest::eFrameViewRequestStatusPending: {
                // We should NEVER be recursively computing twice the same FrameViewRequest.
                assert(false);
                return eActionStatusFailed;
            }
            case FrameViewRequest::eFrameViewRequestStatusNotRendered:
                break;
        }
    }
    FrameViewRequestLocker requestLocker(requestData);
    {
        FrameViewRequest::FrameViewRequestStatusEnum requestStatus = requestData->notifyRenderStarted();
        assert(requestStatus == FrameViewRequest::eFrameViewRequestStatusNotRendered);
        (void)requestStatus;
    }
    ActionRetCodeEnum stat = launchRenderInternal(requestPassSharedData, requestData);

#ifdef DEBUG
    if (stat == eActionStatusOK) {
        ImageCacheEntryPtr cacheEntry = requestData->getRequestedScaleImagePlane()->getCacheEntry();
        if (cacheEntry) {
            bool hasUnrenderedTile, hasPendingResults;
            cacheEntry->getStatus(0, &hasUnrenderedTile, &hasPendingResults);
            assert(!hasPendingResults);
        }
    }
#endif

    // Notify that we are done rendering
    requestData->notifyRenderFinished(stat);
    return stat;
} // launchNodeRender

static void finishProducedPlanesTilesStatesMap(const std::map<ImagePlaneDesc, ImagePtr>& producedPlanes,
                                               bool aborted)
{
    for (std::map<ImagePlaneDesc, ImagePtr>::const_iterator it = producedPlanes.begin(); it!=producedPlanes.end(); ++it) {
        ImageCacheEntryPtr entry = it->second->getCacheEntry();
        if (aborted) {
            entry->markCacheTilesAsAborted();
        } else {
            entry->markCacheTilesAsRendered();
        }
    }
}

ActionRetCodeEnum
EffectInstance::launchRenderInternal(const TreeRenderExecutionDataPtr& requestPassSharedData, const FrameViewRequestPtr& requestData)
{
    assert(isRenderClone() && getCurrentRender());

    const double par = getAspectRatio(-1);
    const unsigned int mappedMipMapLevel = requestData->getRenderMappedMipMapLevel();
    const RenderScale mappedCombinedScale = EffectInstance::getCombinedScale(mappedMipMapLevel, requestData->getProxyScale());

    RectI renderMappedRoI;
    requestData->getCurrentRoI().toPixelEnclosing(mappedCombinedScale, par, &renderMappedRoI);


    // Get the region of definition of the effect at this frame/view in canonical coordinates
    std::vector<RectD> perMipMapLevelRoDCanonical(requestData->getMipMapLevel() + 1);

    // The RoD in pixel coordinates at the scale of mappedCombinedScale
    std::vector<RectI> perMipMapLevelRoDPixel(perMipMapLevelRoDCanonical.size());

    // The RoD must have been computed already in requestRenderInternal
    bool gotRoD = requestData->getRoDAtEachMipMapLevel(&perMipMapLevelRoDCanonical, &perMipMapLevelRoDPixel);
    assert(gotRoD);
    (void)gotRoD;


#ifdef DEBUG
    // Check that the image rendered in output is always rounded to the tile size intersected to the RoD
    {
        ImageBitDepthEnum outputBitDepth = getBitDepth(-1);
        int tileWidth, tileHeight;
        CacheBase::getTileSizePx(outputBitDepth, &tileWidth, &tileHeight);
        assert(renderMappedRoI.x1 % tileWidth == 0 || renderMappedRoI.x1 == perMipMapLevelRoDPixel[mappedMipMapLevel].x1);
        assert(renderMappedRoI.y1 % tileHeight == 0 || renderMappedRoI.y1 == perMipMapLevelRoDPixel[mappedMipMapLevel].y1);
        assert(renderMappedRoI.x2 % tileWidth == 0 || renderMappedRoI.x2 == perMipMapLevelRoDPixel[mappedMipMapLevel].x2);
        assert(renderMappedRoI.y2 % tileHeight == 0 || renderMappedRoI.y2 == perMipMapLevelRoDPixel[mappedMipMapLevel].y2);
    }
#endif


    // Fetch or create a cache image for all other planes that the plug-in produces but are not requested
    std::map<ImagePlaneDesc, ImagePtr> cachedImagePlanes;
    assert(requestData->getComponentsResults());
    const std::list<ImagePlaneDesc>& producedPlanes = requestData->getComponentsResults()->getProducedPlanes();

    // This is the requested image plane
    ImagePtr fullscalePlane = requestData->getFullscaleImagePlane();
    // Allocate the cache storage image now if it was not yet allocated
    assert(fullscalePlane);
    fullscalePlane->ensureBuffersAllocated();

    cachedImagePlanes[requestData->getPlaneDesc()] = fullscalePlane;

    RenderBackendTypeEnum backendType = requestData->getRenderDevice();

    const bool renderAllProducedPlanes = isRenderAllPlanesAtOncePreferred();

    for (std::list<ImagePlaneDesc>::const_iterator it = producedPlanes.begin(); it != producedPlanes.end(); ++it) {
        ImagePtr imagePlane;
        if (*it == requestData->getPlaneDesc()) {
            continue;
        } else {
            if (!renderAllProducedPlanes) {
                continue;
            } else {
                imagePlane = _imp->createCachedImage(renderMappedRoI, perMipMapLevelRoDPixel, mappedMipMapLevel, requestData->getProxyScale(), *it, backendType, requestData->getCachePolicy(), false /*delayAllocation*/);
                ActionRetCodeEnum stat = imagePlane->getCacheEntry()->fetchCachedTilesAndUpdateStatus(false, NULL, NULL, NULL);
                if (isFailureRetCode(stat)) {
                    finishProducedPlanesTilesStatesMap(cachedImagePlanes, true);
                    return stat;
                }
            }
        }
        cachedImagePlanes[*it] = imagePlane;
    }



    ActionRetCodeEnum renderRetCode = eActionStatusOK;
    std::list<RectToRender> renderRects;
    bool hasPendingTiles;

    // Initialize what's left to render, without fetching the tiles state map from the cache because it was already fetched in
    // requestRender()
    renderRetCode = _imp->checkRestToRender(false /*updateTilesStateFromCache*/, requestData, renderMappedRoI, mappedCombinedScale, cachedImagePlanes, &renderRects, &hasPendingTiles);
    if (isFailureRetCode(renderRetCode)) {
        finishProducedPlanesTilesStatesMap(cachedImagePlanes, true);
        return renderRetCode;
    }



    while ((!renderRects.empty() || hasPendingTiles) && !isRenderAborted()) {

        // There may be no rectangles to render if all rectangles are pending (i.e: this render should wait for another thread
        // to complete the render first)
        if (!renderRects.empty()) {
            renderRetCode = _imp->launchRenderForSafetyAndBackend(requestData, mappedCombinedScale, backendType, renderRects, cachedImagePlanes);
        }

        if (isFailureRetCode(renderRetCode)) {
            finishProducedPlanesTilesStatesMap(cachedImagePlanes, true);
            break;
        }

        // Mark what we rendered in the tiles state map
        finishProducedPlanesTilesStatesMap(cachedImagePlanes, false /*aborted*/);

        // Wait for any pending results for the requested plane.
        // After this line other threads that should have computed should be done
        if (fullscalePlane->getCacheEntry()->waitForPendingTiles()) {
            hasPendingTiles = false;
            renderRects.clear();
        } else {

            if (isRenderAborted()) {
                finishProducedPlanesTilesStatesMap(cachedImagePlanes, true /*aborted*/);
                return eActionStatusAborted;
            }

            // Re-fetch the tiles state from the cache which may have changed now
            renderRetCode = _imp->checkRestToRender(true /*updateTilesStateFromCache*/, requestData, renderMappedRoI, mappedCombinedScale, cachedImagePlanes, &renderRects, &hasPendingTiles);
        }
        
    } // while there is still something not rendered

    if (isFailureRetCode(renderRetCode) || isRenderAborted()) {
        if (requestData->getCachePolicy() != eCacheAccessModeNone) {
            finishProducedPlanesTilesStatesMap(cachedImagePlanes, true /*aborted*/);
        }
        return isFailureRetCode(renderRetCode) ? renderRetCode : eActionStatusAborted;
    }

    // If using GPU and out of memory retry on CPU if possible
    if (renderRetCode == eActionStatusOutOfMemory && !renderRects.empty() && backendType == eRenderBackendTypeOpenGL) {

        if (backendType == requestData->getFallbackRenderDevice()) {
            // The fallback device is the same as the device that just rendered, fail
            return eActionStatusOutOfMemory;
        }
        if (requestData->isFallbackRenderDeviceEnabled()) {
            // We already tried the fallback device and it didn't work out too.
            return eActionStatusOutOfMemory;
        }
        requestData->setFallbackRenderDeviceEnabled(true);
        TreeRenderPtr render = getCurrentRender();

        // Recurse by calling launchRenderWithArgs(), this will call requestRender() and this function again.
        RectD roi = requestData->getCurrentRoI();
        ImagePlaneDesc plane = requestData->getPlaneDesc();

        TreeRenderExecutionDataPtr execData = launchSubRender(shared_from_this(), getCurrentRenderTime(), getCurrentRenderView(), requestData->getProxyScale(), requestData->getMipMapLevel(), &plane, &roi, render, eAcceptedRequestConcatenationNone, requestPassSharedData->isNewTreeRenderUponUnrenderedImageEnabled());
        ActionRetCodeEnum stat = render->getStatus();
        return stat;

    }

    if (renderRetCode != eActionStatusOK) {
        return renderRetCode;
    }


    // If the node did not support render scale and the mipmap level rendered was different than what was requested, downscale the image.
    unsigned int dstMipMapLevel = requestData->getMipMapLevel();
    if (mappedMipMapLevel != dstMipMapLevel) {

        RenderScale downscaledCombinedScale = EffectInstance::getCombinedScale(dstMipMapLevel,requestData->getProxyScale());

        RectI downscaledRoI;
        requestData->getCurrentRoI().toPixelEnclosing(downscaledCombinedScale, par, &downscaledRoI);


        ImageBitDepthEnum outputBitDepth = getBitDepth(-1);
        int tileWidth, tileHeight;
        CacheBase::getTileSizePx(outputBitDepth, &tileWidth, &tileHeight);
        downscaledRoI.roundToTileSize(tileWidth, tileHeight);
        // Make sure the RoI falls within the image bounds
        if ( !downscaledRoI.intersect(perMipMapLevelRoDPixel[requestData->getMipMapLevel()], &downscaledRoI) ) {
            return eActionStatusOK;
        }

        // Since the node does not support render scale, we cached the image, thus we can just fetch the image
        // at a our originally requested mipmap level, this will downscale the fullscale image and cache the
        // mipmapped version automatically

        ImagePtr downscaledImage;

        bool hasUnrenderedTile, hasPendingTiles;
        ActionRetCodeEnum stat = _imp->lookupCachedImage(dstMipMapLevel, requestData->getProxyScale(), requestData->getPlaneDesc(), perMipMapLevelRoDPixel, downscaledRoI, eCacheAccessModeReadWrite, backendType, false, &downscaledImage, &hasPendingTiles, &hasUnrenderedTile);

        if (isFailureRetCode(stat)) {
            return stat;
        }

        // We just rendered the full scale version, no tiles should be marked unrendered.
        // In some cases, the image might no longer be in the Cache if the Cache was cleared. In this case, manually downscale
        // the image
        if (hasUnrenderedTile) {
            downscaledImage->getCacheEntry()->markCacheTilesAsAborted();
            downscaledImage = fullscalePlane->downscaleMipMap(fullscalePlane->getBounds(), dstMipMapLevel - mappedMipMapLevel);
        } else {
            // However another thread could have marked pending the tiles at dstMipMapLevel in between, thus we just have to wait for it to be read
            if (hasPendingTiles) {
                if (!downscaledImage->getCacheEntry()->waitForPendingTiles()) {
                    downscaledImage->getCacheEntry()->markCacheTilesAsAborted();
                    return eActionStatusAborted;
                }
            }
            downscaledImage->getCacheEntry()->markCacheTilesAsRendered();
        }




        requestData->setRequestedScaleImagePlane(downscaledImage);
    }
    //QString name = QString::fromUtf8(getScriptName_mt_safe().c_str()) + QString::fromUtf8("_") + QString::number(getCurrentRenderTime()) + QString::fromUtf8("_") +  QDateTime::currentDateTime().toString() + QString::fromUtf8(".png");
    //if (isDuringPaintStrokeCreation()) {
    /*if (getNode()->getPluginID() == PLUGINID_OFX_ROTOMERGE) {
        QString name = QString::number((U64)getCurrentRender().get()) +  QString::fromUtf8("_") + QString::fromUtf8(getScriptName_mt_safe().c_str()) + QString::fromUtf8(".png") ;
        appPTR->debugImage(requestData->getRequestedScaleImagePlane(), requestData->getRequestedScaleImagePlane()->getBounds(), name);
    }*/
    //}
    return isRenderAborted() ? eActionStatusAborted : eActionStatusOK;
} // launchRenderInternal



class HostFrameThreadingRenderProcessor : public MultiThreadProcessorBase
{

    std::vector<RectToRender> _rectsToRender;
    boost::shared_ptr<EffectInstance::Implementation::TiledRenderingFunctorArgs> _args;
    EffectInstance::Implementation* _imp;

public:

    HostFrameThreadingRenderProcessor(const EffectInstancePtr& renderClone)
    : MultiThreadProcessorBase(renderClone)
    {

    }

    virtual ~HostFrameThreadingRenderProcessor()
    {

    }

    void setData(const std::list<RectToRender> &rectsToRender, const boost::shared_ptr<EffectInstance::Implementation::TiledRenderingFunctorArgs>& args, EffectInstance::Implementation* imp)
    {
        int i = 0;
        _rectsToRender.resize(rectsToRender.size());
        for (std::list<RectToRender>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it, ++i) {
            _rectsToRender[i] = *it;
        }
        _args = args;
        _imp = imp;
    }


    virtual ActionRetCodeEnum multiThreadFunction(unsigned int threadID,
                                                  unsigned int nThreads) OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        // If this plug-in has TLS, clear the action stack since it has been copied from the caller thread.
        EffectInstanceTLSDataPtr tlsData = _imp->_publicInterface->getTLSObject();
        if (tlsData) {
            tlsData->clearActionStack();
        }
        int fromIndex, toIndex;
        ImageMultiThreadProcessorBase::getThreadRange(threadID, nThreads, 0, _rectsToRender.size(), &fromIndex, &toIndex);
        for (int i = fromIndex; i < toIndex; ++i) {
            ActionRetCodeEnum stat = _imp->tiledRenderingFunctor(_rectsToRender[i], *_args);
            if (isFailureRetCode(stat)) {
                return stat;
            }
        }
        return eActionStatusOK;
    }
};

ActionRetCodeEnum
EffectInstance::Implementation::launchPluginRenderAndHostFrameThreading(const FrameViewRequestPtr& requestData,
                                                                        const OSGLContextPtr& glContext,
                                                                        const EffectOpenGLContextDataPtr& glContextData,
                                                                        const RenderScale& combinedScale,
                                                                        RenderBackendTypeEnum backendType,
                                                                        const std::list<RectToRender>& renderRects,
                                                                        const std::map<ImagePlaneDesc, ImagePtr>& cachedPlanes)
{
    assert( !renderRects.empty() );
    
    // Notify the gui we're rendering
    NotifyRenderingStarted_RAII renderingNotifier(_publicInterface->getNode().get());

    // If this node is not sequential we at least have to bracket the render action with a call to begin and end sequence render.
    RangeD sequenceRange;
    {
        GetFrameRangeResultsPtr rangeResults;
        ActionRetCodeEnum stat = _publicInterface->getFrameRange_public(&rangeResults);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        rangeResults->getFrameRangeResults(&sequenceRange);
    }

    TreeRenderPtr render = _publicInterface->getCurrentRender();
    
    // We only need to call begin if we've not already called it.
    bool callBeginSequenceRender = false;
    if ( !_publicInterface->isWriter() || (_publicInterface->getSequentialRenderSupport() == eSequentialPreferenceNotSequential) ) {
        callBeginSequenceRender = true;
    }

    bool isPlayback = render->isPlayback();
    TimeValue time = _publicInterface->getCurrentRenderTime();


    if (callBeginSequenceRender) {
        ActionRetCodeEnum stat = _publicInterface->beginSequenceRender_public(time,
                                                                              time,
                                                                              1 /*frameStep*/,
                                                                              !appPTR->isBackground() /*interactive*/,
                                                                              combinedScale,
                                                                              isPlayback,
                                                                              !isPlayback,
                                                                              render->isDraftRender(),
                                                                              _publicInterface->getCurrentRenderView(),
                                                                              backendType,
                                                                              glContextData);
        
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }



#ifdef NATRON_HOSTFRAMETHREADING_SEQUENTIAL
    const bool attemptHostFrameThreading = false;
#else
    const bool attemptHostFrameThreading = _publicInterface->getRenderThreadSafety() == eRenderSafetyFullySafeFrame &&
                                           renderRects.size() > 1 &&
                                           backendType == eRenderBackendTypeCPU;
#endif


    boost::shared_ptr<TiledRenderingFunctorArgs> functorArgs(new TiledRenderingFunctorArgs);
    functorArgs->glContextData = glContextData;
    functorArgs->glContext = glContext;
    functorArgs->requestData = requestData;
    functorArgs->cachedPlanes = cachedPlanes;
    functorArgs->backendType = backendType;

    // Pre-fetch all input images that will be needed by identity rectangles so that the call to getImagePlane
    // is done once for each of them.

    // Get the bbox to fetch in input for identity rectangles
    RectI identityRectanglesBbox;
    for (std::list<RectToRender>::const_iterator it = renderRects.begin(); it != renderRects.end(); ++it) {
        if (it->identityInputNumber == -1) {
            continue;
        }
        if (identityRectanglesBbox.isNull()) {
            identityRectanglesBbox = it->rect;
        } else {
            identityRectanglesBbox.merge(it->rect);
        }
    }
    for (std::list<RectToRender>::const_iterator it = renderRects.begin(); it != renderRects.end(); ++it) {
        if (it->identityInputNumber == -1) {
            continue;
        }

        // Use the plane specified with the getLayersProducedAndNeeded action
        //const std::map<int, std::list<ImagePlaneDesc> >& neededInputPlanes = requestData->getComponentsResults()->getNeededInputPlanes();
        //std::map<int, std::list<ImagePlaneDesc> >::const_iterator foundInputPlane = neededInputPlanes.find(it->identityInputNumber);

        for (std::map<ImagePlaneDesc, ImagePtr>::const_iterator it2 = cachedPlanes.begin(); it2 != cachedPlanes.end(); ++it2) {
            IdentityPlaneKey p;
            p.identityInputNb = it->identityInputNumber;
            /*if (foundInputPlane != neededInputPlanes.end() && foundInputPlane->second.size() > 0) {
                p.identityPlane = foundInputPlane->second.front();
            } else {
                p.identityPlane = it2->first;
            }*/
            p.identityPlane = it->identityPlane;
            p.identityTime = it->identityTime;
            p.identityView = it->identityView;
            IdentityPlanesMap::const_iterator foundFetchedPlane = functorArgs->identityPlanes.find(p);
            if (foundFetchedPlane != functorArgs->identityPlanes.end()) {
                // It was already fetched
                continue;
            }

            boost::scoped_ptr<EffectInstance::GetImageInArgs> inArgs( new EffectInstance::GetImageInArgs() );
            inArgs->renderBackend = &backendType;
            inArgs->currentRenderWindow = &identityRectanglesBbox;
            inArgs->inputTime = &it->identityTime;
            inArgs->inputView = &it->identityView;
            unsigned int curMipMap = requestData->getRenderMappedMipMapLevel();
            inArgs->currentActionMipMapLevel = &curMipMap;
            const RenderScale& curProxyScale = requestData->getProxyScale();
            inArgs->currentActionProxyScale = &curProxyScale;
            inArgs->inputNb = it->identityInputNumber;
            inArgs->plane = &p.identityPlane;

            GetImageOutArgs inputResults;

            if (_publicInterface->getImagePlane(*inArgs, &inputResults)) {
                functorArgs->identityPlanes.insert(std::make_pair(p, inputResults.image));
            }
        }
    }

    if (!attemptHostFrameThreading) {

        for (std::list<RectToRender>::const_iterator it = renderRects.begin(); it != renderRects.end(); ++it) {

            ActionRetCodeEnum functorRet = tiledRenderingFunctor(*it, *functorArgs);
            if (isFailureRetCode(functorRet)) {
                return functorRet;
            }

        } // for (std::list<RectI>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {

    } else { // attemptHostFrameThreading
        HostFrameThreadingRenderProcessor processor(_publicInterface->shared_from_this());
        processor.setData(renderRects, functorArgs, this);
        ActionRetCodeEnum stat = processor.launchThreadsBlocking();
        if (isFailureRetCode(stat)) {
            return stat;
        }
    } // !attemptHostFrameThreading

    ///never call endsequence render here if the render is sequential
    if (callBeginSequenceRender) {

        ActionRetCodeEnum stat = _publicInterface->endSequenceRender_public(time,
                                                                            time,
                                                                            1 /*frameStep*/,
                                                                            !appPTR->isBackground() /*interactive*/,
                                                                            combinedScale,
                                                                            isPlayback,
                                                                            !isPlayback,
                                                                            render->isDraftRender(),
                                                                            _publicInterface->getCurrentRenderView(),
                                                                            backendType,
                                                                            glContextData);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        
    }
    return eActionStatusOK;
    
} // launchPluginRenderAndHostFrameThreading

NATRON_NAMESPACE_EXIT
