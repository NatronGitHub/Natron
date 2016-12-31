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

#include <map>
#include <sstream>
#include <algorithm> // min, max
#include <fstream>
#include <cassert>
#include <stdexcept>

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

#include "Global/MemoryInfo.h"
#include "Global/QtCompat.h"

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/BlockingBackgroundRender.h"
#include "Engine/DiskCacheNode.h"
#include "Engine/EffectInstanceTLSData.h"
#include "Engine/EffectOpenGLContextData.h"
#include "Engine/Distorsion2D.h"
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
EffectInstance::optimizeRectsToRender(const TreeRenderNodeArgsPtr& renderArgs,
                                      const RectI & inputsRoDIntersection,
                                      const std::list<RectI> & rectsToRender,
                                      const TimeValue time,
                                      const ViewIdx view,
                                      const RenderScale & renderMappedScale,
                                      std::list<EffectInstance::RectToRender>* finalRectsToRender)
{
    for (std::list<RectI>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
        if (it->isNull()) {
            continue;
        }
        std::vector<RectI> splits = it->splitIntoSmallerRects(0);
        EffectInstance::RectToRender nonIdentityRect;
        nonIdentityRect.identityInputNumber = -1;
        nonIdentityRect.identityTime = 0;
        nonIdentityRect.rect.x1 = INT_MAX;
        nonIdentityRect.rect.x2 = INT_MIN;
        nonIdentityRect.rect.y1 = INT_MAX;
        nonIdentityRect.rect.y2 = INT_MIN;

        bool nonIdentityRectSet = false;
        for (std::size_t i = 0; i < splits.size(); ++i) {
            TimeValue identityInputTime(0.);
            int identityInputNb = -1;

            ViewIdx inputIdentityView(view);
            if ( !splits[i].intersects(inputsRoDIntersection) ) {
                IsIdentityResultsPtr results;
                StatusEnum stat = isIdentity_public(false, time, renderMappedScale, splits[i], view, renderArgs, &results);
                if (stat == eStatusFailed) {
                    identityInputNb = -1;
                } else {
                    results->getIdentityData(&identityInputNb, &identityInputTime, &inputIdentityView);
                }
            }

            if (identityInputNb == -1) {
                EffectInstance::RectToRender r;

                // Walk along the identity branch until we find the non identity input, or NULL in we case we will
                // just render black and transparent
                EffectInstancePtr identityInput = getInput(identityInputNb);
                if (identityInput) {
                    for (;; ) {
                        IsIdentityResultsPtr results;
                        StatusEnum stat = identityInput->isIdentity_public(false, time, renderMappedScale, splits[i], view, renderArgs, &results);
                        if (stat == eStatusFailed) {
                            identityInputNb = -1;
                        } else {
                            results->getIdentityData(&identityInputNb, &identityInputTime, &inputIdentityView);
                        }

                        if (identityInputNb == -2) {
                            break;
                        }
                        EffectInstancePtr subIdentityInput = identityInput->getInput(identityInputNb);
                        if (subIdentityInput == identityInput) {
                            break;
                        }

                        identityInput = subIdentityInput;
                        if (!subIdentityInput) {
                            break;
                        }
                    }
                }
                r.identityInputNumber = identityInputNb;
                r.identityInput = identityInput;
                r.identityTime = identityInputTime;
                r.identityView = inputIdentityView;
                r.rect = splits[i];
                if (!r.rect.isNull()) {
                    finalRectsToRender->push_back(r);
                }
            } else {
                nonIdentityRectSet = true;
                nonIdentityRect.rect.x1 = std::min(splits[i].x1, nonIdentityRect.rect.x1);
                nonIdentityRect.rect.x2 = std::max(splits[i].x2, nonIdentityRect.rect.x2);
                nonIdentityRect.rect.y1 = std::min(splits[i].y1, nonIdentityRect.rect.y1);
                nonIdentityRect.rect.y2 = std::max(splits[i].y2, nonIdentityRect.rect.y2);
            }
        }
        if (nonIdentityRectSet) {
            if (!nonIdentityRect.rect.isNull()) {
                finalRectsToRender->push_back(nonIdentityRect);
            }
        }
    }
} // optimizeRectsToRender



/**
 * @brief This function determines the planes to render and calls recursively on upstream nodes unavailable planes
 **/
RenderRoIRetCode
EffectInstance::Implementation::determinePlanesToRender(const EffectInstance::RenderRoIArgs& args,
                                                        std::map<int, std::list<ImageComponents> >* inputLayersNeeded,
                                                        std::list<ImageComponents> *planesToRender,
                                                        std::bitset<4>* processChannels,
                                                        std::map<ImageComponents, ImagePtr>* outputPlanes)
{


    std::list<ImageComponents> layersProduced, passThroughLayers;
    int passThroughInputNb;
    TimeValue passThroughTime;
    ViewIdx passThroughView;
    bool processAllLayers;
    {

        GetComponentsResultsPtr results;
        StatusEnum stat = _publicInterface->getComponents_public(args.time, args.view, args.renderArgs, &results);
        if (stat == eStatusFailed) {
            return eRenderRoIRetCodeFailed;
        }
        results->getResults(inputLayersNeeded, &layersProduced, &passThroughLayers, &passThroughInputNb, &passThroughTime, &passThroughView, processChannels, &processAllLayers);
    }

    std::list<ImageComponents> componentsToFetchUpstream;
    /*
     * For all requested planes, check which components can be produced in output by this node.
     * If the components are from the color plane, if another set of components of the color plane is present
     * we try to render with those instead.
     */
    for (std::list<ImageComponents>::const_iterator it = args.components.begin(); it != args.components.end(); ++it) {

        // We may not request paired layers
        assert( *it && !it->isPairedComponents() );
        assert(it->getNumComponents() > 0);

        std::list<ImageComponents>::const_iterator foundProducedLayer = ImageComponents::findEquivalentLayer(*it, layersProduced.begin(), layersProduced.end());
        if (foundProducedLayer != layersProduced.end()) {

            // Make sure that we ask to render what is produced by this effect rather than the original requested component.
            // e.g: The color.RGB might have been requested but this effect might produce color.RGBA
            planesToRender->push_back(*foundProducedLayer);
            continue;
        }

        // If this effect is set to process all, then process the plane even if not produced by this node.
        if (processAllLayers) {
            planesToRender->push_back(*it);
        }

        // This layer was not found, look in pass-through input if this plug-in allows it
        if (passThroughInputNb == -1) {
            continue;
        }

        std::list<ImageComponents>::const_iterator foundInPassThroughPlanes = ImageComponents::findEquivalentLayer(*it, passThroughLayers.begin(), passThroughLayers.end());
        if (foundInPassThroughPlanes != passThroughLayers.end()) {
            componentsToFetchUpstream.push_back(*foundInPassThroughPlanes);
        }

    }

    if (!componentsToFetchUpstream.empty()) {
        return eRenderRoIRetCodeOk;
    }

    assert(passThroughInputNb != -1);
    EffectInstancePtr passThroughInput = _publicInterface->getInput(passThroughInputNb);
    if (!passThroughInput) {
        return eRenderRoIRetCodeFailed;
    }

    boost::scoped_ptr<RenderRoIArgs> inArgs ( new RenderRoIArgs(args) );
    inArgs->components = componentsToFetchUpstream;

    RenderRoIResults inputResults;
    RenderRoIRetCode inputRetCode = passThroughInput->renderRoI(*inArgs, &inputResults);

    if ( (inputRetCode == eRenderRoIRetCodeAborted) || (inputRetCode == eRenderRoIRetCodeFailed) || inputResults.outputPlanes.empty() ) {
        return inputRetCode;
    }

    outputPlanes->insert(inputResults.outputPlanes.begin(),  inputResults.outputPlanes.end());

    return eRenderRoIRetCodeOk;


} // determinePlanesToRender


