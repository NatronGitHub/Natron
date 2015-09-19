/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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
#include <stdexcept>
#include <fstream>

#include <QtConcurrentMap>
#include <QReadWriteLock>
#include <QCoreApplication>
#include <QtConcurrentRun>
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
#include "Engine/PluginMemory.h"
#include "Engine/Project.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/Settings.h"
#include "Engine/ThreadStorage.h"
#include "Engine/Timer.h"
#include "Engine/Transform.h"
#include "Engine/ViewerInstance.h"

//#define NATRON_ALWAYS_ALLOCATE_FULL_IMAGE_BOUNDS


using namespace Natron;


class KnobFile;
class KnobOutputFile;

namespace {

class TransformReroute_RAII
{
    EffectInstance* self;
    InputMatrixMap transforms;

public:

    TransformReroute_RAII(EffectInstance* self,
                          const InputMatrixMap & inputTransforms)
    : self(self)
    , transforms(inputTransforms)
    {
        self->rerouteInputAndSetTransform(inputTransforms);
    }

    ~TransformReroute_RAII()
    {
        for (InputMatrixMap::iterator it = transforms.begin(); it != transforms.end(); ++it) {
            self->clearTransform(it->first);
        }
    }
};

} // anon. namespace

/*
 * @brief Split all rects to render in smaller rects and check if each one of them is identity.
 * For identity rectangles, we just call renderRoI again on the identity input in the tiledRenderingFunctor.
 * For non-identity rectangles, compute the bounding box of them and render it
 */
