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

#include <boost/scoped_ptr.hpp>

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
#include <SequenceParsing.h>

#include "Global/MemoryInfo.h"
#include "Global/QtCompat.h"

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/BlockingBackgroundRender.h"
#include "Engine/DiskCacheNode.h"
#include "Engine/EffectOpenGLContextData.h"
#include "Engine/Cache.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/Log.h"
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
#include "Engine/RotoContext.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/Settings.h"
#include "Engine/Timer.h"
#include "Engine/Transform.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

//#define NATRON_ALWAYS_ALLOCATE_FULL_IMAGE_BOUNDS


NATRON_NAMESPACE_ENTER;

/*
 * @brief Split all rects to render in smaller rects and check if each one of them is identity.
 * For identity rectangles, we just call renderRoI again on the identity input in the tiledRenderingFunctor.
 * For non-identity rectangles, compute the bounding box of them and render it
 */
static void
optimizeRectsToRender(const EffectInstancePtr& self,
                      const RectI & inputsRoDIntersection,
                      const std::list<RectI> & rectsToRender,
                      const double time,
                      const ViewIdx view,
                      const RenderScale & renderMappedScale,
                      std::list<EffectInstance::RectToRender>* finalRectsToRender)
{
    for (std::list<RectI>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
        std::vector<RectI> splits = it->splitIntoSmallerRects(0);
        EffectInstance::RectToRender nonIdentityRect;
        nonIdentityRect.isIdentity = false;
        nonIdentityRect.identityTime = 0;
        nonIdentityRect.rect.x1 = INT_MAX;
        nonIdentityRect.rect.x2 = INT_MIN;
        nonIdentityRect.rect.y1 = INT_MAX;
        nonIdentityRect.rect.y2 = INT_MIN;

        bool nonIdentityRectSet = false;
        for (std::size_t i = 0; i < splits.size(); ++i) {
            double identityInputTime = 0.;
            int identityInputNb = -1;
            bool identity = false;
            ViewIdx inputIdentityView(view);
            if ( !splits[i].intersects(inputsRoDIntersection) ) {
                identity = self->isIdentity_public(false, 0, time, renderMappedScale, splits[i], view, &identityInputTime, &inputIdentityView, &identityInputNb);
            } else {
                identity = false;
            }

            if (identity) {
                EffectInstance::RectToRender r;
                r.isIdentity = true;

                // Walk along the identity branch until we find the non identity input, or NULL in we case we will
                // just render black and transparent
                EffectInstancePtr identityInput = self->getInput(identityInputNb);
                if (identityInput) {
                    for (;; ) {
                        identity = identityInput->isIdentity_public(false, 0, time, renderMappedScale, splits[i], view, &identityInputTime, &inputIdentityView,  &identityInputNb);
                        if ( !identity || (identityInputNb == -2) ) {
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
                r.identityInput = identityInput;
                r.identityTime = identityInputTime;
                r.identityView = inputIdentityView;
                r.rect = splits[i];
                finalRectsToRender->push_back(r);
            } else {
                nonIdentityRectSet = true;
                nonIdentityRect.rect.x1 = std::min(splits[i].x1, nonIdentityRect.rect.x1);
                nonIdentityRect.rect.x2 = std::max(splits[i].x2, nonIdentityRect.rect.x2);
                nonIdentityRect.rect.y1 = std::min(splits[i].y1, nonIdentityRect.rect.y1);
                nonIdentityRect.rect.y2 = std::max(splits[i].y2, nonIdentityRect.rect.y2);
            }
        }
        if (nonIdentityRectSet) {
            finalRectsToRender->push_back(nonIdentityRect);
        }
    }
} // optimizeRectsToRender

ImagePtr
EffectInstance::convertPlanesFormatsIfNeeded(const AppInstancePtr& app,
                                             const ImagePtr& inputImage,
                                             const RectI& roi,
                                             const ImageComponents& targetComponents,
                                             ImageBitDepthEnum targetDepth,
                                             bool useAlpha0ForRGBToRGBAConversion,
                                             ImagePremultiplicationEnum outputPremult,
                                             int channelForAlpha)
{
    // Do not do any conversion for OpenGL textures, OpenGL is managing it for us.
    if (inputImage->getStorageMode() == eStorageModeGLTex) {
        return inputImage;
    }
    bool imageConversionNeeded = ( /*!targetIsMultiPlanar &&*/ targetComponents.getNumComponents() != inputImage->getComponents().getNumComponents() ) || targetDepth != inputImage->getBitDepth();

    if (!imageConversionNeeded) {
        return inputImage;
    } else {
        /**
         * Lock the downscaled image so it cannot be resized while creating the temp image and calling convertToFormat.
         **/
        Image::ReadAccess acc = inputImage->getReadRights();
        RectI bounds = inputImage->getBounds();
        ImagePtr tmp( new Image(targetComponents,
                                inputImage->getRoD(),
                                bounds,
                                inputImage->getMipMapLevel(),
                                inputImage->getPixelAspectRatio(),
                                targetDepth,
                                inputImage->getPremultiplication(),
                                inputImage->getFieldingOrder(),
                                false) );
        tmp->setKey(inputImage->getKey());
        RectI clippedRoi;
        roi.intersect(bounds, &clippedRoi);

        bool unPremultIfNeeded = outputPremult == eImagePremultiplicationPremultiplied && inputImage->getComponentsCount() == 4 && tmp->getComponentsCount() == 3;

        if (useAlpha0ForRGBToRGBAConversion) {
            inputImage->convertToFormatAlpha0( clippedRoi,
                                               app->getDefaultColorSpaceForBitDepth( inputImage->getBitDepth() ),
                                               app->getDefaultColorSpaceForBitDepth(targetDepth),
                                               channelForAlpha, false, unPremultIfNeeded, tmp.get() );
        } else {
            inputImage->convertToFormat( clippedRoi,
                                         app->getDefaultColorSpaceForBitDepth( inputImage->getBitDepth() ),
                                         app->getDefaultColorSpaceForBitDepth(targetDepth),
                                         channelForAlpha, false, unPremultIfNeeded, tmp.get() );
        }

        return tmp;
    }
} // EffectInstance::convertPlanesFormatsIfNeeded

void
EffectInstance::Implementation::determineRectsToRender(ImagePtr& isPlaneCached,
                                                       const ParallelRenderArgsPtr& frameArgs,
                                                       double time,
                                                       ViewIdx view,
                                                       const RenderScale& argsScale,
                                                       const RenderScale& renderMappedScale,
                                                       const RectI& roi,
                                                       const RectI& upscaledImageBounds,
                                                       const RectI& downscaledImageBounds,
                                                       bool renderFullScaleThenDownscale,
                                                       bool isDuringPaintStroke,
                                                       unsigned int mipMapLevel,
                                                       double par,
                                                       bool cacheIsAlmostFull,
                                                       const InputImagesMap& argsInputImageList,
                                                       StorageModeEnum storage,
                                                       const OSGLContextAttacherPtr& glContextLocker,
                                                       const EffectInstance::ImagePlanesToRenderPtr& planesToRender,
                                                       bool *redoCacheLookup,
                                                       bool *fillGrownBoundsWithZeroes,
                                                       bool *tryIdentityOptim,
                                                       RectI *lastStrokePixelRoD,
                                                       RectI *inputsRoDIntersectionPixel) {
    std::list<RectI> rectsLeftToRender;
    *fillGrownBoundsWithZeroes = false;
    *redoCacheLookup = false;
    //While painting, clear only the needed portion of the bitmap
    if ( isDuringPaintStroke && argsInputImageList.empty() ) {
        RectD lastStrokeRoD;
        NodePtr node = _publicInterface->getNode();
        if ( !node->isLastPaintStrokeBitmapCleared() ) {
            lastStrokeRoD = _publicInterface->getApp()->getLastPaintStrokeBbox();
            node->clearLastPaintStrokeRoD();
            // qDebug() << getScriptName_mt_safe().c_str() << "last stroke RoD: " << lastStrokeRoD.x1 << lastStrokeRoD.y1 << lastStrokeRoD.x2 << lastStrokeRoD.y2;
            lastStrokeRoD.toPixelEnclosing(mipMapLevel, par, lastStrokePixelRoD);
        }
    }

    if (!isPlaneCached) {
        if (frameArgs->tilesSupported) {
            rectsLeftToRender.push_back(roi);
        } else {
            rectsLeftToRender.push_back(renderFullScaleThenDownscale ? upscaledImageBounds : downscaledImageBounds);
        }
    } else {
        if ( isDuringPaintStroke && !lastStrokePixelRoD->isNull() ) {

            // We may end-up in a situation where we cached an image in RAM but the paint buffer (which is the GPU texture) does not have the appropriate mipmaplevel
            // To deal with it, we convert the plane cached to the appropriate format
            if (storage != isPlaneCached->getStorageMode()) {
                assert(storage == eStorageModeGLTex && isPlaneCached->getStorageMode() != eStorageModeGLTex);
                assert(glContextLocker);
                assert(isPlaneCached->getMipMapLevel() == mipMapLevel);
                glContextLocker->attach();
                for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it2 = planesToRender->planes.begin();
                     it2 != planesToRender->planes.end(); ++it2) {
                    it2->second.fullscaleImage = convertRAMImageToOpenGLTexture(it2->second.fullscaleImage, glContextLocker->getContext());
                    _publicInterface->getNode()->setPaintBuffer(it2->second.fullscaleImage);
                    it2->second.downscaleImage = it2->second.downscaleImage;
                }
            }
            *fillGrownBoundsWithZeroes = true;
            //Clear the bitmap of the cached image in the portion of the last stroke to only recompute what's needed
            for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it2 = planesToRender->planes.begin();
                 it2 != planesToRender->planes.end(); ++it2) {

                if (!it2->second.fullscaleImage->usesBitMap()) {
                    rectsLeftToRender.push_back(*lastStrokePixelRoD);
                } else {
                    it2->second.fullscaleImage->clearBitmap(*lastStrokePixelRoD);

                    /*
                     * This is useful to optimize the bitmap checking
                     * when we are sure multiple threads are not using the image and we have a very small RoI to render.
                     * For now it's only used for the rotopaint while painting.
                     */
                    it2->second.fullscaleImage->setBitmapDirtyZone(*lastStrokePixelRoD);

                }
            }
        }

        ///We check what is left to render.
        if (isPlaneCached->usesBitMap()) {

#if NATRON_ENABLE_TRIMAP
            if ( frameArgs->isCurrentFrameRenderNotAbortable() ) {
#ifndef DEBUG
                isPlaneCached->getRestToRender_trimap(roi, rectsLeftToRender, &planesToRender->isBeingRenderedElsewhere);
#else
                // in debug mode, check that the result of getRestToRender_trimap and getRestToRender is the same if the image
                // is not currently rendered concurrently
                IBRPtr ibr;
                {
                    QMutexLocker k(&imagesBeingRenderedMutex);
                    EffectInstance::Implementation::IBRMap::const_iterator found = imagesBeingRendered.find(isPlaneCached);
                    if ( ( found != imagesBeingRendered.end() ) && found->second->refCount ) {
                        ibr = found->second;
                    }

                    if (!ibr) {
                        Image::ReadAccess racc( isPlaneCached.get() );
                        isPlaneCached->getRestToRender_trimap(roi, rectsLeftToRender, &planesToRender->isBeingRenderedElsewhere);
                        std::list<RectI> tmpRects;
                        isPlaneCached->getRestToRender(roi, tmpRects);

                        //If it crashes here that means the image is no longer being rendered but its bitmap still contains PIXEL_UNAVAILABLE pixels.
                        //The other thread should have removed that image from the cache or marked the image as rendered.
                        assert(!planesToRender->isBeingRenderedElsewhere);
                        assert( rectsLeftToRender.size() == tmpRects.size() );

                        std::list<RectI>::iterator oIt = rectsLeftToRender.begin();
                        for (std::list<RectI>::iterator it = tmpRects.begin(); it != tmpRects.end(); ++it, ++oIt) {
                            assert(*it == *oIt);
                        }
                    } else {
                        isPlaneCached->getRestToRender_trimap(roi, rectsLeftToRender, &planesToRender->isBeingRenderedElsewhere);
                    }
                }
#endif
            } else {
                isPlaneCached->getRestToRender(roi, rectsLeftToRender);
            }
#else
            isPlaneCached->getRestToRender(roi, rectsLeftToRender);
#endif
        } // usesBitmap

        // During painting, we cannot recompute twice the same rectangles to render
        assert(!isDuringPaintStroke || (isDuringPaintStroke && (!lastStrokePixelRoD->isNull() || rectsLeftToRender.empty())));
        if ( isDuringPaintStroke && !lastStrokePixelRoD->isNull() && !rectsLeftToRender.empty()) {
            rectsLeftToRender.clear();
            RectI intersection;
            if ( downscaledImageBounds.intersect(*lastStrokePixelRoD, &intersection) ) {
                rectsLeftToRender.push_back(intersection);
            }
        }

        if ( !rectsLeftToRender.empty() &&  cacheIsAlmostFull ) {
            ///The node cache is almost full and we need to render  something in the image, if we hold a pointer to this image here
            ///we might recursively end-up in this same situation at each level of the render tree, ending with all images of each level
            ///being held in memory.
            ///Our strategy here is to clear the pointer, hence allowing the cache to remove the image, and ask the inputs to render the full RoI
            ///instead of the rest to render. This way, even if the image is cleared from the cache we already have rendered the full RoI anyway.
            rectsLeftToRender.clear();
            rectsLeftToRender.push_back(roi);
            for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it2 = planesToRender->planes.begin(); it2 != planesToRender->planes.end(); ++it2) {
                //Keep track of the original cached image for the re-lookup afterward, if the pointer doesn't match the first look-up, don't consider
                //the image because the region to render might have changed and we might have to re-trigger a render on inputs again.

                ///Make sure to never dereference originalCachedImage! We only compare it (that's why it s a void*)
                it2->second.originalCachedImage = it2->second.fullscaleImage.get();
                it2->second.fullscaleImage.reset();
                it2->second.downscaleImage.reset();
            }
            isPlaneCached.reset();
            if (cacheIsAlmostFull) {
                *redoCacheLookup = true;
            }
        }


        // If the effect doesn't support tiles and it has something left to render, just render the bounds again except if during a painting operation.
        if (!frameArgs->tilesSupported && !isDuringPaintStroke && !rectsLeftToRender.empty() && isPlaneCached) {
            ///if the effect doesn't support tiles, just render the whole rod again even though
            rectsLeftToRender.clear();
            rectsLeftToRender.push_back(renderFullScaleThenDownscale ? upscaledImageBounds : downscaledImageBounds);
        }
    } // isPlaneCached

    /*
     * If the effect has multiple inputs (such as masks) try to call isIdentity if the RoDs do not intersect the RoI
     */
    *tryIdentityOptim = false;
    if (frameArgs->tilesSupported && !rectsLeftToRender.empty() && isDuringPaintStroke) {
        RectD inputsIntersection;
        bool inputsIntersectionSet = false;
        bool hasDifferentRods = false;
        int maxInput = _publicInterface->getMaxInputCount();
        bool hasMask = false;
        RotoDrawableItemPtr attachedStroke = _publicInterface->getNode()->getAttachedRotoItem();
        for (int i = 0; i < maxInput; ++i) {
            bool isMask = _publicInterface->isInputMask(i);
            RectD inputRod;
            if (attachedStroke && isMask) {
                _publicInterface->getNode()->getPaintStrokeRoD(time, &inputRod);
                hasMask = true;
            } else {
                EffectInstancePtr input = _publicInterface->getInput(i);
                if (!input) {
                    continue;
                }
                bool isProjectFormat;
                ParallelRenderArgsPtr inputFrameArgs = input->getParallelRenderArgsTLS();
                U64 inputHash = (inputFrameArgs) ? inputFrameArgs->nodeHash : input->getHash();
                StatusEnum stat = input->getRegionOfDefinition_public(inputHash, time, argsScale, view, &inputRod, &isProjectFormat);
                if ( (stat != eStatusOK) && !inputRod.isNull() ) {
                    break;
                }
                if (isMask) {
                    hasMask = true;
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
            inputsIntersection.toPixelEnclosing(mipMapLevel, par, inputsRoDIntersectionPixel);
            *tryIdentityOptim = true;
        }
    }

    if (*tryIdentityOptim) {
        optimizeRectsToRender(_publicInterface->shared_from_this(), *inputsRoDIntersectionPixel, rectsLeftToRender, time, view, renderMappedScale, &planesToRender->rectsToRender);
    } else {
        for (std::list<RectI>::iterator it = rectsLeftToRender.begin(); it != rectsLeftToRender.end(); ++it) {
            RectToRender r;
            r.rect = *it;
            r.isIdentity = false;
            planesToRender->rectsToRender.push_back(r);
        }
    }

} // determineRectsToRender

/**
 * @brief This function determines the planes to render and calls recursively on upstream nodes unavailable planes
 **/
EffectInstance::RenderRoIRetCode
EffectInstance::Implementation::determinePlanesToRender(const EffectInstance::RenderRoIArgs& args, std::list<ImageComponents> *requestedComponents, std::map<ImageComponents, ImagePtr>* outputPlanes)
{
    EffectInstance::ComponentsAvailableList componentsToFetchUpstream;
    {
        EffectInstance::ComponentsAvailableMap componentsAvailables;

        // Available planes/components is view agnostic
        _publicInterface->getComponentsAvailable(true, true, args.time, &componentsAvailables);


        /*
         * For all requested planes, check which components can be produced in output by this node.
         * If the components are from the color plane, if another set of components of the color plane is present
         * we try to render with those instead.
         */
        for (std::list<ImageComponents>::const_iterator it = args.components.begin(); it != args.components.end(); ++it) {
            // We may not request paired layers
            assert( *it && !it->isPairedComponents() );
            assert(it->getNumComponents() > 0);

            bool isColorComponents = it->isColorPlane();
            bool found = false;
            ImageComponents foundComponent;
            NodePtr foundNode;

            for (ComponentsAvailableMap::iterator it2 = componentsAvailables.begin(); it2 != componentsAvailables.end(); ++it2) {
                if (it2->first == *it) {
                    found = true;
                    foundComponent = *it;
                    foundNode = it2->second.lock();
                    break;
                } else {
                    if ( isColorComponents && it2->first.isColorPlane() && _publicInterface->isSupportedComponent(-1, it2->first) ) {
                        //We found another set of components in the color plane, take it
                        found = true;
                        foundComponent = it2->first;
                        foundNode = it2->second.lock();
                        break;
                    }
                }
            }

            // If  the requested component is not present, then it will just return black and transparent to the plug-in.
            if (found) {
                if ( foundNode == _publicInterface->getNode() ) {
                    requestedComponents->push_back(foundComponent);
                } else {
                    //The component is not available directly from this node, fetch it upstream
                    componentsToFetchUpstream.push_back( std::make_pair(foundComponent, foundNode) );
                }
            }
        }
    }
    //Render planes that we are not able to render on this node from upstream
    for (ComponentsAvailableList::iterator it = componentsToFetchUpstream.begin(); it != componentsToFetchUpstream.end(); ++it) {
        NodePtr node = it->second.lock();
        if (node) {
            boost::scoped_ptr<RenderRoIArgs> inArgs ( new RenderRoIArgs(args) );
            inArgs->preComputedRoD.clear();
            inArgs->components.clear();
            inArgs->components.push_back(it->first);
            std::map<ImageComponents, ImagePtr> inputPlanes;
            RenderRoIRetCode inputRetCode = node->getEffectInstance()->renderRoI(*inArgs, &inputPlanes);
            assert( inputPlanes.size() == 1 || inputPlanes.empty() );
            if ( (inputRetCode == eRenderRoIRetCodeAborted) || (inputRetCode == eRenderRoIRetCodeFailed) || inputPlanes.empty() ) {
                return inputRetCode;
            }
            outputPlanes->insert( std::make_pair(it->first, inputPlanes.begin()->second) );
        }
    }

    return eRenderRoIRetCodeOk;


} // determinePlanesToRender


EffectInstance::RenderRoIRetCode
EffectInstance::Implementation::handleIdentityEffect(const EffectInstance::RenderRoIArgs& args,
                                                     const ParallelRenderArgsPtr& frameArgs,
                                                     const FrameViewRequest* requestPassData,
                                                     SupportsEnum supportsRS,
                                                     double par,
                                                     unsigned int mipMapLevel,
                                                     U64 nodeHash,
                                                     ImagePremultiplicationEnum thisEffectOutputPremult,
                                                     const RectD& rod,
                                                     const std::list<ImageComponents> &requestedComponents,
                                                     const std::vector<ImageComponents>& outputComponents,
                                                     const ComponentsNeededMapPtr& neededComps,
                                                     RenderScale& renderMappedScale,
                                                     unsigned int &renderMappedMipMapLevel,
                                                     bool &renderFullScaleThenDownscale,
                                                     std::map<ImageComponents, ImagePtr>* outputPlanes,
                                                     bool *isIdentity)
{
    double inputTimeIdentity = 0.;
    int inputNbIdentity;
    ViewIdx inputIdentityView(args.view);
    assert( !( (supportsRS == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );
    RectI pixelRod;
    rod.toPixelEnclosing(args.mipMapLevel, par, &pixelRod);
    ViewInvarianceLevel viewInvariance = _publicInterface->isViewInvariant();

    if ( (args.view != 0) && (viewInvariance == eViewInvarianceAllViewsInvariant) ) {
        *isIdentity = true;
        inputNbIdentity = -2;
        inputTimeIdentity = args.time;
    } else {
        try {
            if (requestPassData) {
                inputTimeIdentity = requestPassData->globalData.inputIdentityTime;
                inputNbIdentity = requestPassData->globalData.identityInputNb;
                *isIdentity = requestPassData->globalData.isIdentity;
                inputIdentityView = requestPassData->globalData.identityView;
            } else {
                *isIdentity = _publicInterface->isIdentity_public(true, nodeHash, args.time, renderMappedScale, pixelRod, args.view, &inputTimeIdentity, &inputIdentityView, &inputNbIdentity);
            }
        } catch (...) {
            return eRenderRoIRetCodeFailed;
        }
    }

    if ( (supportsRS == eSupportsMaybe) && (mipMapLevel != 0) ) {
        // supportsRenderScaleMaybe may have changed, update it
        renderFullScaleThenDownscale = true;
        renderMappedScale.x = renderMappedScale.y = 1.;
        renderMappedMipMapLevel = 0;
    }

    if (*isIdentity) {
        ///The effect is an identity but it has no inputs
        if (inputNbIdentity == -1) {
            return eRenderRoIRetCodeOk;
        } else if (inputNbIdentity == -2) {
            // there was at least one crash if you set the first frame to a negative value
            assert(inputTimeIdentity != args.time || viewInvariance == eViewInvarianceAllViewsInvariant);

            // be safe in release mode otherwise we hit an infinite recursion
            if ( (inputTimeIdentity != args.time) || (viewInvariance == eViewInvarianceAllViewsInvariant) ) {
                ///This special value of -2 indicates that the plugin is identity of itself at another time
                boost::scoped_ptr<RenderRoIArgs> argCpy ( new RenderRoIArgs(args) );
                argCpy->time = inputTimeIdentity;

                if (viewInvariance == eViewInvarianceAllViewsInvariant) {
                    argCpy->view = ViewIdx(0);
                } else {
                    argCpy->view = inputIdentityView;
                }

                argCpy->preComputedRoD.clear(); //< clear as the RoD of the identity input might not be the same (reproducible with Blur)

                return _publicInterface->renderRoI(*argCpy, outputPlanes);
            }
        }

        double firstFrame, lastFrame;
        _publicInterface->getFrameRange_public(nodeHash, &firstFrame, &lastFrame);

        RectD canonicalRoI;
        ///WRONG! We can't clip against the RoD of *this* effect. We should clip against the RoD of the input effect, but this is done
        ///later on for us already.
        //args.roi.toCanonical(args.mipMapLevel, rod, &canonicalRoI);
        args.roi.toCanonical_noClipping(args.mipMapLevel, par,  &canonicalRoI);

        EffectInstancePtr inputEffectIdentity = _publicInterface->getInput(inputNbIdentity);
        if (inputEffectIdentity) {
            if ( frameArgs->stats && frameArgs->stats->isInDepthProfilingEnabled() ) {
                frameArgs->stats->setNodeIdentity( _publicInterface->getNode(), inputEffectIdentity->getNode() );
            }


            boost::scoped_ptr<RenderRoIArgs> inputArgs ( new RenderRoIArgs(args) );
            inputArgs->time = inputTimeIdentity;
            inputArgs->view = inputIdentityView;

            // Make sure we do not hold the RoD for this effect
            inputArgs->preComputedRoD.clear();


            /*
             When the effect is identity, we can make 2 different requests upstream:
             A) If they do not exist upstream, then this will result in a black image
             B) If instead we request what this node (the identity node) has set to the corresponding layer
             selector for the identity input, we may end-up with something different.

             So we have to use option B), but for some cases it requires behaviour A), e.g:
             1 - A Dot node does not have any channel selector and is expected to be a pass-through for layers.
             2 - A node's Output Layer choice set on All is expected to act as a Dot (because it is identity).
             This second case is already covered above in the code when choice is All, so we only have to worry
             about case 1
             */

            bool fetchUserSelectedComponentsUpstream = _publicInterface->getNode()->getChannelSelectorKnob(inputNbIdentity).get() != 0;

            if (fetchUserSelectedComponentsUpstream) {
                /// This corresponds to choice B)
                EffectInstance::ComponentsNeededMap::const_iterator foundCompsNeeded = neededComps->find(inputNbIdentity);
                if ( foundCompsNeeded != neededComps->end() ) {
                    inputArgs->components.clear();
                    for (std::size_t i = 0; i < foundCompsNeeded->second.size(); ++i) {
                        if (foundCompsNeeded->second[i].getNumComponents() != 0) {
                            inputArgs->components.push_back(foundCompsNeeded->second[i]);
                        }
                    }
                }
            } else {
                /// This corresponds to choice A)
                inputArgs->components = requestedComponents;
            }


            std::map<ImageComponents, ImagePtr> identityPlanes;
            RenderRoIRetCode ret =  inputEffectIdentity->renderRoI(*inputArgs, &identityPlanes);
            if (ret == eRenderRoIRetCodeOk) {
                outputPlanes->insert( identityPlanes.begin(), identityPlanes.end() );

                if (fetchUserSelectedComponentsUpstream) {
                    // We fetched potentially different components, so convert them to the format requested
                    std::map<ImageComponents, ImagePtr> convertedPlanes;
                    AppInstancePtr app = _publicInterface->getApp();
                    bool useAlpha0ForRGBToRGBAConversion = args.caller ? args.caller->getNode()->usesAlpha0ToConvertFromRGBToRGBA() : false;
                    std::list<ImageComponents>::const_iterator compIt = args.components.begin();

                    for (std::map<ImageComponents, ImagePtr>::iterator it = outputPlanes->begin(); it != outputPlanes->end(); ++it, ++compIt) {
                        ImagePremultiplicationEnum premult;
                        const ImageComponents & outComp = outputComponents.front();
                        if ( outComp.isColorPlane() ) {
                            premult = thisEffectOutputPremult;
                        } else {
                            premult = eImagePremultiplicationOpaque;
                        }

                        ImagePtr tmp = convertPlanesFormatsIfNeeded(app, it->second, args.roi, *compIt, inputArgs->bitdepth, useAlpha0ForRGBToRGBAConversion, premult, -1);
                        assert(tmp);
                        convertedPlanes[it->first] = tmp;
                    }
                    *outputPlanes = convertedPlanes;
                }
            } else {
                return ret;
            }
        } else {
            assert( outputPlanes->empty() );
        }

        return eRenderRoIRetCodeOk;
    } // if (identity)


    // At this point, if only the pass through planes are view variant and the rendered view is different than 0,
    // just call renderRoI again for the components left to render on the view 0.
    if ( (args.view != 0) && (viewInvariance == eViewInvarianceOnlyPassThroughPlanesVariant) ) {
        boost::scoped_ptr<RenderRoIArgs> argCpy( new RenderRoIArgs(args) );
        argCpy->view = ViewIdx(0);
        argCpy->preComputedRoD.clear();
        *isIdentity = true;
        return _publicInterface->renderRoI(*argCpy, outputPlanes);
    }

    assert(!*isIdentity);
    return eRenderRoIRetCodeOk;
} // EffectInstance::Implementation::handleIdentityEffect


bool
EffectInstance::Implementation::setupRenderRoIParams(const RenderRoIArgs & args,
                                                     EffectDataTLSPtr* tls,
                                                     AbortableRenderInfoPtr *abortInfo,
                                                     ParallelRenderArgsPtr* frameArgs,
                                                     OSGLContextPtr *glGpuContext,
                                                     OSGLContextPtr *glCpuContext,
                                                     U64 *nodeHash,
                                                     double *par,
                                                     ImageFieldingOrderEnum *fieldingOrder,
                                                     ImagePremultiplicationEnum *thisEffectOutputPremult,
                                                     SupportsEnum *supportsRS,
                                                     bool *renderFullScaleThenDownscale,
                                                     unsigned int *renderMappedMipMapLevel,
                                                     RenderScale *renderMappedScale,
                                                     const FrameViewRequest** requestPassData,
                                                     RectD* rod,
                                                     RectI* roi,
                                                     bool* isProjectFormat,
                                                     ComponentsNeededMapPtr *neededComps,
                                                     std::bitset<4> *processChannels,
                                                     const std::vector<ImageComponents>** outputComponents)
{
    // Do nothing if no components were requested
    if ( args.components.empty() ) {
        qDebug() << _publicInterface->getScriptName_mt_safe().c_str() << "renderRoi: Early bail-out components requested empty";

        return false;
    }
    if ( args.roi.isNull() ) {
        qDebug() << _publicInterface->getScriptName_mt_safe().c_str() << "renderRoi: Early bail-out ROI requested empty ";

        return false;
    }

    // Create the TLS data for this node if it did not exist yet
    *tls = tlsData->getOrCreateTLSData();
    assert(*tls);
    if ( (*tls)->frameArgs.empty() ) {
        qDebug() << QThread::currentThread() << "[BUG]:" << _publicInterface->getScriptName_mt_safe().c_str() <<  "Thread-storage for the render of the frame was not set.";

        frameArgs->reset(new ParallelRenderArgs);
        {
            NodesWList outputs;
            _publicInterface->getNode()->getOutputs_mt_safe(outputs);
            (*frameArgs)->visitsCount = (int)outputs.size();
        }
        (*frameArgs)->time = args.time;
        (*frameArgs)->nodeHash = _publicInterface->getHash();
        (*frameArgs)->view = args.view;
        (*frameArgs)->isSequentialRender = false;
        (*frameArgs)->isRenderResponseToUserInteraction = true;
        (*tls)->frameArgs.push_back(*frameArgs);
    } else {
        // The hash must not have changed if we did a pre-pass.
        (*frameArgs) = (*tls)->frameArgs.back();
        *glGpuContext = (*frameArgs)->openGLContext.lock();
        *glCpuContext = (*frameArgs)->cpuOpenGLContext.lock();
        *abortInfo = (*frameArgs)->abortInfo.lock();
        if (!*abortInfo) {
            // If we don't have info to identify the render, we cannot manage the OpenGL context properly, so don't try to render with OpenGL.
            glGpuContext->reset();
            glCpuContext->reset();
        }
        assert(!(*frameArgs)->request || (*frameArgs)->nodeHash == (*frameArgs)->request->nodeHash);
    }

    // Use the hash at this time, and then copy it to the clips in the thread local storage to use the same value
    // through all the rendering of this frame.
    *nodeHash = (*frameArgs)->nodeHash;

    *par = _publicInterface->getAspectRatio(-1);
    *fieldingOrder = _publicInterface->getFieldingOrder();
    *thisEffectOutputPremult = _publicInterface->getPremult();
    *supportsRS = _publicInterface->supportsRenderScaleMaybe();
    *renderFullScaleThenDownscale = (*supportsRS == eSupportsNo && args.mipMapLevel != 0);
    *renderMappedMipMapLevel = *renderFullScaleThenDownscale ? 0 : args.mipMapLevel;
    *renderMappedScale = ( Image::getScaleFromMipMapLevel(*renderMappedMipMapLevel) );
    assert( !( (*supportsRS == eSupportsNo) && !(renderMappedScale->x == 1. && renderMappedScale->y == 1.) ) );

    *requestPassData = 0;
    if ((*frameArgs)->request) {
        *requestPassData = (*frameArgs)->request->getFrameViewRequest(args.time, args.view);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Get the RoD ///////////////////////////////////////////////////////////////
    *isProjectFormat = false;
    {
        ///if the rod is already passed as parameter, just use it and don't call getRegionOfDefinition
        if ( !args.preComputedRoD.isNull() ) {
            *rod = args.preComputedRoD;
        } else {
            ///Check if the pre-pass already has the RoD
            if (*requestPassData) {
                *rod = (*requestPassData)->globalData.rod;
                *isProjectFormat = (*requestPassData)->globalData.isProjectFormat;
            } else {
                assert( !( (*supportsRS == eSupportsNo) && !(renderMappedScale->x == 1. && renderMappedScale->y == 1.) ) );
                StatusEnum stat = _publicInterface->getRegionOfDefinition_public(*nodeHash, args.time, *renderMappedScale, args.view, rod, isProjectFormat);

                // The rod might be NULL for a roto that has no beziers and no input
                if (stat == eStatusFailed) {
                    // If getRoD fails, this might be because the RoD is null after all (e.g: an empty Roto node), we don't want the render to fail
                    return false;
                } else if ( rod->isNull() ) {
                    // Nothing to render
                    return false;
                }
                if ( (*supportsRS == eSupportsMaybe) && (*renderMappedMipMapLevel != 0) ) {
                    // supportsRenderScaleMaybe may have changed, update it
                    *supportsRS = _publicInterface->supportsRenderScaleMaybe();
                    *renderFullScaleThenDownscale = (*supportsRS == eSupportsNo && args.mipMapLevel != 0);
                    if (*renderFullScaleThenDownscale) {
                        renderMappedScale->x = renderMappedScale->y = 1.;
                        *renderMappedMipMapLevel = 0;
                    }
                }
            }
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End get RoD ///////////////////////////////////////////////////////////////

    {
        if (*renderFullScaleThenDownscale) {
            //We cache 'image', hence the RoI should be expressed in its coordinates
            //renderRoIInternal should check the bitmap of 'image' and not downscaledImage!
            RectD canonicalRoI;
            args.roi.toCanonical(args.mipMapLevel, *par, *rod, &canonicalRoI);
            canonicalRoI.toPixelEnclosing(0, *par, roi);
        } else {
            *roi = args.roi;
        }
    }

    // Determine render planes
    neededComps->reset(new ComponentsNeededMap);
    ComponentsNeededMap::iterator foundOutputNeededComps;
    *outputComponents = 0;
    {
        bool processAllComponentsRequested;

        {
            SequenceTime ptTime;
            int ptView;
            NodePtr ptInput;
            _publicInterface->getComponentsNeededAndProduced_public(true, true, args.time, args.view, neededComps->get(), &processAllComponentsRequested, &ptTime, &ptView, processChannels, &ptInput);


            foundOutputNeededComps = (*neededComps)->find(-1);
            if ( foundOutputNeededComps == (*neededComps)->end() ) {
                return false;
            }
        }
        if (processAllComponentsRequested) {
            std::vector<ImageComponents> compVec;
            for (std::list<ImageComponents>::const_iterator it = args.components.begin(); it != args.components.end(); ++it) {
                bool found = false;
                assert( *it && !it->isPairedComponents() );
                //Change all needed comps in output to the requested components
                for (std::vector<ImageComponents>::const_iterator it2 = foundOutputNeededComps->second.begin(); it2 != foundOutputNeededComps->second.end(); ++it2) {
                    if ( ( it2->isColorPlane() && it->isColorPlane() ) ) {
                        compVec.push_back(*it2);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    compVec.push_back(*it);
                }
            }
            for (ComponentsNeededMap::iterator it = (*neededComps)->begin(); it != (*neededComps)->end(); ++it) {
                it->second = compVec;
            }
        }
    }
    *outputComponents = &foundOutputNeededComps->second;

    return true;
} // EffectInstance::Implementation::setupRenderRoIParams

static bool resolveRoI(unsigned int mipMapLevel,
                       double par,
                       const RectI& argsRoI,
                       const RectD& rod,
                       bool tilesSupported,
                       bool renderFullScaleThenDownscale,
                       RectI *downscaledImageBoundsNc,
                       RectI *upscaledImageBoundsNc,
                       RectI *roi,
                       RectI *originalRoI,
                       RectD *canonicalRoI) {

    {
        rod.toPixelEnclosing(mipMapLevel, par, downscaledImageBoundsNc);
        rod.toPixelEnclosing(0, par, upscaledImageBoundsNc);


        ///Make sure the RoI falls within the image bounds
        ///Intersection will be in pixel coordinates
        if (tilesSupported) {
            if (renderFullScaleThenDownscale) {
                if ( !roi->intersect(*upscaledImageBoundsNc, roi) ) {
                    return false;
                }
                assert(roi->x1 >= upscaledImageBoundsNc->x1 && roi->y1 >= upscaledImageBoundsNc->y1 &&
                       roi->x2 <= upscaledImageBoundsNc->x2 && roi->y2 <= upscaledImageBoundsNc->y2);
            } else {
                if ( !roi->intersect(*downscaledImageBoundsNc, roi) ) {
                    return false;
                }
                assert(roi->x1 >= downscaledImageBoundsNc->x1 && roi->y1 >= downscaledImageBoundsNc->y1 &&
                       roi->x2 <= downscaledImageBoundsNc->x2 && roi->y2 <= downscaledImageBoundsNc->y2);
            }
#ifndef NATRON_ALWAYS_ALLOCATE_FULL_IMAGE_BOUNDS
            ///just allocate the roi
            upscaledImageBoundsNc->intersect(*roi, upscaledImageBoundsNc);
            downscaledImageBoundsNc->intersect(argsRoI, downscaledImageBoundsNc);
#endif
        }
    }

    *originalRoI = *roi;
    if (!tilesSupported) {
        *roi = renderFullScaleThenDownscale ? *upscaledImageBoundsNc : *downscaledImageBoundsNc;
    }


    if (renderFullScaleThenDownscale) {
        roi->toCanonical(0, par, rod, canonicalRoI);
    } else {
        roi->toCanonical(mipMapLevel, par, rod, canonicalRoI);
    }



    return true;
} // resolveRoI

bool
EffectInstance::Implementation::resolveRenderDevice(const RenderRoIArgs & args,
                                                    const ParallelRenderArgsPtr& frameArgs,
                                                    const AbortableRenderInfoPtr& abortInfo,
                                                    const OSGLContextPtr& glGpuContext,
                                                    const OSGLContextPtr& glCpuContext,
                                                    const RectI& downscaledImageBounds,
                                                    RenderScale* renderMappedScale,
                                                    unsigned int *renderMappedMipMapLevel,
                                                    bool *renderFullScaleThenDownscale,
                                                    RectI* roi,
                                                    StorageModeEnum* storage,
                                                    OSGLContextPtr *glRenderContext,
                                                    OSGLContextAttacherPtr *glContextLocker,
                                                    bool *useOpenGL,
                                                    EffectInstance::RenderRoIRetCode* resolvedError)
{
    *storage = eStorageModeRAM;
    if ( dynamic_cast<DiskCacheNode*>(_publicInterface) ) {
        *storage = eStorageModeDisk;
    } else if (glGpuContext && ( (frameArgs->currentOpenglSupport == ePluginOpenGLRenderSupportNeeded) ||
                                ( ( frameArgs->currentOpenglSupport == ePluginOpenGLRenderSupportYes) && args.allowGPURendering) ) ) {
        // Enable GPU render if the plug-in cannot render another way or if all conditions are met
        if (frameArgs->currentOpenglSupport == ePluginOpenGLRenderSupportNeeded && !_publicInterface->getNode()->getPlugin()->isOpenGLEnabled()) {
            QString message = tr("OpenGL render is required for  %1 but was disabled in the Preferences for this plug-in, please enable it and restart %2").arg(QString::fromUtf8(_publicInterface->getNode()->getLabel().c_str())).arg(QString::fromUtf8(NATRON_APPLICATION_NAME));
            _publicInterface->setPersistentMessage(eMessageTypeError, message.toStdString());
            *resolvedError = eRenderRoIRetCodeFailed;
            return false;
        }

        *glRenderContext = glGpuContext;

        *storage = eStorageModeGLTex;

        // If the plug-in knows how to render on CPU, check if we actually should not render on CPU instead.
        if (frameArgs->currentOpenglSupport == ePluginOpenGLRenderSupportYes) {
            // User want to force caching of this node but we cannot cache OpenGL renders, so fallback on CPU.
            if ( _publicInterface->getNode()->isForceCachingEnabled() ) {
                *storage = eStorageModeRAM;
            }

            // If a node has multiple outputs, do not render it on OpenGL since we do not use the cache. We could end-up with this render being executed multiple times.
            // Also, if the render time is different from the caller render time, don't render using OpenGL otherwise we could computed this render multiple times.

            if (*storage == eStorageModeGLTex) {
                if ( (frameArgs->visitsCount > 1) ||
                    ( args.time != args.callerRenderTime) ) {
                    *storage = eStorageModeRAM;
                }
            }

            // Ensure that the texture will be at least smaller than the maximum OpenGL texture size
            if (*storage == eStorageModeGLTex) {
                if ( (roi->width() >= (*glRenderContext)->getMaxOpenGLWidth()) ||
                    ( roi->height() >= (*glRenderContext)->getMaxOpenGLHeight()) ) {
                    // Fallback on CPU rendering since the image is larger than the maximum allowed OpenGL texture size
                    *storage = eStorageModeRAM;
                }
            }
        }
        if (*storage == eStorageModeGLTex) {
            // OpenGL renders always support render scale...
            if (*renderFullScaleThenDownscale) {
                *renderFullScaleThenDownscale = false;
                *renderMappedMipMapLevel = args.mipMapLevel;
                renderMappedScale->x = renderMappedScale->y = Image::getScaleFromMipMapLevel(*renderMappedMipMapLevel);
                if (frameArgs->tilesSupported) {
                    *roi = args.roi;
                    if ( !roi->intersect(downscaledImageBounds, roi) ) {
                        *resolvedError = eRenderRoIRetCodeOk;
                        return false;
                    }
                } else {
                    *roi = downscaledImageBounds;
                }
            }
        }
    }

    bool supportsOSMesa = _publicInterface->canCPUImplementationSupportOSMesa() && glCpuContext;
    if (*storage == eStorageModeGLTex) {
        // Make the OpenGL context current to this thread
        glContextLocker->reset( new OSGLContextAttacher(*glRenderContext, abortInfo
#ifdef DEBUG
                                                       , frameArgs->time
#endif
                                                       ) );
    } else {
        if (supportsOSMesa) {
            *glRenderContext = glCpuContext;
            if ( (roi->width() >= (*glRenderContext)->getMaxOpenGLWidth()) ||
                ( roi->height() >= (*glRenderContext)->getMaxOpenGLHeight()) ) {
                supportsOSMesa = false;
            }
        }
    }

    *useOpenGL = *storage == eStorageModeGLTex || supportsOSMesa;
    assert(!*useOpenGL || *glRenderContext);
    *resolvedError = eRenderRoIRetCodeOk;
    return true;
} // EffectInstance::Implementation::resolveRenderDevice



bool
EffectInstance::Implementation::renderRoILookupCacheFirstTime(const EffectInstance::RenderRoIArgs & args,
                                                              const ParallelRenderArgsPtr& frameArgs,
                                                              StorageModeEnum storage,
                                                              const OSGLContextPtr& glRenderContext,
                                                              const OSGLContextAttacherPtr& glContextLocker,
                                                              const ImagePlanesToRenderPtr &planesToRender,
                                                              const std::list<ImageComponents>& requestedComponents,
                                                              const std::vector<ImageComponents>& outputComponents,
                                                              const RectD& rod,
                                                              const RectI& roi,
                                                              const RectI& upscaledImageBounds,
                                                              const RectI& downscaledImageBounds,
                                                              bool renderFullScaleThenDownscale,
                                                              U64 nodeHash,
                                                              unsigned int renderMipMapLevel,
                                                              bool* createInCache,
                                                              bool *renderScaleOneUpstreamIfRenderScaleSupportDisabled,
                                                              ImagePtr *isPlaneCached,
                                                              boost::scoped_ptr<ImageKey>* key,
                                                              std::map<ImageComponents, ImagePtr>* outputPlanes)
{
    const bool isFrameVaryingOrAnimated = _publicInterface->isFrameVaryingOrAnimated_Recursive();

    // Do not use the cache for OpenGL rendering
    if (storage == eStorageModeGLTex && glRenderContext->isGPUContext()) {
        *createInCache = false;
    } else {
        // in Analysis, the node upstream of the analysis node should always cache
        *createInCache = (frameArgs->isAnalysis && frameArgs->treeRoot->getEffectInstance() == args.caller) ? true : _publicInterface->shouldCacheOutput(isFrameVaryingOrAnimated, args.time, args.view, frameArgs->visitsCount);
    }

    // Do we want to render the graph upstream at scale 1 or at the requested render scale ? (user setting)
    *renderScaleOneUpstreamIfRenderScaleSupportDisabled = false;
    if (renderFullScaleThenDownscale) {

        *renderScaleOneUpstreamIfRenderScaleSupportDisabled = _publicInterface->getNode()->useScaleOneImagesWhenRenderScaleSupportIsDisabled();

        // For multi-resolution we want input images with exactly the same size as the output image
        if ( !*renderScaleOneUpstreamIfRenderScaleSupportDisabled && !_publicInterface->supportsMultiResolution() ) {
            *renderScaleOneUpstreamIfRenderScaleSupportDisabled = true;
        }
    }


    const bool draftModeSupported = _publicInterface->getNode()->isDraftModeUsed();
    key->reset( new ImageKey(_publicInterface->getNode().get(),
                             nodeHash,
                             isFrameVaryingOrAnimated,
                             args.time,
                             args.view,
                             1.,
                             draftModeSupported && frameArgs->draftMode,
                             renderMipMapLevel == 0 && args.mipMapLevel != 0 && !*renderScaleOneUpstreamIfRenderScaleSupportDisabled) );
    boost::scoped_ptr<ImageKey> nonDraftKey( new ImageKey(_publicInterface->getNode().get(),
                                                          nodeHash,
                                                          isFrameVaryingOrAnimated,
                                                          args.time,
                                                          args.view,
                                                          1.,
                                                          false,
                                                          renderMipMapLevel == 0 && args.mipMapLevel != 0 && !*renderScaleOneUpstreamIfRenderScaleSupportDisabled) );




    {
        //If one plane is missing from cache, we will have to render it all. For all other planes, either they have nothing
        //left to render, otherwise we render them for all the roi again.
        bool missingPlane = false;

        for (std::list<ImageComponents>::const_iterator it = requestedComponents.begin(); it != requestedComponents.end(); ++it) {
            EffectInstance::PlaneToRender plane;

            /*
             * If the plane is the color plane, we might have to convert between components, hence we always
             * try to find in the cache the "preferred" components of this node for the color plane.
             * For all other planes, just consider this set of components, we do not allow conversion.
             */
            const ImageComponents* components = 0;
            if ( !it->isColorPlane() ) {
                components = &(*it);
            } else {
                for (std::vector<ImageComponents>::const_iterator it2 = outputComponents.begin(); it2 != outputComponents.end(); ++it2) {
                    if ( it2->isColorPlane() ) {
                        components = &(*it2);
                        break;
                    }
                }
            }
            assert(components);
            if (!components) {
                continue;
            }
            //For writers, we always want to call the render action when doing a sequential render, but we still want to use the cache for nodes upstream
            bool doCacheLookup = !_publicInterface->isWriter() || !frameArgs->isSequentialRender;
            if (doCacheLookup) {
                int nLookups = draftModeSupported && frameArgs->draftMode ? 2 : 1;

                for (int n = 0; n < nLookups; ++n) {
                    _publicInterface->getImageFromCacheAndConvertIfNeeded(createInCache,
                                                                          frameArgs->isDuringPaintStrokeCreation,
                                                                          storage,
                                                                          args.returnStorage,
                                                                          n == 0 ? *nonDraftKey : **key,
                                                                          renderMipMapLevel,
                                                                          renderFullScaleThenDownscale ? &upscaledImageBounds : &downscaledImageBounds,
                                                                          &rod,
                                                                          roi,
                                                                          args.bitdepth,
                                                                          *it,
                                                                          args.inputImagesList,
                                                                          frameArgs->stats,
                                                                          glContextLocker,
                                                                          &plane.fullscaleImage);
                    if (plane.fullscaleImage) {
                        break;
                    }
                }
            }

            if (args.byPassCache) {
                if (plane.fullscaleImage) {
                    appPTR->removeFromNodeCache( (*key)->getHash() );
                    plane.fullscaleImage.reset();
                }
            }
            if (plane.fullscaleImage) {
                if (missingPlane) {
                    std::list<RectI> restToRender;
                    plane.fullscaleImage->getRestToRender(roi, restToRender);
                    if ( !restToRender.empty() ) {
                        appPTR->removeFromNodeCache(plane.fullscaleImage);
                        plane.fullscaleImage.reset();
                    } else {
                        outputPlanes->insert( std::make_pair(*it, plane.fullscaleImage) );
                        continue;
                    }
                }
            } else {
                if (!missingPlane) {
                    missingPlane = true;
                    //Ensure that previous planes are either already rendered or otherwise render them  again
                    std::map<ImageComponents, EffectInstance::PlaneToRender> newPlanes;
                    for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it2 = planesToRender->planes.begin();
                         it2 != planesToRender->planes.end(); ++it2) {
                        if (it2->second.fullscaleImage) {
                            std::list<RectI> restToRender;
                            it2->second.fullscaleImage->getRestToRender(roi, restToRender);
                            if ( !restToRender.empty() ) {
                                appPTR->removeFromNodeCache(it2->second.fullscaleImage);
                                it2->second.fullscaleImage.reset();
                                it2->second.downscaleImage.reset();
                                newPlanes.insert(*it2);
                            } else {
                                outputPlanes->insert( std::make_pair(it2->first, it2->second.fullscaleImage) );
                            }
                        } else {
                            newPlanes.insert(*it2);
                        }
                    }
                    planesToRender->planes = newPlanes;
                }
            }

            plane.downscaleImage = plane.fullscaleImage;
            plane.isAllocatedOnTheFly = false;
            planesToRender->planes.insert( std::make_pair(*it, plane) );
        }
    }

    assert( !planesToRender->planes.empty() );

    // Release the context from this thread as it may have been used  when calling getImageFromCacheAndConvertIfNeeded
    // This will enable all threads to be concurrent again to render input images
    if (glContextLocker) {
        glContextLocker->dettach();
    }

    ///In the event where we had the image from the cache, but it wasn't completly rendered over the RoI but the cache was almost full,
    ///we don't hold a pointer to it, allowing the cache to free it.
    ///Hence after rendering all the input images, we redo a cache look-up to check whether the image is still here

    if ( !planesToRender->planes.empty() ) {
        *isPlaneCached = planesToRender->planes.begin()->second.fullscaleImage;
    }

    if ( !*isPlaneCached && args.roi.isNull() ) {
        ///Empty RoI and nothing in the cache with matching args, return empty planes.
        return false;
    }

    return true;

} // EffectInstance::Implementation;;renderRoILookupCacheFirstTime


EffectInstance::RenderRoIRetCode
EffectInstance::Implementation::renderRoIRenderInputImages(const RenderRoIArgs & args,
                                                           const EffectDataTLSPtr& tls,
                                                           const ComponentsNeededMapPtr& neededComps,
                                                           const FrameViewRequest* requestPassData,
                                                           const ImagePlanesToRenderPtr &planesToRender,
                                                           bool useTransforms,
                                                           StorageModeEnum storage,
                                                           const std::vector<ImageComponents>& outputComponents,
                                                           ImagePremultiplicationEnum thisEffectOutputPremult,
                                                           const RectD& rod,
                                                           double par,
                                                           U64 nodeHash,
                                                           bool renderFullScaleThenDownscale,
                                                           unsigned int renderMappedMipMapLevel,
                                                           const RenderScale& renderMappedScale,
                                                           bool renderScaleOneUpstreamIfRenderScaleSupportDisabled,
                                                           boost::shared_ptr<FramesNeededMap>* framesNeeded)
{
    framesNeeded->reset(new FramesNeededMap);
    if (requestPassData) {
        **framesNeeded = requestPassData->globalData.frameViewsNeeded;
    } else {
        **framesNeeded = _publicInterface->getFramesNeeded_public(nodeHash, args.time, args.view, renderMappedMipMapLevel);
    }


    // Pre-render input images before allocating the image if we need to render
    {
        if ( !outputComponents.empty() && outputComponents.front().isColorPlane() ) {
            planesToRender->outputPremult = thisEffectOutputPremult;
        } else {
            planesToRender->outputPremult = eImagePremultiplicationOpaque;
        }
    }
    for (std::list<RectToRender>::iterator it = planesToRender->rectsToRender.begin(); it != planesToRender->rectsToRender.end(); ++it) {
        if (it->isIdentity) {
            continue;
        }
        RenderRoIRetCode inputCode;
        {
            RectD canonicalRoI;
            if (renderFullScaleThenDownscale) {
                it->rect.toCanonical(0, par, rod, &canonicalRoI);
            } else {
                it->rect.toCanonical(args.mipMapLevel, par, rod, &canonicalRoI);
            }

            inputCode = _publicInterface->renderInputImagesForRoI(requestPassData,
                                                                  useTransforms,
                                                                  storage,
                                                                  args.time,
                                                                  args.view,
                                                                  rod,
                                                                  canonicalRoI,
                                                                  tls->currentRenderArgs.transformRedirections,
                                                                  args.mipMapLevel,
                                                                  renderMappedScale,
                                                                  renderScaleOneUpstreamIfRenderScaleSupportDisabled,
                                                                  args.byPassCache,
                                                                  **framesNeeded,
                                                                  *neededComps,
                                                                  &it->imgs,
                                                                  &it->inputRois);
        }
        if ( planesToRender->inputPremult.empty() ) {
            for (InputImagesMap::iterator it2 = it->imgs.begin(); it2 != it->imgs.end(); ++it2) {
                EffectInstancePtr input = _publicInterface->getInput(it2->first);
                if (input) {
                    ImagePremultiplicationEnum inputPremult = input->getPremult();
                    if ( !it2->second.empty() ) {
                        const ImageComponents & comps = it2->second.front()->getComponents();
                        if ( !comps.isColorPlane() ) {
                            inputPremult = eImagePremultiplicationOpaque;
                        }
                    }

                    planesToRender->inputPremult[it2->first] = inputPremult;
                }
            }
        }

        // Render was aborted
        if (inputCode != eRenderRoIRetCodeOk) {
            return inputCode;
        }
    }

    return eRenderRoIRetCodeOk;
} // EffectInstance::Implementation::renderRoIRenderInputImages

EffectInstance::RenderRoIRetCode
EffectInstance::Implementation::renderRoISecondCacheLookup(const RenderRoIArgs & args,
                                                           const EffectDataTLSPtr& tls,
                                                           const ParallelRenderArgsPtr& frameArgs,
                                                           const ComponentsNeededMapPtr& neededComps,
                                                           const FrameViewRequest* requestPassData,
                                                           const ImagePlanesToRenderPtr &planesToRender,
                                                           const OSGLContextAttacherPtr& glContextLocker,
                                                           bool useTransforms,
                                                           StorageModeEnum storage,
                                                           const std::vector<ImageComponents>& outputComponents,
                                                           const RectD& rod,
                                                           const RectI& roi,
                                                           const RectI& upscaledImageBounds,
                                                           const RectI& downscaledImageBounds,
                                                           const RectI& inputsRoDIntersectionPixel,
                                                           bool tryIdentityOptim,
                                                           double par,
                                                           bool renderFullScaleThenDownscale,
                                                           bool createInCache,
                                                           unsigned int renderMappedMipMapLevel,
                                                           const RenderScale& renderMappedScale,
                                                           bool renderScaleOneUpstreamIfRenderScaleSupportDisabled,
                                                           const boost::shared_ptr<FramesNeededMap>& framesNeeded,
                                                           const boost::scoped_ptr<ImageKey>& key,
                                                           ImagePtr *isPlaneCached)
{
    for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
        /*
         * If the plane is the color plane, we might have to convert between components, hence we always
         * try to find in the cache the "preferred" components of this node for the color plane.
         * For all other planes, just consider this set of components, we do not allow conversion.
         */
        const ImageComponents* components = 0;
        if ( !it->first.isColorPlane() ) {
            components = &(it->first);
        } else {
            for (std::vector<ImageComponents>::const_iterator it = outputComponents.begin(); it != outputComponents.end(); ++it) {
                if ( it->isColorPlane() ) {
                    components = &(*it);
                    break;
                }
            }
        }

        assert(components);
        if (!components) {
            continue;
        }
        _publicInterface->getImageFromCacheAndConvertIfNeeded(createInCache,
                                                              frameArgs->isDuringPaintStrokeCreation,
                                                              storage,
                                                              args.returnStorage,
                                                              *key,
                                                              renderMappedMipMapLevel,
                                                              renderFullScaleThenDownscale ? &upscaledImageBounds : &downscaledImageBounds,
                                                              &rod,
                                                              roi,
                                                              args.bitdepth,
                                                              it->first,
                                                              args.inputImagesList,
                                                              frameArgs->stats,
                                                              glContextLocker,
                                                              &it->second.fullscaleImage);

        ///We must retrieve from the cache exactly the originally retrieved image, otherwise we might have to call  renderInputImagesForRoI
        ///again, which could create a vicious cycle.
        if ( it->second.fullscaleImage && (it->second.fullscaleImage.get() == it->second.originalCachedImage) ) {
            it->second.downscaleImage = it->second.fullscaleImage;
        } else {
            for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it2 = planesToRender->planes.begin(); it2 != planesToRender->planes.end(); ++it2) {
                it2->second.fullscaleImage.reset();
                it2->second.downscaleImage.reset();
            }
            break;
        }
    }

    *isPlaneCached = planesToRender->planes.begin()->second.fullscaleImage;

    if (!*isPlaneCached) {
        planesToRender->rectsToRender.clear();

        std::list<RectI> rectsLeftToRender;
        if (frameArgs->tilesSupported) {
            rectsLeftToRender.push_back(roi);
        } else {
            rectsLeftToRender.push_back(renderFullScaleThenDownscale ? upscaledImageBounds : downscaledImageBounds);
        }


        if ( tryIdentityOptim && !rectsLeftToRender.empty() ) {
            optimizeRectsToRender(_publicInterface->shared_from_this(), inputsRoDIntersectionPixel, rectsLeftToRender, args.time, args.view, renderMappedScale, &planesToRender->rectsToRender);
        } else {
            for (std::list<RectI>::iterator it = rectsLeftToRender.begin(); it != rectsLeftToRender.end(); ++it) {
                if ( it->isNull() ) {
                    continue;
                }
                RectToRender r;
                r.rect = *it;
                r.identityTime = 0;
                r.isIdentity = false;
                planesToRender->rectsToRender.push_back(r);
            }
        }

        ///We must re-copute input images because we might not have rendered what's needed
        for (std::list<RectToRender>::iterator it = planesToRender->rectsToRender.begin();
             it != planesToRender->rectsToRender.end(); ++it) {
            if (it->isIdentity) {
                continue;
            }

            RectD canonicalRoI;
            if (renderFullScaleThenDownscale) {
                it->rect.toCanonical(0, par, rod, &canonicalRoI);
            } else {
                it->rect.toCanonical(args.mipMapLevel, par, rod, &canonicalRoI);
            }

            RenderRoIRetCode inputRetCode = _publicInterface->renderInputImagesForRoI(requestPassData,
                                                                                      useTransforms,
                                                                                      storage,
                                                                                      args.time,
                                                                                      args.view,
                                                                                      rod,
                                                                                      canonicalRoI,
                                                                                      tls->currentRenderArgs.transformRedirections,
                                                                                      args.mipMapLevel,
                                                                                      renderMappedScale,
                                                                                      renderScaleOneUpstreamIfRenderScaleSupportDisabled,
                                                                                      args.byPassCache,
                                                                                      *framesNeeded,
                                                                                      *neededComps,
                                                                                      &it->imgs,
                                                                                      &it->inputRois);
            //Render was aborted
            if (inputRetCode != eRenderRoIRetCodeOk) {
                return inputRetCode;
            }
        }
    }
    return eRenderRoIRetCodeOk;
} // EffectInstance::Implementation::renderRoISecondCacheLookup

void
EffectInstance::Implementation::renderRoIAllocateOutputPlanes(const RenderRoIArgs & args,
                                                              const ParallelRenderArgsPtr& frameArgs,
                                                              const ImagePlanesToRenderPtr &planesToRender,
                                                              const OSGLContextAttacherPtr& glContextLocker,
                                                              const OSGLContextPtr& glRenderContext,
                                                              ImageFieldingOrderEnum fieldingOrder,
                                                              ImageBitDepthEnum outputDepth,
                                                              bool isProjectFormat,
                                                              bool fillGrownBoundsWithZeroes,
                                                              StorageModeEnum storage,
                                                              const std::vector<ImageComponents>& outputComponents,
                                                              const RectD& rod,
                                                              const RectI& upscaledImageBounds,
                                                              const RectI& downscaledImageBounds,
                                                              const RectI& lastStrokePixelRoD,
                                                              double par,
                                                              bool renderFullScaleThenDownscale,
                                                              bool createInCache,
                                                              const boost::scoped_ptr<ImageKey>& key)
{
    // For all planes, if needed allocate the associated image

    if (glContextLocker) {
        glContextLocker->attach();
    }
    for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planesToRender->planes.begin();
         it != planesToRender->planes.end(); ++it) {
        const ImageComponents *components = 0;

        if ( !it->first.isColorPlane() ) {
            //This plane is not color, there can only be a single set of components
            components = &(it->first);
        } else {
            //Find color plane from clip preferences
            for (std::vector<ImageComponents>::const_iterator it = outputComponents.begin(); it != outputComponents.end(); ++it) {
                if ( it->isColorPlane() ) {
                    components = &(*it);
                    break;
                }
            }
        }
        assert(components);
        if (!components) {
            continue;
        }

        RotoDrawableItemPtr rotoItem = _publicInterface->getNode()->getAttachedRotoItem();
        if (!it->second.fullscaleImage) {
            ///The image is not cached
            _publicInterface->allocateImagePlane(*key,
                               rod,
                               downscaledImageBounds,
                               upscaledImageBounds,
                               isProjectFormat,
                               *components,
                               args.bitdepth,
                               planesToRender->outputPremult,
                               fieldingOrder,
                               par,
                               args.mipMapLevel,
                               renderFullScaleThenDownscale,
                               glRenderContext,
                               storage,
                               createInCache,
                               &it->second.fullscaleImage,
                               &it->second.downscaleImage);
            
            // For plug-ins that paint over themselves, the first time clear the image out
            if (_publicInterface->isPaintingOverItselfEnabled()) {
                it->second.downscaleImage->fillBoundsZero(glRenderContext);
            }

            // Set the painting buffer for this node if we are creating a paint stroke or doing the "clean" render following up a drawing
            // to prepare the potential next paint brush stroke made by the user
            if (rotoItem && (frameArgs->isDuringPaintStrokeCreation || rotoItem->getContext()->isDoingNeatRender())) {
                _publicInterface->getNode()->setPaintBuffer(it->second.downscaleImage);
            }
        } else {
            /*
             * There might be a situation  where the RoD of the cached image
             * is not the same as this RoD even though the hash is the same.
             * This seems to happen with the Roto node. This hack just updates the
             * image's RoD to prevent an assert from triggering in the call to ensureBounds() below.
             */
            RectD oldRod = it->second.fullscaleImage->getRoD();
            if (oldRod != rod) {
                oldRod.merge(rod);
                it->second.fullscaleImage->setRoD(oldRod);
            }


            /*
             * Another thread might have allocated the same image in the cache but with another RoI, make sure
             * it is big enough for us, or resize it to our needs.
             */
            bool hasResized;
            if (it->second.fullscaleImage->getStorageMode() == eStorageModeGLTex) {
                assert(glContextLocker);
                glContextLocker->attach();
            }
            if (args.calledFromGetImage) {
                /*
                 * When called from EffectInstance::getImage() we must prevent from taking any write lock because
                 * this image probably already has a lock for read on it. To overcome the write lock, we resize in a
                 * separate image and then we swap the images in the cache directly, without taking the image write lock.
                 */

                hasResized = it->second.fullscaleImage->copyAndResizeIfNeeded(renderFullScaleThenDownscale ? upscaledImageBounds : downscaledImageBounds, fillGrownBoundsWithZeroes, fillGrownBoundsWithZeroes, &it->second.cacheSwapImage, glRenderContext);
                if (hasResized) {
                    ///Work on the swapImg and then swap in the cache
                    ImagePtr swapImg = it->second.cacheSwapImage;
                    it->second.cacheSwapImage = it->second.fullscaleImage;
                    it->second.fullscaleImage = swapImg;
                    if (!renderFullScaleThenDownscale) {
                        it->second.downscaleImage = it->second.fullscaleImage;
                    }
                }
            } else {
                hasResized = it->second.fullscaleImage->ensureBounds(glRenderContext, renderFullScaleThenDownscale ? upscaledImageBounds : downscaledImageBounds,
                                                                     fillGrownBoundsWithZeroes, fillGrownBoundsWithZeroes);
                if (hasResized) {
                    // Set the painting buffer for this node if we are creating a paint stroke or doing the "clean" render following up a drawing
                    // to prepare the potential next paint brush stroke made by the user
                    it->second.downscaleImage = it->second.fullscaleImage;
                    if (rotoItem && (frameArgs->isDuringPaintStrokeCreation || rotoItem->getContext()->isDoingNeatRender())) {
                        _publicInterface->getNode()->setPaintBuffer(it->second.fullscaleImage);
                    }
                }
            }


            /*
             * Note that the image has been resized and the bitmap explicitly set to 1 in the newly allocated portions (for rotopaint purpose).
             * We must reset it back to 0 in the last stroke tick RoD.
             */
            if (hasResized && fillGrownBoundsWithZeroes && it->second.fullscaleImage->usesBitMap()) {
                it->second.fullscaleImage->clearBitmap(lastStrokePixelRoD);
            }

            if ( renderFullScaleThenDownscale && (it->second.fullscaleImage->getMipMapLevel() == 0) ) {
                RectI bounds;
                rod.toPixelEnclosing(args.mipMapLevel, par, &bounds);
                it->second.downscaleImage.reset( new Image(*components,
                                                           rod,
                                                           downscaledImageBounds,
                                                           args.mipMapLevel,
                                                           it->second.fullscaleImage->getPixelAspectRatio(),
                                                           outputDepth,
                                                           planesToRender->outputPremult,
                                                           fieldingOrder,
                                                           true) );

                it->second.fullscaleImage->downscaleMipMap( rod, it->second.fullscaleImage->getBounds(), 0, args.mipMapLevel, true, it->second.downscaleImage.get() );
            }
        }

        ///The image and downscaled image are pointing to the same image in 2 cases:
        ///1) Proxy mode is turned off
        ///2) Proxy mode is turned on but plug-in supports render scale
        ///Subsequently the image and downscaled image are different only if the plug-in
        ///does not support the render scale and the proxy mode is turned on.
        assert( (it->second.fullscaleImage == it->second.downscaleImage && !renderFullScaleThenDownscale) ||
               ( ( it->second.fullscaleImage != it->second.downscaleImage || it->second.fullscaleImage->getMipMapLevel() == it->second.downscaleImage->getMipMapLevel() ) && renderFullScaleThenDownscale ) );
    }

} // EffectInstance::Implementation::renderRoIAllocateOutputPlanes

EffectInstance::RenderRoIStatusEnum
EffectInstance::Implementation::renderRoILaunchInternalRender(const RenderRoIArgs & args,
                                                              const ParallelRenderArgsPtr& frameArgs,
                                                              const ImagePlanesToRenderPtr &planesToRender,
                                                              const ComponentsNeededMapPtr& neededComps,
                                                              const OSGLContextPtr& glRenderContext,
                                                              ImageBitDepthEnum outputDepth,
                                                              const ImageComponents &outputClipPrefComps,
                                                              bool hasSomethingToRender,
                                                              StorageModeEnum storage,
                                                              U64 nodeHash,
                                                              const RectD& rod,
                                                              const RectI& roi,
                                                              unsigned int renderMappedMipMapLevel,
                                                              const std::bitset<4> &processChannels,
                                                              double par,
                                                              bool renderFullScaleThenDownscale,
                                                              bool *renderAborted)
{

    // If we reach here, it can be either because the planes are cached or not, either way
    // the planes are NOT a total identity, and they may have some content left to render.
    EffectInstance::RenderRoIStatusEnum renderRetCode = eRenderRoIStatusImageAlreadyRendered;
    if (!hasSomethingToRender && !planesToRender->isBeingRenderedElsewhere) {
        *renderAborted = _publicInterface->aborted();
    } else {

        // There should always be at least 1 plane to render (The color plane)
        assert( !planesToRender->planes.empty() );

#if NATRON_ENABLE_TRIMAP
        ///Only use trimap system if the render cannot be aborted.
        if ( frameArgs->isCurrentFrameRenderNotAbortable() ) {
            for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
                markImageAsBeingRendered(renderFullScaleThenDownscale ? it->second.fullscaleImage : it->second.downscaleImage);
            }
        }
#endif

        if (hasSomethingToRender) {
            // eRenderSafetyInstanceSafe means that there is at most one render per instance
            // NOTE: the per-instance lock should probably be shared between
            // all clones of the same instance, because an InstanceSafe plugin may assume it is the sole owner of the output image,
            // and read-write on it.
            // It is probably safer to assume that several clones may write to the same output image only in the eRenderSafetyFullySafe case.

            // eRenderSafetyFullySafe means that there is only one render per FRAME : the lock is per image

            boost::scoped_ptr<QMutexLocker> locker;
            RenderSafetyEnum safety = frameArgs->currentThreadSafety;

            EffectInstancePtr renderInstance;
            /**
             * Figure out If this node should use a render clone rather than execute renderRoIInternal on the main (this) instance.
             * Reasons to use a render clone is be because a plug-in is eRenderSafetyInstanceSafe or does not support
             * concurrent GL renders.
             **/
            bool useRenderClone = (safety == eRenderSafetyInstanceSafe && !frameArgs->isDuringPaintStrokeCreation) || (safety != eRenderSafetyUnsafe && storage == eStorageModeGLTex && !_publicInterface->supportsConcurrentOpenGLRenders());
            if (useRenderClone) {
                renderInstance = _publicInterface->getOrCreateRenderInstance();
            } else {
                renderInstance = _publicInterface->shared_from_this();
            }
            assert(renderInstance);

            if (safety == eRenderSafetyInstanceSafe) {
                locker.reset( new QMutexLocker( &_publicInterface->getNode()->getRenderInstancesSharedMutex() ) );
            } else if (safety == eRenderSafetyUnsafe) {
                const Plugin* p = _publicInterface->getNode()->getPlugin();
                assert(p);
                locker.reset( new QMutexLocker( p->getPluginLock() ) );
            } else {
                // no need to lock
                Q_UNUSED(locker);
            }


            ///For eRenderSafetyFullySafe, don't take any lock, the image already has a lock on itself so we're sure it can't be written to by 2 different threads.

            if ( frameArgs->stats && frameArgs->stats->isInDepthProfilingEnabled() ) {
                frameArgs->stats->setGlobalRenderInfosForNode(_publicInterface->getNode(), rod, planesToRender->outputPremult, processChannels, frameArgs->tilesSupported, !renderFullScaleThenDownscale, renderMappedMipMapLevel);
            }

# ifdef DEBUG

            /*{
             const std::list<RectToRender>& rectsToRender = planesToRender->rectsToRender;
             qDebug() <<'('<<QThread::currentThread()<<")--> "<< getNode()->getScriptName_mt_safe().c_str() << ": render view: " << args.view << ", time: " << args.time << " No. tiles: " << rectsToRender.size() << " rectangles";
             for (std::list<RectToRender>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
             qDebug() << "rect: " << "x1= " <<  it->rect.x1 << " , y1= " << it->rect.y1 << " , x2= " << it->rect.x2 << " , y2= " << it->rect.y2 << "(identity:" << it->isIdentity << ")";
             }
             for (std::map<ImageComponents, PlaneToRender> ::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
             qDebug() << "plane: " <<  it->second.downscaleImage.get() << it->first.getLayerName().c_str();
             }
             qDebug() << "Cached:" << (isPlaneCached.get() != 0) << "Rendered elsewhere:" << planesToRender->isBeingRenderedElsewhere;

             }*/
# endif


            bool attachGLOK = true;
            if (planesToRender->useOpenGL) {
                assert(glRenderContext);
                Natron::StatusEnum stat = renderInstance->attachOpenGLContext_public(glRenderContext, &planesToRender->glContextData);
                if (stat == eStatusOutOfMemory) {
                    renderRetCode = eRenderRoIStatusRenderOutOfGPUMemory;
                    attachGLOK = false;
                } else if (stat == eStatusFailed) {
                    // Should we not use eRenderRoIStatusRenderOutOfGPUMemory also so that we try CPU rendering instead of failing the render alltogether ?
                    renderRetCode = eRenderRoIStatusRenderFailed;
                    attachGLOK = false;
                }
            }
            if (attachGLOK) {
                renderRetCode = _publicInterface->renderRoIInternal(renderInstance,
                                                                    glRenderContext,
                                                                    args.time,
                                                                    frameArgs,
                                                                    safety,
                                                                    args.mipMapLevel,
                                                                    args.view,
                                                                    rod,
                                                                    par,
                                                                    planesToRender,
                                                                    frameArgs->isSequentialRender,
                                                                    frameArgs->isRenderResponseToUserInteraction,
                                                                    nodeHash,
                                                                    renderFullScaleThenDownscale,
                                                                    args.byPassCache,
                                                                    outputDepth,
                                                                    outputClipPrefComps,
                                                                    neededComps,
                                                                    processChannels);
                if (planesToRender->useOpenGL) {
                    // If the plug-in doesn't support concurrent OpenGL renders, release the lock that was taken in the call to attachOpenGLContext_public() above.
                    // For safe plug-ins, we call dettachOpenGLContext_public when the effect is destroyed in Node::deactivate() with the function EffectInstance::dettachAllOpenGLContexts().
                    // If we were the last render to use this context, clear the data now
                    if ( planesToRender->glContextData->getHasTakenLock() || !_publicInterface->supportsConcurrentOpenGLRenders() || planesToRender->glContextData.use_count() == 1) {
                        renderInstance->dettachOpenGLContext_public(glRenderContext, planesToRender->glContextData);
                    }
                }
            }
            if (useRenderClone) {
                _publicInterface->releaseRenderInstance(renderInstance);
            }
        } // if (hasSomethingToRender) {

        *renderAborted = _publicInterface->aborted();
#if NATRON_ENABLE_TRIMAP

        if ( frameArgs->isCurrentFrameRenderNotAbortable() ) {
            ///Only use trimap system if the render cannot be aborted.
            ///If we were aborted after all (because the node got deleted) then return a NULL image and empty the cache
            ///of this image
            for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
                if (!*renderAborted) {
                    if ( (renderRetCode == eRenderRoIStatusRenderFailed) || !planesToRender->isBeingRenderedElsewhere ) {
                        unmarkImageAsBeingRendered(renderFullScaleThenDownscale ? it->second.fullscaleImage : it->second.downscaleImage,
                                                         renderRetCode == eRenderRoIStatusRenderFailed);
                    } else {
                        if ( !waitForImageBeingRenderedElsewhereAndUnmark(roi, renderFullScaleThenDownscale ? it->second.fullscaleImage : it->second.downscaleImage) ) {
                            *renderAborted = true;
                        }
                    }
                } else {
                    appPTR->removeFromNodeCache(renderFullScaleThenDownscale ? it->second.fullscaleImage : it->second.downscaleImage);
                    unmarkImageAsBeingRendered(renderFullScaleThenDownscale ? it->second.fullscaleImage : it->second.downscaleImage, true);
                }
            }
        }
#endif
    } // if (!hasSomethingToRender && !planesToRender->isBeingRenderedElsewhere) {
    return renderRetCode;
} // EffectInstance::Implementation::::renderRoILaunchInternalRender


void
EffectInstance::Implementation::renderRoITermination(const RenderRoIArgs & args,
                                                     const ParallelRenderArgsPtr& frameArgs,
                                                     const AbortableRenderInfoPtr& abortInfo,
                                                     const ImagePlanesToRenderPtr &planesToRender,
                                                     const OSGLContextPtr& glGpuContext,
                                                     bool hasSomethingToRender,
                                                     const RectD& rod,
                                                     const RectI& roi,
                                                     const RectI& downscaledImageBounds,
                                                     const RectI& originalRoI,
                                                     double par,
                                                     bool renderFullScaleThenDownscale,
                                                     bool renderAborted,
                                                     EffectInstance::RenderRoIStatusEnum renderRetCode,
                                                     std::map<ImageComponents, ImagePtr>* outputPlanes,
                                                     OSGLContextAttacherPtr* glContextLocker)
{
#if DEBUG
    // Kindly check that everything we asked for is rendered!
    if (hasSomethingToRender && (renderRetCode != eRenderRoIStatusRenderFailed) && !renderAborted) {
        for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
            if (!frameArgs->tilesSupported) {
                //assert that bounds are consistent with the RoD if tiles are not supported
                const RectD & srcRodCanonical = renderFullScaleThenDownscale ? it->second.fullscaleImage->getRoD() : it->second.downscaleImage->getRoD();
                RectI srcBounds;
                srcRodCanonical.toPixelEnclosing(renderFullScaleThenDownscale ? it->second.fullscaleImage->getMipMapLevel() : it->second.downscaleImage->getMipMapLevel(), par, &srcBounds);
                RectI srcRealBounds = renderFullScaleThenDownscale ? it->second.fullscaleImage->getBounds() : it->second.downscaleImage->getBounds();
                assert(srcRealBounds.x1 == srcBounds.x1);
                assert(srcRealBounds.x2 == srcBounds.x2);
                assert(srcRealBounds.y1 == srcBounds.y1);
                assert(srcRealBounds.y2 == srcBounds.y2);
            }

            std::list<RectI> restToRender;
            if (renderFullScaleThenDownscale) {
                it->second.fullscaleImage->getRestToRender(roi, restToRender);
            } else {
                it->second.downscaleImage->getRestToRender(roi, restToRender);
            }
            /*
             We cannot assert that the bitmap is empty because another thread might have started rendering the same image again but
             needed a different portion of the image. The trimap system does not work for abortable renders
             */

            if ( frameArgs->isCurrentFrameRenderNotAbortable() ) {
                if ( !restToRender.empty() ) {
                    it->second.downscaleImage->printUnrenderedPixels(roi);
                }
                /*
                 If crashing on this assert this is likely due to a bug of the Trimap system.
                 Most likely another thread started rendering the portion that is in restToRender but did not fill the bitmap with 1
                 yet. Do not remove this assert, there should never be 2 threads running concurrently renderHandler for the same roi
                 on the same image.
                 */
                assert( restToRender.empty() );
            }
        }
    }
#endif

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////// Make sure all planes rendered have the requested  format ///////////////////////////

    bool useAlpha0ForRGBToRGBAConversion = args.caller ? args.caller->getNode()->usesAlpha0ToConvertFromRGBToRGBA() : false;

    // If the caller is not multiplanar, for the color plane we remap it to the components metadata obtained from the metadata pass, otherwise we stick to returning
    //bool callerIsMultiplanar = args.caller ? args.caller->isMultiPlanar() : false;

    //bool multiplanar = isMultiPlanar();
    for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
        //If we have worked on a local swaped image, swap it in the cache
        if (it->second.cacheSwapImage) {
            const CacheAPI* cache = it->second.cacheSwapImage->getCacheAPI();
            const ImageCache* imgCache = dynamic_cast<const ImageCache*>(cache);
            if (imgCache) {
                ImageCache* ccImgCache = const_cast<ImageCache*>(imgCache);
                assert(ccImgCache);
                ccImgCache->swapOrInsert(it->second.cacheSwapImage, it->second.fullscaleImage);
            }
        }

        //We have to return the downscale image, so make sure it has been computed
        if ( (renderRetCode != eRenderRoIStatusRenderFailed) &&
            renderFullScaleThenDownscale &&
            ( it->second.fullscaleImage->getMipMapLevel() != args.mipMapLevel) &&
            !hasSomethingToRender ) {
            assert(it->second.fullscaleImage->getMipMapLevel() == 0);
            if (it->second.downscaleImage == it->second.fullscaleImage) {
                it->second.downscaleImage.reset( new Image(it->second.fullscaleImage->getComponents(),
                                                           it->second.fullscaleImage->getRoD(),
                                                           downscaledImageBounds,
                                                           args.mipMapLevel,
                                                           it->second.fullscaleImage->getPixelAspectRatio(),
                                                           it->second.fullscaleImage->getBitDepth(),
                                                           it->second.fullscaleImage->getPremultiplication(),
                                                           it->second.fullscaleImage->getFieldingOrder(),
                                                           false) );
                it->second.downscaleImage->setKey(it->second.fullscaleImage->getKey());
            }

            it->second.fullscaleImage->downscaleMipMap( it->second.fullscaleImage->getRoD(), originalRoI, 0, args.mipMapLevel, false, it->second.downscaleImage.get() );
        }

        const ImageComponents* comp = 0;
        if ( !it->first.isColorPlane() ) {
            comp = &it->first;
        } else {
            // If we were requested the color plane, we rendered what the node's metadata is for the color plane. Map it to what was requested
            for (std::list<ImageComponents>::const_iterator it2 = args.components.begin(); it2 != args.components.end(); ++it2) {
                if ( it2->isColorPlane() ) {
                    comp = &(*it2);
                    break;
                }
            }
        }
        assert(comp);
        ///The image might need to be converted to fit the original requested format
        if (comp) {
            it->second.downscaleImage = convertPlanesFormatsIfNeeded(_publicInterface->getApp(), it->second.downscaleImage, originalRoI, *comp, args.bitdepth, useAlpha0ForRGBToRGBAConversion, planesToRender->outputPremult, -1);
            assert(it->second.downscaleImage->getStorageMode() == eStorageModeGLTex ||  (it->second.downscaleImage->getComponents() == *comp && it->second.downscaleImage->getBitDepth() == args.bitdepth));

            StorageModeEnum imageStorage = it->second.downscaleImage->getStorageMode();
            if ( args.returnStorage == eStorageModeGLTex && (imageStorage != eStorageModeGLTex) ) {
                if (!*glContextLocker) {
                    // Make the OpenGL context current to this thread since we may use it for convertRAMImageToOpenGLTexture
                    glContextLocker->reset( new OSGLContextAttacher(glGpuContext, abortInfo
#ifdef DEBUG
                                                                   , frameArgs->time
#endif
                                                                   ) );
                }
                (*glContextLocker)->attach();
                it->second.downscaleImage = convertRAMImageToOpenGLTexture(it->second.downscaleImage, glGpuContext);
            } else if ( args.returnStorage != eStorageModeGLTex && (imageStorage == eStorageModeGLTex) ) {
                assert(args.returnStorage == eStorageModeRAM);
                assert(glContextLocker);
                if (*glContextLocker) {
                    (*glContextLocker)->attach();
                }
                // Don't cache the converted RAM image if we are drawing
                it->second.downscaleImage = _publicInterface->convertOpenGLTextureToCachedRAMImage(it->second.downscaleImage, !frameArgs->isDuringPaintStrokeCreation);
            }

            outputPlanes->insert( std::make_pair(*comp, it->second.downscaleImage) );
        }

#ifdef DEBUG
        if (!frameArgs->isDuringPaintStrokeCreation) {
            RectI renderedImageBounds;
            rod.toPixelEnclosing(args.mipMapLevel, par, &renderedImageBounds);
            RectI expectedContainedRoI;
            if (args.roi.intersect(renderedImageBounds, &expectedContainedRoI)) {
                if ( !it->second.downscaleImage->getBounds().contains(expectedContainedRoI) ) {
                    qDebug() << "[WARNING]:" << _publicInterface->getScriptName_mt_safe().c_str() << "rendered an image with an RoI that fell outside its bounds.";
                }
            }
        }
#endif
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////// End requested format convertion ////////////////////////////////////////////////////////////////////


    // Termination, check that we rendered things correctly or we should have failed earlier otherwise.
#ifdef DEBUG
    if ( outputPlanes->size() != args.components.size() ) {
        qDebug() << "Requested:";
        for (std::list<ImageComponents>::const_iterator it = args.components.begin(); it != args.components.end(); ++it) {
            qDebug() << it->getLayerName().c_str();
        }
        qDebug() << "But rendered:";
        for (std::map<ImageComponents, ImagePtr>::iterator it = outputPlanes->begin(); it != outputPlanes->end(); ++it) {
            if (it->second) {
                qDebug() << it->first.getLayerName().c_str();
            }
        }
    }
#endif
    
    assert( !outputPlanes->empty() );

} // EffectInstance::Implementation::renderRoITermination

EffectInstance::RenderRoIRetCode
EffectInstance::renderRoI(const RenderRoIArgs & args,
                          std::map<ImageComponents, ImagePtr>* outputPlanes)
{


    // Make sure this call is not made recursively from getImage on a render clone on which we are already calling renderRoI.
    // If so, forward the call to the main instance
    if (_imp->mainInstance) {
        return _imp->mainInstance->renderRoI(args, outputPlanes);
    }

    // Setup args for the render
    const FrameViewRequest* requestPassData;
    EffectDataTLSPtr tls;
    OSGLContextPtr glGpuContext, glCpuContext;
    AbortableRenderInfoPtr abortInfo;
    ParallelRenderArgsPtr  frameArgs;
    U64 nodeHash;
    SupportsEnum supportsRS;

    // This flag is relevant only when the mipMapLevel is different than 0. We use it to determine
    // wether the plug-in should render in the full scale image, and then we downscale afterwards or
    // if the plug-in can just use the downscaled image to render.
    bool renderFullScaleThenDownscale;
    unsigned int renderMappedMipMapLevel;
    RenderScale renderMappedScale;
    RectD rod; //!< rod is in canonical coordinates
    bool isProjectFormat;
    RectI roi;
    ImagePremultiplicationEnum thisEffectOutputPremult;
    double par;
    ImageFieldingOrderEnum fieldingOrder;
    ComponentsNeededMapPtr neededComps;
    std::bitset<4> processChannels;
    const std::vector<ImageComponents>* outputComponents;
    if (!_imp->setupRenderRoIParams(args, &tls, &abortInfo, &frameArgs, &glGpuContext, &glCpuContext, &nodeHash, &par, &fieldingOrder, &thisEffectOutputPremult, &supportsRS, &renderFullScaleThenDownscale, &renderMappedMipMapLevel, &renderMappedScale, &requestPassData, &rod, &roi, &isProjectFormat, &neededComps, &processChannels, &outputComponents)) {
        return eRenderRoIRetCodeOk;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Handle pass-through for planes //////////////////////////////////////////////////////////
    std::list<ImageComponents> requestedComponents;
    {
        
        RenderRoIRetCode upstreamRetCode = _imp->determinePlanesToRender(args, &requestedComponents, outputPlanes);
        if (upstreamRetCode != eRenderRoIRetCodeOk) {
            return upstreamRetCode;
        }

        // There might be only planes to render that were fetched from upstream
        if ( requestedComponents.empty() ) {
            return eRenderRoIRetCodeOk;
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Check if effect is identity /////////////////////////////////////////////////////////////
    {
        bool isIdentity;
        RenderRoIRetCode upstreamRetCode = _imp->handleIdentityEffect(args, frameArgs, requestPassData, supportsRS, par, args.mipMapLevel, nodeHash, thisEffectOutputPremult, rod, requestedComponents, *outputComponents, neededComps, renderMappedScale, renderMappedMipMapLevel, renderFullScaleThenDownscale, outputPlanes, &isIdentity);
        if (isIdentity) {
            return upstreamRetCode;
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Transform concatenations ///////////////////////////////////////////////////////////////
    bool useTransforms;

    if (requestPassData) {
        tls->currentRenderArgs.transformRedirections.reset(new InputMatrixMap);
        tls->currentRenderArgs.transformRedirections = requestPassData->globalData.transforms;
        useTransforms = !tls->currentRenderArgs.transformRedirections->empty();
    } else {
        useTransforms = appPTR->getCurrentSettings()->isTransformConcatenationEnabled();
        if (useTransforms) {
            tls->currentRenderArgs.transformRedirections.reset(new InputMatrixMap);
            tryConcatenateTransforms( args.time, args.view, args.scale, tls->currentRenderArgs.transformRedirections.get() );
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Compute RoI depending on render scale ///////////////////////////////////////////////////


    RectI downscaledImageBounds;
    RectI upscaledImageBounds;
    RectD canonicalRoI;

    // Keep in memory what the user has requested, and change the roi to the full bounds if the effect doesn't support tiles
    RectI originalRoI;
    if (!resolveRoI(args.mipMapLevel, par, args.roi, rod, frameArgs->tilesSupported, renderFullScaleThenDownscale, &downscaledImageBounds, &upscaledImageBounds, &roi, &originalRoI, &canonicalRoI)) {
        return eRenderRoIRetCodeOk;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Resolve render device ///////////////////////////////////////////////////////////////////
    StorageModeEnum storage;
    OSGLContextPtr glRenderContext;
    ImagePlanesToRenderPtr planesToRender(new ImagePlanesToRender);
    OSGLContextAttacherPtr glContextLocker;
    {
        RenderRoIRetCode errorCode;
        if (!_imp->resolveRenderDevice(args, frameArgs, abortInfo, glGpuContext, glCpuContext, downscaledImageBounds, &renderMappedScale, &renderMappedMipMapLevel, &renderFullScaleThenDownscale, &roi, &storage, &glRenderContext, &glContextLocker, &planesToRender->useOpenGL, &errorCode)) {
            return errorCode;
        }
    }
    
    

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Look-up the cache //////////////////////////////////////////////////////////////////////

    ImagePtr isPlaneCached;
    bool createInCache;
    bool renderScaleOneUpstreamIfRenderScaleSupportDisabled;
    boost::scoped_ptr<ImageKey> key;

    {
        if (!_imp->renderRoILookupCacheFirstTime(args, frameArgs, storage, glRenderContext, glContextLocker, planesToRender, requestedComponents, *outputComponents, rod, roi, upscaledImageBounds, downscaledImageBounds, renderFullScaleThenDownscale, nodeHash, renderMappedMipMapLevel, &createInCache, &renderScaleOneUpstreamIfRenderScaleSupportDisabled, &isPlaneCached, &key, outputPlanes)) {
            return eRenderRoIRetCodeFailed;
        }
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Determine rectangles left to render /////////////////////////////////////////////////////
    bool cacheAlmostFull = appPTR->isNodeCacheAlmostFull();
    bool fillGrownBoundsWithZeroes, tryIdentityOptim, redoCacheLookup;
    RectI inputsRoDIntersectionPixel;
    RectI lastStrokePixelRoD;
    _imp->determineRectsToRender(isPlaneCached, frameArgs, args.time, args.view, args.scale, renderMappedScale, roi, upscaledImageBounds, downscaledImageBounds, renderFullScaleThenDownscale, frameArgs->isDuringPaintStrokeCreation, args.mipMapLevel, par, cacheAlmostFull, args.inputImagesList, storage, glContextLocker, planesToRender, &redoCacheLookup, &fillGrownBoundsWithZeroes, &tryIdentityOptim, &lastStrokePixelRoD, &inputsRoDIntersectionPixel);
    bool hasSomethingToRender = !planesToRender->rectsToRender.empty();

    ImageBitDepthEnum outputDepth = getBitDepth(-1);
    ImageComponents outputClipPrefComps = getComponents(-1);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Pre-render input images ////////////////////////////////////////////////////////////////
    boost::shared_ptr<FramesNeededMap> framesNeeded;
    {
        RenderRoIRetCode upstreamRetCode = _imp->renderRoIRenderInputImages(args, tls, neededComps, requestPassData, planesToRender, useTransforms, storage, *outputComponents, thisEffectOutputPremult, rod, par, nodeHash, renderFullScaleThenDownscale, renderMappedMipMapLevel, renderMappedScale, renderScaleOneUpstreamIfRenderScaleSupportDisabled, &framesNeeded);
        if (upstreamRetCode != eRenderRoIRetCodeOk) {
            return upstreamRetCode;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Redo - cache lookup if memory almost full //////////////////////////////////////////////
    if (redoCacheLookup) {
        RenderRoIRetCode upstreamRetCode = _imp->renderRoISecondCacheLookup(args, tls, frameArgs, neededComps, requestPassData, planesToRender, glContextLocker, useTransforms, storage, *outputComponents, rod, roi, upscaledImageBounds, downscaledImageBounds, inputsRoDIntersectionPixel, tryIdentityOptim, par, renderFullScaleThenDownscale, createInCache, renderMappedMipMapLevel, renderMappedScale, renderScaleOneUpstreamIfRenderScaleSupportDisabled, framesNeeded, key, &isPlaneCached);
        if (upstreamRetCode != eRenderRoIRetCodeOk) {
            return upstreamRetCode;
        }

    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Allocate planes in the cache ////////////////////////////////////////////////////////////

    if (hasSomethingToRender) {
        _imp->renderRoIAllocateOutputPlanes(args, frameArgs, planesToRender, glContextLocker, glRenderContext, fieldingOrder, outputDepth, isProjectFormat, fillGrownBoundsWithZeroes, storage, *outputComponents, rod, upscaledImageBounds, downscaledImageBounds, lastStrokePixelRoD, par, renderFullScaleThenDownscale, createInCache, key);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Launch actual internal render ///////////////////////////////////////////////////////////
    bool renderAborted;
    RenderRoIStatusEnum renderRetCode = _imp->renderRoILaunchInternalRender(args, frameArgs, planesToRender, neededComps, glRenderContext, outputDepth, outputClipPrefComps, hasSomethingToRender, storage, nodeHash, rod, roi, renderMappedMipMapLevel, processChannels, par, renderFullScaleThenDownscale, &renderAborted);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Check for failure or abortion ///////////////////////////////////////////////////////////
    if ( renderAborted && (renderRetCode != eRenderRoIStatusImageAlreadyRendered) ) {
        ///Return a NULL image

        if (frameArgs->isDuringPaintStrokeCreation) {
            //We know the image will never be used ever again
            getNode()->removeAllImagesFromCache(false);
        }

        return eRenderRoIRetCodeAborted;
    } else if (renderRetCode == eRenderRoIStatusRenderFailed) {
        // Throwing this exception will ensure the render stops.
        // This is slightly clumsy since we already have a render return code indicating it, we should
        // use the ret code instead.
        throw std::runtime_error("Rendering Failed");
    } else if (renderRetCode == eRenderRoIStatusRenderOutOfGPUMemory) {
        // Recall renderRoI on this node, but don't use GPU this time if possible
        if (frameArgs->currentOpenglSupport != ePluginOpenGLRenderSupportYes) {
            // The plug-in can only use GPU or doesn't support GPU
            throw std::runtime_error("Rendering Failed");
        }
        boost::scoped_ptr<RenderRoIArgs> newArgs( new RenderRoIArgs(args) );
        newArgs->allowGPURendering = false;

        return renderRoI(*newArgs, outputPlanes);
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Termination /////////////////////////////////////////////////////////////////////////////
    _imp->renderRoITermination(args, frameArgs, abortInfo, planesToRender, glGpuContext, hasSomethingToRender, rod, roi, downscaledImageBounds, originalRoI, par, renderFullScaleThenDownscale, renderAborted, renderRetCode, outputPlanes, &glContextLocker);

    return eRenderRoIRetCodeOk;
} // renderRoI

EffectInstance::RenderRoIStatusEnum
EffectInstance::renderRoIInternal(const EffectInstancePtr& self,
                                  const OSGLContextPtr& glContext,
                                  double time,
                                  const ParallelRenderArgsPtr & frameArgs,
                                  RenderSafetyEnum safety,
                                  unsigned int mipMapLevel,
                                  ViewIdx view,
                                  const RectD & rod, //!< effect rod in canonical coords
                                  const double par,
                                  const ImagePlanesToRenderPtr & planesToRender,
                                  bool isSequentialRender,
                                  bool isRenderMadeInResponseToUserInteraction,
                                  U64 nodeHash,
                                  bool renderFullScaleThenDownscale,
                                  bool byPassCache,
                                  ImageBitDepthEnum outputClipPrefDepth,
                                  const ImageComponents& outputClipPrefsComps,
                                  const ComponentsNeededMapPtr & compsNeeded,
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
        RectI pixelRoD;
        rod.toPixelEnclosing(0, par, &pixelRoD);
        frmt.set(pixelRoD);
        frmt.setPixelAspectRatio(par);
        self->getApp()->getProject()->setOrAddProjectFormat(frmt);
    }

    unsigned int renderMappedMipMapLevel = 0;

    for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
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

    ///depending on the thread-safety of the plug-in we render with a different
    ///amount of threads.
    ///If the project lock is already locked at this point, don't start any other thread
    ///as it would lead to a deadlock when the project is loading.
    ///Just fall back to Fully_safe
    int nbThreads = appPTR->getCurrentSettings()->getNumberOfThreads();
    if (safety == eRenderSafetyFullySafeFrame) {
        ///If the plug-in is eRenderSafetyFullySafeFrame that means it wants the host to perform SMP aka slice up the RoI into chunks
        ///but if the effect doesn't support tiles it won't work.
        ///Also check that the number of threads indicating by the settings are appropriate for this render mode.
        if ( !frameArgs->tilesSupported || (nbThreads == -1) || (nbThreads == 1) ||
             ( (nbThreads == 0) && (appPTR->getHardwareIdealThreadCount() == 1) ) ||
             ( QThreadPool::globalInstance()->activeThreadCount() >= QThreadPool::globalInstance()->maxThreadCount() ) ||
             self->isRotoPaintNode() ) {
            safety = eRenderSafetyFullySafe;
        }
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
    self->getFrameRange_public(nodeHash, &firstFrame, &lastFrame);


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
            tiledArgs->firstFrame = firstFrame;
            tiledArgs->lastFrame = lastFrame;
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


            QFuture<RenderingFunctorRetEnum> ret = QtConcurrent::mapped( planesToRender->rectsToRender,
                                                                         boost::bind(&EffectInstance::Implementation::tiledRenderingFunctor,
                                                                                     self->_imp.get(),
                                                                                     *tiledArgs,
                                                                                     _1,
                                                                                     currentThread) );
            ret.waitForFinished();
            QFuture<EffectInstance::RenderingFunctorRetEnum>::const_iterator it2;

#endif
            for (it2 = ret.begin(); it2 != ret.end(); ++it2) {
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
                RenderingFunctorRetEnum functorRet = self->_imp->tiledRenderingFunctor(*it, glContext, renderFullScaleThenDownscale, isSequentialRender, isRenderMadeInResponseToUserInteraction, firstFrame, lastFrame, preferredInput, mipMapLevel, renderMappedMipMapLevel, rod, time, view, par, byPassCache, outputClipPrefDepth, outputClipPrefsComps, compsNeeded, processChannels, planesToRender);

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