RenderRoIRetCode
EffectInstance::Implementation::handleIdentityEffect(const EffectInstance::RenderRoIArgs& args,
                                                     double par,
                                                     const RectD& rod,
                                                     const std::list<ImageComponents> &requestedComponents,
                                                     const std::map<int, std::list<ImageComponents> >& neededComps,
                                                     RenderRoIResults* results,
                                                     bool *isIdentity)
{
    TimeValue inputTimeIdentity;
    int inputNbIdentity;
    ViewIdx inputIdentityView;

    {
        // If the effect is identity over the whole RoD then we can forward the render completly to the identity node
        RectI pixelRod;
        rod.toPixelEnclosing(args.scale, par, &pixelRod);

        IsIdentityResultsPtr results;
        StatusEnum stat = _publicInterface->isIdentity_public(true, args.time, args.scale, pixelRod, args.view, args.renderArgs, &results);
        if (stat == eStatusFailed) {
            return eRenderRoIRetCodeFailed;
        }
        results->getIdentityData(&inputNbIdentity, &inputTimeIdentity, &inputIdentityView);

    }
    *isIdentity = inputNbIdentity != -1;
    if (*isIdentity) {

        if (inputNbIdentity == -2) {

            // Be safe: we may hit an infinite recursion without this check
            assert(inputTimeIdentity != args.time);
            if ( inputTimeIdentity == args.time) {
                return eRenderRoIRetCodeFailed;
            }

            // This special value of -2 indicates that the plugin is identity of itself at another time
            boost::scoped_ptr<RenderRoIArgs> argCpy ( new RenderRoIArgs(args) );
            argCpy->time = inputTimeIdentity;
            argCpy->view = inputIdentityView;

            return _publicInterface->renderRoI(*argCpy, results);

        }

        RectD canonicalRoI;
        // WRONG! We can't clip against the RoD of *this* effect. We should clip against the RoD of the input effect, but this is done
        // later on for us already.
        // args.roi.toCanonical(args.mipMapLevel, rod, &canonicalRoI);
        args.roi.toCanonical_noClipping(args.scale, par,  &canonicalRoI);

        // Check if identity input really exists, otherwise fail
        EffectInstancePtr inputEffectIdentity = _publicInterface->getInput(inputNbIdentity);
        if (!inputEffectIdentity) {
            return eRenderRoIRetCodeFailed;
        }



        boost::scoped_ptr<RenderRoIArgs> inputArgs ( new RenderRoIArgs(args) );
        inputArgs->time = inputTimeIdentity;
        inputArgs->view = inputIdentityView;


        // If the node has a layer selector knob, request what was selected in input.
        // If the node does not have a layer selector knob then just fetch the originally requested layers.
        bool fetchUserSelectedComponentsUpstream = _publicInterface->getNode()->getChannelSelectorKnob(inputNbIdentity).get() != 0;

        if (!fetchUserSelectedComponentsUpstream) {
            inputArgs->components = requestedComponents;
        } else {
            std::map<int, std::list<ImageComponents> >::const_iterator foundCompsNeeded = neededComps.find(inputNbIdentity);
            if ( foundCompsNeeded != neededComps.end() ) {
                inputArgs->components = foundCompsNeeded->second;
            }
        }

        // Save the planes that may already have been fetched from pass-through planes
        std::map<ImageComponents, ImagePtr> finalPlanes = results->outputPlanes;
        results->outputPlanes.clear();

        RenderRoIRetCode ret =  inputEffectIdentity->renderRoI(*inputArgs, results);
        if (ret == eRenderRoIRetCodeOk) {
            // Add planes to existing outputPlanes
            finalPlanes.insert( results->outputPlanes.begin(), results->outputPlanes.end() );
        }
        return ret;
    } // if (identity)

    assert(!*isIdentity);
    return eRenderRoIRetCodeOk;
} // EffectInstance::Implementation::handleIdentityEffect