static void
optimizeRectsToRender(Natron::EffectInstance* self,
                      const RectI & inputsRoDIntersection,
                      const std::list<RectI> & rectsToRender,
                      const double time,
                      const int view,
                      const RenderScale & renderMappedScale,
                      std::list<EffectInstance::RectToRender>* finalRectsToRender)
{
    for (std::list<RectI>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
        std::vector<RectI> splits = it->splitIntoSmallerRects(0);
        EffectInstance::RectToRender nonIdentityRect;
        nonIdentityRect.isIdentity = false;
        nonIdentityRect.identityInput = 0;
        nonIdentityRect.rect.x1 = INT_MAX;
        nonIdentityRect.rect.x2 = INT_MIN;
        nonIdentityRect.rect.y1 = INT_MAX;
        nonIdentityRect.rect.y2 = INT_MIN;

        bool nonIdentityRectSet = false;
        for (std::size_t i = 0; i < splits.size(); ++i) {
            double identityInputTime;
            int identityInputNb;
            bool identity;

            if ( !splits[i].intersects(inputsRoDIntersection) ) {
                identity = self->isIdentity_public(false, 0, time, renderMappedScale, splits[i], view, &identityInputTime, &identityInputNb);
            } else {
                identity = false;
            }

            if (identity) {
                EffectInstance::RectToRender r;
                r.isIdentity = true;

                //Walk along the identity branch until we find the non identity input, or NULL in we case we will
                //just render black and transparant
                Natron::EffectInstance* identityInput = self->getInput(identityInputNb);
                if (identityInput) {
                    for (;; ) {
                        identity = identityInput->isIdentity_public(false, 0, time, renderMappedScale, splits[i], view, &identityInputTime, &identityInputNb);
                        if ( !identity || (identityInputNb == -2) ) {
                            break;
                        }
                        Natron::EffectInstance* subIdentityInput = identityInput->getInput(identityInputNb);
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

EffectInstance::RenderRoIRetCode
EffectInstance::renderRoI(const RenderRoIArgs & args,
                          ImageList* outputPlanes)
{
    //Do nothing if no components were requested
    if ( args.components.empty() || args.roi.isNull() ) {
        qDebug() << getScriptName_mt_safe().c_str() << "renderRoi: Early bail-out components requested empty or RoI is NULL";

        return eRenderRoIRetCodeOk;
    }

    ParallelRenderArgs & frameRenderArgs = _imp->frameRenderArgs.localData();
    if (!frameRenderArgs.validArgs) {
        qDebug() << "Thread-storage for the render of the frame was not set, this is a bug.";
        frameRenderArgs.time = args.time;
        frameRenderArgs.nodeHash = getHash();
        frameRenderArgs.view = args.view;
        frameRenderArgs.isSequentialRender = false;
        frameRenderArgs.isRenderResponseToUserInteraction = true;
        frameRenderArgs.validArgs = true;
    } else {
        //The hash must not have changed if we did a pre-pass.
        assert(!frameRenderArgs.request || frameRenderArgs.nodeHash == frameRenderArgs.request->nodeHash);
    }

    ///The args must have been set calling setParallelRenderArgs
    assert(frameRenderArgs.validArgs);

    ///For writer we never want to cache otherwise the next time we want to render it will skip writing the image on disk!
    bool byPassCache = args.byPassCache;

    ///Use the hash at this time, and then copy it to the clips in the thread local storage to use the same value
    ///through all the rendering of this frame.
    U64 nodeHash = frameRenderArgs.nodeHash;
    const double par = getPreferredAspectRatio();
    RectD rod; //!< rod is in canonical coordinates
    bool isProjectFormat = false;
    const unsigned int mipMapLevel = args.mipMapLevel;
    SupportsEnum supportsRS = supportsRenderScaleMaybe();
    ///This flag is relevant only when the mipMapLevel is different than 0. We use it to determine
    ///wether the plug-in should render in the full scale image, and then we downscale afterwards or
    ///if the plug-in can just use the downscaled image to render.
    bool renderFullScaleThenDownscale = (supportsRS == eSupportsNo && mipMapLevel != 0);
    unsigned int renderMappedMipMapLevel;
    if (renderFullScaleThenDownscale) {
        renderMappedMipMapLevel = 0;
    } else {
        renderMappedMipMapLevel = args.mipMapLevel;
    }
    RenderScale renderMappedScale;
    renderMappedScale.x = renderMappedScale.y = Image::getScaleFromMipMapLevel(renderMappedMipMapLevel);
    assert( !( (supportsRS == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );

    ///Do we want to render the graph upstream at scale 1 or at the requested render scale ? (user setting)
    bool renderScaleOneUpstreamIfRenderScaleSupportDisabled = false;
    if (renderFullScaleThenDownscale) {
        renderScaleOneUpstreamIfRenderScaleSupportDisabled = getNode()->useScaleOneImagesWhenRenderScaleSupportIsDisabled();

        ///For multi-resolution we want input images with exactly the same size as the output image
        if ( !renderScaleOneUpstreamIfRenderScaleSupportDisabled && !supportsMultiResolution() ) {
            renderScaleOneUpstreamIfRenderScaleSupportDisabled = true;
        }
    }

    ViewInvarianceLevel viewInvariance = isViewInvariant();
    const FrameViewRequest* requestPassData = 0;
    if (frameRenderArgs.request) {
        requestPassData = frameRenderArgs.request->getFrameViewRequest(args.time, args.view);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Get the RoD ///////////////////////////////////////////////////////////////

    ///if the rod is already passed as parameter, just use it and don't call getRegionOfDefinition
    if ( !args.preComputedRoD.isNull() ) {
        rod = args.preComputedRoD;
    } else {
        ///Check if the pre-pass already has the RoD
        if (requestPassData) {
            rod = requestPassData->globalData.rod;
            isProjectFormat = requestPassData->globalData.isProjectFormat;
        } else {
            assert( !( (supportsRS == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );
            StatusEnum stat = getRegionOfDefinition_public(nodeHash, args.time, renderMappedScale, args.view, &rod, &isProjectFormat);

            ///The rod might be NULL for a roto that has no beziers and no input
            if (stat == eStatusFailed) {
                ///if getRoD fails, this might be because the RoD is null after all (e.g: an empty Roto node), we don't want the render to fail
                return eRenderRoIRetCodeOk;
            } else if ( rod.isNull() ) {
                //Nothing to render
                return eRenderRoIRetCodeOk;
            }
            if ( (supportsRS == eSupportsMaybe) && (renderMappedMipMapLevel != 0) ) {
                // supportsRenderScaleMaybe may have changed, update it
                supportsRS = supportsRenderScaleMaybe();
                renderFullScaleThenDownscale = (supportsRS == eSupportsNo && mipMapLevel != 0);
                if (renderFullScaleThenDownscale) {
                    renderMappedScale.x = renderMappedScale.y = 1.;
                    renderMappedMipMapLevel = 0;
                }
            }
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End get RoD ///////////////////////////////////////////////////////////////


    /*We pass the 2 images (image & downscaledImage). Depending on the context we want to render in one or the other one:
       If (renderFullScaleThenDownscale and renderScaleOneUpstreamIfRenderScaleSupportDisabled)
       the image that is held by the cache will be 'image' and it will then be downscaled if needed.
       However if the render scale is not supported but input images are not rendered at full-scale  ,
       we don't want to cache the full-scale image because it will be low res. Instead in that case we cache the downscaled image
     */
    bool useImageAsOutput;
    RectI roi,upscaledRoi;

    if (renderFullScaleThenDownscale && renderScaleOneUpstreamIfRenderScaleSupportDisabled) {
        //We cache 'image', hence the RoI should be expressed in its coordinates
        //renderRoIInternal should check the bitmap of 'image' and not downscaledImage!
        RectD canonicalRoI;
        args.roi.toCanonical(args.mipMapLevel, par, rod, &canonicalRoI);
        canonicalRoI.toPixelEnclosing(0, par, &roi);
        upscaledRoi = roi;
        useImageAsOutput = true;
    } else {
        //In that case the plug-in either supports render scale or doesn't support render scale but uses downscaled inputs
        //renderRoIInternal should check the bitmap of downscaledImage and not 'image'!
        roi = args.roi;
        if (!renderFullScaleThenDownscale) {
            upscaledRoi = roi;
        } else {
            RectD canonicalRoI;
            args.roi.toCanonical(args.mipMapLevel, par, rod, &canonicalRoI);
            canonicalRoI.toPixelEnclosing(0, par, &upscaledRoi);
        }
        useImageAsOutput = false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Check if effect is identity ///////////////////////////////////////////////////////////////
    {
        double inputTimeIdentity = 0.;
        int inputNbIdentity;

        assert( !( (supportsRS == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );
        bool identity;
        RectI pixelRod;
        rod.toPixelEnclosing(args.mipMapLevel, par, &pixelRod);

        if ( (args.view != 0) && (viewInvariance == eViewInvarianceAllViewsInvariant) ) {
            identity = true;
            inputNbIdentity = -2;
            inputTimeIdentity = args.time;
        } else {
            try {
                if (requestPassData) {
                    inputTimeIdentity = requestPassData->globalData.inputIdentityTime;
                    inputNbIdentity = requestPassData->globalData.identityInputNb;
                    identity = inputNbIdentity != -1;
                } else {
                    identity = isIdentity_public(true, nodeHash, args.time, renderMappedScale, pixelRod, args.view, &inputTimeIdentity, &inputNbIdentity);
                }
            } catch (...) {
                return eRenderRoIRetCodeFailed;
            }
        }

        if ( (supportsRS == eSupportsMaybe) && (renderMappedMipMapLevel != 0) ) {
            // supportsRenderScaleMaybe may have changed, update it
            supportsRS = supportsRenderScaleMaybe();
            renderFullScaleThenDownscale = true;
            renderMappedScale.x = renderMappedScale.y = 1.;
            renderMappedMipMapLevel = 0;
        }

        if (identity) {
            ///The effect is an identity but it has no inputs
            if (inputNbIdentity == -1) {
                return eRenderRoIRetCodeOk;
            } else if (inputNbIdentity == -2) {
                // there was at least one crash if you set the first frame to a negative value
                assert(inputTimeIdentity != args.time || viewInvariance == eViewInvarianceAllViewsInvariant);

                // be safe in release mode otherwise we hit an infinite recursion
                if ( (inputTimeIdentity != args.time) || (viewInvariance == eViewInvarianceAllViewsInvariant) ) {
                    ///This special value of -2 indicates that the plugin is identity of itself at another time
                    RenderRoIArgs argCpy = args;
                    argCpy.time = inputTimeIdentity;

                    if (viewInvariance == eViewInvarianceAllViewsInvariant) {
                        argCpy.view = 0;
                    }

                    argCpy.preComputedRoD.clear(); //< clear as the RoD of the identity input might not be the same (reproducible with Blur)

                    return renderRoI(argCpy, outputPlanes);
                }
            }

            double firstFrame, lastFrame;
            getFrameRange_public(nodeHash, &firstFrame, &lastFrame);

            RectD canonicalRoI;
            ///WRONG! We can't clip against the RoD of *this* effect. We should clip against the RoD of the input effect, but this is done
            ///later on for us already.
            //args.roi.toCanonical(args.mipMapLevel, rod, &canonicalRoI);
            args.roi.toCanonical_noClipping(args.mipMapLevel, par,  &canonicalRoI);

            Natron::EffectInstance* inputEffectIdentity = getInput(inputNbIdentity);
            if (inputEffectIdentity) {
                if ( frameRenderArgs.stats && frameRenderArgs.stats->isInDepthProfilingEnabled() ) {
                    frameRenderArgs.stats->setNodeIdentity( getNode(), inputEffectIdentity->getNode() );
                }


                RenderRoIArgs inputArgs = args;
                inputArgs.time = inputTimeIdentity;

                // Make sure we do not hold the RoD for this effect
                inputArgs.preComputedRoD.clear();


                return inputEffectIdentity->renderRoI(inputArgs, outputPlanes);
            } else {
                assert( outputPlanes->empty() );
            }

            return eRenderRoIRetCodeOk;
        } // if (identity)
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End identity check ///////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Handle pass-through for planes //////////////////////////////////////////////////////////
    ComponentsAvailableMap componentsAvailables;

    //Available planes/components is view agnostic
    getComponentsAvailable(args.time, &componentsAvailables);

    ComponentsNeededMap neededComps;
    bool processAllComponentsRequested;
    bool processChannels[4];
    ComponentsNeededMap::iterator foundOutputNeededComps;

    {
        SequenceTime ptTime;
        int ptView;
        boost::shared_ptr<Natron::Node> ptInput;
        getComponentsNeededAndProduced_public(args.time, args.view, &neededComps, &processAllComponentsRequested, &ptTime, &ptView, processChannels, &ptInput);

        foundOutputNeededComps = neededComps.find(-1);
        if ( foundOutputNeededComps == neededComps.end() ) {
            return eRenderRoIRetCodeOk;
        }

        if (processAllComponentsRequested) {
            std::vector<ImageComponents> compVec;
            for (std::list<Natron::ImageComponents>::const_iterator it = args.components.begin(); it != args.components.end(); ++it) {
                bool found = false;
                for (std::vector<Natron::ImageComponents>::const_iterator it2 = foundOutputNeededComps->second.begin(); it2 != foundOutputNeededComps->second.end(); ++it2) {
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
            for (ComponentsNeededMap::iterator it = neededComps.begin(); it != neededComps.end(); ++it) {
                it->second = compVec;
            }
            //neededComps[-1] = compVec;
        }
    }
    const std::vector<Natron::ImageComponents> & outputComponents = foundOutputNeededComps->second;

    /*
     * For all requested planes, check which components can be produced in output by this node.
     * If the components are from the color plane, if another set of components of the color plane is present
     * we try to render with those instead.
     */
    std::list<Natron::ImageComponents> requestedComponents;
    ComponentsAvailableMap componentsToFetchUpstream;
    for (std::list<Natron::ImageComponents>::const_iterator it = args.components.begin(); it != args.components.end(); ++it) {
        assert(it->getNumComponents() > 0);

        bool isColorComponents = it->isColorPlane();
        ComponentsAvailableMap::iterator found = componentsAvailables.end();
        for (ComponentsAvailableMap::iterator it2 = componentsAvailables.begin(); it2 != componentsAvailables.end(); ++it2) {
            if (it2->first == *it) {
                found = it2;
                break;
            } else {
                if ( isColorComponents && it2->first.isColorPlane() && isSupportedComponent(-1, it2->first) ) {
                    //We found another set of components in the color plane, take it
                    found = it2;
                    break;
                }
            }
        }

        // If  the requested component is not present, then it will just return black and transparant to the plug-in.
        if ( found != componentsAvailables.end() ) {
            if ( found->second.lock() == getNode() ) {
                requestedComponents.push_back(*it);
            } else {
                //The component is not available directly from this node, fetch it upstream
                componentsToFetchUpstream.insert( std::make_pair( *it, found->second.lock() ) );
            }
        }
    }

    //Render planes that we are not able to render on this node from upstream
    for (ComponentsAvailableMap::iterator it = componentsToFetchUpstream.begin(); it != componentsToFetchUpstream.end(); ++it) {
        NodePtr node = it->second.lock();
        if (node) {
            RenderRoIArgs inArgs = args;
            inArgs.components.clear();
            inArgs.components.push_back(it->first);
            ImageList inputPlanes;
            RenderRoIRetCode inputRetCode = node->getLiveInstance()->renderRoI(inArgs, &inputPlanes);
            assert( inputPlanes.size() == 1 || inputPlanes.empty() );
            if ( (inputRetCode == eRenderRoIRetCodeAborted) || (inputRetCode == eRenderRoIRetCodeFailed) || inputPlanes.empty() ) {
                return inputRetCode;
            }
            outputPlanes->push_back( inputPlanes.front() );
        }
    }

    ///There might be only planes to render that were fetched from upstream
    if ( requestedComponents.empty() ) {
        return eRenderRoIRetCodeOk;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End pass-through for planes //////////////////////////////////////////////////////////

    // At this point, if only the pass through planes are view variant and the rendered view is different than 0,
    // just call renderRoI again for the components left to render on the view 0.
    if ( (args.view != 0) && (viewInvariance == eViewInvarianceOnlyPassThroughPlanesVariant) ) {
        RenderRoIArgs argCpy = args;
        argCpy.view = 0;
        argCpy.preComputedRoD.clear();

        return renderRoI(argCpy, outputPlanes);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Transform concatenations ///////////////////////////////////////////////////////////////
    ///Try to concatenate transform effects
    InputMatrixMap inputsToTransform;
    bool useTransforms;
    if (requestPassData) {
        inputsToTransform = requestPassData->globalData.transforms;
        useTransforms = !inputsToTransform.empty();
    } else {
        useTransforms = appPTR->getCurrentSettings()->isTransformConcatenationEnabled();
        if (useTransforms) {
            tryConcatenateTransforms(args.time, args.view, args.scale, &inputsToTransform);
        }
    }

    ///Ok now we have the concatenation of all matrices, set it on the associated clip and reroute the tree
    boost::shared_ptr<TransformReroute_RAII> transformConcatenationReroute;
    if ( !inputsToTransform.empty() ) {
        transformConcatenationReroute.reset( new TransformReroute_RAII(this, inputsToTransform) );
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////End transform concatenations//////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Compute RoI depending on render scale ///////////////////////////////////////////////////


    RectI downscaledImageBoundsNc, upscaledImageBoundsNc;
    rod.toPixelEnclosing(args.mipMapLevel, par, &downscaledImageBoundsNc);
    rod.toPixelEnclosing(0, par, &upscaledImageBoundsNc);


    ///Make sure the RoI falls within the image bounds
    ///Intersection will be in pixel coordinates
    if (frameRenderArgs.tilesSupported) {
        if (useImageAsOutput) {
            if ( !roi.intersect(upscaledImageBoundsNc, &roi) ) {
                return eRenderRoIRetCodeOk;
            }
            assert(roi.x1 >= upscaledImageBoundsNc.x1 && roi.y1 >= upscaledImageBoundsNc.y1 &&
                   roi.x2 <= upscaledImageBoundsNc.x2 && roi.y2 <= upscaledImageBoundsNc.y2);
        } else {
            if ( !roi.intersect(downscaledImageBoundsNc, &roi) ) {
                return eRenderRoIRetCodeOk;
            }
            assert(roi.x1 >= downscaledImageBoundsNc.x1 && roi.y1 >= downscaledImageBoundsNc.y1 &&
                   roi.x2 <= downscaledImageBoundsNc.x2 && roi.y2 <= downscaledImageBoundsNc.y2);
        }
#ifndef NATRON_ALWAYS_ALLOCATE_FULL_IMAGE_BOUNDS
        ///just allocate the roi
        upscaledImageBoundsNc.intersect(upscaledRoi, &upscaledImageBoundsNc);
        downscaledImageBoundsNc.intersect(args.roi, &downscaledImageBoundsNc);
#endif
    } else {
        roi = useImageAsOutput ? upscaledImageBoundsNc : downscaledImageBoundsNc;
    }

    const RectI & downscaledImageBounds = downscaledImageBoundsNc;
    const RectI & upscaledImageBounds = upscaledImageBoundsNc;
    RectD canonicalRoI;
    if (useImageAsOutput) {
        roi.toCanonical(0, par, rod, &canonicalRoI);
    } else {
        roi.toCanonical(args.mipMapLevel, par, rod, &canonicalRoI);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End Compute RoI /////////////////////////////////////////////////////////////////////////


    bool isFrameVaryingOrAnimated = isFrameVaryingOrAnimated_Recursive();
    bool createInCache = shouldCacheOutput(isFrameVaryingOrAnimated);
    Natron::ImageKey key = Natron::Image::makeKey(getNode().get(),nodeHash, isFrameVaryingOrAnimated, args.time, args.view, frameRenderArgs.draftMode);
    bool useDiskCacheNode = dynamic_cast<DiskCacheNode*>(this) != NULL;


    /*
     * Get the bitdepth and output components that the plug-in expects to render. The cached image does not necesserarily has the bitdepth
     * that the plug-in expects.
     */
    Natron::ImageBitDepthEnum outputDepth;
    std::list<Natron::ImageComponents> outputClipPrefComps;
    getPreferredDepthAndComponents(-1, &outputClipPrefComps, &outputDepth);
    assert( !outputClipPrefComps.empty() );


    ImagePlanesToRender planesToRender;
    FramesNeededMap framesNeeded;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Look-up the cache ///////////////////////////////////////////////////////////////

    {
        //If one plane is missing from cache, we will have to render it all. For all other planes, either they have nothing
        //left to render, otherwise we render them for all the roi again.
        bool missingPlane = false;

        for (std::list<ImageComponents>::iterator it = requestedComponents.begin(); it != requestedComponents.end(); ++it) {
            PlaneToRender plane;

            /*
             * If the plane is the color plane, we might have to convert between components, hence we always
             * try to find in the cache the "preferred" components of this node for the color plane.
             * For all other planes, just consider this set of components, we do not allow conversion.
             */
            const ImageComponents* components = 0;
            if ( !it->isColorPlane() ) {
                components = &(*it);
            } else {
                for (std::vector<Natron::ImageComponents>::const_iterator it2 = outputComponents.begin(); it2 != outputComponents.end(); ++it2) {
                    if ( it2->isColorPlane() ) {
                        components = &(*it2);
                        break;
                    }
                }
            }
            assert(components);
            getImageFromCacheAndConvertIfNeeded(createInCache, useDiskCacheNode, key, renderMappedMipMapLevel,
                                                useImageAsOutput ? &upscaledImageBounds : &downscaledImageBounds,
                                                &rod,
                                                args.bitdepth, *it,
                                                outputDepth,
                                                *components,
                                                args.inputImagesList,
                                                frameRenderArgs.stats,
                                                &plane.fullscaleImage);


            if (byPassCache) {
                if (plane.fullscaleImage) {
                    appPTR->removeFromNodeCache( key.getHash() );
                    plane.fullscaleImage.reset();
                }
                //For writers, we always want to call the render action, but we still want to use the cache for nodes upstream
                if ( isWriter() ) {
                    byPassCache = false;
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
                        outputPlanes->push_back(plane.fullscaleImage);
                        continue;
                    }
                } else {
                    //Overwrite the RoD with the RoD contained in the image.
                    //This is to deal with the situation with an image rendered at scale 1 in the cache, but a new render asking for the same
                    //image at scale 0.5. The RoD will then be slightly larger at scale 0.5 thus re-rendering a few pixels. If the effect
                    //wouldn't support tiles, then it'b problematic as it would need to render the whole frame again just for a few pixels.
//                    if (!tilesSupported) {
//                        rod = plane.fullscaleImage->getRoD();
//                    }
                    framesNeeded = plane.fullscaleImage->getParams()->getFramesNeeded();
                }
            } else {
                if (!missingPlane) {
                    missingPlane = true;
                    //Ensure that previous planes are either already rendered or otherwise render them  again
                    std::map<ImageComponents, PlaneToRender> newPlanes;
                    for (std::map<ImageComponents, PlaneToRender>::iterator it2 = planesToRender.planes.begin();
                         it2 != planesToRender.planes.end(); ++it2) {
                        if (it2->second.fullscaleImage) {
                            std::list<RectI> restToRender;
                            it2->second.fullscaleImage->getRestToRender(roi, restToRender);
                            if ( !restToRender.empty() ) {
                                appPTR->removeFromNodeCache(it2->second.fullscaleImage);
                                it2->second.fullscaleImage.reset();
                                it2->second.downscaleImage.reset();
                                newPlanes.insert(*it2);
                            } else {
                                outputPlanes->push_back(it2->second.fullscaleImage);
                            }
                        } else {
                            newPlanes.insert(*it2);
                        }
                    }
                    planesToRender.planes = newPlanes;
                }
            }

            plane.downscaleImage = plane.fullscaleImage;
            plane.isAllocatedOnTheFly = false;
            planesToRender.planes.insert( std::make_pair(*it, plane) );
        }
    }

    assert( !planesToRender.planes.empty() );

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////End cache lookup//////////////////////////////////////////////////////////

    if ( framesNeeded.empty() ) {
        if (requestPassData) {
            framesNeeded = requestPassData->globalData.frameViewsNeeded;
        } else {
            framesNeeded = getFramesNeeded_public(nodeHash, args.time, args.view, renderMappedMipMapLevel);
        }
    }


    ///In the event where we had the image from the cache, but it wasn't completly rendered over the RoI but the cache was almost full,
    ///we don't hold a pointer to it, allowing the cache to free it.
    ///Hence after rendering all the input images, we redo a cache look-up to check whether the image is still here
    bool redoCacheLookup = false;
    bool cacheAlmostFull = appPTR->isNodeCacheAlmostFull();
    ImagePtr isPlaneCached;

    if ( !planesToRender.planes.empty() ) {
        isPlaneCached = planesToRender.planes.begin()->second.fullscaleImage;
    }

    if ( !isPlaneCached && args.roi.isNull() ) {
        ///Empty RoI and nothing in the cache with matching args, return empty planes.
        return eRenderRoIRetCodeFailed;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Determine rectangles left to render /////////////////////////////////////////////////////

    std::list<RectI> rectsLeftToRender;
    bool isDuringPaintStroke = isDuringPaintStrokeCreationThreadLocal();
    bool fillGrownBoundsWithZeroes = false;
    RectI lastStrokePixelRoD;
    if ( isDuringPaintStroke && args.inputImagesList.empty() ) {
        RectD lastStrokeRoD;
        NodePtr node = getNode();
        if ( !node->isLastPaintStrokeBitmapCleared() ) {
            node->getLastPaintStrokeRoD(&lastStrokeRoD);
            node->clearLastPaintStrokeRoD();
            // qDebug() << getScriptName_mt_safe().c_str() << "last stroke RoD: " << lastStrokeRoD.x1 << lastStrokeRoD.y1 << lastStrokeRoD.x2 << lastStrokeRoD.y2;
            lastStrokeRoD.toPixelEnclosing(mipMapLevel, par, &lastStrokePixelRoD);
        }
    }

    if (isPlaneCached) {
        if ( isDuringPaintStroke && !lastStrokePixelRoD.isNull() ) {
            fillGrownBoundsWithZeroes = true;
            //Clear the bitmap of the cached image in the portion of the last stroke to only recompute what's needed
            for (std::map<ImageComponents, PlaneToRender>::iterator it2 = planesToRender.planes.begin();
                 it2 != planesToRender.planes.end(); ++it2) {
                it2->second.fullscaleImage->clearBitmap(lastStrokePixelRoD);

                /*
                 * This is useful to optimize the bitmap checking
                 * when we are sure multiple threads are not using the image and we have a very small RoI to render.
                 * For now it's only used for the rotopaint while painting.
                 */
                it2->second.fullscaleImage->setBitmapDirtyZone(lastStrokePixelRoD);
            }
        }

        ///We check what is left to render.
#if NATRON_ENABLE_TRIMAP
        if (!frameRenderArgs.canAbort && frameRenderArgs.isRenderResponseToUserInteraction) {
#ifndef DEBUG
            isPlaneCached->getRestToRender_trimap(roi, rectsLeftToRender, &planesToRender.isBeingRenderedElsewhere);
#else
            // in debug mode, check that the result of getRestToRender_trimap and getRestToRender is the same if the image
            // is not currently rendered concurrently
            EffectInstance::Implementation::IBRPtr ibr;
            {
                QMutexLocker k(&_imp->imagesBeingRenderedMutex);
                EffectInstance::Implementation::IBRMap::const_iterator found = _imp->imagesBeingRendered.find(isPlaneCached);
                if ( ( found != _imp->imagesBeingRendered.end() ) && found->second->refCount ) {
                    ibr = found->second;
                }
            }
            if (!ibr) {
                Image::ReadAccess racc( isPlaneCached.get() );
                isPlaneCached->getRestToRender_trimap(roi, rectsLeftToRender, &planesToRender.isBeingRenderedElsewhere);
                std::list<RectI> tmpRects;
                isPlaneCached->getRestToRender(roi, tmpRects);

                //If it crashes here that means the image is no longer being rendered but its bitmap still contains PIXEL_UNAVAILABLE pixels.
                //The other thread should have removed that image from the cache or marked the image as rendered.
                assert(!planesToRender.isBeingRenderedElsewhere);
                assert( rectsLeftToRender.size() == tmpRects.size() );

                std::list<RectI>::iterator oIt = rectsLeftToRender.begin();
                for (std::list<RectI>::iterator it = tmpRects.begin(); it != tmpRects.end(); ++it, ++oIt) {
                    assert(*it == *oIt);
                }
            } else {
                isPlaneCached->getRestToRender_trimap(roi, rectsLeftToRender, &planesToRender.isBeingRenderedElsewhere);
            }
#endif
        } else {
            isPlaneCached->getRestToRender(roi, rectsLeftToRender);
        }
#else
        isPlaneCached->getRestToRender(roi, rectsLeftToRender);
#endif
        if ( isDuringPaintStroke && !rectsLeftToRender.empty() && !lastStrokePixelRoD.isNull() ) {
            rectsLeftToRender.clear();
            RectI intersection;
            if ( downscaledImageBounds.intersect(lastStrokePixelRoD, &intersection) ) {
                rectsLeftToRender.push_back(intersection);
            }
        }

        if (!rectsLeftToRender.empty() && cacheAlmostFull) {
            ///The node cache is almost full and we need to render  something in the image, if we hold a pointer to this image here
            ///we might recursively end-up in this same situation at each level of the render tree, ending with all images of each level
            ///being held in memory.
            ///Our strategy here is to clear the pointer, hence allowing the cache to remove the image, and ask the inputs to render the full RoI
            ///instead of the rest to render. This way, even if the image is cleared from the cache we already have rendered the full RoI anyway.
            rectsLeftToRender.clear();
            rectsLeftToRender.push_back(roi);
            for (std::map<ImageComponents, PlaneToRender>::iterator it2 = planesToRender.planes.begin(); it2 != planesToRender.planes.end(); ++it2) {
                //Keep track of the original cached image for the re-lookup afterward, if the pointer doesn't match the first look-up, don't consider
                //the image because the region to render might have changed and we might have to re-trigger a render on inputs again.

                ///Make sure to never dereference originalCachedImage! We only compare it (that's why it s a void*)
                it2->second.originalCachedImage = it2->second.fullscaleImage.get();
                it2->second.fullscaleImage.reset();
                it2->second.downscaleImage.reset();
            }
            isPlaneCached.reset();
            redoCacheLookup = true;
        }


        ///If the effect doesn't support tiles and it has something left to render, just render the bounds again
        ///Note that it should NEVER happen because if it doesn't support tiles in the first place, it would
        ///have rendered the rod already.
        if (!frameRenderArgs.tilesSupported && !rectsLeftToRender.empty() && isPlaneCached) {
            ///if the effect doesn't support tiles, just render the whole rod again even though
            rectsLeftToRender.clear();
            rectsLeftToRender.push_back(useImageAsOutput ? upscaledImageBounds : downscaledImageBounds);
        }
    } else {
        if (frameRenderArgs.tilesSupported) {
            rectsLeftToRender.push_back(roi);
        } else {
            rectsLeftToRender.push_back(useImageAsOutput ? upscaledImageBounds : downscaledImageBounds);
        }
    }

    /*
     * If the effect has multiple inputs (such as masks) try to call isIdentity if the RoDs do not intersect the RoI
     */
    bool tryIdentityOptim = false;
    RectI inputsRoDIntersectionPixel;
    if ( frameRenderArgs.tilesSupported && !rectsLeftToRender.empty()
         && (frameRenderArgs.viewerProgressReportEnabled || isDuringPaintStroke) ) {
        RectD inputsIntersection;
        bool inputsIntersectionSet = false;
        bool hasDifferentRods = false;
        int maxInput = getMaxInputCount();
        bool hasMask = false;
        boost::shared_ptr<RotoDrawableItem> attachedStroke = getNode()->getAttachedRotoItem();
        for (int i = 0; i < maxInput; ++i) {
            bool isMask = isInputMask(i) || isInputRotoBrush(i);
            RectD inputRod;
            if (attachedStroke && isMask) {
                getNode()->getPaintStrokeRoD(args.time, &inputRod);
                hasMask = true;
            } else {
                EffectInstance* input = getInput(i);
                if (!input) {
                    continue;
                }
                bool isProjectFormat;
                const ParallelRenderArgs* inputFrameArgs = input->getParallelRenderArgsTLS();
                U64 inputHash = (inputFrameArgs && inputFrameArgs->validArgs) ? inputFrameArgs->nodeHash : input->getHash();
                Natron::StatusEnum stat = input->getRegionOfDefinition_public(inputHash, args.time, args.scale, args.view, &inputRod, &isProjectFormat);
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
            inputsIntersection.toPixelEnclosing(mipMapLevel, par, &inputsRoDIntersectionPixel);
            tryIdentityOptim = true;
        }
    }

    if (tryIdentityOptim) {
        optimizeRectsToRender(this, inputsRoDIntersectionPixel, rectsLeftToRender, args.time, args.view, renderMappedScale, &planesToRender.rectsToRender);
    } else {
        for (std::list<RectI>::iterator it = rectsLeftToRender.begin(); it != rectsLeftToRender.end(); ++it) {
            RectToRender r;
            r.rect = *it;
            r.identityInput = 0;
            r.isIdentity = false;
            planesToRender.rectsToRender.push_back(r);
        }
    }

    bool hasSomethingToRender = !planesToRender.rectsToRender.empty();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End Determine rectangles left to render /////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Pre-render input images ////////////////////////////////////////////////////////////////

    ///Pre-render input images before allocating the image if we need to render
    {
        const ImageComponents & outComp = outputComponents.front();
        if ( outComp.isColorPlane() ) {
            planesToRender.outputPremult = getOutputPremultiplication();
        } else {
            planesToRender.outputPremult = eImagePremultiplicationOpaque;
        }
    }
    for (std::list<RectToRender>::iterator it = planesToRender.rectsToRender.begin(); it != planesToRender.rectsToRender.end(); ++it) {
        if (it->isIdentity) {
            continue;
        }

        RectD canonicalRoI;
        if (useImageAsOutput) {
            it->rect.toCanonical(0, par, rod, &canonicalRoI);
        } else {
            it->rect.toCanonical(args.mipMapLevel, par, rod, &canonicalRoI);
        }

        RenderRoIRetCode inputCode = renderInputImagesForRoI(requestPassData,
                                                             useTransforms,
                                                             args.time,
                                                             args.view,
                                                             par,
                                                             rod,
                                                             canonicalRoI,
                                                             inputsToTransform,
                                                             args.mipMapLevel,
                                                             args.scale,
                                                             renderMappedScale,
                                                             renderScaleOneUpstreamIfRenderScaleSupportDisabled,
                                                             byPassCache,
                                                             framesNeeded,
                                                             neededComps,
                                                             &it->imgs,
                                                             &it->inputRois);

        if ( planesToRender.inputPremult.empty() ) {
            for (InputImagesMap::iterator it2 = it->imgs.begin(); it2 != it->imgs.end(); ++it2) {
                EffectInstance* input = getInput(it2->first);
                if (input) {
                    Natron::ImagePremultiplicationEnum inputPremult = input->getOutputPremultiplication();
                    if ( !it2->second.empty() ) {
                        const ImageComponents & comps = it2->second.front()->getComponents();
                        if ( !comps.isColorPlane() ) {
                            inputPremult = eImagePremultiplicationOpaque;
                        }
                    }

                    planesToRender.inputPremult[it2->first] = inputPremult;
                }
            }
        }

        //Render was aborted
        if (inputCode != eRenderRoIRetCodeOk) {
            return inputCode;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End Pre-render input images ////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Redo - cache lookup if memory almost full //////////////////////////////////////////////


    if (redoCacheLookup) {
        for (std::map<ImageComponents, PlaneToRender>::iterator it = planesToRender.planes.begin(); it != planesToRender.planes.end(); ++it) {
            /*
             * If the plane is the color plane, we might have to convert between components, hence we always
             * try to find in the cache the "preferred" components of this node for the color plane.
             * For all other planes, just consider this set of components, we do not allow conversion.
             */
            const ImageComponents* components = 0;
            if ( !it->first.isColorPlane() ) {
                components = &(it->first);
            } else {
                for (std::vector<Natron::ImageComponents>::const_iterator it = outputComponents.begin(); it != outputComponents.end(); ++it) {
                    if ( it->isColorPlane() ) {
                        components = &(*it);
                        break;
                    }
                }
            }

            assert(components);
            getImageFromCacheAndConvertIfNeeded(createInCache, useDiskCacheNode, key, renderMappedMipMapLevel,
                                                useImageAsOutput ? &upscaledImageBounds : &downscaledImageBounds,
                                                &rod,
                                                args.bitdepth, it->first,
                                                outputDepth, *components,
                                                args.inputImagesList, frameRenderArgs.stats, &it->second.fullscaleImage);

            ///We must retrieve from the cache exactly the originally retrieved image, otherwise we might have to call  renderInputImagesForRoI
            ///again, which could create a vicious cycle.
            if ( it->second.fullscaleImage && (it->second.fullscaleImage.get() == it->second.originalCachedImage) ) {
                it->second.downscaleImage = it->second.fullscaleImage;
            } else {
                for (std::map<ImageComponents, PlaneToRender>::iterator it2 = planesToRender.planes.begin(); it2 != planesToRender.planes.end(); ++it2) {
                    it2->second.fullscaleImage.reset();
                    it2->second.downscaleImage.reset();
                }
                break;
            }
        }

        isPlaneCached = planesToRender.planes.begin()->second.fullscaleImage;

        if (!isPlaneCached) {
            planesToRender.rectsToRender.clear();
            rectsLeftToRender.clear();
            if (frameRenderArgs.tilesSupported) {
                rectsLeftToRender.push_back(roi);
            } else {
                rectsLeftToRender.push_back(useImageAsOutput ? upscaledImageBounds : downscaledImageBounds);
            }


            if ( tryIdentityOptim && !rectsLeftToRender.empty() ) {
                optimizeRectsToRender(this, inputsRoDIntersectionPixel, rectsLeftToRender, args.time, args.view, renderMappedScale, &planesToRender.rectsToRender);
            } else {
                for (std::list<RectI>::iterator it = rectsLeftToRender.begin(); it != rectsLeftToRender.end(); ++it) {
                    RectToRender r;
                    r.rect = *it;
                    r.identityInput = 0;
                    r.isIdentity = false;
                    planesToRender.rectsToRender.push_back(r);
                }
            }

            ///We must re-copute input images because we might not have rendered what's needed
            for (std::list<RectToRender>::iterator it = planesToRender.rectsToRender.begin();
                 it != planesToRender.rectsToRender.end(); ++it) {
                if (it->isIdentity) {
                    continue;
                }

                RectD canonicalRoI;
                if (useImageAsOutput) {
                    it->rect.toCanonical(0, par, rod, &canonicalRoI);
                } else {
                    it->rect.toCanonical(args.mipMapLevel, par, rod, &canonicalRoI);
                }

                RenderRoIRetCode inputRetCode = renderInputImagesForRoI(requestPassData,
                                                                        useTransforms,
                                                                        args.time,
                                                                        args.view,
                                                                        par,
                                                                        rod,
                                                                        canonicalRoI,
                                                                        inputsToTransform,
                                                                        args.mipMapLevel,
                                                                        args.scale,
                                                                        renderMappedScale,
                                                                        renderScaleOneUpstreamIfRenderScaleSupportDisabled,
                                                                        byPassCache,
                                                                        framesNeeded,
                                                                        neededComps,
                                                                        &it->imgs,
                                                                        &it->inputRois);
                //Render was aborted
                if (inputRetCode != eRenderRoIRetCodeOk) {
                    return inputRetCode;
                }
            }
        }
    } // if (redoCacheLookup) {

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End 2nd cache lookup ///////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Allocate planes in the cache ////////////////////////////////////////////////////////////

    ///For all planes, if needed allocate the associated image
    if (hasSomethingToRender) {
        for (std::map<ImageComponents, PlaneToRender>::iterator it = planesToRender.planes.begin();
             it != planesToRender.planes.end(); ++it) {
            const ImageComponents *components = 0;

            if ( !it->first.isColorPlane() ) {
                //This plane is not color, there can only be a single set of components
                components = &(it->first);
            } else {
                //Find color plane from clip preferences
                for (std::vector<Natron::ImageComponents>::const_iterator it = outputComponents.begin(); it != outputComponents.end(); ++it) {
                    if ( it->isColorPlane() ) {
                        components = &(*it);
                        break;
                    }
                }
            }
            assert(components);

            if (!it->second.fullscaleImage) {
                ///The image is not cached
                allocateImagePlane(key, rod, downscaledImageBounds, upscaledImageBounds, isProjectFormat, framesNeeded, *components, args.bitdepth, par, args.mipMapLevel, renderFullScaleThenDownscale, renderScaleOneUpstreamIfRenderScaleSupportDisabled, useDiskCacheNode, createInCache, &it->second.fullscaleImage, &it->second.downscaleImage);
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
                bool hasResized = it->second.fullscaleImage->ensureBounds(useImageAsOutput ? upscaledImageBounds : downscaledImageBounds,
                                                                          fillGrownBoundsWithZeroes, fillGrownBoundsWithZeroes);


                /*
                 * Note that the image has been resized and the bitmap explicitly set to 1 in the newly allocated portions (for rotopaint purpose).
                 * We must reset it back to 0 in the last stroke tick RoD.
                 */
                if (hasResized && fillGrownBoundsWithZeroes) {
                    it->second.fullscaleImage->clearBitmap(lastStrokePixelRoD);
                }

                if ( renderFullScaleThenDownscale && (it->second.fullscaleImage->getMipMapLevel() == 0) ) {
                    //Allocate a downscale image that will be cheap to create
                    ///The upscaled image will be rendered using input images at lower def... which means really crappy results, don't cache this image!
                    RectI bounds;
                    rod.toPixelEnclosing(args.mipMapLevel, par, &bounds);
                    it->second.downscaleImage.reset( new Natron::Image(*components, rod, downscaledImageBounds, args.mipMapLevel, it->second.fullscaleImage->getPixelAspectRatio(), outputDepth, true) );
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
    } // hasSomethingToRender
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      ////////////////////////////// End allocation of planes ///////////////////////////////////////////////////////////////


    //There should always be at least 1 plane to render (The color plane)
    assert( !planesToRender.planes.empty() );

    ///If we reach here, it can be either because the planes are cached or not, either way
    ///the planes are NOT a total identity, and they may have some content left to render.
    EffectInstance::RenderRoIStatusEnum renderRetCode = eRenderRoIStatusImageAlreadyRendered;
    bool renderAborted;

    if (!hasSomethingToRender && !planesToRender.isBeingRenderedElsewhere) {
        renderAborted = aborted();
    } else {
#if NATRON_ENABLE_TRIMAP
        ///Only use trimap system if the render cannot be aborted.
        if (!frameRenderArgs.canAbort && frameRenderArgs.isRenderResponseToUserInteraction) {
            for (std::map<ImageComponents, PlaneToRender>::iterator it = planesToRender.planes.begin(); it != planesToRender.planes.end(); ++it) {
                _imp->markImageAsBeingRendered(useImageAsOutput ? it->second.fullscaleImage : it->second.downscaleImage);
            }
        }
#endif

        if (hasSomethingToRender) {
            


            // eRenderSafetyInstanceSafe means that there is at most one render per instance
            // NOTE: the per-instance lock should probably be shared between
            // all clones of the same instance, because an InstanceSafe plugin may assume it is the sole owner of the output image,
            // and read-write on it.
            // It is probably safer to assume that several clones may write to the same output image only in the eRenderSafetyFullySafe case.

            // eRenderSafetyFullySafe means that there is only one render per FRAME : the lock is by image and handled in Node.cpp
            ///locks belongs to an instance)

            boost::shared_ptr<QMutexLocker> locker;
            Natron::RenderSafetyEnum safety = getCurrentThreadSafetyThreadLocal();
            if (safety == eRenderSafetyInstanceSafe) {
                locker.reset( new QMutexLocker( &getNode()->getRenderInstancesSharedMutex() ) );
            } else if (safety == eRenderSafetyUnsafe) {
                const Natron::Plugin* p = getNode()->getPlugin();
                assert(p);
                locker.reset( new QMutexLocker( p->getPluginLock() ) );
            }
            ///For eRenderSafetyFullySafe, don't take any lock, the image already has a lock on itself so we're sure it can't be written to by 2 different threads.

            if ( frameRenderArgs.stats && frameRenderArgs.stats->isInDepthProfilingEnabled() ) {
                frameRenderArgs.stats->setGlobalRenderInfosForNode(getNode(), rod, planesToRender.outputPremult, processChannels, frameRenderArgs.tilesSupported, !renderFullScaleThenDownscale, renderMappedMipMapLevel);
            }

# ifdef DEBUG

            /*{
                const std::list<RectToRender>& rectsToRender = planesToRender.rectsToRender;
                qDebug() <<'('<<QThread::currentThread()<<")--> "<< getNode()->getScriptName_mt_safe().c_str() << ": render view: " << args.view << ", time: " << args.time << " No. tiles: " << rectsToRender.size() << " rectangles";
                for (std::list<RectToRender>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
                    qDebug() << "rect: " << "x1= " <<  it->rect.x1 << " , y1= " << it->rect.y1 << " , x2= " << it->rect.x2 << " , y2= " << it->rect.y2 << "(identity:" << it->isIdentity << ")";
                }
                for (std::map<Natron::ImageComponents, PlaneToRender> ::iterator it = planesToRender.planes.begin(); it != planesToRender.planes.end(); ++it) {
                    qDebug() << "plane: " <<  it->second.downscaleImage.get() << it->first.getLayerName().c_str();
                }
                qDebug() << "Cached:" << (isPlaneCached.get() != 0) << "Rendered elsewhere:" << planesToRender.isBeingRenderedElsewhere;

               }*/
# endif
            renderRetCode = renderRoIInternal(args.time,
                                              frameRenderArgs,
                                              safety,
                                              args.mipMapLevel,
                                              args.view,
                                              rod,
                                              par,
                                              planesToRender,
                                              frameRenderArgs.isSequentialRender,
                                              frameRenderArgs.isRenderResponseToUserInteraction,
                                              nodeHash,
                                              renderFullScaleThenDownscale,
                                              renderScaleOneUpstreamIfRenderScaleSupportDisabled,
                                              byPassCache,
                                              outputDepth,
                                              outputClipPrefComps,
                                              processChannels);
        } // if (hasSomethingToRender) {

        renderAborted = aborted();
#if NATRON_ENABLE_TRIMAP

        if (!frameRenderArgs.canAbort && frameRenderArgs.isRenderResponseToUserInteraction) {
            ///Only use trimap system if the render cannot be aborted.
            ///If we were aborted after all (because the node got deleted) then return a NULL image and empty the cache
            ///of this image
            for (std::map<ImageComponents, PlaneToRender>::iterator it = planesToRender.planes.begin(); it != planesToRender.planes.end(); ++it) {
                if (!renderAborted) {
                    if ( (renderRetCode == eRenderRoIStatusRenderFailed) || !planesToRender.isBeingRenderedElsewhere ) {
                        _imp->unmarkImageAsBeingRendered(useImageAsOutput ? it->second.fullscaleImage : it->second.downscaleImage,
                                                         renderRetCode == eRenderRoIStatusRenderFailed);
                    } else {
                        if ( !_imp->waitForImageBeingRenderedElsewhereAndUnmark(roi,
                                                                                useImageAsOutput ? it->second.fullscaleImage : it->second.downscaleImage) ) {
                            renderAborted = true;
                        }
                    }
                } else {
                    appPTR->removeFromNodeCache(useImageAsOutput ? it->second.fullscaleImage : it->second.downscaleImage);
                    _imp->unmarkImageAsBeingRendered(useImageAsOutput ? it->second.fullscaleImage : it->second.downscaleImage, true);

                    return eRenderRoIRetCodeAborted;
                }
            }
        }
#endif
    } // if (!hasSomethingToRender && !planesToRender.isBeingRenderedElsewhere) {


    if ( renderAborted && (renderRetCode != eRenderRoIStatusImageAlreadyRendered) ) {
        ///Return a NULL image

        if (isDuringPaintStroke) {
            //We know the image will never be used ever again
            getNode()->removeAllImagesFromCache();
        }

        return eRenderRoIRetCodeAborted;
    } else if (renderRetCode == eRenderRoIStatusRenderFailed) {
        ///Throwing this exception will ensure the render stops.
        ///This is slightly clumsy since we already have a render rect code indicating it, we should
        ///use the ret code instead.
        throw std::runtime_error("Rendering Failed");
    }


#if DEBUG
    if (hasSomethingToRender && (renderRetCode != eRenderRoIStatusRenderFailed) && !renderAborted) {
        // Kindly check that everything we asked for is rendered!

        for (std::map<ImageComponents, PlaneToRender>::iterator it = planesToRender.planes.begin(); it != planesToRender.planes.end(); ++it) {
            if (!frameRenderArgs.tilesSupported) {
                //assert that bounds are consistent with the RoD if tiles are not supported
                const RectD & srcRodCanonical = useImageAsOutput ? it->second.fullscaleImage->getRoD() : it->second.downscaleImage->getRoD();
                RectI srcBounds;
                srcRodCanonical.toPixelEnclosing(useImageAsOutput ? it->second.fullscaleImage->getMipMapLevel() : it->second.downscaleImage->getMipMapLevel(), par, &srcBounds);
                RectI srcRealBounds = useImageAsOutput ? it->second.fullscaleImage->getBounds() : it->second.downscaleImage->getBounds();
                assert(srcRealBounds.x1 == srcBounds.x1);
                assert(srcRealBounds.x2 == srcBounds.x2);
                assert(srcRealBounds.y1 == srcBounds.y1);
                assert(srcRealBounds.y2 == srcBounds.y2);
            }

            std::list<RectI> restToRender;
            if (useImageAsOutput) {
                it->second.fullscaleImage->getRestToRender(roi, restToRender);
            } else {
                it->second.downscaleImage->getRestToRender(roi, restToRender);
            }
            assert( restToRender.empty() );
        }
    }
#endif

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////// Make sure all planes rendered have the requested mipmap level and format ///////////////////////////

    bool useAlpha0ForRGBToRGBAConversion = args.caller ? args.caller->getNode()->usesAlpha0ToConvertFromRGBToRGBA() : false;

    for (std::map<ImageComponents, PlaneToRender>::iterator it = planesToRender.planes.begin(); it != planesToRender.planes.end(); ++it) {
        //We have to return the downscale image, so make sure it has been computed
        if ( (renderRetCode != eRenderRoIStatusRenderFailed) && renderFullScaleThenDownscale && renderScaleOneUpstreamIfRenderScaleSupportDisabled ) {
            assert(it->second.fullscaleImage->getMipMapLevel() == 0);
            roi.intersect(it->second.fullscaleImage->getBounds(), &roi);
            if (it->second.downscaleImage == it->second.fullscaleImage) {
                it->second.downscaleImage.reset( new Image(it->second.fullscaleImage->getComponents(),
                                                           it->second.fullscaleImage->getRoD(),
                                                           downscaledImageBounds,
                                                           args.mipMapLevel,
                                                           it->second.fullscaleImage->getPixelAspectRatio(),
                                                           it->second.fullscaleImage->getBitDepth(),
                                                           false) );
            }

            it->second.fullscaleImage->downscaleMipMap( it->second.fullscaleImage->getRoD(), roi, 0, args.mipMapLevel, false, it->second.downscaleImage.get() );
        }
        ///The image might need to be converted to fit the original requested format
        bool imageConversionNeeded = it->first != it->second.downscaleImage->getComponents() || args.bitdepth != it->second.downscaleImage->getBitDepth();

        if ( imageConversionNeeded && (renderRetCode != eRenderRoIStatusRenderFailed) ) {
            /**
             * Lock the downscaled image so it cannot be resized while creating the temp image and calling convertToFormat.
             **/
            boost::shared_ptr<Image> tmp;
            {
                Image::ReadAccess acc = it->second.downscaleImage->getReadRights();
                RectI bounds = it->second.downscaleImage->getBounds();

                tmp.reset( new Image(it->first, it->second.downscaleImage->getRoD(), bounds, mipMapLevel, it->second.downscaleImage->getPixelAspectRatio(), args.bitdepth, false) );

                bool unPremultIfNeeded = planesToRender.outputPremult == eImagePremultiplicationPremultiplied && it->second.downscaleImage->getComponentsCount() == 4 && tmp->getComponentsCount() == 3;

                if (useAlpha0ForRGBToRGBAConversion) {
                    it->second.downscaleImage->convertToFormatAlpha0( bounds,
                                                                      getApp()->getDefaultColorSpaceForBitDepth( it->second.downscaleImage->getBitDepth() ),
                                                                      getApp()->getDefaultColorSpaceForBitDepth(args.bitdepth),
                                                                      -1, false, unPremultIfNeeded, tmp.get() );
                } else {
                    it->second.downscaleImage->convertToFormat( bounds,
                                                                getApp()->getDefaultColorSpaceForBitDepth( it->second.downscaleImage->getBitDepth() ),
                                                                getApp()->getDefaultColorSpaceForBitDepth(args.bitdepth),
                                                                -1, false, unPremultIfNeeded, tmp.get() );
                }
            }
            it->second.downscaleImage = tmp;
        }
        assert(it->second.downscaleImage->getComponents() == it->first && it->second.downscaleImage->getBitDepth() == args.bitdepth);
        outputPlanes->push_back(it->second.downscaleImage);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////// End requested format convertion ////////////////////////////////////////////////////////////////////


    ///// Termination, update last rendered planes
    assert( !outputPlanes->empty() );

    return eRenderRoIRetCodeOk;
} // renderRoI

EffectInstance::RenderRoIStatusEnum
EffectInstance::renderRoIInternal(double time,
                                  const ParallelRenderArgs & frameArgs,
                                  Natron::RenderSafetyEnum safety,
                                  unsigned int mipMapLevel,
                                  int view,
                                  const RectD & rod, //!< effect rod in canonical coords
                                  const double par,
                                  ImagePlanesToRender & planesToRender,
                                  bool isSequentialRender,
                                  bool isRenderMadeInResponseToUserInteraction,
                                  U64 nodeHash,
                                  bool renderFullScaleThenDownscale,
                                  bool useScaleOneInputImages,
                                  bool byPassCache,
                                  Natron::ImageBitDepthEnum outputClipPrefDepth,
                                  const std::list<Natron::ImageComponents> & outputClipPrefsComps,
                                  bool* processChannels)
{
    EffectInstance::RenderRoIStatusEnum retCode;

    assert( !planesToRender.planes.empty() );

    ///Add the window to the project's available formats if the effect is a reader
    ///This is the only reliable place where I could put these lines...which don't seem to feel right here.
    ///Plus setOrAddProjectFormat will actually set the project format the first time we read an image in the project
    ///hence ask for a new render... which can be expensive!
    ///Any solution how to work around this ?
    ///Edit: do not do this if in the main-thread (=noRenderThread = -1) otherwise we will change the parallel render args TLS
    ///which will lead to asserts down the stream
    if ( isReader() && ( QThread::currentThread() != qApp->thread() ) ) {
        Format frmt;
        RectI pixelRoD;
        rod.toPixelEnclosing(0, par, &pixelRoD);
        frmt.set(pixelRoD);
        frmt.setPixelAspectRatio(par);
        getApp()->getProject()->setOrAddProjectFormat(frmt);
    }

    RenderScale renderMappedScale;
    unsigned int renderMappedMipMapLevel = 0;

    for (std::map<ImageComponents, PlaneToRender>::iterator it = planesToRender.planes.begin(); it != planesToRender.planes.end(); ++it) {
        it->second.renderMappedImage = renderFullScaleThenDownscale ? it->second.fullscaleImage : it->second.downscaleImage;
        if ( it == planesToRender.planes.begin() ) {
            renderMappedMipMapLevel = it->second.renderMappedImage->getMipMapLevel();
        }
    }

    renderMappedScale.x = Image::getScaleFromMipMapLevel(renderMappedMipMapLevel);
    renderMappedScale.y = renderMappedScale.x;


    RenderingFunctorRetEnum renderStatus = eRenderingFunctorRetOK;
    if ( planesToRender.rectsToRender.empty() ) {
        retCode = EffectInstance::eRenderRoIStatusImageAlreadyRendered;
    } else {
        retCode = EffectInstance::eRenderRoIStatusImageRendered;
    }


    ///Notify the gui we're rendering
    boost::shared_ptr<NotifyRenderingStarted_RAII> renderingNotifier;
    if ( !planesToRender.rectsToRender.empty() ) {
        renderingNotifier.reset( new NotifyRenderingStarted_RAII( getNode().get() ) );
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
        if ( !frameArgs.tilesSupported || (nbThreads == -1) || (nbThreads == 1) ||
            ( (nbThreads == 0) && (appPTR->getHardwareIdealThreadCount() == 1) ) ||
            ( QThreadPool::globalInstance()->activeThreadCount() >= QThreadPool::globalInstance()->maxThreadCount() ) ||
            isRotoPaintNode() ) {
            safety = eRenderSafetyFullySafe;
        }
    }


    std::map<boost::shared_ptr<Natron::Node>, ParallelRenderArgs > tlsCopy;
    if (safety == eRenderSafetyFullySafeFrame) {
        /*
         * Since we're about to start new threads potentially, copy all the thread local storage on all nodes (any node may be involved in
         * expressions, and we need to retrieve the exact local time of render).
         */
        getApp()->getProject()->getParallelRenderArgs(tlsCopy);
    }

    double firstFrame, lastFrame;
    getFrameRange_public(nodeHash, &firstFrame, &lastFrame);


    ///We only need to call begin if we've not already called it.
    bool callBegin = false;

    /// call beginsequenceRender here if the render is sequential
    Natron::SequentialPreferenceEnum pref = getSequentialPreference();
    if ( !isWriter() || (pref == eSequentialPreferenceNotSequential) ) {
        callBegin = true;
    }


    if (callBegin) {
        assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );
        if (beginSequenceRender_public(time, time, 1, !appPTR->isBackground(), renderMappedScale, isSequentialRender,
                                       isRenderMadeInResponseToUserInteraction, frameArgs.draftMode, view) == eStatusFailed) {
            renderStatus = eRenderingFunctorRetFailed;
        }
    }


    /*
     * All channels will be taken from this input if some channels are marked to be not processed
     */
    int preferredInput = getNode()->getPreferredInput();
    if ( (preferredInput != -1) && isInputMask(preferredInput) ) {
        preferredInput = -1;
    }

    const QThread* currentThread = QThread::currentThread();

    if (renderStatus != eRenderingFunctorRetFailed) {
        if (safety == eRenderSafetyFullySafeFrame) {
            TiledRenderingFunctorArgs tiledArgs;
            tiledArgs.frameArgs = frameArgs;
            tiledArgs.frameTLS = tlsCopy;
            tiledArgs.renderFullScaleThenDownscale = renderFullScaleThenDownscale;
            tiledArgs.renderUseScaleOneInputs = useScaleOneInputImages;
            tiledArgs.isRenderResponseToUserInteraction = isRenderMadeInResponseToUserInteraction;
            tiledArgs.firstFrame = firstFrame;
            tiledArgs.lastFrame = lastFrame;
            tiledArgs.preferredInput = preferredInput;
            tiledArgs.mipMapLevel = mipMapLevel;
            tiledArgs.renderMappedMipMapLevel = renderMappedMipMapLevel;
            tiledArgs.rod = rod;
            tiledArgs.time = time;
            tiledArgs.view = view;
            tiledArgs.par = par;
            tiledArgs.byPassCache = byPassCache;
            tiledArgs.outputClipPrefDepth = outputClipPrefDepth;
            tiledArgs.outputClipPrefsComps = outputClipPrefsComps;
            tiledArgs.processChannels = processChannels;
            tiledArgs.planes = planesToRender;


#ifdef NATRON_HOSTFRAMETHREADING_SEQUENTIAL
            std::vector<EffectInstance::RenderingFunctorRetEnum> ret( tiledData.size() );
            int i = 0;
            for (std::list<RectToRender>::const_iterator it = planesToRender.rectsToRender.begin(); it != planesToRender.rectsToRender.end(); ++it, ++i) {
                ret[i] = tiledRenderingFunctor(tiledArgs,
                                               *it,
                                               currentThread);
            }
            std::vector<EffectInstance::RenderingFunctorRetEnum>::const_iterator it2;

#else


            QFuture<RenderingFunctorRetEnum> ret = QtConcurrent::mapped( planesToRender.rectsToRender,
                                                                        boost::bind(&EffectInstance::tiledRenderingFunctor,
                                                                                    this,
                                                                                    tiledArgs,
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
                    planesToRender.isBeingRenderedElsewhere = true;
                }
#endif
                else if ( (*it2) == EffectInstance::eRenderingFunctorRetAborted ) {
                    renderStatus = eRenderingFunctorRetFailed;
                    break;
                }
            }
        } else {
            for (std::list<RectToRender>::const_iterator it = planesToRender.rectsToRender.begin(); it != planesToRender.rectsToRender.end(); ++it) {
                RenderingFunctorRetEnum functorRet = tiledRenderingFunctor(currentThread, frameArgs, *it, tlsCopy, renderFullScaleThenDownscale, useScaleOneInputImages, isSequentialRender, isRenderMadeInResponseToUserInteraction, firstFrame, lastFrame, preferredInput, mipMapLevel, renderMappedMipMapLevel, rod, time, view, par, byPassCache, outputClipPrefDepth, outputClipPrefsComps, processChannels, planesToRender);

                if ( (functorRet == eRenderingFunctorRetFailed) || (functorRet == eRenderingFunctorRetAborted) ) {
                    renderStatus = functorRet;
                    break;
                }

                if  (functorRet == eRenderingFunctorRetTakeImageLock) {
                    renderStatus = eRenderingFunctorRetOK;
#if NATRON_ENABLE_TRIMAP
                    planesToRender.isBeingRenderedElsewhere = true;
#endif
                }
            } // for (std::list<RectI>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
        }
    } // if (renderStatus != eRenderingFunctorRetFailed) {

    ///never call endsequence render here if the render is sequential
    if (callBegin) {
        assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );
        if (endSequenceRender_public(time, time, time, false, renderMappedScale,
                                     isSequentialRender,
                                     isRenderMadeInResponseToUserInteraction,
                                     frameArgs.draftMode,
                                     view) == eStatusFailed) {
            renderStatus = eRenderingFunctorRetFailed;
        }
    }
    
    if (renderStatus != eRenderingFunctorRetOK) {
        retCode = eRenderRoIStatusRenderFailed;
    }
    
    return retCode;
} // renderRoIInternal
