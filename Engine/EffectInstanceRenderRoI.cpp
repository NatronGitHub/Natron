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
        nonIdentityRect.identityTime = TimeValue(0.);


        bool nonIdentityRectSet = false;
        for (std::size_t i = 0; i < splits.size(); ++i) {
            TimeValue identityInputTime(0.);
            int identityInputNb = -1;

            ViewIdx inputIdentityView(view);
            if ( !splits[i].intersects(inputsRoDIntersection) ) {
                IsIdentityResultsPtr results;
                ActionRetCodeEnum stat = isIdentity_public(false, time, renderMappedScale, splits[i], view, renderArgs, &results);
                if (isFailureRetCode(stat)) {
                    identityInputNb = -1;
                } else {
                    results->getIdentityData(&identityInputNb, &identityInputTime, &inputIdentityView);
                }
            }

            if (identityInputNb != -1) {
                EffectInstance::RectToRender r;
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
    }
} // optimizeRectsToRender



/**
 * @brief This function determines the planes to render and calls recursively on upstream nodes unavailable planes
 **/
ActionRetCodeEnum
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
        ActionRetCodeEnum stat = _publicInterface->getComponents_public(args.time, args.view, args.renderArgs, &results);
        if (isFailureRetCode(stat)) {
            return stat;
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

    if (componentsToFetchUpstream.empty()) {
        return eActionStatusOK;
    }

    assert(passThroughInputNb != -1);
    EffectInstancePtr passThroughInput = _publicInterface->getInput(passThroughInputNb);
    if (!passThroughInput) {
        return eActionStatusInputDisconnected;
    }

    boost::scoped_ptr<RenderRoIArgs> inArgs ( new RenderRoIArgs(args) );
    inArgs->components = componentsToFetchUpstream;

    RenderRoIResults inputResults;
    ActionRetCodeEnum inputRetCode = passThroughInput->renderRoI(*inArgs, &inputResults);

    if (isFailureRetCode(inputRetCode) || inputResults.outputPlanes.empty() ) {
        return inputRetCode;
    }

    outputPlanes->insert(inputResults.outputPlanes.begin(),  inputResults.outputPlanes.end());

    return eActionStatusOK;


} // determinePlanesToRender


ActionRetCodeEnum
EffectInstance::Implementation::handleIdentityEffect(const EffectInstance::RenderRoIArgs& args,
                                                     double par,
                                                     const RectD& rod,
                                                     const RenderScale& scale,
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
        rod.toPixelEnclosing(scale, par, &pixelRod);

        IsIdentityResultsPtr results;
        ActionRetCodeEnum stat = _publicInterface->isIdentity_public(true, args.time, scale, pixelRod, args.view, args.renderArgs, &results);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        results->getIdentityData(&inputNbIdentity, &inputTimeIdentity, &inputIdentityView);

    }
    *isIdentity = inputNbIdentity != -1;
    if (*isIdentity) {

        if (inputNbIdentity == -2) {

            // Be safe: we may hit an infinite recursion without this check
            assert(inputTimeIdentity != args.time);
            if ( inputTimeIdentity == args.time) {
                return eActionStatusFailed;
            }

            // This special value of -2 indicates that the plugin is identity of itself at another time
            boost::scoped_ptr<RenderRoIArgs> argCpy ( new RenderRoIArgs(args) );
            argCpy->time = inputTimeIdentity;
            argCpy->view = inputIdentityView;

            return _publicInterface->renderRoI(*argCpy, results);

        }

        // Check if identity input really exists, otherwise fail
        EffectInstancePtr inputEffectIdentity = _publicInterface->getInput(inputNbIdentity);
        if (!inputEffectIdentity) {
            return eActionStatusInputDisconnected;
        }



        boost::scoped_ptr<RenderRoIArgs> inputArgs ( new RenderRoIArgs(args) );
        inputArgs->time = inputTimeIdentity;
        inputArgs->view = inputIdentityView;


        // If the node has a layer selector knob, request what was selected in input.
        // If the node does not have a layer selector knob then just fetch the originally requested layers.
#pragma message WARN("Use components needed in input here instead")
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

        ActionRetCodeEnum ret =  inputEffectIdentity->renderRoI(*inputArgs, results);
        if (ret == eActionStatusOK) {
            // Add planes to existing outputPlanes
            finalPlanes.insert( results->outputPlanes.begin(), results->outputPlanes.end() );
        }
        return ret;
    } // if (identity)

    assert(!*isIdentity);
    return eActionStatusOK;
} // EffectInstance::Implementation::handleIdentityEffect