RenderRoIRetCode
EffectInstance::Implementation::handleConcatenation(const EffectInstance::RenderRoIArgs& args,
                                                    const RenderScale& renderMappedScale,
                                                    RenderRoIResults* results,
                                                    bool *concatenated)
{
    *concatenated = false;
    bool useTransforms = appPTR->getCurrentSettings()->isTransformConcatenationEnabled();
    if (useTransforms && args.caller && args.caller.get() != _publicInterface && args.renderArgs->getCurrentDistortSupport()) {

        assert(args.inputNbInCaller != -1);

        // If the caller can apply a distorsion, then check if this effect has a distorsion

        if (args.caller->getInputCanReceiveDistorsion(args.inputNbInCaller)) {

            GetDistorsionResultsPtr actionResults;
            StatusEnum stat = _publicInterface->getDistorsion_public(args.time, renderMappedScale, args.view, args.renderArgs, &actionResults);
            if (stat == eStatusFailed) {
                return eRenderRoIRetCodeFailed;
            }
            const DistorsionFunction2DPtr& disto = actionResults->getDistorsionResults();
            if (stat == eStatusOK && disto->inputNbToDistort != -1) {

                // Recurse on input given by plug-in
                EffectInstancePtr distoInput = _publicInterface->getInput(disto->inputNbToDistort);
                if (!distoInput) {
                    return eRenderRoIRetCodeFailed;
                }
                boost::scoped_ptr<RenderRoIArgs> argsCpy(new RenderRoIArgs(args));
                argsCpy->caller = _publicInterface->shared_from_this();
                argsCpy->inputNbInCaller = disto->inputNbToDistort;
                RenderRoIRetCode ret = distoInput->renderRoI(*argsCpy, results);
                if (ret != eRenderRoIRetCodeOk) {
                    return ret;
                }

                // Create the stack if it was not already
                if (!results->distorsionStack) {
                    results->distorsionStack.reset(new Distorsion2DStack);
                }
                
                // And then push our distorsion to the stack...
                results->distorsionStack->pushDistorsion(disto);

                *concatenated = true;
            }
        }

    }
    return eRenderRoIRetCodeOk;
} // handleConcatenation


void
EffectInstance::Implementation::fetchOrCreateOutputPlanes(const RenderRoIArgs & args,
                                                          const FrameViewRequestPtr& requestPassData,
                                                          CacheAccessModeEnum cacheAccess,
                                                          const ImagePlanesToRenderPtr &planesToRender,
                                                          const std::list<ImageComponents>& requestedComponents,
                                                          const RectI& roi,
                                                          const RenderScale& renderMappedScale,
                                                          std::map<ImageComponents, ImagePtr>* outputPlanes)
{


    // Mark the image as draft in the cache
    bool isDraftRender = args.renderArgs->getParentRender()->isDraftRender();

    // The node frame/view hash to identify the image in the cache
    U64 nodeFrameViewHash;
    if (!requestPassData->getHash(&nodeFrameViewHash)) {
        HashableObject::ComputeHashArgs hashArgs;
        hashArgs.hashType = HashableObject::eComputeHashTypeTimeViewVariant;
        hashArgs.time = args.time;
        hashArgs.view = args.view;
        hashArgs.render = args.renderArgs;
        nodeFrameViewHash = _publicInterface->computeHash(hashArgs);
    }

    // The bitdepth of the image
    ImageBitDepthEnum outputBitDepth = _publicInterface->getBitDepth(args.renderArgs, -1);

    // The image format supported by the plug-in (co-planar, packed RGBA, etc...)
    ImageBufferLayoutEnum pluginBufferLayout = _publicInterface->getPreferredBufferLayout();

    std::map<ImageComponents, PlaneToRender> planes;
    for (std::list<ImageComponents>::const_iterator it = requestedComponents.begin(); it != requestedComponents.end(); ++it) {


        Image::InitStorageArgs initArgs;
        {
            initArgs.bounds = roi;
            initArgs.cachePolicy = cacheAccess;
            initArgs.renderArgs = args.renderArgs;
            initArgs.scale = renderMappedScale;
            initArgs.isDraft = isDraftRender;
            initArgs.nodeHashKey = nodeFrameViewHash;
            if (cacheAccess != eCacheAccessModeNone) {
                // Cache format is tiled and mmap backend
                initArgs.storage = eStorageModeDisk;
                initArgs.bufferFormat = eImageBufferLayoutMonoChannelTiled;
            } else {
                switch (planesToRender->backendType) {
                    case eRenderBackendTypeOpenGL:
                        initArgs.storage = eStorageModeGLTex;
                        break;
                    case eRenderBackendTypeCPU:
                    case eRenderBackendTypeOSMesa:
                        initArgs.storage = eStorageModeRAM;
                        break;
                }
                initArgs.bufferFormat = pluginBufferLayout;
            }
            initArgs.bitdepth = outputBitDepth;
            initArgs.layer = *it;
        }
        
        PlaneToRender &plane = planes[*it];

        // Image::create will lookup the cache (if asked for)
        // Since multiple threads may want to access to the same image in the cache concurrently,
        // the first thread that gets onto a tile to render will render it and lock-out other threads
        // until it is rendered entirely.
        plane.cacheImage = Image::create(initArgs);
        plane.tmpImage = plane.cacheImage;

    } // for each requested plane

    RectI renderWindow = roi;

    if (cacheAccess != eCacheAccessModeNone) {

        // Remove from planes to render the planes that are already cached
        // The render window is the bounding box of all portions left to render across all planes.


        {
            bool renderWindowSet = false;
            for (std::map<ImageComponents, PlaneToRender>::const_iterator it = planes.begin(); it != planes.end(); ++it) {

                // If the image is entirely cached, do not even compute it and insert it in the output planes map
                std::list<RectI> restToRender = it->second.cacheImage->getRestToRender();
                if (restToRender.empty()) {
                    (*outputPlanes)[it->first] = it->second.cacheImage;
                    continue;
                }

                planesToRender->planes.insert(*it);

                // The temporary image will have the bounding box of rectangles left to render.
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
                if (!renderWindowSet) {
                    renderWindowSet = true;
                    renderWindow = restToRenderBoundingBox;
                } else {
                    renderWindow.merge(restToRenderBoundingBox);
                }
            } // for each plane
        }

        for (std::map<ImageComponents, PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {

            // The image planes left are not entirely cached (or not at all): create a temporary image
            // with the memory layout supported by the plug-in that we will write to.
            // When the temporary image will be destroyed, it will automatically copy pixels
            // to the cache image, which in turn when destroyed will push the tiles to the cache.
            Image::InitStorageArgs tmpImgInitArgs;
            {
                tmpImgInitArgs.bounds = renderWindow;
                tmpImgInitArgs.renderArgs = args.renderArgs;
                tmpImgInitArgs.isDraft = isDraftRender;
                tmpImgInitArgs.nodeHashKey = nodeFrameViewHash;
                tmpImgInitArgs.cachePolicy = eCacheAccessModeNone;
                tmpImgInitArgs.bufferFormat = pluginBufferLayout;
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
                tmpImgInitArgs.scale = renderMappedScale;

                tmpImgInitArgs.mirrorImage = it->second.cacheImage;
                tmpImgInitArgs.mirrorImageRoI = renderWindow;
            }
            it->second.tmpImage = Image::create(tmpImgInitArgs);
        } // for each plane to render

    } // useCache

    // If the effect does not support tiles, render everything again
    // The RoI has already been set to the pixelRoD in this case
    if (!args.renderArgs->getCurrentTilesSupport()) {
        // If not using the cache, render the full RoI
        RectToRender r;
        r.rect = roi;
        r.identityInputNumber = -1;
        planesToRender->rectsToRender.push_back(r);
    } else {

        //
        // If the effect has multiple inputs (such as masks) (e.g: Merge), try to call isIdentity
        // if the RoDs do not intersect the RoI
        //
        // We only do this when drawing with the RotoPaint node as this requires to render a lot of tiles which may
        // add a lot of overhead in a conventional compositing tree.
        bool isDrawing = _publicInterface->getNode()->isDuringPaintStrokeCreation();

        // True if we already added rectsToRender
        bool didSomething = false;
        if (isDrawing) {

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

                    TreeRenderNodeArgsPtr inputRenderArgs = args.renderArgs->getInputRenderArgs(i);
                    GetRegionOfDefinitionResultsPtr rodResults;
                    StatusEnum stat = input->getRegionOfDefinition_public(args.time, renderMappedScale, args.view, inputRenderArgs, &rodResults);
                    if (stat == eStatusFailed) {
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

                RectI inputsRoDIntersectionPixel;
                double par = _publicInterface->getAspectRatio(args.renderArgs, -1);
                inputsIntersection.toPixelEnclosing(renderMappedScale, par, &inputsRoDIntersectionPixel);

                std::list<RectI> rectsToOptimize;
                rectsToOptimize.push_back(renderWindow);

                _publicInterface->optimizeRectsToRender(args.renderArgs, inputsRoDIntersectionPixel, rectsToOptimize, args.time, args.view, renderMappedScale, &planesToRender->rectsToRender);
                didSomething = true;

            }
        } // isDrawing

        if (!didSomething) {

            // If plug-in wants host frame threading and there is only 1 rect to render, split it
            if (args.renderArgs->getCurrentRenderSafety() == eRenderSafetyFullySafeFrame) {

                unsigned int nThreads = MultiThread::getNCPUsAvailable();

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
                        planesToRender->rectsToRender.push_back(r);
                    }
                }
            } else {
                RectToRender r;
                r.rect = renderWindow;
                r.identityInputNumber = -1;
                planesToRender->rectsToRender.push_back(r);

            }
        } // !didSomething
    } // supportsTiles

} // fetchOrCreateOutputPlanes



