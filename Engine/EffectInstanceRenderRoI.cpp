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
#include "Engine/TreeRenderNodeArgs.h"
#include "Engine/ThreadPool.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"


NATRON_NAMESPACE_ENTER;

/*
 * @brief Split all rects to render in smaller rects and check if each one of them is identity.
 * For identity rectangles, we just call renderRoI again on the identity input in the tiledRenderingFunctor.
 * For non-identity rectangles, compute the bounding box of them and render it
 */
void
EffectInstance::Implementation::checkIdentityRectsToRender(const FrameViewRequestPtr& requestData,
                                                           const RectI & inputsRoDIntersection,
                                                           const std::list<RectI> & rectsToRender,
                                                           const RenderScale & renderMappedScale,
                                                           std::list<RectToRender>* finalRectsToRender)
{
    for (std::list<RectI>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
        if (it->isNull()) {
            continue;
        }
        std::vector<RectI> splits = it->splitIntoSmallerRects(0);
        RectToRender nonIdentityRect;
        nonIdentityRect.identityInputNumber = -1;
        nonIdentityRect.identityTime = TimeValue(0.);


        bool nonIdentityRectSet = false;
        for (std::size_t i = 0; i < splits.size(); ++i) {
            TimeValue identityInputTime(0.);
            int identityInputNb = -1;

            ViewIdx inputIdentityView(requestData->getView());
            if ( !splits[i].intersects(inputsRoDIntersection) ) {
                IsIdentityResultsPtr results;
                ActionRetCodeEnum stat = _publicInterface->isIdentity_public(false, requestData->getTime(), renderMappedScale, splits[i], requestData->getView(), requestData->getRenderArgs(), &results);
                if (isFailureRetCode(stat)) {
                    identityInputNb = -1;
                } else {
                    results->getIdentityData(&identityInputNb, &identityInputTime, &inputIdentityView);
                }
            }

            if (identityInputNb != -1) {
                RectToRender r;
                r.identityInputNumber = identityInputNb;
                r.identityTime = identityInputTime;
                r.identityView = inputIdentityView;
                r.rect = splits[i];
                if (!r.rect.isNull()) {
                    finalRectsToRender->push_back(r);
                }
            } else {
                if (!nonIdentityRectSet) {
                    nonIdentityRectSet = true;
                    nonIdentityRect.rect = splits[i];
                } else {
                    nonIdentityRect.rect.x1 = std::min(splits[i].x1, nonIdentityRect.rect.x1);
                    nonIdentityRect.rect.x2 = std::max(splits[i].x2, nonIdentityRect.rect.x2);
                    nonIdentityRect.rect.y1 = std::min(splits[i].y1, nonIdentityRect.rect.y1);
                    nonIdentityRect.rect.y2 = std::max(splits[i].y2, nonIdentityRect.rect.y2);
                }
            }
        }
        if (nonIdentityRectSet) {
            if (!nonIdentityRect.rect.isNull()) {
                finalRectsToRender->push_back(nonIdentityRect);
            }
        }
    } // for each rect to render
} // checkIdentityRectsToRender


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
        assert(plane.getNumComponents() > 0);

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
                    return passThroughRenderArgs->requestRender(passThroughTime, passThroughView, mipMapLevel, plane, roiCanonical, requestData, &createdRequest);
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

        IsIdentityResultsPtr results = requestData->getIdentityResults();
        if (!results) {
            ActionRetCodeEnum stat = _publicInterface->isIdentity_public(true, requestData->getTime(), combinedScale, pixelRod, requestData->getView(), renderArgs, &results);
            if (isFailureRetCode(stat)) {
                return stat;
            }
            requestData->setIdentityResults(results);
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
        return requestData->getRenderArgs()->requestRender(inputTimeIdentity, inputIdentityView, requestData->getMipMapLevel(), requestData->getPlaneDesc(), canonicalRoi, requestData, &createdRequest);

    } else {
        assert(inputNbIdentity != -1);
        TreeRenderNodeArgsPtr passThroughRenderArgs = renderArgs->getInputRenderArgs(inputNbIdentity);
        if (!passThroughRenderArgs) {
            return eActionStatusFailed;
        }

        FrameViewRequestPtr createdRequest;
        return passThroughRenderArgs->requestRender(inputTimeIdentity, inputIdentityView, requestData->getMipMapLevel(), requestData->getPlaneDesc(), canonicalRoi, requestData, &createdRequest);

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
    distoRenderArgs->requestRender(requestData->getTime(), requestData->getView(), requestData->getMipMapLevel(), requestData->getPlaneDesc(), canonicalRoi, requestData, &inputRequest);

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
EffectInstance::Implementation::checkRestToRender(const FrameViewRequestPtr& requestData, std::list<RectToRender>* renderRects, bool* hasPendingTiles)
{
    // Compute the rectangle portion (renderWindow) left to render.
    // The renderwindow is the bounding box of all tiles that are left to render (not the tiles that are pending)

    RectI renderWindow;

    TreeRenderNodeArgsPtr renderArgs = requestData->getRenderArgs();

    // If the image is entirely cached, do not even compute it and insert it in the output planes map
    std::list<RectI> restToRender = requestData->getImagePlane()->getRestToRender(hasPendingTiles);

    if (!restToRender.empty()) {
        if (!renderArgs->getCurrentTilesSupport()) {
            // If the effect does not support tiles, we must render the RoI which has been set to the pixel RoD.
            renderWindow = requestData->getImagePlane()->getBounds();

        } else {
            // The temporary image will have the bounding box of tiles left to render.
#pragma message WARN("Make it smarter: instead compute ABCD rectangles")
            RectI restToRenderBoundingBox;
            {
                bool bboxSet = false;
                for (std::list<RectI>::const_iterator it = restToRender.begin(); it != restToRender.end(); ++it) {
                    if (!bboxSet) {
                        bboxSet = true;
                        restToRenderBoundingBox = *it;
                    } else {
                        restToRenderBoundingBox.merge(*it);
                    }
                }
            }
            renderWindow = restToRenderBoundingBox;
        }
    }

    if (renderWindow.isNull()) {
        return;
    }


    // If the effect does not support tiles, render everything again
    if (!renderArgs->getCurrentTilesSupport()) {
        // If not using the cache, render the full RoI
        // The RoI has already been set to the pixelRoD in this case
        RectToRender r;
        r.rect = renderWindow;
        r.identityInputNumber = -1;
        renderRects->push_back(r);
        return;
    }


    //
    // If the effect has multiple inputs (such as masks) (e.g: Merge), try to call isIdentity
    // if the RoDs do not intersect the RoI
    //
    // Edit: We don't do this anymore: it requires to render a lot of tiles which may
    // add a lot of overhead in a non-linear compositing tree.
    // Maybe this should be based on the render-cost of a tile given by the plug-in ?
    bool didSomething = false;

#if 0
    {
        RectD inputRodIntersection;
        RectI inputRodIntersectionPixel;
        if (canSplitRenderWindowWithIdentityRectangles(args, renderMappedScale, &inputRodIntersection)) {
            double par = _publicInterface->getAspectRatio(args.renderArgs, -1);
            inputRodIntersection.toPixelEnclosing(renderMappedScale, par, &inputRodIntersectionPixel);

            std::list<RectI> rectsToOptimize;
            rectsToOptimize.push_back(renderWindow);

            _publicInterface->checkIdentityRectsToRender(args.renderArgs, inputRodIntersectionPixel, rectsToOptimize, args.time, args.view, renderMappedScale, &planesToRender->rectsToRender);
            didSomething = true;
        }
    }
#endif

    if (!didSomething) {

        if (renderArgs->getCurrentRenderSafety() != eRenderSafetyFullySafeFrame) {

            // Plug-in did not enable host frame threading, it is expected that it handles multi-threading itself
            // with the multi-thread suite

            RectToRender r;
            r.rect = renderWindow;
            r.identityInputNumber = -1;
            renderRects->push_back(r);

        } else {

            // If plug-in wants host frame threading and there is only 1 rect to render, split it
            // in the number of available threads in the thread-pool

            const unsigned int nThreads = MultiThread::getNCPUsAvailable();

            std::vector<RectI> splits;
            if (nThreads > 1) {
                splits = renderWindow.splitIntoSmallerRects(nThreads);
            } else {
                splits.push_back(renderWindow);
            }

            for (std::vector<RectI>::iterator it = splits.begin(); it != splits.end(); ++it) {
                if (!it->isNull()) {
                    RectToRender r;
                    r.rect = *it;
                    r.identityInputNumber = -1;
                    renderRects->push_back(r);
                }
            }
        }
    } // !didSomething

} // checkRestToRender

void
EffectInstance::Implementation::fetchCachedTiles(const FrameViewRequestPtr& requestPassData,
                                                 const RectI& roiPixels,
                                                 unsigned int mappedMipMapLevel)
{

    TreeRenderNodeArgsPtr renderArgs = requestPassData->getRenderArgs();

    // Mark the image as draft in the cache
    bool isDraftRender = renderArgs->getParentRender()->isDraftRender();

    // The node frame/view hash to identify the image in the cache
    U64 nodeFrameViewHash;
    if (!requestPassData->getHash(&nodeFrameViewHash)) {
        HashableObject::ComputeHashArgs hashArgs;
        hashArgs.hashType = HashableObject::eComputeHashTypeTimeViewVariant;
        hashArgs.time = requestPassData->getTime();
        hashArgs.view = requestPassData->getView();
        hashArgs.render = renderArgs;
        nodeFrameViewHash = _publicInterface->computeHash(hashArgs);
        requestPassData->setHash(nodeFrameViewHash);
    }

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
            initArgs.layer = requestPassData->getPlaneDesc();

            // Do not allocate the image buffers yet, instead do it before rendering.
            // We need to create the image before because it does the cache look-up itself, and we don't want to recurse if
            // there's something cached.
            initArgs.delayAllocation = true;
        }


        // Image::create will lookup the cache (if asked for)
        // Since multiple threads may want to access to the same image in the cache concurrently,
        // the first thread that gets onto a tile to render will render it and lock-out other threads
        // until it is rendered entirely.
        image = Image::create(initArgs);
        requestPassData->setImagePlane(image);
    } // image

} // fetchOrCreateOutputPlanes