ActionRetCodeEnum
EffectInstance::Implementation::handleConcatenation(const EffectInstance::RenderRoIArgs& args,
                                                    const RenderScale& renderScale,
                                                    RenderRoIResults* results,
                                                    bool *concatenated)
{
    *concatenated = false;
    bool concatenate = args.renderArgs->getParentRender()->isConcatenationEnabled();
    if (concatenate && args.caller && args.caller.get() != _publicInterface && args.renderArgs->getCurrentDistortSupport()) {

        assert(args.inputNbInCaller != -1);

        // If the caller can apply a distorsion, then check if this effect has a distorsion

        if (args.caller->getInputCanReceiveDistorsion(args.inputNbInCaller)) {

            GetDistorsionResultsPtr actionResults;
            ActionRetCodeEnum stat = _publicInterface->getDistorsion_public(args.time, renderScale, args.view, args.renderArgs, &actionResults);
            if (isFailureRetCode(stat)) {
                return stat;
            }
            const DistorsionFunction2DPtr& disto = actionResults->getDistorsionResults();
            if (disto && disto->inputNbToDistort != -1) {

                // Recurse on input given by plug-in
                EffectInstancePtr distoInput = _publicInterface->getInput(disto->inputNbToDistort);
                if (!distoInput) {
                    return eActionStatusInputDisconnected;
                }
                boost::scoped_ptr<RenderRoIArgs> argsCpy(new RenderRoIArgs(args));
                argsCpy->caller = _publicInterface->shared_from_this();
                argsCpy->inputNbInCaller = disto->inputNbToDistort;
                ActionRetCodeEnum ret = distoInput->renderRoI(*argsCpy, results);
                if (isFailureRetCode(ret)) {
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
    return eActionStatusOK;
} // handleConcatenation

bool
EffectInstance::Implementation::canSplitRenderWindowWithIdentityRectangles(const RenderRoIArgs& args,
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

            TreeRenderNodeArgsPtr inputRenderArgs = args.renderArgs->getInputRenderArgs(i);
            GetRegionOfDefinitionResultsPtr rodResults;
            ActionRetCodeEnum stat = input->getRegionOfDefinition_public(args.time, renderMappedScale, args.view, inputRenderArgs, &rodResults);
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
EffectInstance::Implementation::computeRectanglesToRender(const RenderRoIArgs& args, const ImagePlanesToRenderPtr &planesToRender, const RectI& renderWindow)
{


    planesToRender->rectsToRender.clear();

    // The renderwindow may be null if this render only have to wait results from another render
    if (renderWindow.isNull()) {
        return;
    }

    // If the effect does not support tiles, render everything again

    if (!args.renderArgs->getCurrentTilesSupport()) {
        // If not using the cache, render the full RoI
        // The RoI has already been set to the pixelRoD in this case
        RectToRender r;
        r.rect = renderWindow;
        r.identityInputNumber = -1;
        planesToRender->rectsToRender.push_back(r);
        return;
    }


    //
    // If the effect has multiple inputs (such as masks) (e.g: Merge), try to call isIdentity
    // if the RoDs do not intersect the RoI
    //
    // Edit: We don't do this anymore: it requires to render a lot of tiles which may
    // add a lot of overhead in a conventional compositing tree.

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

            _publicInterface->optimizeRectsToRender(args.renderArgs, inputRodIntersectionPixel, rectsToOptimize, args.time, args.view, renderMappedScale, &planesToRender->rectsToRender);
            didSomething = true;
        }
    }
#endif

    if (!didSomething) {

        if (args.renderArgs->getCurrentRenderSafety() != eRenderSafetyFullySafeFrame) {

            // Plug-in did not enable host frame threading, it is expected that it handles multi-threading itself
            // with the multi-thread suite it needs so.

            RectToRender r;
            r.rect = renderWindow;
            r.identityInputNumber = -1;
            planesToRender->rectsToRender.push_back(r);

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
                    planesToRender->rectsToRender.push_back(r);
                }
            }
        }
    } // !didSomething
} // computeRectanglesToRender

void
EffectInstance::Implementation::checkPlanesToRenderAndComputeRectanglesToRender(const RenderRoIArgs & args,
                                                                                   const ImagePlanesToRenderPtr &planesToRender,
                                                                                   CacheAccessModeEnum cacheAccess,
                                                                                   const RectI& roi,
                                                                                   std::map<ImageComponents, ImagePtr>* outputPlanes)
{
    // The renderwindow is the bounding box of all tiles that are left to render (not the tiles that are pending)
    RectI renderWindow;
    {
        bool renderWindowSet = false;

        std::map<ImageComponents, PlaneToRender> planes = planesToRender->planes;
        planesToRender->planes.clear();
        for (std::map<ImageComponents, PlaneToRender>::const_iterator it = planes.begin(); it != planes.end(); ++it) {

            // If the image is entirely cached, do not even compute it and insert it in the output planes map
            bool planeHasPendingResults;
            std::list<RectI> restToRender = it->second.cacheImage->getRestToRender(&planeHasPendingResults);

            if (restToRender.empty() && !planeHasPendingResults) {
                (*outputPlanes)[it->first] = it->second.cacheImage;
                continue;
            }

            planesToRender->planes.insert(*it);

            // if there's nothing left to render but only pending results, do not mark it has a portion to render.
            if (restToRender.empty()) {
                continue;
            }

            if (!args.renderArgs->getCurrentTilesSupport()) {
                // If the effect does not support tiles, we must render the RoI which has been set to the pixel RoD.
                renderWindow = roi;
                continue;
            }

            // The temporary image will have the bounding box of tiles left to render.
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

    // No planes left to render, don't even compute rectangles to render
    if (planesToRender->planes.empty()) {
        return;
    }

    // The image format supported by the plug-in (co-planar, packed RGBA, etc...)
    ImageBufferLayoutEnum pluginBufferLayout = _publicInterface->getPreferredBufferLayout();

    StorageModeEnum cacheStorage;
    ImageBufferLayoutEnum cacheBufferLayout;

    if (cacheAccess != eCacheAccessModeNone) {
        // Cache format is tiled and mmap backend
        cacheStorage = eStorageModeDisk;
        cacheBufferLayout = eImageBufferLayoutMonoChannelTiled;
    } else {
        switch (planesToRender->backendType) {
            case eRenderBackendTypeOpenGL:
                cacheStorage = eStorageModeGLTex;
                break;
            case eRenderBackendTypeCPU:
            case eRenderBackendTypeOSMesa:
                cacheStorage = eStorageModeRAM;
                break;
        }
        cacheBufferLayout = pluginBufferLayout;
    }


    if (cacheAccess != eCacheAccessModeNone && (cacheBufferLayout != pluginBufferLayout) && !renderWindow.isNull()) {

        // The bitdepth of the image
        ImageBitDepthEnum outputBitDepth = _publicInterface->getBitDepth(args.renderArgs, -1);

        for (std::map<ImageComponents, PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {

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

            }
            it->second.tmpImage = Image::create(tmpImgInitArgs);
        } // for each plane to render

    } // useCache

    computeRectanglesToRender(args, planesToRender, renderWindow);

} // checkPlanesToRenderAndComputeRectanglesToRender

void
EffectInstance::Implementation::fetchOrCreateOutputPlanes(const RenderRoIArgs & args,
                                                          const FrameViewRequestPtr& requestPassData,
                                                          CacheAccessModeEnum cacheAccess,
                                                          const ImagePlanesToRenderPtr &planesToRender,
                                                          const std::list<ImageComponents>& requestedComponents,
                                                          const RectI& roi,
                                                          const RenderScale& mappedProxyScale,
                                                          unsigned int mappedMipMapLevel,
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

    StorageModeEnum cacheStorage;
    ImageBufferLayoutEnum cacheBufferLayout;
    if (cacheAccess != eCacheAccessModeNone) {
        // Cache format is tiled and mmap backend
        cacheStorage = eStorageModeDisk;
        cacheBufferLayout = eImageBufferLayoutMonoChannelTiled;
    } else {
        switch (planesToRender->backendType) {
            case eRenderBackendTypeOpenGL:
                cacheStorage = eStorageModeGLTex;
                break;
            case eRenderBackendTypeCPU:
            case eRenderBackendTypeOSMesa:
                cacheStorage = eStorageModeRAM;
                break;
        }
        cacheBufferLayout = pluginBufferLayout;
    }

    // For each requested components, create the corresponding image plane.
    // If this plug-in does not use the cache, we directly allocate an image using the plug-in preferred buffer format.
    // If using the cache, the image has to be in a mono-channel tiled format, hence we later create a temporary copy
    // on which the plug-in will work on.
    std::map<ImageComponents, PlaneToRender> planes;
    for (std::list<ImageComponents>::const_iterator it = requestedComponents.begin(); it != requestedComponents.end(); ++it) {


        Image::InitStorageArgs initArgs;
        {
            initArgs.bounds = roi;
            initArgs.cachePolicy = cacheAccess;
            initArgs.renderArgs = args.renderArgs;
            initArgs.proxyScale = mappedProxyScale;
            initArgs.mipMapLevel = mappedMipMapLevel;
            initArgs.isDraft = isDraftRender;
            initArgs.nodeHashKey = nodeFrameViewHash;
            initArgs.storage = cacheStorage;
            initArgs.bufferFormat = cacheBufferLayout;
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

    checkPlanesToRenderAndComputeRectanglesToRender(args, planesToRender, cacheAccess, roi, outputPlanes);

} // fetchOrCreateOutputPlanes

ActionRetCodeEnum
EffectInstance::Implementation::launchRenderAndWaitForPendingTiles(const RenderRoIArgs & args,
                                                                   const ImagePlanesToRenderPtr &planesToRender,
                                                                   const OSGLContextAttacherPtr& glRenderContext,
                                                                   CacheAccessModeEnum cacheAccess,
                                                                   const RectI& roi,
                                                                   const RenderScale& renderMappedScale,
                                                                   const std::bitset<4> &processChannels,
                                                                   const std::map<int, std::list<ImageComponents> >& neededInputLayers,
                                                                   std::map<ImageComponents, ImagePtr>* outputPlanes)
{

    while (!planesToRender->planes.empty()) {

        ActionRetCodeEnum renderRetCode = eActionStatusOK;
        // There may be no rectangles to render if all rectangles are pending (i.e: this render should wait for another thread
        // to complete the render first)
        if (!planesToRender->rectsToRender.empty()) {
            renderRetCode = renderRoILaunchInternalRender(args, planesToRender, glRenderContext, renderMappedScale, processChannels, neededInputLayers);
        }
        if (isFailureRetCode(renderRetCode)) {
            return renderRetCode;
        }

        // The render went OK: push the cache images tiles to the cache
        std::map<ImageComponents, PlaneToRender> planes = planesToRender->planes;
        planesToRender->planes.clear();
        for (std::map<ImageComponents, PlaneToRender>::iterator it = planes.begin(); it != planes.end(); ++it) {
            if (it->second.cacheImage->getCachePolicy() != eCacheAccessModeNone) {

                // Destroy the temporary image if there was any.
                // The cached image has been copied from the temporary image in tiledRenderingFunctor
                it->second.tmpImage.reset();

                // Push to the cache the tiles that we rendered
                it->second.cacheImage->pushTilesToCacheIfNotAborted();

                // Wait for any pending results. After this line other threads that should have computed should be done
                it->second.cacheImage->waitForPendingTiles();
            }
        }
        checkPlanesToRenderAndComputeRectanglesToRender(args, planesToRender, cacheAccess, roi, outputPlanes);

    } // while there is still planes to render
    
    return eActionStatusOK;
} // launcRenderAndWaitForPendingTiles


ActionRetCodeEnum
EffectInstance::Implementation::renderRoILaunchInternalRender(const RenderRoIArgs & args,
                                                              const ImagePlanesToRenderPtr &planesToRender,
                                                              const OSGLContextAttacherPtr& glRenderContext,
                                                              const RenderScale& renderMappedScale,
                                                              const std::bitset<4> &processChannels,
                                                              const std::map<int, std::list<ImageComponents> >& neededInputLayers)
{

    // If we reach here, it can be either because the planes are cached or not, either way
    // the planes are NOT a total identity, and they may have some content left to render.
    ActionRetCodeEnum renderRetCode = eActionStatusOK;

    // There should always be at least 1 plane to render (The color plane)
    assert( !planesToRender->planes.empty() && !planesToRender->rectsToRender.empty() );


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



    if (planesToRender->backendType == eRenderBackendTypeOpenGL ||
        planesToRender->backendType == eRenderBackendTypeOSMesa) {
        assert(glRenderContext);
        ActionRetCodeEnum stat = renderInstance->attachOpenGLContext_public(args.time, args.view, renderMappedScale, args.renderArgs, glRenderContext->getContext(), &planesToRender->glContextData);
        if (isFailureRetCode(stat)) {
            renderRetCode = stat;
        }
    }
    if (renderRetCode == eActionStatusOK) {
        
        renderRetCode = renderInstance->renderForClone(glRenderContext,
                                                          args,
                                                          renderMappedScale,
                                                          planesToRender,
                                                          processChannels,
                                                          neededInputLayers);

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


ActionRetCodeEnum
EffectInstance::renderRoI(const RenderRoIArgs & args, RenderRoIResults* results)
{

    // Do nothing if no components were requested
    if ( args.components.empty() ) {
        qDebug() << getScriptName_mt_safe().c_str() << "renderRoi: Early bail-out components requested empty";
        return eActionStatusOK;
    }
    if ( args.roi.isNull() ) {
        qDebug() << getScriptName_mt_safe().c_str() << "renderRoi: Early bail-out RoI requested empty ";
        return eActionStatusOK;
    }

    // Make sure this call is not made recursively from getImage on a render clone on which we are already calling renderRoI.
    // If so, forward the call to the main instance
    if (_imp->mainInstance) {
        return _imp->mainInstance->renderRoI(args, results);
    }

    assert(args.renderArgs && args.renderArgs->getNode() == getNode());


    TreeRenderPtr parentRender = args.renderArgs->getParentRender();
    const RenderScale& renderProxyScale = parentRender->getProxyScale();
    unsigned int renderMipMapLevel = parentRender->getMipMapLevel();
    const RenderScale& renderProxyMipMapedScale = parentRender->getProxyMipMapScale();

    // This flag is relevant only when the mipMapLevel is different than 0. We use it to determine
    // wether the plug-in should render in the full scale image, and then we downscale afterwards or
    // if the plug-in can just use the downscaled image to render.
    const RenderScale scaleOne(1.);
    const bool renderScaleOneThenResize = false;


    const RenderScale mappedProxyScale = renderScaleOneThenResize ? scaleOne : renderProxyScale;
    const unsigned int mappedMipMapLevel = renderScaleOneThenResize ? 0 : renderMipMapLevel;
    const RenderScale mappedCombinedScale = renderScaleOneThenResize ? scaleOne : renderProxyMipMapedScale;

    // The region of definition of the effect at this frame/view in canonical coordinates
    RectD rod;
    {
        GetRegionOfDefinitionResultsPtr results;
        ActionRetCodeEnum stat = getRegionOfDefinition_public(args.time, mappedCombinedScale, args.view, args.renderArgs, &results);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        rod = results->getRoD();

        // If the plug-in RoD is null, there's nothing to render.
        if (rod.isNull()) {
            return eActionStatusInputDisconnected;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Handle pass-through planes /////////////////////////////////////////////////////////////
    std::list<ImageComponents> requestedPlanes;
    std::bitset<4> processChannels;
    std::map<int, std::list<ImageComponents> > inputLayersNeeded;
    {

        ActionRetCodeEnum upstreamRetCode = _imp->determinePlanesToRender(args, &inputLayersNeeded, &requestedPlanes, &processChannels, &results->outputPlanes);
        if (isFailureRetCode(upstreamRetCode)) {
            return upstreamRetCode;
        }

        // There might no plane produced by this node that were requested
        if ( requestedPlanes.empty() ) {
            return eActionStatusOK;
        }
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Check if effect is identity /////////////////////////////////////////////////////////////
    const double par = getAspectRatio(args.renderArgs, -1);
    {
        bool isIdentity;
        ActionRetCodeEnum upstreamRetCode = _imp->handleIdentityEffect(args, par, rod, mappedCombinedScale, requestedPlanes, inputLayersNeeded, results, &isIdentity);
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
        ActionRetCodeEnum errorCode = _imp->resolveRenderBackend(args, requestPassData, args.roi, &planesToRender->backendType, &glRenderContext);
        if (isFailureRetCode(errorCode)) {
            return errorCode;
        }
        assert(((planesToRender->backendType == eRenderBackendTypeOpenGL || planesToRender->backendType == eRenderBackendTypeOSMesa) && glRenderContext) || (planesToRender->backendType != eRenderBackendTypeOpenGL && planesToRender->backendType != eRenderBackendTypeOSMesa && !glRenderContext));

        // This will create a locker for the OpenGL context but not attach it yet. It will be attached when needed
        if (glRenderContext) {
            glContextLocker = OSGLContextAttacher::create(glRenderContext);
        }

    }



    // This is the region to render in pixel coordinates at the scale of renderMappedScale
    RectI renderMappedRoI;

    bool isDrawing = getNode()->isDuringPaintStrokeCreation();
    // When drawing, only render the portion covered by the bounding box of the roto stroke samples.
    if (isDrawing) {
        RotoStrokeItemPtr attachedStroke = toRotoStrokeItem(getNode()->getAttachedRotoItem());
        if (attachedStroke) {
            RectD lastStrokeRoD = attachedStroke->getLastStrokeMovementBbox();
            lastStrokeRoD.toPixelEnclosing(mappedCombinedScale, par, &renderMappedRoI);
        }
    } else {

        if (!renderScaleOneThenResize) {
            renderMappedRoI = args.roi;
        } else {
            RectD canonicalRoI;
            args.roi.toCanonical(mappedCombinedScale, par, rod, &canonicalRoI);
            canonicalRoI.toPixelEnclosing(scaleOne, par, &renderMappedRoI);
        }
    }

    // The RoI cannot be null here, either we are in !renderFullScaleThenDownscale and we already checked at the begining
    // of the function that the RoI was Null, either the RoD was checked for NULL.
    assert(!renderMappedRoI.isNull());

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Handle Concatenations //////////////////////////////////////////////////////////////////
    {

        bool concatenated;
        ActionRetCodeEnum upstreamRetCode = _imp->handleConcatenation(args, mappedCombinedScale, results, &concatenated);
        if (isFailureRetCode(upstreamRetCode)) {
            return upstreamRetCode;
        }
        if (concatenated) {
            return eActionStatusOK;
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Compute RoI depending on render scale ///////////////////////////////////////////////////

    // Should the output of this render be cached ?
    CacheAccessModeEnum cacheAccess = _imp->shouldRenderUseCache(args, requestPassData);


    // The RoD in pixel coordinates at the scale of renderMappedScale
    RectI pixelRoDRenderMapped;

    // Depending on tiles support we are going to modify the RoI to clip it to the bounds of this effect.
    // We still keep in memory what was requested for later on
    const RectI originalRenderMappedRoI = renderMappedRoI;

    rod.toPixelEnclosing(mappedCombinedScale, par, &pixelRoDRenderMapped);

    if (!args.renderArgs->getCurrentTilesSupport()) {
        // If tiles are not supported the RoI is the full image bounds
        renderMappedRoI = pixelRoDRenderMapped;
    } else {

        // Round the roi to the tile size if the render is cached
        if (cacheAccess != eCacheAccessModeNone) {
            ImageBitDepthEnum outputBitDepth = getBitDepth(args.renderArgs, -1);
            int tileWidth, tileHeight;
            appPTR->getCache()->getTileSizePx(outputBitDepth, &tileWidth, &tileHeight);

            RectI tiledRoundedRoI = renderMappedRoI;

            tiledRoundedRoI.x1 = renderMappedRoI.x1 +  (int)std::floor((double)(renderMappedRoI.x1 - pixelRoDRenderMapped.x1) / tileWidth ) * tileWidth;

            tiledRoundedRoI.y1 = renderMappedRoI.y1 + (int)std::floor((double)(renderMappedRoI.y1 - pixelRoDRenderMapped.y1) / tileHeight ) * tileHeight;

            tiledRoundedRoI.x2 = tiledRoundedRoI.x1 + (int)std::ceil((double)(renderMappedRoI.x2 - renderMappedRoI.x1)  / tileWidth ) * tileWidth;

            tiledRoundedRoI.y2 = tiledRoundedRoI.y1 + (int)std::ceil((double)(renderMappedRoI.y2 - renderMappedRoI.y1) / tileHeight ) * tileHeight;
            renderMappedRoI = tiledRoundedRoI;
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
    RectD canonicalRoI;
    renderMappedRoI.toCanonical(mappedCombinedScale, par, rod, &canonicalRoI);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Allocate images and look-up cache ///////////////////////////////////////////////////////

    _imp->fetchOrCreateOutputPlanes(args, requestPassData, cacheAccess, planesToRender, requestedPlanes, renderMappedRoI, mappedProxyScale, mappedMipMapLevel, &results->outputPlanes);

    bool hasSomethingToRender = !planesToRender->rectsToRender.empty();


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Pre-render input images ////////////////////////////////////////////////////////////////
    if (hasSomethingToRender) {

        // Ensure that we release the context while waiting for input images to be rendered.
        if (glContextLocker) {
            glContextLocker->dettach();
        }

        ActionRetCodeEnum upstreamRetCode = args.renderArgs->preRenderInputImages(args.time, args.view, inputLayersNeeded);

        if (isFailureRetCode(upstreamRetCode)) {
            return upstreamRetCode;
        }
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Launch actual internal render ///////////////////////////////////////////////////////////

    ActionRetCodeEnum renderRetCode = eActionStatusOK;
    if (hasSomethingToRender) {
        renderRetCode = _imp->launchRenderAndWaitForPendingTiles(args, planesToRender, glContextLocker, cacheAccess, renderMappedRoI, mappedCombinedScale, processChannels, inputLayersNeeded, &results->outputPlanes);
    }

    // Now that this effect has rendered, clear pre-rendered inputs
    requestPassData->clearPreRenderedInputs();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Check for failure or abortion ///////////////////////////////////////////////////////////

    // If using GPU and out of memory retry on CPU if possible
    if (renderRetCode == eActionStatusOutOfMemory && planesToRender->backendType == eRenderBackendTypeOpenGL) {
        if (args.renderArgs->getCurrentRenderOpenGLSupport() != ePluginOpenGLRenderSupportYes) {
            // The plug-in can only use GPU or doesn't support GPU
            return eActionStatusFailed;
        }
        boost::scoped_ptr<RenderRoIArgs> newArgs( new RenderRoIArgs(args) );
        newArgs->allowGPURendering = false;
        return renderRoI(*newArgs, results);
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Termination ///////////////////////////////////////////////////////////////////////////
  


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

    return eActionStatusOK;
} // renderRoI

ActionRetCodeEnum
EffectInstance::renderForClone(const OSGLContextAttacherPtr& glContext,
                               const RenderRoIArgs& args,
                               const RenderScale& renderMappedScale,
                               const ImagePlanesToRenderPtr & planesToRender,
                               const std::bitset<4> processChannels,
                               const std::map<int, std::list<ImageComponents> >& neededInputLayers)
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
                                                            renderMappedScale,
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
        std::map<int, std::list<ImageComponents> >::const_iterator foundNeededLayers = neededInputLayers.find(mainInputNb);
        if (foundNeededLayers != neededInputLayers.end()) {
            inArgs.layers = &foundNeededLayers->second;
            inArgs.inputNb = mainInputNb;
            inArgs.currentTime = args.time;
            inArgs.currentView = args.view;
            inArgs.inputTime = inArgs.currentTime;
            inArgs.inputView = inArgs.currentView;
            inArgs.currentScale = renderMappedScale;
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

            ActionRetCodeEnum functorRet = _imp->tiledRenderingFunctor(*it, &args, renderMappedScale, processChannels, mainInputImage, planesToRender, glContext);
            if (isFailureRetCode(functorRet)) {
                return functorRet;
            }

        } // for (std::list<RectI>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {

    } else { // attemptHostFrameThreading


        boost::scoped_ptr<Implementation::TiledRenderingFunctorArgs> tiledArgs(new Implementation::TiledRenderingFunctorArgs);
        tiledArgs->args = &args;
        tiledArgs->renderMappedScale = renderMappedScale;
        tiledArgs->mainInputImage = mainInputImage;
        tiledArgs->processChannels = processChannels;
        tiledArgs->planesToRender = planesToRender;
        tiledArgs->glContext = glContext;



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
                                     renderMappedScale,
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