EffectInstance::RenderRoIStatusEnum
EffectInstance::Implementation::renderRoILaunchInternalRender(const RenderRoIArgs & args,
                                                              const ImagePlanesToRenderPtr &planesToRender,
                                                              const OSGLContextAttacherPtr& glRenderContext,
                                                              const RectD& rod,
                                                              const RenderScale& renderMappedScale,
                                                              const std::bitset<4> &processChannels)
{

    // If we reach here, it can be either because the planes are cached or not, either way
    // the planes are NOT a total identity, and they may have some content left to render.
    EffectInstance::RenderRoIStatusEnum renderRetCode = eRenderRoIStatusImageRendered;

    // There should always be at least 1 plane to render (The color plane)
    assert( !planesToRender->planes.empty() );

    // eRenderSafetyInstanceSafe means that there is at most one render per instance
    // NOTE: the per-instance lock should be shared between
    // all clones of the same instance, because an InstanceSafe plugin may assume it is the sole owner of the output image,
    // and read-write on it.
    // It is probably safer to assume that several clones may write to the same output image only in the eRenderSafetyFullySafe case.

    // eRenderSafetyFullySafe means that there is only one render per FRAME : the lock is per image

    boost::scoped_ptr<QMutexLocker> locker;

    EffectInstancePtr renderInstance;
    //
    // Figure out If this node should use a render clone rather than execute renderRoIInternal on the main (this) instance.
    // Reasons to use a render clone is because a plug-in is eRenderSafetyInstanceSafe or does not support
    // concurrent GL renders.
    //
    bool isDuringPaintStrokeDrawing = _publicInterface->getNode()->isDuringPaintStrokeCreation();

    bool useRenderClone = false;

    RenderSafetyEnum safety = args.renderArgs->getCurrentRenderSafety();

    useRenderClone |= (safety == eRenderSafetyInstanceSafe && !isDuringPaintStrokeDrawing);

    useRenderClone |= safety != eRenderSafetyUnsafe && planesToRender->backendType == eRenderBackendTypeOpenGL && !_publicInterface->supportsConcurrentOpenGLRenders();

    if (useRenderClone) {
        renderInstance = _publicInterface->getOrCreateRenderInstance();
    } else {
        renderInstance = _publicInterface->shared_from_this();
    }
    assert(renderInstance);

    if (safety == eRenderSafetyInstanceSafe && !useRenderClone) {
        locker.reset( new QMutexLocker( &_publicInterface->getNode()->getRenderInstancesSharedMutex() ) );
    } else if (safety == eRenderSafetyUnsafe) {
        PluginPtr p = _publicInterface->getNode()->getPlugin();
        assert(p);
        locker.reset( new QMutexLocker( p->getPluginLock().get() ) );
    } else {
        // no need to lock
        Q_UNUSED(locker);
    }



    if (planesToRender->backendType == eRenderBackendTypeOpenGL ||
        planesToRender->backendType == eRenderBackendTypeOSMesa) {
        assert(glRenderContext);
        Natron::StatusEnum stat = renderInstance->attachOpenGLContext_public(args.time, args.view, renderMappedScale, args.renderArgs, glRenderContext->getContext(), &planesToRender->glContextData);
        if (stat == eStatusOutOfMemory) {
            renderRetCode = eRenderRoIStatusRenderOutOfGPUMemory;
        } else if (stat == eStatusFailed) {
            // Should we not use eRenderRoIStatusRenderOutOfGPUMemory also so that we try CPU rendering instead of failing the render alltogether ?
            renderRetCode = eRenderRoIStatusRenderFailed;
        }
    }
    if (renderRetCode == eRenderRoIStatusImageRendered) {
        
        renderRetCode = renderInstance->renderRoIInternal(glRenderContext,
                                                          args,
                                                          renderMappedScale,
                                                          rod,
                                                          planesToRender,
                                                          processChannels);

        if (planesToRender->backendType == eRenderBackendTypeOpenGL ||
            planesToRender->backendType == eRenderBackendTypeOSMesa) {

            // If the plug-in doesn't support concurrent OpenGL renders, release the lock that was taken in the call to attachOpenGLContext_public() above.
            // For safe plug-ins, we call dettachOpenGLContext_public when the effect is destroyed in Node::deactivate() with the function EffectInstance::dettachAllOpenGLContexts().
            // If we were the last render to use this context, clear the data now

            if ( planesToRender->glContextData->getHasTakenLock() ||
                !_publicInterface->supportsConcurrentOpenGLRenders() ||
                planesToRender->glContextData.use_count() == 1) {

                renderInstance->dettachOpenGLContext_public(args.renderArgs, glRenderContext->getContext(), planesToRender->glContextData);
            }
        }
    }
    if (useRenderClone) {
        _publicInterface->releaseRenderInstance(renderInstance);
    }

    return renderRetCode;
} // EffectInstance::Implementation::::renderRoILaunchInternalRender


