/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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
#include "Engine/Timer.h"
#include "Engine/Transform.h"
#include "Engine/TreeRender.h"
#include "Engine/ThreadPool.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"


NATRON_NAMESPACE_ENTER;


/**
 * @brief This function determines the planes to render and calls recursively on upstream nodes unavailable planes
 **/
ActionRetCodeEnum
EffectInstance::Implementation::handlePassThroughPlanes(const FrameViewRequestPtr& requestData,
                                                        unsigned int mipMapLevel,
                                                        const RectD& roiCanonical,
                                                        std::map<int, std::list<ImagePlaneDesc> >* inputLayersNeeded,
                                                        bool *isPassThrough)
{

    *isPassThrough = false;
    TreeRenderNodeArgsPtr renderArgs = requestData->getRenderArgs();

    std::list<ImagePlaneDesc> layersProduced, passThroughLayers;
    int passThroughInputNb;
    TimeValue passThroughTime;
    ViewIdx passThroughView;
    bool processAllLayers;
    std::bitset<4> processChannels;
    {


        GetComponentsResultsPtr results = requestData->getComponentsResults();
        if (!results) {
            ActionRetCodeEnum stat = _publicInterface->getLayersProducedAndNeeded_public(requestData->getTime(), requestData->getView(), renderArgs, &results);
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
                    return eActionStatusFailed;
                } else {
                    // Fetch the plane on the pass-through input
                    TreeRenderNodeArgsPtr passThroughRenderArgs = renderArgs->getInputRenderArgs(passThroughInputNb);
                    if (!passThroughRenderArgs) {
                        return eActionStatusFailed;
                    }
                    *isPassThrough = true;

                    FrameViewRequestPtr createdRequest;
                    return passThroughRenderArgs->requestRender(passThroughTime, passThroughView, mipMapLevel, plane, roiCanonical, passThroughInputNb, requestData, &createdRequest);
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
                                                     bool *isIdentity)
{
    TreeRenderNodeArgsPtr renderArgs = requestData->getRenderArgs();

    TimeValue inputTimeIdentity;
    int inputNbIdentity;
    ViewIdx inputIdentityView;

    {
        // If the effect is identity over the whole RoD then we can forward the render completly to the identity node
        RectI pixelRod;
        rod.toPixelEnclosing(combinedScale, par, &pixelRod);

        IsIdentityResultsPtr results;
        {
            ActionRetCodeEnum stat = _publicInterface->isIdentity_public(true, requestData->getTime(), combinedScale, pixelRod, requestData->getView(), renderArgs, &results);
            if (isFailureRetCode(stat)) {
                return stat;
            }
        }

        results->getIdentityData(&inputNbIdentity, &inputTimeIdentity, &inputIdentityView);

    }
    *isIdentity = inputNbIdentity != -1 && inputNbIdentity != -2;
    if (!*isIdentity) {
        return eActionStatusOK;
    }

    // If effect is identity on itself, call renderRoI again at different time and view.
    if (inputNbIdentity == -2) {

        // Be safe: we may hit an infinite recursion without this check
        assert(inputTimeIdentity != requestData->getTime());
        if ( inputTimeIdentity == requestData->getTime()) {
            return eActionStatusFailed;
        }

        FrameViewRequestPtr createdRequest;
        return requestData->getRenderArgs()->requestRender(inputTimeIdentity, inputIdentityView, requestData->getMipMapLevel(), requestData->getPlaneDesc(), canonicalRoi, -1, requestData, &createdRequest);

    } else {
        assert(inputNbIdentity != -1);
        TreeRenderNodeArgsPtr passThroughRenderArgs = renderArgs->getInputRenderArgs(inputNbIdentity);
        if (!passThroughRenderArgs) {
            return eActionStatusFailed;
        }

        FrameViewRequestPtr createdRequest;
        return passThroughRenderArgs->requestRender(inputTimeIdentity, inputIdentityView, requestData->getMipMapLevel(), requestData->getPlaneDesc(), canonicalRoi, inputNbIdentity, requestData, &createdRequest);

    }
} // EffectInstance::Implementation::handleIdentityEffect

ActionRetCodeEnum
EffectInstance::Implementation::handleConcatenation(const FrameViewRequestPtr& requestData,
                                                    const FrameViewRequestPtr& requester,
                                                    int inputNbInRequester,
                                                    const RenderScale& renderScale,
                                                    const RectD& canonicalRoi,
                                                    bool *concatenated)
{
    TreeRenderNodeArgsPtr renderArgs = requestData->getRenderArgs();

    *concatenated = false;
    if (!renderArgs->getParentRender()->isConcatenationEnabled()) {
        return eActionStatusOK;
    }

    bool canDistort = renderArgs->getCurrentDistortSupport();
    bool canTransform = renderArgs->getCurrentTransformationSupport_deprecated();

    if (!canDistort && !canTransform) {
        return eActionStatusOK;
    }

    EffectInstancePtr requesterEffect = requester->getRenderArgs()->getNode()->getEffectInstance();
    bool canReturnDeprecatedTransform3x3 = requesterEffect->getInputCanReceiveTransform(inputNbInRequester);
    bool canReturnDistortionFunc = requesterEffect->getInputCanReceiveDistortion(inputNbInRequester);

    // If the caller can apply a distortion, then check if this effect has a distortion
    if (!canReturnDeprecatedTransform3x3 && !canReturnDistortionFunc) {
        return eActionStatusOK;
    }
    assert((canReturnDeprecatedTransform3x3 && !canReturnDistortionFunc) || (!canReturnDeprecatedTransform3x3 && canReturnDistortionFunc));

    DistortionFunction2DPtr disto = requestData->getDistortionResults();
    if (!disto) {
        ActionRetCodeEnum stat = _publicInterface->getDistortion_public(requestData->getTime(), renderScale, requestData->getView(), renderArgs, &disto);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        requestData->setDistortionResults(disto);
    }
    if (!disto || disto->inputNbToDistort == -1) {
        return eActionStatusOK;
    }
    {
        // Copy the original distortion held in the results in case we convert the transformation matrix from canonical to pixels.
        DistortionFunction2DPtr copy(new DistortionFunction2D(*disto));
        disto = copy;
    }


    // We support backward compatibility for plug-ins that only support Transforms: if a function is returned we do not concatenate.
    if (disto->func && !canReturnDistortionFunc) {
        return eActionStatusOK;
    }

    assert((disto->func && canReturnDistortionFunc) || disto->transformMatrix);

    if (disto->transformMatrix) {

        // The caller expects a transformation matrix in pixel coordinates
        double par = _publicInterface->getAspectRatio(renderArgs, -1);
        Transform::Matrix3x3 canonicalToPixel = Transform::matCanonicalToPixel(par, renderScale.x, renderScale.y, false);
        Transform::Matrix3x3 pixelToCanonical = Transform::matPixelToCanonical(par, renderScale.x, renderScale.y, false);
        *disto->transformMatrix = Transform::matMul(Transform::matMul(canonicalToPixel, *disto->transformMatrix), pixelToCanonical);
    }

    // Recurse on input given by plug-in
    TreeRenderNodeArgsPtr distoRenderArgs = renderArgs->getInputRenderArgs(disto->inputNbToDistort);
    if (!distoRenderArgs) {
        return eActionStatusFailed;
    }

    FrameViewRequestPtr inputRequest;
    distoRenderArgs->requestRender(requestData->getTime(), requestData->getView(), requestData->getMipMapLevel(), requestData->getPlaneDesc(), canonicalRoi, disto->inputNbToDistort, requestData, &inputRequest);

    // Create a distorsion functions stack
    Distortion2DStackPtr distoStack(new Distortion2DStack);

    // Append the list of upstream distorsions
    Distortion2DStackPtr upstreamDistoStack = inputRequest->getDistorsionStack();
    if (upstreamDistoStack) {
        distoStack->pushDistortionStack(*upstreamDistoStack);
    }

    // If this the caller effect also supports transforms, append this effect transformation, otherwise no need to append it, this effect will render anyway.
    bool requesterCanTransform = requester->getRenderArgs()->getCurrentTransformationSupport_deprecated();
    bool requesterCanDistort = requester->getRenderArgs()->getCurrentDistortSupport();

    if (requesterCanDistort || requesterCanTransform) {
        distoStack->pushDistortion(disto);
    }

    // Set the stack on the frame view request
    requestData->setDistorsionStack(distoStack);

    *concatenated = true;

    return eActionStatusOK;
} // handleConcatenation

bool
EffectInstance::Implementation::canSplitRenderWindowWithIdentityRectangles(const FrameViewRequestPtr& requestPassData,
                                                                           const RenderScale& renderMappedScale,
                                                                           RectD* inputRoDIntersectionCanonical)
{
    RectD inputsIntersection;
    bool inputsIntersectionSet = false;
    bool hasDifferentRods = false;
    int maxInput = _publicInterface->getMaxInputCount();
    bool hasMask = false;
    for (int i = 0; i < maxInput; ++i) {

        hasMask |= _publicInterface->isInputMask(i);
        RectD inputRod;

        EffectInstancePtr input = _publicInterface->getInput(i);
        if (!input) {
            continue;
        }

        RotoShapeRenderNode* isRotoShapeRenderNode = dynamic_cast<RotoShapeRenderNode*>(input.get());
        if (isRotoShapeRenderNode) {
            RotoStrokeItemPtr attachedStroke = toRotoStrokeItem(_publicInterface->getNode()->getAttachedRotoItem());
            assert(attachedStroke);
            inputRod = attachedStroke->getLastStrokeMovementBbox();
        } else {

            TreeRenderNodeArgsPtr inputRenderArgs = requestPassData->getRenderArgs()->getInputRenderArgs(i);
            GetRegionOfDefinitionResultsPtr rodResults;
            ActionRetCodeEnum stat = input->getRegionOfDefinition_public(requestPassData->getTime(), renderMappedScale, requestPassData->getView(), inputRenderArgs, &rodResults);
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

void
EffectInstance::Implementation::checkRestToRender(const FrameViewRequestPtr& requestData, const RectI& renderMappedRoI, const RenderScale& renderMappedScale, std::list<RectToRender>* renderRects, bool* hasPendingTiles)
{
    // Compute the rectangle portion (renderWindow) left to render.
    // The renderwindow is the bounding box of all tiles that are left to render (not the tiles that are pending)

    TreeRenderNodeArgsPtr renderArgs = requestData->getRenderArgs();

    // If the image is entirely cached, do not even compute it and insert it in the output planes map
    Image::TileStateMap tilesState;
    bool hasUnRenderedTile;
    ImagePtr cacheImage = requestData->getImagePlane();
    cacheImage->getRestToRender(&tilesState, &hasUnRenderedTile, hasPendingTiles);

    if (!hasUnRenderedTile) {
        return;
    }

    // If the effect does not support tiles, render everything again
    if (!renderArgs->getCurrentTilesSupport()) {
        // If not using the cache, render the full RoI
        // The RoI has already been set to the pixelRoD in this case
        RectToRender r;
        r.rect = renderMappedRoI;
        r.identityInputNumber = -1;
        renderRects->push_back(r);
        return;
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
        if (canSplitRenderWindowWithIdentityRectangles(requestData, renderMappedScale, &inputRodIntersection)) {

            double par = _publicInterface->getAspectRatio(requestData->getRenderArgs(), -1);
            inputRodIntersection.toPixelEnclosing(renderMappedScale, par, &inputRodIntersectionPixel);
            
            // For each tile, if outside of the input intersections, check if it is identity.
            // If identity mark as rendered, and add to the RectToRender list.
            for (Image::TileStateMap::iterator it = tilesState.begin(); it != tilesState.end(); ++it) {


                if ( !it->second.bounds.intersects(inputRodIntersectionPixel) ) {
                    TimeValue identityInputTime;
                    int identityInputNb;
                    ViewIdx inputIdentityView;
                    {
                        IsIdentityResultsPtr results;
                        ActionRetCodeEnum stat = _publicInterface->isIdentity_public(false, requestData->getTime(), renderMappedScale, it->second.bounds, requestData->getView(), requestData->getRenderArgs(), &results);
                        if (isFailureRetCode(stat)) {
                            continue;
                        } else {
                            results->getIdentityData(&identityInputNb, &identityInputTime, &inputIdentityView);
                        }
                    }
                    if (identityInputNb >= 0) {

                        // Mark the tile rendered
                        it->second.status = Image::eTileStatusRendered;

                        // Add this rectangle to the rects to render list (it will just copy the source image and
                        // not actually call render on it)
                        RectToRender r;
                        r.rect = it->second.bounds;
                        r.identityInputNumber = identityInputNb;
                        r.identityTime = identityInputTime;
                        r.identityView = inputIdentityView;
                        identityRects.push_back(r);
                    }
                } // if outside of inputs intersection
            } // for each tile to render
        } // canSplitRenderWindowWithIdentityRectangles
    }

    // Now we try to reduce the unrendered tiles in bigger rectangles so that there's a lot less calls to
    // the render action.
    std::list<RectI> reducedRects;
    {
        int tileSizeX, tileSizeY;
        cacheImage->getTileSize(&tileSizeX, &tileSizeY);
        Image::getMinimalRectsToRenderFromTilesState(tilesState, renderMappedRoI, tileSizeX, tileSizeY, &reducedRects);
    }
    if (reducedRects.empty()) {
        return;
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

    // For each reduced rect to render, add it to the final list
    if (reducedRects.size() == 1 && renderArgs->getCurrentRenderSafety() == eRenderSafetyFullySafeFrame) {
        RectI mainRenderRect = reducedRects.front();

        // If plug-in wants host frame threading and there is only 1 rect to render, split it
        // in the number of available threads in the thread-pool

        const unsigned int nThreads = MultiThread::getNCPUsAvailable();
        reducedRects = mainRenderRect.splitIntoSmallerRects(nThreads);
    }
    for (std::list<RectI>::const_iterator it = reducedRects.begin(); it != reducedRects.end(); ++it) {
        if (!it->isNull()) {
            RectToRender r;
            r.rect = *it;
            renderRects->push_back(r);
        }
    }
    
} // checkRestToRender

ImagePtr
EffectInstance::Implementation::fetchCachedTiles(const FrameViewRequestPtr& requestPassData,
                                                 const RectI& roiPixels,
                                                 unsigned int mappedMipMapLevel,
                                                 const ImagePlaneDesc& plane,
                                                 bool delayAllocation)
{

    TreeRenderNodeArgsPtr renderArgs = requestPassData->getRenderArgs();

    // Mark the image as draft in the cache
    bool isDraftRender = renderArgs->getParentRender()->isDraftRender();

    // The node frame/view hash to identify the image in the cache
    U64 nodeFrameViewHash = requestPassData->getHash();

    // The bitdepth of the image
    ImageBitDepthEnum outputBitDepth = _publicInterface->getBitDepth(renderArgs, -1);

    // Create the corresponding image plane.
    // If this plug-in does not use the cache, we directly allocate an image using the plug-in preferred buffer format.
    // If using the cache, the image has to be in a mono-channel tiled format, hence we later create a temporary copy
    // on which the plug-in will work on.
    ImagePtr image = requestPassData->getImagePlane();
    if (image) {
        image->ensureBounds(roiPixels);
    } else {
        Image::InitStorageArgs initArgs;
        {
            initArgs.bounds = roiPixels;
            initArgs.cachePolicy = eCacheAccessModeReadWrite;
            initArgs.renderArgs = renderArgs;
            initArgs.proxyScale = renderArgs->getParentRender()->getProxyScale();
            initArgs.mipMapLevel = mappedMipMapLevel;
            initArgs.isDraft = isDraftRender;
            initArgs.nodeTimeInvariantHash = nodeFrameViewHash;
            initArgs.time = requestPassData->getTime();
            initArgs.view = requestPassData->getView();
            // Cache storage is always disk
            initArgs.storage = eStorageModeDisk;
            // Cache format is always mono channel tiled
            initArgs.bufferFormat = eImageBufferLayoutMonoChannelTiled;
            initArgs.bitdepth = outputBitDepth;
            initArgs.layer = plane;

            // Do not allocate the image buffers yet, instead do it before rendering.
            // We need to create the image before because it does the cache look-up itself, and we don't want to recurse if
            // there's something cached.
            initArgs.delayAllocation = delayAllocation;
        }


        // Image::create will lookup the cache (if asked for)
        // Since multiple threads may want to access to the same image in the cache concurrently,
        // the first thread that gets onto a tile to render will render it and lock-out other threads
        // until it is rendered entirely.
        try {
            image = Image::create(initArgs);
        } catch (...) {
            return ImagePtr();
        }
    } // image
    return image;
} // fetchCachedTiles


ActionRetCodeEnum
EffectInstance::Implementation::allocateRenderBackendStorageForRenderRects(const FrameViewRequestPtr& requestData,
                                                                           const RectI& roiPixels,
                                                                           unsigned int mipMapLevel,
                                                                           const RenderScale& combinedScale,
                                                                           std::map<ImagePlaneDesc, ImagePtr> *producedImagePlanes,
                                                                           std::list<RectToRender>* renderRects)
{


    RenderBackendTypeEnum backendType;
    resolveRenderBackend(requestData, roiPixels, &backendType);
    
    // The image format supported by the plug-in (co-planar, packed RGBA, etc...)
    ImageBufferLayoutEnum imageBufferLayout = _publicInterface->getPreferredBufferLayout();
    StorageModeEnum imageStorage;

    switch (backendType) {
        case eRenderBackendTypeOpenGL:
            imageStorage = eStorageModeGLTex;
            break;
        case eRenderBackendTypeCPU:
        case eRenderBackendTypeOSMesa:
            imageStorage = eStorageModeRAM;
            break;
    }


    OSGLContextPtr glContext;
    switch (backendType) {
        case eRenderBackendTypeOpenGL:
            glContext = requestData->getRenderArgs()->getParentRender()->getGPUOpenGLContext();
            break;
        case eRenderBackendTypeOSMesa:
            glContext = requestData->getRenderArgs()->getParentRender()->getCPUOpenGLContext();
            break;
        default:
            break;
    }


    // When accumulating, re-use the same buffer of previous steps and resize it if needed.
    // Note that in this mode only a single plane can be rendered at once
    RotoStrokeItemPtr attachedStroke = toRotoStrokeItem(_publicInterface->getNode()->getAttachedRotoItem());
    assert(attachedStroke->isRenderClone());
    bool isAccumulating = attachedStroke && attachedStroke->isCurrentlyDrawing();
    ImagePtr accumBuffer;
    if (isAccumulating) {

        // Get the accum buffer on the node. Note that this is not concurrent renders safe.
        accumBuffer = _publicInterface->getNode()->getAccumBuffer();

        // If we do not have an accumulation buffer, we follow the usual code path
        if (accumBuffer) {

            // We got an existing buffer
            // Ensure the accumBuffer contains at least the RoI
            accumBuffer->ensureBounds(roiPixels);

            // When drawing with a paint brush, we may only render the bounding box of the un-rendered points.
            RectI drawingLastMovementBBoxPixel;
            {
                RectD lastStrokeRoD = attachedStroke->getLastStrokeMovementBbox();
                double par = _publicInterface->getBitDepth(requestData->getRenderArgs(), -1);
                lastStrokeRoD.toPixelEnclosing(combinedScale, par, &drawingLastMovementBBoxPixel);
            }
            {
                renderRects->clear();
                RectToRender r;
                r.rect = drawingLastMovementBBoxPixel;
                r.identityInputNumber = -1;
                r.backendType = backendType;
                r.tmpRenderPlanes[requestData->getPlaneDesc()] = accumBuffer;
                renderRects->push_back(r);
            }
            return eActionStatusOK;
        } // accumBuffer
    } // isAccumulating

    // The bitdepth of the image
    ImageBitDepthEnum outputBitDepth = _publicInterface->getBitDepth(requestData->getRenderArgs(), -1);


    ImagePtr outputTmpImage;
    // If we don't use the cache image, we must allocate the output image
    if (requestData->getCachePolicy() == eCacheAccessModeNone) {
        Image::InitStorageArgs tmpImgInitArgs;
        {
            tmpImgInitArgs.bounds = roiPixels;
            tmpImgInitArgs.renderArgs = requestData->getRenderArgs();
            tmpImgInitArgs.cachePolicy = eCacheAccessModeNone;
            tmpImgInitArgs.bufferFormat = imageBufferLayout;
            tmpImgInitArgs.mipMapLevel = mipMapLevel;
            tmpImgInitArgs.proxyScale = requestData->getRenderArgs()->getParentRender()->getProxyScale();
            tmpImgInitArgs.glContext = glContext;
            switch (backendType) {
                case eRenderBackendTypeOpenGL:
                    tmpImgInitArgs.storage = eStorageModeGLTex;
                    break;
                case eRenderBackendTypeCPU:
                case eRenderBackendTypeOSMesa:
                    tmpImgInitArgs.storage = eStorageModeRAM;
                    break;
            }
            tmpImgInitArgs.bitdepth = outputBitDepth;
            tmpImgInitArgs.layer = requestData->getPlaneDesc();

        }
        try {
            outputTmpImage = Image::create(tmpImgInitArgs);
        } catch (...) {
            return eActionStatusFailed;
        }
        requestData->setImagePlane(outputTmpImage);

        if (isAccumulating) {
            _publicInterface->getNode()->setAccumBuffer(outputTmpImage);
        }

        // Storage the temporary image in the output planes
        (*producedImagePlanes)[requestData->getPlaneDesc()] = outputTmpImage;
    }



    assert(requestData->getComponentsResults());
    const std::list<ImagePlaneDesc>& producedPlanes = requestData->getComponentsResults()->getProducedPlanes();


    // Set or create the temporary image for each rectangle to render and for each plane
    // with the memory layout supported by the plug-in that we will write to.
    for (std::list<RectToRender>::iterator it = renderRects->begin(); it != renderRects->end(); ++it) {
        for (std::list<ImagePlaneDesc>::const_iterator it2 = producedPlanes.begin(); it2 != producedPlanes.end(); ++it2) {

            ImagePtr tmpImage;
            // If we have a cache image we did not allocate yet a temporary image for the plug-in to render onto:
            // Allocate a small image for each rectangle.
            // If we do not have a cache image, we just allocated a single image for the requested plane but none
            // for other planes.
            if (requestData->getCachePolicy() == eCacheAccessModeNone && *it2 == requestData->getPlaneDesc()) {
                tmpImage = outputTmpImage;
            } else {
                Image::InitStorageArgs tmpImgInitArgs;
                {
                    tmpImgInitArgs.bounds = it->rect;
                    tmpImgInitArgs.renderArgs = requestData->getRenderArgs();
                    tmpImgInitArgs.cachePolicy = eCacheAccessModeNone;
                    tmpImgInitArgs.bufferFormat = imageBufferLayout;
                    tmpImgInitArgs.mipMapLevel = mipMapLevel;
                    tmpImgInitArgs.proxyScale = requestData->getRenderArgs()->getParentRender()->getProxyScale();
                    tmpImgInitArgs.glContext = glContext;
                    switch (backendType) {
                        case eRenderBackendTypeOpenGL:
                            tmpImgInitArgs.storage = eStorageModeGLTex;
                            break;
                        case eRenderBackendTypeCPU:
                        case eRenderBackendTypeOSMesa:
                            tmpImgInitArgs.storage = eStorageModeRAM;
                            break;
                    }
                    tmpImgInitArgs.bitdepth = outputBitDepth;
                    tmpImgInitArgs.layer = *it2;

                }
                try {
                    tmpImage = Image::create(tmpImgInitArgs);
                } catch (...) {
                    return eActionStatusFailed;
                }
            }

            it->tmpRenderPlanes[*it2] = tmpImage;
        }
    } // for each produced plane
    return eActionStatusOK;
    
} // allocateRenderBackendStorageForRenderRects

ActionRetCodeEnum
EffectInstance::Implementation::launchInternalRender(const FrameViewRequestPtr& requestData,
                                                     const RenderScale& combinedScale,
                                                     const std::list<RectToRender>& renderRects,
                                                     const std::map<ImagePlaneDesc, ImagePtr>& producedImagePlanes)
{

    // If we reach here, it can be either because the planes are cached or not, either way
    // the planes are NOT a total identity, and they may have some content left to render.
    ActionRetCodeEnum renderRetCode = eActionStatusOK;

    // There should always be at least 1 plane to render (The color plane)
    assert(!renderRects.empty());

    RenderSafetyEnum safety = requestData->getRenderArgs()->getCurrentRenderSafety();

    EffectInstancePtr renderInstance;
    //
    // Figure out If this node should use a render clone rather than execute renderRoIInternal on the main (this) instance.
    // Reasons to use a render clone is because a plug-in is eRenderSafetyInstanceSafe or does not support
    // concurrent GL renders.
    //
    const RectToRender& firstRectToRender = renderRects.front();
    bool useRenderClone = false;
    useRenderClone |= (safety == eRenderSafetyInstanceSafe && !_publicInterface->getNode()->isDuringPaintStrokeCreation());
    useRenderClone |= safety != eRenderSafetyUnsafe && firstRectToRender.backendType == eRenderBackendTypeOpenGL && !_publicInterface->supportsConcurrentOpenGLRenders();

    if (useRenderClone) {
        renderInstance = _publicInterface->getOrCreateRenderInstance();
    } else {
        renderInstance = _publicInterface->shared_from_this();
    }
    assert(renderInstance);

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
    bool hasReleasedThread = false;
    if (safety == eRenderSafetyInstanceSafe && !useRenderClone) {
        QThreadPool::globalInstance()->releaseThread();
        locker.reset( new QMutexLocker( &_publicInterface->getNode()->getRenderInstancesSharedMutex() ) );
        hasReleasedThread = true;
    } else if (safety == eRenderSafetyUnsafe) {
        PluginPtr p = _publicInterface->getNode()->getPlugin();
        assert(p);
        QThreadPool::globalInstance()->releaseThread();
        locker.reset( new QMutexLocker( p->getPluginLock().get() ) );
        hasReleasedThread = true;
    } else {
        // no need to lock
        Q_UNUSED(locker);
    }
    if (hasReleasedThread) {
        QThreadPool::globalInstance()->reserveThread();
    }

    OSGLContextPtr glContext;
    switch (firstRectToRender.backendType) {
        case eRenderBackendTypeOpenGL:
            glContext = requestData->getRenderArgs()->getParentRender()->getGPUOpenGLContext();
            break;
        case eRenderBackendTypeOSMesa:
            glContext = requestData->getRenderArgs()->getParentRender()->getCPUOpenGLContext();
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
    if (firstRectToRender.backendType == eRenderBackendTypeOpenGL ||
        firstRectToRender.backendType == eRenderBackendTypeOSMesa) {
        ActionRetCodeEnum stat = renderInstance->attachOpenGLContext_public(requestData->getTime(), requestData->getView(), combinedScale, requestData->getRenderArgs(), glContext, &glContextData);
        if (isFailureRetCode(stat)) {
            renderRetCode = stat;
        }
    }
    if (renderRetCode == eActionStatusOK) {

        renderRetCode = renderInstance->_imp->renderForClone(requestData, glContext, glContextData, combinedScale, renderRects, producedImagePlanes);

        if (firstRectToRender.backendType == eRenderBackendTypeOpenGL ||
            firstRectToRender.backendType == eRenderBackendTypeOSMesa) {

            // If the plug-in doesn't support concurrent OpenGL renders, release the lock that was taken in the call to attachOpenGLContext_public() above.
            // For safe plug-ins, we call dettachOpenGLContext_public when the effect is destroyed in Node::deactivate() with the function EffectInstance::dettachAllOpenGLContexts().
            // If we were the last render to use this context, clear the data now

            if ( glContextData->getHasTakenLock() ||
                !_publicInterface->supportsConcurrentOpenGLRenders() ||
                glContextData.use_count() == 1) {

                renderInstance->dettachOpenGLContext_public(requestData->getRenderArgs(), glContext, glContextData);
            }
        }
    }
    if (useRenderClone) {
        _publicInterface->releaseRenderInstance(renderInstance);
    }
    

    return renderRetCode;
} // launchInternalRender



ActionRetCodeEnum
EffectInstance::Implementation::handleUpstreamFramesNeeded(const FrameViewRequestPtr& requestPassData,
                                                           const RenderScale& combinedScale,
                                                           unsigned int mipMapLevel,
                                                           const RectD& roiCanonical,
                                                           const std::map<int, std::list<ImagePlaneDesc> >& neededInputLayers)
{
    // For all frames/views needed, recurse on inputs with the appropriate RoI

    TreeRenderNodeArgsPtr renderArgs = requestPassData->getRenderArgs();

    // Get frames needed to recurse upstream
    FramesNeededMap framesNeeded;
    {
        GetFramesNeededResultsPtr results = requestPassData->getFramesNeededResults();
        if (!results) {
            ActionRetCodeEnum stat = _publicInterface->getFramesNeeded_public(requestPassData->getTime(), requestPassData->getView(), renderArgs, &results);
            if (isFailureRetCode(stat)) {
                return stat;
            }
            requestPassData->setFramesNeededResults(results);
        }
        results->getFramesNeeded(&framesNeeded);
    }


    // Compute the regions of interest in input for this RoI.
    // The regions of interest returned is only valid for this RoI, we don't cache it. Rather we cache on the input the bounding box
    // of all the calls of getRegionsOfInterest that were made down-stream so that the node gets rendered only once.
    RoIMap inputsRoi;
    {
        ActionRetCodeEnum stat = _publicInterface->getRegionsOfInterest_public(requestPassData->getTime(), combinedScale, roiCanonical, requestPassData->getView(), renderArgs, &inputsRoi);
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }

    for (FramesNeededMap::const_iterator it = framesNeeded.begin(); it != framesNeeded.end(); ++it) {

        int inputNb = it->first;
        EffectInstancePtr inputEffect = _publicInterface->resolveInputEffectForFrameNeeded(inputNb, 0);
        if (!inputEffect) {
            continue;
        }
        NodePtr inputNode = inputEffect->getNode();
        assert(inputNode);

        assert(inputNb != -1);


        TreeRenderNodeArgsPtr inputRenderArgs = renderArgs->getInputRenderArgs(inputNb);
        assert(inputRenderArgs);

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

        if ( inputRoI.isInfinite() ) {
            _publicInterface->setPersistentMessage( eMessageTypeError, _publicInterface->tr("%1 asked for an infinite region of interest upstream.").arg( QString::fromUtf8( _publicInterface->getNode()->getScriptName_mt_safe().c_str() ) ).toStdString() );
            return eActionStatusFailed;
        }

        bool inputIsContinuous = inputEffect->canRenderContinuously(inputRenderArgs);

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
                        for (std::list<ImagePlaneDesc>::const_iterator planeIt = inputPlanesNeeded->begin(); planeIt != inputPlanesNeeded->end(); ++planeIt) {
                            FrameViewRequestPtr createdRequest;
                            ActionRetCodeEnum stat = inputRenderArgs->requestRender(inputTime, viewIt->first, mipMapLevel, *planeIt, inputRoI, inputNb, requestPassData, &createdRequest);
                            if (isFailureRetCode(stat)) {
                                return stat;
                            }
                        } // for each plane needed

                    } // for all frames

                } // for all ranges
            } // for all views
        } // EffectInstance::NotifyInputNRenderingStarted_RAII
    } // for all inputs

    return eActionStatusOK;
} // handleUpstreamFramesNeeded


ActionRetCodeEnum
EffectInstance::requestRender(TimeValue time,
                              ViewIdx view,
                              unsigned int mipMapLevel,
                              const ImagePlaneDesc& plane,
                              const RectD & roiCanonical,
                              int inputNbInRequester,
                              const FrameViewRequestPtr& requester)
{

    FrameViewRequestPtr requestData;
    {
        FrameViewPair frameView;
        // Requested time is rounded to an epsilon so we can be sure to find it again in getImage, accounting for precision
        frameView.time = roundImageTimeToEpsilon(time);
        frameView.view = view;

        // Create the frame/view request object
        {
            bool created = getOrCreateFrameViewRequest(frameView.time, frameView.view, mipMapLevel, plane, &requestData);
            (void)created;
        }
        *createdRequest = thisFrameView;
    }

    // If this render has no dependencies, add it to the things to render
#pragma message WARN("do this block on all exit points of the func")
    if (requestData->getNumDependencies() == 0) {
        getParentRender()->addDependencyFreeRender(requestData);
    }
    getParentRender()->addTaskToRender(thisFrameView);

    // Add this frame/view as depdency of the requester
    if (requester) {
        requester->addDependency(thisFrameView);
    }



    // Set the frame view request on the TLS
    EffectInstanceTLSDataPtr tls = getTLSObject();
    assert(tls);
    FrameViewRequestSetter_RAII tlsSetFrameViewRequest(tls, requestData);


    // This call cannot be made on a render clone itself.
    assert(_imp->mainInstance);
    TreeRenderNodeArgsPtr renderArgs = requestData->getRenderArgs();

    // Ensure the render args are the ones of this node.
    assert(renderArgs && renderArgs->getNode() == getNode());

    // Some nodes do not support render-scale and can only render at scale 1.
    // If the render requested a mipmap level different than 0, we must render at mipmap level 0 then downscale to the requested
    // mipmap level.
    // If the render requested a proxy scale different than 1, we fail because we cannot render at scale 1 then resize at an arbitrary scale.

    const bool renderFullScaleThenDownScale = !renderArgs->getCurrentRenderScaleSupport() && requestData->getMipMapLevel() > 0;
    const RenderScale& proxyScale = renderArgs->getParentRender()->getProxyScale();

    if (!renderArgs->getCurrentRenderScaleSupport() && (proxyScale.x != 1. || proxyScale.y != 1.)) {
        setPersistentMessage(eMessageTypeError, tr("This node does not support custom proxy scale. It can only render at full resolution").toStdString());
        return eActionStatusFailed;
    }

    const unsigned int mappedMipMapLevel = renderFullScaleThenDownScale ? 0 : requestData->getMipMapLevel();
    requestData->setRenderMappedMipMapLevel(mappedMipMapLevel);
    RenderScale originalCombinedScale = EffectInstance::Implementation::getCombinedScale(requestData->getMipMapLevel(), proxyScale);
    const RenderScale mappedCombinedScale = renderFullScaleThenDownScale ? RenderScale(1.) : originalCombinedScale;

    // Get the region of definition of the effect at this frame/view in canonical coordinates
    RectD rod;
    {
        GetRegionOfDefinitionResultsPtr results;
        {
            ActionRetCodeEnum stat = getRegionOfDefinition_public(requestData->getTime(), mappedCombinedScale, requestData->getView(), renderArgs, &results);
            if (isFailureRetCode(stat)) {
                return stat;
            }
        }
        rod = results->getRoD();

        // If the plug-in RoD is null, there's nothing to render.
        if (rod.isNull()) {
            return eActionStatusInputDisconnected;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Handle pass-through planes /////////////////////////////////////////////////////////////
    std::map<int, std::list<ImagePlaneDesc> > inputLayersNeeded;
    {
        bool isPassThrough;
        ActionRetCodeEnum upstreamRetCode = _imp->handlePassThroughPlanes(requestData, mappedMipMapLevel, roiCanonical, &inputLayersNeeded, &isPassThrough);
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
    const double par = getAspectRatio(renderArgs, -1);
    {
        bool isIdentity;
        ActionRetCodeEnum upstreamRetCode = _imp->handleIdentityEffect(par, rod, mappedCombinedScale, roiCanonical, requestData, &isIdentity);
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
        ActionRetCodeEnum upstreamRetCode = _imp->handleConcatenation(requestData, requester, inputNbInRequester, mappedCombinedScale, roiCanonical, &concatenated);
        if (isFailureRetCode(upstreamRetCode)) {
            return upstreamRetCode;
        }
        if (concatenated) {
            requestData->initStatus(FrameViewRequest::eFrameViewRequestStatusPassThrough);
            return eActionStatusOK;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Resolve render device ///////////////////////////////////////////////////////////////////


    // This is the region to render in pixel coordinates at the scale of renderMappedScale
    RectI renderMappedRoI;
    roiCanonical.toPixelEnclosing(mappedCombinedScale, par, &renderMappedRoI);

    // The RoI cannot be null here, either we are in !renderFullScaleThenDownscale and we already checked at the begining
    // of the function that the RoI was Null, either the RoD was checked for NULL.
    assert(!renderMappedRoI.isNull());


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Compute RoI depending on render scale ///////////////////////////////////////////////////

    // Should the output of this render be cached ?
    CacheAccessModeEnum cachePolicy = _imp->shouldRenderUseCache(requestData);
    requestData->setCachePolicy(cachePolicy);

    // The RoD in pixel coordinates at the scale of mappedCombinedScale
    RectI pixelRoDRenderMapped;

    // Depending on tiles support we are going to modify the RoI to clip it to the bounds of this effect.
    // We still keep in memory what was requested for later on
    const RectI originalRenderMappedRoI = renderMappedRoI;

    rod.toPixelEnclosing(mappedCombinedScale, par, &pixelRoDRenderMapped);

    if (!renderArgs->getCurrentTilesSupport()) {
        // If tiles are not supported the RoI is the full image bounds
        renderMappedRoI = pixelRoDRenderMapped;
    } else {

        // Round the roi to the tile size if the render is cached
        if (cachePolicy != eCacheAccessModeNone) {
            ImageBitDepthEnum outputBitDepth = getBitDepth(renderArgs, -1);
            int tileWidth, tileHeight;
            Cache::getTileSizePx(outputBitDepth, &tileWidth, &tileHeight);
            renderMappedRoI.roundToTileSize(tileWidth, tileHeight);
        }

        // Make sure the RoI falls within the image bounds
        if ( !renderMappedRoI.intersect(pixelRoDRenderMapped, &renderMappedRoI) ) {
            return eActionStatusOK;
        }

        // The RoI falls into the effect pixel region of definition
        assert(renderMappedRoI.x1 >= pixelRoDRenderMapped.x1 && renderMappedRoI.y1 >= pixelRoDRenderMapped.y1 &&
               renderMappedRoI.x2 <= pixelRoDRenderMapped.x2 && renderMappedRoI.y2 <= pixelRoDRenderMapped.y2);
    }

    assert(!renderMappedRoI.isNull());

    // The requested portion to render in canonical coordinates
    RectD roundedCanonicalRoI;
    renderMappedRoI.toCanonical(mappedCombinedScale, par, rod, &roundedCanonicalRoI);

    // Merge the roi requested onto the existing RoI requested for this frame/view
    {
        RectD curRoI = requestData->getCurrentRoI();
        if (curRoI.isNull()) {
            curRoI = roiCanonical;
        } else {
            curRoI.merge(roiCanonical);
        }
        requestData->setCurrentRoI(curRoI);
    }

    // Fetch tiles from cache
    // Note that no memory allocation is done here, images are only fetched from the cache.
    bool hasUnRenderedTile;
    bool hasPendingTiles;
    {
        ImagePtr image = _imp->fetchCachedTiles(requestData, renderMappedRoI, mappedMipMapLevel, requestData->getPlaneDesc(), true);
        if (!image) {
            return eActionStatusFailed;
        }
        requestData->setImagePlane(image);

        Image::TileStateMap tilesState;
        image->getRestToRender(&tilesState, &hasUnRenderedTile, &hasPendingTiles);
    }

    // If there's nothing to render, do not even add the inputs as needed dependencies.
    if (!hasUnRenderedTile && !hasPendingTiles) {
        requestData->initStatus(FrameViewRequest::eFrameViewRequestStatusRendered);
    } else {
        requestData->initStatus(FrameViewRequest::eFrameViewRequestStatusNotRendered);
        ActionRetCodeEnum upstreamRetCode = _imp->handleUpstreamFramesNeeded(requestData, mappedCombinedScale, mappedMipMapLevel, roundedCanonicalRoI, inputLayersNeeded);
        
        if (isFailureRetCode(upstreamRetCode)) {
            return upstreamRetCode;
        }
    }
    return eActionStatusOK;
} // requestRender


static void invalidateCachedPlanesToRender(const std::map<ImagePlaneDesc, ImagePtr>& cachedPlanes)
{
    for (std::map<ImagePlaneDesc, ImagePtr>::const_iterator it = cachedPlanes.begin(); it != cachedPlanes.end(); ++it) {
        it->second->discardTiles();
    }
}

ActionRetCodeEnum
EffectInstance::launchRender(const FrameViewRequestPtr& requestData)
{
    // Set the frame view request on the TLS
    EffectInstanceTLSDataPtr tls = getTLSObject();
    assert(tls);
    FrameViewRequestSetter_RAII tlsSetFrameViewRequest(tls, requestData);

    {
        FrameViewRequest::FrameViewRequestStatusEnum requestStatus = requestData->notifyRenderStarted();
        switch (requestStatus) {
            case FrameViewRequest::eFrameViewRequestStatusRendered:
            case FrameViewRequest::eFrameViewRequestStatusPassThrough:
                return eActionStatusOK;
            case FrameViewRequest::eFrameViewRequestStatusPending: {
                ActionRetCodeEnum stat = requestData->waitForPendingResults();
                return stat;
            }
            case FrameViewRequest::eFrameViewRequestStatusNotRendered:
                break;
        }
    }
    ActionRetCodeEnum stat = launchRenderInternal(requestData);
    requestData->notifyRenderFinished(stat);
    return stat;
} // launchRender

ActionRetCodeEnum
EffectInstance::launchRenderInternal(const FrameViewRequestPtr& requestData)
{

    TreeRenderNodeArgsPtr renderArgs = requestData->getRenderArgs();

    const double par = getAspectRatio(renderArgs, -1);
    const unsigned int mappedMipMapLevel = requestData->getRenderMappedMipMapLevel();
    const RenderScale mappedCombinedScale = EffectInstance::Implementation::getCombinedScale(mappedMipMapLevel, renderArgs->getParentRender()->getProxyScale());

    RectI renderMappedRoI;
    requestData->getCurrentRoI().toPixelEnclosing(mappedCombinedScale, par, &renderMappedRoI);

    ImagePtr cacheImage = requestData->getImagePlane();
    if (requestData->getCachePolicy() != eCacheAccessModeNone) {
        // Allocate the cache storage image now.
        // Already cached tiles will be left untouched.
        cacheImage->ensureBuffersAllocated();
    }

    // Fetch or create a cache image for all other planes that the plug-in produces but are not requested
    std::map<ImagePlaneDesc, ImagePtr> producedImagePlanes;
    if (requestData->getCachePolicy() != eCacheAccessModeNone) {
        assert(requestData->getComponentsResults());
        const std::list<ImagePlaneDesc>& producedPlanes = requestData->getComponentsResults()->getProducedPlanes();
        for (std::list<ImagePlaneDesc>::const_iterator it = producedPlanes.begin(); it != producedPlanes.end(); ++it) {
            ImagePtr imagePlane;
            if (*it == requestData->getPlaneDesc()) {
                imagePlane = cacheImage;
            } else {
                imagePlane = _imp->fetchCachedTiles(requestData, renderMappedRoI, mappedMipMapLevel, *it, false);
            }
            producedImagePlanes[*it] = imagePlane;
        }
    }

    ActionRetCodeEnum renderRetCode = eActionStatusOK;
    std::list<RectToRender> renderRects;
    bool hasPendingTiles;
    _imp->checkRestToRender(requestData, renderMappedRoI, mappedCombinedScale, &renderRects, &hasPendingTiles);
    while (!renderRects.empty() || hasPendingTiles) {

        // There may be no rectangles to render if all rectangles are pending (i.e: this render should wait for another thread
        // to complete the render first)
        if (!renderRects.empty()) {
            renderRetCode = _imp->allocateRenderBackendStorageForRenderRects(requestData, renderMappedRoI, mappedMipMapLevel, mappedCombinedScale, &producedImagePlanes, &renderRects);
            if (isFailureRetCode(renderRetCode)) {
                break;
            }
            renderRetCode = _imp->launchInternalRender(requestData, mappedCombinedScale, renderRects, producedImagePlanes);
            if (isFailureRetCode(renderRetCode)) {
                break;
            }
        }


        // The render went OK: push the cache images tiles to the cache

        if (requestData->getCachePolicy() != eCacheAccessModeNone) {

            // Push to the cache the tiles that we rendered
            cacheImage->pushTilesToCacheIfNotAborted();

            // Wait for any pending results. After this line other threads that should have computed should be done
            cacheImage->waitForPendingTiles();
        }

        _imp->checkRestToRender(requestData, renderMappedRoI, mappedCombinedScale, &renderRects, &hasPendingTiles);
    } // while there is still something not rendered

    // If using GPU and out of memory retry on CPU if possible
    if (renderRetCode == eActionStatusOutOfMemory && !renderRects.empty() && renderRects.front().backendType == eRenderBackendTypeOpenGL) {
        if (renderArgs->getCurrentRenderOpenGLSupport() != ePluginOpenGLRenderSupportYes) {
            // The plug-in can only use GPU or doesn't support GPU
            invalidateCachedPlanesToRender(producedImagePlanes);
            return eActionStatusFailed;
        }
       
        return launchRenderInternal(requestData);
    }

    if (renderRetCode != eActionStatusOK) {
        invalidateCachedPlanesToRender(producedImagePlanes);
        return renderRetCode;
    }

    std::map<ImagePlaneDesc, ImagePtr>::iterator foundRenderedPlane = producedImagePlanes.find(requestData->getPlaneDesc());
    if (foundRenderedPlane == producedImagePlanes.end()) {
        return eActionStatusFailed;
    }

    const ImagePtr& outputImage = foundRenderedPlane->second;

    // If the node did not support render scale and the mipmap level rendered was different than what was requested, downscale the image.
    if (mappedMipMapLevel != requestData->getMipMapLevel()) {
        assert(requestData->getMipMapLevel() > 0);
        assert(outputImage->getMipMapLevel() == 0);
        ImagePtr downscaledImage = outputImage->downscaleMipMap(outputImage->getBounds(), requestData->getMipMapLevel());
        requestData->setImagePlane(downscaledImage);
    }

    return eActionStatusOK;
} // launchRenderInternal


ActionRetCodeEnum
EffectInstance::Implementation::renderForClone(const FrameViewRequestPtr& requestData,
                                               const OSGLContextPtr& glContext,
                                               const EffectOpenGLContextDataPtr& glContextData,
                                               const RenderScale& combinedScale,
                                               const std::list<RectToRender>& renderRects,
                                               const std::map<ImagePlaneDesc, ImagePtr>& producedImagePlanes)
{
    assert( !renderRects.empty() );

    // Notify the gui we're rendering
    NotifyRenderingStarted_RAII renderingNotifier(_publicInterface->getNode().get());

    // If this node is not sequential we at least have to bracket the render action with a call to begin and end sequence render.
    TreeRenderNodeArgsPtr renderArgs = requestData->getRenderArgs();

    RangeD sequenceRange;
    {
        GetFrameRangeResultsPtr rangeResults;
        ActionRetCodeEnum stat = _publicInterface->getFrameRange_public(renderArgs, &rangeResults);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        rangeResults->getFrameRangeResults(&sequenceRange);
    }

    // We only need to call begin if we've not already called it.
    bool callBeginSequenceRender = false;
    if ( !_publicInterface->isWriter() || (requestData->getRenderArgs()->getCurrentRenderSequentialPreference() == eSequentialPreferenceNotSequential) ) {
        callBeginSequenceRender = true;
    }

    bool isPlayback = requestData->getRenderArgs()->getParentRender()->isPlayback();
    TimeValue time = requestData->getTime();


    if (callBeginSequenceRender) {
        ActionRetCodeEnum stat = _publicInterface->beginSequenceRender_public(time,
                                                                              time,
                                                                              1 /*frameStep*/,
                                                                              !appPTR->isBackground() /*interactive*/,
                                                                              combinedScale,
                                                                              isPlayback,
                                                                              !isPlayback,
                                                                              renderArgs->getParentRender()->isDraftRender(),
                                                                              requestData->getView(),
                                                                              renderRects.front().backendType,
                                                                              glContextData,
                                                                              renderArgs);
        
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }



#ifdef NATRON_HOSTFRAMETHREADING_SEQUENTIAL
    const bool attemptHostFrameThreading = false;
#else
    const bool attemptHostFrameThreading = renderArgs->getCurrentRenderSafety() == eRenderSafetyFullySafeFrame &&
                                           renderRects.size() > 1 &&
                                           renderRects.front().backendType == eRenderBackendTypeCPU;
#endif


    TiledRenderingFunctorArgs functorArgs;
    functorArgs.glContextData = glContextData;
    functorArgs.glContext = glContext;
    functorArgs.requestData = requestData;
    functorArgs.producedImagePlanes = producedImagePlanes;

    if (!attemptHostFrameThreading) {

        for (std::list<RectToRender>::const_iterator it = renderRects.begin(); it != renderRects.end(); ++it) {

            ActionRetCodeEnum functorRet = tiledRenderingFunctor(*it, functorArgs);
            if (isFailureRetCode(functorRet)) {
                return functorRet;
            }

        } // for (std::list<RectI>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {

    } else { // attemptHostFrameThreading

        std::list<RectToRender> rectsToRenderList = renderRects;
        RectToRender lastRectToRender = rectsToRenderList.back();

        bool isThreadPoolThread = isRunningInThreadPoolThread();

        // If the current thread is a thread-pool thread, make it also do an iteration instead
        // of waiting for other threads
        if (isThreadPoolThread) {
            rectsToRenderList.pop_back();
        }

        QThread* curThread = QThread::currentThread();

        std::vector<ActionRetCodeEnum> threadReturnCodes;
        QFuture<ActionRetCodeEnum> ret = QtConcurrent::mapped( rectsToRenderList,
                                                              boost::bind(&EffectInstance::Implementation::tiledRenderingFunctorInSeparateThread,
                                                                          this,
                                                                          _1,
                                                                          functorArgs,
                                                                          curThread) );
        if (isThreadPoolThread) {
            ActionRetCodeEnum retCode = tiledRenderingFunctor(lastRectToRender, functorArgs);
            threadReturnCodes.push_back(retCode);
        }

        // Wait for other threads to be finished
        ret.waitForFinished();
        
        for (QFuture<ActionRetCodeEnum>::const_iterator  it2 = ret.begin(); it2 != ret.end(); ++it2) {
            threadReturnCodes.push_back(*it2);
        }
        for (std::vector<ActionRetCodeEnum>::const_iterator it2 = threadReturnCodes.begin(); it2 != threadReturnCodes.end(); ++it2) {
            if (isFailureRetCode(*it2)) {
                return *it2;
            }
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
                                                                            renderArgs->getParentRender()->isDraftRender(),
                                                                            requestData->getView(),
                                                                            renderRects.front().backendType,
                                                                            glContextData,
                                                                            renderArgs);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        
    }
    return eActionStatusOK;
    
} // renderForClone

NATRON_NAMESPACE_EXIT;