ActionRetCodeEnum
EffectInstance::Implementation::allocateRenderBackendStorageForRenderRects(const FrameViewRequestPtr& requestData, const RectI& roiPixels, std::list<RectToRender>* renderRects)
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
    if (attachedStroke && attachedStroke->isCurrentlyDrawing()) {

        ImagePtr accumBuffer = _publicInterface->getNode()->getLastRenderedImage();

        // If we do not have an accumulation buffer, we follow the usual code path
        if (accumBuffer) {

            // We got an existing buffer
            // The accumulation buffer should be using the same OpenGL context as the current render's one!
            assert((accumBuffer->getStorageMode() == eStorageModeGLTex && accumBuffer->getGLImageStorage()->getOpenGLContext() == glContext) || accumBuffer->getStorageMode() != eStorageModeGLTex);

            // Ensure the accumBuffer contains at least the RoI
            accumBuffer->ensureBounds(roiPixels);

            // We must render the last stroke movement bounding box
            RectI drawingLastMovementBBoxPixel;
            {
                RectD lastStrokeRoD = attachedStroke->getLastStrokeMovementBbox();
                double par = _publicInterface->getBitDepth(requestData->getRenderArgs(), -1);
                lastStrokeRoD.toPixelEnclosing(mappedCombinedScale, par, &drawingLastMovementBBoxPixel);
            }
            {
                planesToRender->rectsToRender.clear();
                RectToRender r;
                r.rect = drawingLastMovementBBoxPixel;
                r.identityInputNumber = -1;
                planesToRender->rectsToRender.push_back(r);
            }

            PlaneToRender& plane = planesToRender->planes[requestedComponents.front()];
            plane.cacheImage = plane.tmpImage = accumBuffer;
            return;
        }
    } // isDrawing


    if (planesToRender->cacheAccess != eCacheAccessModeNone && (cacheBufferLayout != pluginBufferLayout) && !renderWindow.isNull()) {

        // The bitdepth of the image
        ImageBitDepthEnum outputBitDepth = _publicInterface->getBitDepth(args.renderArgs, -1);

        for (std::map<ImagePlaneDesc, PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {

            // The image planes left are not entirely cached (or not at all): create a temporary image
            // with the memory layout supported by the plug-in that we will write to.
            // When the temporary image will be destroyed, it will automatically copy pixels
            // to the cache image, which in turn when destroyed will push the tiles to the cache.
            Image::InitStorageArgs tmpImgInitArgs;
            {
                tmpImgInitArgs.bounds = renderWindow;
                tmpImgInitArgs.renderArgs = args.renderArgs;
                tmpImgInitArgs.cachePolicy = eCacheAccessModeNone;
                tmpImgInitArgs.bufferFormat = pluginBufferLayout;
                tmpImgInitArgs.mipMapLevel = it->second.cacheImage->getMipMapLevel();
                tmpImgInitArgs.proxyScale = it->second.cacheImage->getProxyScale();
                tmpImgInitArgs.glContext = planesToRender->glContext;
                switch (planesToRender->backendType) {
                    case eRenderBackendTypeOpenGL:
                        tmpImgInitArgs.storage = eStorageModeGLTex;
                        break;
                    case eRenderBackendTypeCPU:
                    case eRenderBackendTypeOSMesa:
                        tmpImgInitArgs.storage = eStorageModeRAM;
                        break;
                }
                tmpImgInitArgs.bitdepth = outputBitDepth;
                tmpImgInitArgs.layer = it->first;
                // Do not allocate the image buffers yet since we initialize the image BEFORE recursing on input nodes.
                // We need to create the image before because it does the cache look-up itself, and we don't want to recurse if
                // there's something cached.
                // Instead we allocate the necessary image buffers AFTER the recursion on input nodes returns.
                tmpImgInitArgs.delayAllocation = delayAllocation;

            }
            it->second.tmpImage = Image::create(tmpImgInitArgs);
        } // for each plane to render
        
    } // useCache

} // allocateRenderBackendStorageForRenderRects