void
EffectInstance::Implementation::ensureImagesToRequestedScale(const RenderRoIArgs & args,
                                                             const ImagePlanesToRenderPtr &planesToRender,
                                                             const RectI& roi,
                                                             std::map<ImageComponents, ImagePtr>* outputPlanes)
{

    // Ensure all planes are in the scale that was requested if this effect does not support render scale
    
    for (std::map<ImageComponents, PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {

        const RenderScale& imageScale = it->second.tmpImage->getScale();

        if (imageScale.x != args.scale.x || imageScale.y != args.scale.y) {



            it->second.fullscaleImage->downscaleMipMap( it->second.fullscaleImage->getRoD(), originalRoI, 0, args.mipMapLevel, false, it->second.downscaleImage.get() );
        }

        outputPlanes->insert( std::make_pair(*comp, it->second.downscaleImage) );

    }


} // EffectInstance::Implementation::ensureImagesToRequestedScale

RenderRoIRetCode
EffectInstance::renderRoI(const RenderRoIArgs & args, RenderRoIResults* results)
{

    // Do nothing if no components were requested
    if ( args.components.empty() ) {
        qDebug() << getScriptName_mt_safe().c_str() << "renderRoi: Early bail-out components requested empty";
        return eRenderRoIRetCodeOk;
    }
    if ( args.roi.isNull() ) {
        qDebug() << getScriptName_mt_safe().c_str() << "renderRoi: Early bail-out RoI requested empty ";
        return eRenderRoIRetCodeOk;
    }

    // Make sure this call is not made recursively from getImage on a render clone on which we are already calling renderRoI.
    // If so, forward the call to the main instance
    if (_imp->mainInstance) {
        return _imp->mainInstance->renderRoI(args, results);
    }

    assert(args.renderArgs && args.renderArgs->getNode() == getNode());

    // The region of definition of the effect at this frame/view in canonical coordinates
    RectD rod;
    {
        GetRegionOfDefinitionResultsPtr results;
        StatusEnum stat = getRegionOfDefinition_public(args.time, args.scale, args.view, args.renderArgs, &results);
        if (stat == eStatusFailed) {
            return eRenderRoIRetCodeFailed;
        }
        rod = results->getRoD();

        // If the plug-in RoD is null, there's nothing to render.
        if (rod.isNull()) {
            return eRenderRoIRetCodeOk;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Handle pass-through planes /////////////////////////////////////////////////////////////
    std::list<ImageComponents> requestedPlanes;
    std::bitset<4> processChannels;
    std::map<int, std::list<ImageComponents> > inputLayersNeeded;
    {

        RenderRoIRetCode upstreamRetCode = _imp->determinePlanesToRender(args, &inputLayersNeeded, &requestedPlanes, &processChannels, &results->outputPlanes);
        if (upstreamRetCode != eRenderRoIRetCodeOk) {
            return upstreamRetCode;
        }

        // There might no plane produced by this node that were requested
        if ( requestedPlanes.empty() ) {
            return eRenderRoIRetCodeOk;
        }
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Check if effect is identity /////////////////////////////////////////////////////////////
    const double par = getAspectRatio(args.renderArgs, -1);
    {
        bool isIdentity;
        RenderRoIRetCode upstreamRetCode = _imp->handleIdentityEffect(args, par, rod, requestedPlanes, inputLayersNeeded, results, &isIdentity);
        if (isIdentity) {
            return upstreamRetCode;
        }
    }


    // Get the data associated to this frame. It must have been created before.
    FrameViewRequestPtr requestPassData = args.renderArgs->getFrameViewRequest(args.time, args.view);
    assert(requestPassData);

    TreeRenderPtr renderObj = args.renderArgs->getParentRender();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Resolve render device ///////////////////////////////////////////////////////////////////

    // Point to either a GPU or CPU (OSMesa) OpenGL context
    OSGLContextPtr glRenderContext;

    // These are the planes rendered
    ImagePlanesToRenderPtr planesToRender(new ImagePlanesToRender);

    // The OpenGL context locker
    OSGLContextAttacherPtr glContextLocker;
    {
        RenderRoIRetCode errorCode = _imp->resolveRenderBackend(args, requestPassData, args.roi, &planesToRender->backendType, &glRenderContext);
        if (errorCode == eRenderRoIRetCodeFailed) {
            return errorCode;
        }
        assert(((planesToRender->backendType == eRenderBackendTypeOpenGL || planesToRender->backendType == eRenderBackendTypeOSMesa) && glRenderContext) || (planesToRender->backendType != eRenderBackendTypeOpenGL && planesToRender->backendType != eRenderBackendTypeOSMesa && !glRenderContext));

        // This will create a locker for the OpenGL context but not attach it yet. It will be attached when needed
        if (glRenderContext) {
            glContextLocker = OSGLContextAttacher::create(glRenderContext);
        }

    }


    // Does this render supports render-scale ? OpenGL always supports it
    const bool supportsRenderScale = (planesToRender->backendType == eRenderBackendTypeOpenGL || planesToRender->backendType == eRenderBackendTypeOSMesa) ? true : getNode()->supportsRenderScale();

    // This flag is relevant only when the mipMapLevel is different than 0. We use it to determine
    // wether the plug-in should render in the full scale image, and then we downscale afterwards or
    // if the plug-in can just use the downscaled image to render.
#pragma message WARN("Full-scale images are assumed to have a render scale of 1 here.")
    const RenderScale fullScale(1.);
    const bool renderFullScaleThenDownscale = !supportsRenderScale && (args.scale.x < fullScale.x || args.scale.y < fullScale.y);
    const RenderScale renderMappedScale = renderFullScaleThenDownscale ? fullScale : args.scale;

    // This is the region to render in pixel coordinates at the scale of renderMappedScale
    RectI renderMappedRoI;
    if (!renderFullScaleThenDownscale) {
        renderMappedRoI = args.roi;
    } else {
        RectD canonicalRoI;
        args.roi.toCanonical(args.scale, par, rod, &canonicalRoI);
        canonicalRoI.toPixelEnclosing(fullScale, par, &renderMappedRoI);
    }

    // The RoI cannot be null here, either we are in !renderFullScaleThenDownscale and we already checked at the begining
    // of the function that the RoI was Null, either the RoD was checked for NULL.
    assert(!renderMappedRoI.isNull());

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Handle Concatenations //////////////////////////////////////////////////////////////////
    {

        bool concatenated;
        RenderRoIRetCode upstreamRetCode = _imp->handleConcatenation(args, renderMappedScale, results, &concatenated);
        if (upstreamRetCode == eRenderRoIRetCodeFailed) {
            return upstreamRetCode;
        }
        if (concatenated) {
            return eRenderRoIRetCodeOk;
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Compute RoI depending on render scale ///////////////////////////////////////////////////

    // The RoD in pixel coordinates at the requested mip map level
    RectI pixelRodRequestedScale;

    // The RoD in pixel coordinates at scale 1
    RectI pixelRoDScaleOne;

    // The RoD in pixel coordinates at the scale of renderMappedScale
    RectI pixelRoDRenderMapped;

    // Depending on tiles support we are going to modify the RoI to clip it to the bounds of this effect.
    // We still keep in memory what was requested for later on
    const RectI originalRenderMappedRoI = renderMappedRoI;

    rod.toPixelEnclosing(args.scale, par, &pixelRodRequestedScale);
    rod.toPixelEnclosing(0, par, &pixelRoDScaleOne);

    pixelRoDRenderMapped = renderFullScaleThenDownscale ? pixelRoDScaleOne : pixelRodRequestedScale;

    if (!args.renderArgs->getCurrentTilesSupport()) {
        // If tiles are not supported the RoI is the full image bounds
        renderMappedRoI = pixelRoDRenderMapped;
    } else {

        // Make sure the RoI falls within the image bounds
        if ( !renderMappedRoI.intersect(pixelRoDRenderMapped, &renderMappedRoI) ) {
            return eRenderRoIRetCodeOk;
        }

        // The RoI falls into the effect pixel region of definition
        assert(renderMappedRoI.x1 >= pixelRoDRenderMapped.x1 && renderMappedRoI.y1 >= pixelRoDRenderMapped.y1 &&
               renderMappedRoI.x2 <= pixelRoDRenderMapped.x2 && renderMappedRoI.y2 <= pixelRoDRenderMapped.y2);
    }

    assert(!renderMappedRoI.isNull());

    // The requested portion to render in canonical coordinates
    RectD canonicalRoI;
    renderMappedRoI.toCanonical(renderMappedScale, par, rod, &canonicalRoI);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Allocate images and look-up cache ///////////////////////////////////////////////////////
    CacheAccessModeEnum cacheAccess = _imp->shouldRenderUseCache(args, requestPassData, planesToRender->backendType);

    _imp->fetchOrCreateOutputPlanes(args, requestPassData, cacheAccess, planesToRender, requestedPlanes, renderMappedRoI, renderMappedScale, &results->outputPlanes);

    bool hasSomethingToRender = !planesToRender->rectsToRender.empty();


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Determine rectangles left to render /////////////////////////////////////////////////////
    RectI lastStrokePixelRoD;



    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Pre-render input images ////////////////////////////////////////////////////////////////
    if (hasSomethingToRender) {

        // Ensure that we release the context while waiting for input images to be rendered.
        if (glContextLocker) {
            glContextLocker->dettach();
        }

        InputImagesMap inputImages;
        RenderRoIRetCode upstreamRetCode = args.renderArgs->preRenderInputImages(args.time, args.view, renderMappedScale, inputLayersNeeded, &inputImages);

        if (upstreamRetCode != eRenderRoIRetCodeOk) {
            return upstreamRetCode;
        }
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Launch actual internal render ///////////////////////////////////////////////////////////

    RenderRoIStatusEnum renderRetCode;
    if (hasSomethingToRender) {
        renderRetCode = _imp->renderRoILaunchInternalRender(args, planesToRender, glContextLocker, rod, renderMappedScale, processChannels);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Check for failure or abortion ///////////////////////////////////////////////////////////

    bool renderAborted = args.renderArgs->isAborted();
    switch (renderRetCode) {
        case eRenderRoIStatusImageRendered:
            if (renderAborted) {
                return eRenderRoIRetCodeOk;
            }
            break;
        case eRenderRoIStatusRenderFailed:
            return eRenderRoIRetCodeFailed;
            break;
        case eRenderRoIStatusRenderOutOfGPUMemory: {

            // Call renderRoI again, but don't use GPU this time if possible
            if (args.renderArgs->getCurrentRenderOpenGLSupport() != ePluginOpenGLRenderSupportYes) {
                // The plug-in can only use GPU or doesn't support GPU
                return eRenderRoIRetCodeFailed;
            }
            boost::scoped_ptr<RenderRoIArgs> newArgs( new RenderRoIArgs(args) );
            newArgs->allowGPURendering = false;
            return renderRoI(*newArgs, results);
        }   break;
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Termination /////////////////////////////////////////////////////////////////////////////
    _imp->ensureImagesToRequestedScale(args, planesToRender, renderMappedRoI, &results->outputPlanes);


    // Termination, check that we rendered things correctly or we should have failed earlier otherwise.
#ifdef DEBUG
    if ( results->outputPlanes.size() != args.components.size() ) {
        qDebug() << "Requested:";
        for (std::list<ImageComponents>::const_iterator it = args.components.begin(); it != args.components.end(); ++it) {
            qDebug() << it->getLayerName().c_str();
        }
        qDebug() << "But rendered:";
        for (std::map<ImageComponents, ImagePtr>::iterator it = results->outputPlanes.begin(); it != results->outputPlanes.end(); ++it) {
            if (it->second) {
                qDebug() << it->first.getLayerName().c_str();
            }
        }
    }
    assert( !results->outputPlanes.empty() );
#endif

    return eRenderRoIRetCodeOk;
} // renderRoI

EffectInstance::RenderRoIStatusEnum
EffectInstance::renderRoIInternal(const OSGLContextAttacherPtr& glContext,
                                  const RenderRoIArgs& args,
                                  const RenderScale& renderMappedScale,
                                  const RectD & rod, //!< rod in canonical coordinates
                                  const ImagePlanesToRenderPtr & planes,
                                  const std::bitset<4> processChannels)
{
    EffectInstance::RenderRoIStatusEnum retCode;

    assert( !planesToRender->planes.empty() );

    ///Add the window to the project's available formats if the effect is a reader
    ///This is the only reliable place where I could put these lines...which don't seem to feel right here.
    ///Plus setOrAddProjectFormat will actually set the project format the first time we read an image in the project
    ///hence ask for a new render... which can be expensive!
    ///Any solution how to work around this ?
    ///Edit: do not do this if in the main-thread (=noRenderThread = -1) otherwise we will change the parallel render args TLS
    ///which will lead to asserts down the stream
    if ( self->isReader() && ( QThread::currentThread() != qApp->thread() ) ) {
        Format frmt;
        RectI formatRect = self->getOutputFormat();
        frmt.set(formatRect);
        frmt.setPixelAspectRatio(par);
        // Don't add if project format already set: if reading a sequence with auto-crop data we would just litterally add one format for each frame read
        self->getApp()->getProject()->setOrAddProjectFormat(frmt, true);
    }

    unsigned int renderMappedMipMapLevel = 0;

    for (std::map<ImageComponents, PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
        it->second.renderMappedImage = renderFullScaleThenDownscale ? it->second.fullscaleImage : it->second.downscaleImage;
        if ( it == planesToRender->planes.begin() ) {
            renderMappedMipMapLevel = it->second.renderMappedImage->getMipMapLevel();
        }
    }

    RenderScale renderMappedScale( Image::getScaleFromMipMapLevel(renderMappedMipMapLevel) );
    RenderingFunctorRetEnum renderStatus = eRenderingFunctorRetOK;
    if ( planesToRender->rectsToRender.empty() ) {
        retCode = EffectInstance::eRenderRoIStatusImageAlreadyRendered;
    } else {
        retCode = EffectInstance::eRenderRoIStatusImageRendered;
    }


    ///Notify the gui we're rendering
    boost::shared_ptr<NotifyRenderingStarted_RAII> renderingNotifier;
    if ( !planesToRender->rectsToRender.empty() ) {
        renderingNotifier.reset( new NotifyRenderingStarted_RAII( self->getNode().get() ) );
    }


    boost::shared_ptr<std::map<NodePtr, ParallelRenderArgsPtr > > tlsCopy;
    if (safety == eRenderSafetyFullySafeFrame) {
        tlsCopy.reset(new std::map<NodePtr, ParallelRenderArgsPtr >);
        /*
         * Since we're about to start new threads potentially, copy all the thread local storage on all nodes (any node may be involved in
         * expressions, and we need to retrieve the exact local time of render).
         */
        self->getApp()->getProject()->getParallelRenderArgs(*tlsCopy);
    }


    double firstFrame, lastFrame;
    self->getFrameRange_public(frameViewHash, &firstFrame, &lastFrame);


    ///We only need to call begin if we've not already called it.
    bool callBegin = false;

    /// call beginsequenceRender here if the render is sequential
    SequentialPreferenceEnum pref = self->getNode()->getCurrentSequentialRenderSupport();
    if ( !self->isWriter() || (pref == eSequentialPreferenceNotSequential) ) {
        callBegin = true;
    }


    if (callBegin) {
        assert( !( (self->supportsRenderScaleMaybe() == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );
        if (self->beginSequenceRender_public(time, time, 1, !appPTR->isBackground(), renderMappedScale, isSequentialRender,
                                             isRenderMadeInResponseToUserInteraction, frameArgs->draftMode, view, planesToRender->useOpenGL, planesToRender->glContextData) == eStatusFailed) {
            renderStatus = eRenderingFunctorRetFailed;
        }
    }


    /*
     * All channels will be taken from this input if some channels are marked to be not processed
     */
    int preferredInput = self->getNode()->getPreferredInput();
    if ( (preferredInput != -1) && self->isInputMask(preferredInput) ) {
        preferredInput = -1;
    }


    if (renderStatus != eRenderingFunctorRetFailed) {
        if ( (safety == eRenderSafetyFullySafeFrame) && (planesToRender->rectsToRender.size() > 1) && !planesToRender->useOpenGL ) {
            QThread* currentThread = QThread::currentThread();
            boost::scoped_ptr<Implementation::TiledRenderingFunctorArgs> tiledArgs(new Implementation::TiledRenderingFunctorArgs);
            tiledArgs->renderFullScaleThenDownscale = renderFullScaleThenDownscale;
            tiledArgs->isRenderResponseToUserInteraction = isRenderMadeInResponseToUserInteraction;
            tiledArgs->preferredInput = preferredInput;
            tiledArgs->mipMapLevel = mipMapLevel;
            tiledArgs->renderMappedMipMapLevel = renderMappedMipMapLevel;
            tiledArgs->rod = rod;
            tiledArgs->time = time;
            tiledArgs->view = view;
            tiledArgs->par = par;
            tiledArgs->byPassCache = byPassCache;
            tiledArgs->outputClipPrefDepth = outputClipPrefDepth;
            tiledArgs->outputClipPrefsComps = outputClipPrefsComps;
            tiledArgs->processChannels = processChannels;
            tiledArgs->planes = planesToRender;
            tiledArgs->compsNeeded = compsNeeded;
            tiledArgs->glContext = glContext;


#ifdef NATRON_HOSTFRAMETHREADING_SEQUENTIAL
            std::vector<EffectInstance::RenderingFunctorRetEnum> ret( tiledData.size() );
            int i = 0;
            for (std::list<RectToRender>::const_iterator it = planesToRender->rectsToRender.begin(); it != planesToRender->rectsToRender.end(); ++it, ++i) {
                ret[i] = self->_imp->tiledRenderingFunctor(tiledArgs,
                                               *it,
                                               currentThread);
            }
            std::vector<EffectInstance::RenderingFunctorRetEnum>::const_iterator it2;

#else

            std::list<RectToRender> rectsToRenderList = planesToRender->rectsToRender;

            RectToRender lastRectToRender = rectsToRenderList.back();

            bool isThreadPoolThread = isRunningInThreadPoolThread();

            // If the current thread is a thread-pool thread, make it also do an iteration instead
            // of waiting for other threads
            if (isThreadPoolThread) {
                rectsToRenderList.pop_back();
            }
            std::vector<RenderingFunctorRetEnum> threadReturnCodes;
            QFuture<RenderingFunctorRetEnum> ret = QtConcurrent::mapped( rectsToRenderList,
                                       boost::bind(&EffectInstance::Implementation::tiledRenderingFunctor,
                                                   self->_imp.get(),
                                                   *tiledArgs,
                                                   _1,
                                                   currentThread) );
            if (isThreadPoolThread) {
                EffectInstance::RenderingFunctorRetEnum retCode = self->_imp->tiledRenderingFunctor(*tiledArgs, lastRectToRender, currentThread);
                threadReturnCodes.push_back(retCode);
            }

            // Wait for other threads to be finished
            ret.waitForFinished();

#endif
            for (QFuture<EffectInstance::RenderingFunctorRetEnum>::const_iterator  it2 = ret.begin(); it2 != ret.end(); ++it2) {
                threadReturnCodes.push_back(*it2);
            }
            for (std::vector<RenderingFunctorRetEnum>::const_iterator it2 = threadReturnCodes.begin(); it2 != threadReturnCodes.end(); ++it2) {
                if ( (*it2) == EffectInstance::eRenderingFunctorRetFailed ) {
                    renderStatus = eRenderingFunctorRetFailed;
                    break;
                }
#if NATRON_ENABLE_TRIMAP
                else if ( (*it2) == EffectInstance::eRenderingFunctorRetTakeImageLock ) {
                    planesToRender->isBeingRenderedElsewhere = true;
                }
#endif
                else if ( (*it2) == EffectInstance::eRenderingFunctorRetAborted ) {
                    renderStatus = eRenderingFunctorRetFailed;
                    break;
                } else if ( (*it2) == EffectInstance::eRenderingFunctorRetOutOfGPUMemory ) {
                    renderStatus = eRenderingFunctorRetOutOfGPUMemory;
                    break;
                }
            }
        } else {
            for (std::list<RectToRender>::const_iterator it = planesToRender->rectsToRender.begin(); it != planesToRender->rectsToRender.end(); ++it) {
                RenderingFunctorRetEnum functorRet = self->_imp->tiledRenderingFunctor(*it, glContext, renderFullScaleThenDownscale, isSequentialRender, isRenderMadeInResponseToUserInteraction, preferredInput, mipMapLevel, renderMappedMipMapLevel, rod, time, view, par, byPassCache, outputClipPrefDepth, outputClipPrefsComps, compsNeeded, processChannels, planesToRender);

                if ( (functorRet == eRenderingFunctorRetFailed) || (functorRet == eRenderingFunctorRetAborted) || (functorRet == eRenderingFunctorRetOutOfGPUMemory) ) {
                    renderStatus = functorRet;
                    break;
                }

                if  (functorRet == eRenderingFunctorRetTakeImageLock) {
                    renderStatus = eRenderingFunctorRetOK;
#if NATRON_ENABLE_TRIMAP
                    planesToRender->isBeingRenderedElsewhere = true;
#endif
                }
            } // for (std::list<RectI>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
        }
    } // if (renderStatus != eRenderingFunctorRetFailed) {

    ///never call endsequence render here if the render is sequential
    if (callBegin) {
        assert( !( (self->supportsRenderScaleMaybe() == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );
        if (self->endSequenceRender_public(time, time, time, false, renderMappedScale,
                                     isSequentialRender,
                                     isRenderMadeInResponseToUserInteraction,
                                     frameArgs->draftMode,
                                     view, planesToRender->useOpenGL, planesToRender->glContextData) == eStatusFailed) {
            renderStatus = eRenderingFunctorRetFailed;
        }
    }

    if (renderStatus != eRenderingFunctorRetOK) {
        if (renderStatus == eRenderingFunctorRetOutOfGPUMemory) {
            retCode = eRenderRoIStatusRenderOutOfGPUMemory;
        } else {
            retCode = eRenderRoIStatusRenderFailed;
        }
    }

    return retCode;
} // renderRoIInternal

NATRON_NAMESPACE_EXIT;