ActionRetCodeEnum
EffectInstance::Implementation::launchInternalRender(const FrameViewRequestPtr& requestData, const std::list<RectToRender>& renderRects)
{

    // If we reach here, it can be either because the planes are cached or not, either way
    // the planes are NOT a total identity, and they may have some content left to render.
    ActionRetCodeEnum renderRetCode = eActionStatusOK;

    // There should always be at least 1 plane to render (The color plane)
    assert(!renderRects.empty());

    EffectInstancePtr renderInstance;
    //
    // Figure out If this node should use a render clone rather than execute renderRoIInternal on the main (this) instance.
    // Reasons to use a render clone is because a plug-in is eRenderSafetyInstanceSafe or does not support
    // concurrent GL renders.
    //

    bool useRenderClone = false;

    RenderSafetyEnum safety = args.renderArgs->getCurrentRenderSafety();

    useRenderClone |= (safety == eRenderSafetyInstanceSafe && !_publicInterface->getNode()->isDuringPaintStrokeCreation());

    useRenderClone |= safety != eRenderSafetyUnsafe && planesToRender->backendType == eRenderBackendTypeOpenGL && !_publicInterface->supportsConcurrentOpenGLRenders();

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

    // Bind the OpenGL context if there's any
    OSGLContextAttacherPtr glContextAttacher;
    if (planesToRender->glContext) {
        glContextAttacher = OSGLContextAttacher::create(planesToRender->glContext);
        glContextAttacher->attach();
    }

    RenderScale combinedScale = EffectInstance::Implementation::getCombinedScale(planesToRender->mappedMipMapLevel, args.proxyScale);

    if (planesToRender->backendType == eRenderBackendTypeOpenGL ||
        planesToRender->backendType == eRenderBackendTypeOSMesa) {
        assert(planesToRender->glContext);
        ActionRetCodeEnum stat = renderInstance->attachOpenGLContext_public(args.time, args.view, combinedScale, args.renderArgs, planesToRender->glContext, &planesToRender->glContextData);
        if (isFailureRetCode(stat)) {
            renderRetCode = stat;
        }
    }
    if (renderRetCode == eActionStatusOK) {

        renderRetCode = renderInstance->renderForClone(args, planesToRender);

        if (planesToRender->backendType == eRenderBackendTypeOpenGL ||
            planesToRender->backendType == eRenderBackendTypeOSMesa) {

            // If the plug-in doesn't support concurrent OpenGL renders, release the lock that was taken in the call to attachOpenGLContext_public() above.
            // For safe plug-ins, we call dettachOpenGLContext_public when the effect is destroyed in Node::deactivate() with the function EffectInstance::dettachAllOpenGLContexts().
            // If we were the last render to use this context, clear the data now

            if ( planesToRender->glContextData->getHasTakenLock() ||
                !_publicInterface->supportsConcurrentOpenGLRenders() ||
                planesToRender->glContextData.use_count() == 1) {

                renderInstance->dettachOpenGLContext_public(args.renderArgs, planesToRender->glContext, planesToRender->glContextData);
            }
        }
    }
    if (useRenderClone) {
        _publicInterface->releaseRenderInstance(renderInstance);
    }
    

    return renderRetCode;
} // EffectInstance::Implementation::renderRoILaunchInternalRender



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
                            ActionRetCodeEnum stat = inputRenderArgs->requestRender(inputTime, viewIt->first, mipMapLevel, *planeIt, inputRoI, requestPassData, &createdRequest);
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
EffectInstance::requestRender(const FrameViewRequestPtr& requestData,
                              const RectD& roiCanonical,
                              int inputNbInRequester,
                              const FrameViewRequestPtr& requester)
{


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
        GetRegionOfDefinitionResultsPtr results = requestData->getRegionOfDefinitionResults();
        if (!results) {
            ActionRetCodeEnum stat = getRegionOfDefinition_public(requestData->getTime(), mappedCombinedScale, requestData->getView(), renderArgs, &results);
            if (isFailureRetCode(stat)) {
                return stat;
            }
            requestData->setRegionOfDefinitionResults(results);
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
            return eActionStatusOK;
        }
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Handle identity effects /////////////////////////////////////////////////////////////////
    const double par = getAspectRatio(renderArgs, -1);
    {
        bool isIdentity;
        ActionRetCodeEnum upstreamRetCode = _imp->handleIdentityEffect(par, rod, mappedCombinedScale, roiCanonical, requestData, &isIdentity);
        if (isIdentity) {
            return upstreamRetCode;
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
    CacheAccessModeEnum cachePolicy = _imp->shouldRenderUseCache(renderArgs, requestData);
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
    _imp->fetchCachedTiles(requestData, renderMappedRoI, mappedMipMapLevel);

    std::list<RectToRender> renderRects;
    bool hasPendingTiles;
    _imp->checkRestToRender(requestData, &renderRects, &hasPendingTiles);

    // If there's nothing to render, do not even add the inputs as needed dependencies.
    if (!renderRects.empty() || hasPendingTiles) {

        ActionRetCodeEnum upstreamRetCode = _imp->handleUpstreamFramesNeeded(requestData, mappedCombinedScale, mappedMipMapLevel, roundedCanonicalRoI, inputLayersNeeded);
        
        if (isFailureRetCode(upstreamRetCode)) {
            return upstreamRetCode;
        }
    }
    return eActionStatusOK;
} // requestRender


static void invalidateCachedPlanesToRender(const EffectInstance::ImagePlanesToRenderPtr& planes)
{
    for (std::map<ImagePlaneDesc, PlaneToRender>::const_iterator it = planes->planes.begin(); it != planes->planes.end(); ++it) {
        it->second.cacheImage->discardTiles();
    }
}

ActionRetCodeEnum
EffectInstance::launchRender(const FrameViewRequestPtr& requestData)
{

    TreeRenderNodeArgsPtr renderArgs = requestData->getRenderArgs();
    const double par = getAspectRatio(renderArgs, -1);
    RenderScale mappedCombinedScale = EffectInstance::Implementation::getCombinedScale(requestData->getRenderMappedMipMapLevel(), renderArgs->getParentRender()->getProxyScale());
    RectI renderMappedRoI;
    requestData->getCurrentRoI().toPixelEnclosing(mappedCombinedScale, par, &renderMappedRoI);

    ImagePtr cacheImage = requestData->getImagePlane();
    if (requestData->getCachePolicy() == eCacheAccessModeReadWrite) {
        // Allocate the cache storage image now.
        // Already cached tiles will be left untouched.
        cacheImage->ensureBuffersAllocated();
    }

    std::list<RectToRender> renderRects;
    bool hasPendingTiles;
    _imp->checkRestToRender(requestData, &renderRects, &hasPendingTiles);
    while (!renderRects.empty() || hasPendingTiles) {

        ActionRetCodeEnum renderRetCode = eActionStatusOK;
        // There may be no rectangles to render if all rectangles are pending (i.e: this render should wait for another thread
        // to complete the render first)
        if (!renderRects.empty()) {
            renderRetCode = _imp->allocateRenderBackendStorageForRenderRects(requestData, renderMappedRoI, &renderRects);
            if (isFailureRetCode(renderRetCode)) {
                return renderRetCode;
            }
            renderRetCode = _imp->launchInternalRender(requestData, renderRects);
            if (isFailureRetCode(renderRetCode)) {
                return renderRetCode;
            }
        }


        // The render went OK: push the cache images tiles to the cache

        if (requestData->getCachePolicy() != eCacheAccessModeNone) {

            // Push to the cache the tiles that we rendered
            cacheImage->pushTilesToCacheIfNotAborted();

            // Wait for any pending results. After this line other threads that should have computed should be done
            cacheImage->waitForPendingTiles();
        }

        checkRestToRender(requestData, &renderRects, &hasPendingTiles);
    } // while there is still something not rendered


    // Now that this effect has rendered, clear pre-rendered inputs
    requestPassData->clearPreRenderedInputs();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Check for failure or abortion ///////////////////////////////////////////////////////////

    // If using GPU and out of memory retry on CPU if possible
    if (renderRetCode == eActionStatusOutOfMemory && planesToRender->backendType == eRenderBackendTypeOpenGL) {
        if (args.renderArgs->getCurrentRenderOpenGLSupport() != ePluginOpenGLRenderSupportYes) {
            // The plug-in can only use GPU or doesn't support GPU
            invalidateCachedPlanesToRender(planesToRender);
            return eActionStatusFailed;
        }
        boost::scoped_ptr<RenderRoIArgs> newArgs( new RenderRoIArgs(args) );
        newArgs->allowGPURendering = false;
        return renderPlanes(*newArgs, planesToRender, results);
    }

    if (renderRetCode != eActionStatusOK) {
        invalidateCachedPlanesToRender(planesToRender);
        return renderRetCode;
    }

    // If the node did not support render scale and the mipmap level rendered was different than what was requested, downscale the image.
    if (args.mipMapLevel != planesToRender->mappedMipMapLevel) {
        assert(args.mipMapLevel > 0);
        for (std::map<ImagePlaneDesc, ImagePtr>::iterator it = results->outputPlanes.begin(); it != results->outputPlanes.end(); ++it) {
            assert(it->second->getMipMapLevel() == 0);
            ImagePtr downscaledImage = it->second->downscaleMipMap(it->second->getBounds(), args.mipMapLevel);
            it->second = downscaledImage;
        }
    }

    // Termination, check that we rendered things correctly or we should have failed earlier otherwise.
#ifdef DEBUG
    if ( results->outputPlanes.size() != args.components.size() ) {
        qDebug() << "Requested:";
        for (std::list<ImagePlaneDesc>::const_iterator it = args.components.begin(); it != args.components.end(); ++it) {
            qDebug() << it->getPlaneID().c_str();
        }
        qDebug() << "But rendered:";
        for (std::map<ImagePlaneDesc, ImagePtr>::iterator it = results->outputPlanes.begin(); it != results->outputPlanes.end(); ++it) {
            if (it->second) {
                qDebug() << it->first.getPlaneID().c_str();
            }
        }
    }
    assert( !results->outputPlanes.empty() );
#endif
    
    return eActionStatusOK;
} // renderPlanes


ActionRetCodeEnum
EffectInstance::renderForClone(const RenderRoIArgs& args,
                               const ImagePlanesToRenderPtr & planesToRender)
{
    assert( !planesToRender->planes.empty() && !planesToRender->rectsToRender.empty() );

    // Notify the gui we're rendering
    NotifyRenderingStarted_RAII renderingNotifier(getNode().get());

    // If this node is not sequential we at least have to bracket the render action with a call to begin and end sequence render.

    RangeD sequenceRange;
    {
        GetFrameRangeResultsPtr rangeResults;
        ActionRetCodeEnum stat = getFrameRange_public(args.renderArgs, &rangeResults);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        rangeResults->getFrameRangeResults(&sequenceRange);
    }

    RenderScale combinedScale = EffectInstance::Implementation::getCombinedScale(planesToRender->mappedMipMapLevel, args.proxyScale);

    // We only need to call begin if we've not already called it.
    bool callBeginSequenceRender = false;
    if ( !isWriter() || (args.renderArgs->getCurrentRenderSequentialPreference() == eSequentialPreferenceNotSequential) ) {
        callBeginSequenceRender = true;
    }

    bool isPlayback = args.renderArgs->getParentRender()->isPlayback();

    if (callBeginSequenceRender) {
        ActionRetCodeEnum stat = beginSequenceRender_public(args.time,
                                                            args.time,
                                                            1 /*frameStep*/,
                                                            !appPTR->isBackground() /*interactive*/,
                                                            combinedScale,
                                                            isPlayback,
                                                            !isPlayback,
                                                            args.renderArgs->getParentRender()->isDraftRender(),
                                                            args.view,
                                                            planesToRender->backendType,
                                                            planesToRender->glContextData,
                                                            args.renderArgs);

        if (isFailureRetCode(stat)) {
            return stat;
        }
    }


    // Get the main input image to copy channels from it if a RGBA checkbox is unchecked
    int mainInputNb = getNode()->getPreferredInput();
    if ( (mainInputNb != -1) && isInputMask(mainInputNb) ) {
        mainInputNb = -1;
    }

    ImagePtr mainInputImage;
    if (mainInputNb != -1) {
        GetImageInArgs inArgs;
        std::map<int, std::list<ImagePlaneDesc> >::const_iterator foundNeededLayers = planesToRender->inputLayersNeeded.find(mainInputNb);
        if (foundNeededLayers != planesToRender->inputLayersNeeded.end()) {
            inArgs.layers = &foundNeededLayers->second;
            inArgs.inputNb = mainInputNb;
            inArgs.currentTime = args.time;
            inArgs.currentView = args.view;
            inArgs.inputTime = inArgs.currentTime;
            inArgs.inputView = inArgs.currentView;
            inArgs.inputMipMapLevel = planesToRender->mappedMipMapLevel;
            inArgs.inputProxyScale = args.proxyScale;
            inArgs.currentScale = combinedScale;
            {

                //inArgs.layers = args.components.front();
            }
            inArgs.renderBackend = &planesToRender->backendType;
            inArgs.renderArgs = args.renderArgs;
            GetImageOutArgs outArgs;
            bool gotImage = getImagePlanes(inArgs, &outArgs);
            if (gotImage) {
                assert(!outArgs.imagePlanes.empty());
                mainInputImage = outArgs.imagePlanes.begin()->second;
            }

        }
    }


    const bool attemptHostFrameThreading =
#ifndef NATRON_HOSTFRAMETHREADING_SEQUENTIAL
    args.renderArgs->getCurrentRenderSafety() == eRenderSafetyFullySafeFrame &&
    planesToRender->rectsToRender.size() > 1 &&
    planesToRender->backendType == eRenderBackendTypeCPU;
#else
    true;
#endif

    if (!attemptHostFrameThreading) {

        for (std::list<RectToRender>::const_iterator it = planesToRender->rectsToRender.begin(); it != planesToRender->rectsToRender.end(); ++it) {

            ActionRetCodeEnum functorRet = _imp->tiledRenderingFunctor(*it, &args, mainInputImage, planesToRender);
            if (isFailureRetCode(functorRet)) {
                return functorRet;
            }

        } // for (std::list<RectI>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {

    } else { // attemptHostFrameThreading


        boost::scoped_ptr<Implementation::TiledRenderingFunctorArgs> tiledArgs(new Implementation::TiledRenderingFunctorArgs);
        tiledArgs->args = &args;
        tiledArgs->mainInputImage = mainInputImage;
        tiledArgs->planesToRender = planesToRender;

        std::list<RectToRender> rectsToRenderList = planesToRender->rectsToRender;

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
                                                              boost::bind(&EffectInstance::Implementation::tiledRenderingFunctor,
                                                                          _imp.get(),
                                                                          *tiledArgs,
                                                                          _1,
                                                                          curThread) );
        if (isThreadPoolThread) {
            ActionRetCodeEnum retCode = _imp->tiledRenderingFunctor(*tiledArgs, lastRectToRender, curThread);
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

        ActionRetCodeEnum stat = endSequenceRender_public(args.time,
                                                          args.time,
                                                          1 /*frameStep*/,
                                                          !appPTR->isBackground() /*interactive*/,
                                                          combinedScale,
                                                          isPlayback,
                                                          !isPlayback,
                                                          args.renderArgs->getParentRender()->isDraftRender(),
                                                          args.view,
                                                          planesToRender->backendType,
                                                          planesToRender->glContextData,
                                                          args.renderArgs);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        
    }
    return eActionStatusOK;
    
} // renderRoIInternal

NATRON_NAMESPACE_EXIT;
