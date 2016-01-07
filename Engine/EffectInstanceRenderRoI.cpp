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
#include <stdexcept>
#include <fstream>

#include <QtCore/QThreadPool>
#include <QtConcurrentMap> // QtCore on Qt4, QtConcurrent on Qt5
#include <QtCore/QReadWriteLock>
#include <QtCore/QCoreApplication>
#include <QtCore/QtConcurrentRun>
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
#include "Engine/PluginMemory.h"
#include "Engine/Project.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/Settings.h"
#include "Engine/Timer.h"
#include "Engine/Transform.h"
#include "Engine/ViewerInstance.h"

//#define NATRON_ALWAYS_ALLOCATE_FULL_IMAGE_BOUNDS


using namespace Natron;

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

ImagePtr
EffectInstance::convertPlanesFormatsIfNeeded(const AppInstance* app,
                                             const ImagePtr& inputImage,
                                             const RectI& roi,
                                             const ImageComponents& targetComponents,
                                             ImageBitDepthEnum targetDepth,
                                             bool useAlpha0ForRGBToRGBAConversion,
                                             ImagePremultiplicationEnum outputPremult,
                                             int channelForAlpha)
{
    bool imageConversionNeeded = (/*!targetIsMultiPlanar &&*/ targetComponents.getNumComponents() != inputImage->getComponents().getNumComponents()) || targetDepth != inputImage->getBitDepth();
    if (!imageConversionNeeded) {
        return inputImage;
    } else {
        /**
         * Lock the downscaled image so it cannot be resized while creating the temp image and calling convertToFormat.
         **/
        Image::ReadAccess acc = inputImage->getReadRights();
        RectI bounds = inputImage->getBounds();
        
        ImagePtr tmp(new Image(targetComponents, inputImage->getRoD(), bounds, inputImage->getMipMapLevel(), inputImage->getPixelAspectRatio(), targetDepth, false));
        
        RectI clippedRoi;
        roi.intersect(bounds, &clippedRoi);
        
        bool unPremultIfNeeded = outputPremult == eImagePremultiplicationPremultiplied && inputImage->getComponentsCount() == 4 && tmp->getComponentsCount() == 3;
        
        if (useAlpha0ForRGBToRGBAConversion) {
            inputImage->convertToFormatAlpha0( clippedRoi,
                                              app->getDefaultColorSpaceForBitDepth(inputImage->getBitDepth()),
                                              app->getDefaultColorSpaceForBitDepth(targetDepth),
                                              channelForAlpha, false, unPremultIfNeeded, tmp.get() );
        } else {
            inputImage->convertToFormat( clippedRoi,
                                        app->getDefaultColorSpaceForBitDepth(inputImage->getBitDepth()),
                                        app->getDefaultColorSpaceForBitDepth(targetDepth),
                                        channelForAlpha, false, unPremultIfNeeded, tmp.get() );
        }
        
        return tmp;
    }
    
}

EffectInstance::RenderRoIRetCode
EffectInstance::renderRoI(const RenderRoIArgs & args,
                          ImageList* outputPlanes)
{
    //Do nothing if no components were requested
    if ( args.components.empty() || args.roi.isNull() ) {
        qDebug() << getScriptName_mt_safe().c_str() << "renderRoi: Early bail-out components requested empty or RoI is NULL";

        return eRenderRoIRetCodeOk;
    }

    //Create the TLS data for this node if it did not exist yet
    EffectDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
    assert(tls);
    if (!tls->frameArgs.validArgs) {
        qDebug() << QThread::currentThread() << "[BUG]:" << getScriptName_mt_safe().c_str() <<  "Thread-storage for the render of the frame was not set.";
        tls->frameArgs.time = args.time;
        tls->frameArgs.nodeHash = getHash();
        tls->frameArgs.view = args.view;
        tls->frameArgs.isSequentialRender = false;
        tls->frameArgs.isRenderResponseToUserInteraction = true;
        tls->frameArgs.validArgs = 0;
    } else {
        //The hash must not have changed if we did a pre-pass.
        assert(!tls->frameArgs.request || tls->frameArgs.nodeHash == tls->frameArgs.request->nodeHash);
    }


    ///For writer we never want to cache otherwise the next time we want to render it will skip writing the image on disk!
    bool byPassCache = args.byPassCache;

    ///Use the hash at this time, and then copy it to the clips in the thread local storage to use the same value
    ///through all the rendering of this frame.
    U64 nodeHash = tls->frameArgs.nodeHash;
    const double par = getPreferredAspectRatio();
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
    RenderScale renderMappedScale(Image::getScaleFromMipMapLevel(renderMappedMipMapLevel));
    assert( !( (supportsRS == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );


    const FrameViewRequest* requestPassData = 0;
    if (tls->frameArgs.request) {
        requestPassData = tls->frameArgs.request->getFrameViewRequest(args.time, args.view);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Get the RoD ///////////////////////////////////////////////////////////////
    RectD rod; //!< rod is in canonical coordinates
    bool isProjectFormat = false;
    {
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
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End get RoD ///////////////////////////////////////////////////////////////
    RectI roi;
    {
        if (renderFullScaleThenDownscale) {
            //We cache 'image', hence the RoI should be expressed in its coordinates
            //renderRoIInternal should check the bitmap of 'image' and not downscaledImage!
            RectD canonicalRoI;
            args.roi.toCanonical(args.mipMapLevel, par, rod, &canonicalRoI);
            canonicalRoI.toPixelEnclosing(0, par, &roi);
        } else {
            roi = args.roi;
        }
    }
    
    ///Determine needed planes
    boost::shared_ptr<ComponentsNeededMap> neededComps(new ComponentsNeededMap);
    ComponentsNeededMap::iterator foundOutputNeededComps;
    std::bitset<4> processChannels;

    {
        bool processAllComponentsRequested;

        {
            SequenceTime ptTime;
            int ptView;
            boost::shared_ptr<Natron::Node> ptInput;
            getComponentsNeededAndProduced_public(true, true, args.time, args.view, neededComps.get(), &processAllComponentsRequested, &ptTime, &ptView, &processChannels, &ptInput);


            foundOutputNeededComps = neededComps->find(-1);
            if ( foundOutputNeededComps == neededComps->end() ) {
                return eRenderRoIRetCodeOk;
            }
        }
        if (processAllComponentsRequested) {
            std::vector<ImageComponents> compVec;
            for (std::list<Natron::ImageComponents>::const_iterator it = args.components.begin(); it != args.components.end(); ++it) {
                bool found = false;

                //Change all needed comps in output to the requested components
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
            for (ComponentsNeededMap::iterator it = neededComps->begin(); it != neededComps->end(); ++it) {
                it->second = compVec;
            }
        }
    }
    const std::vector<Natron::ImageComponents> & outputComponents = foundOutputNeededComps->second;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Handle pass-through for planes //////////////////////////////////////////////////////////
    std::list<Natron::ImageComponents> requestedComponents;
    {
        ComponentsAvailableList componentsToFetchUpstream;
        {
            ComponentsAvailableMap componentsAvailables;

            //Available planes/components is view agnostic
            getComponentsAvailable(true, true, args.time, &componentsAvailables);


            /*
             * For all requested planes, check which components can be produced in output by this node.
             * If the components are from the color plane, if another set of components of the color plane is present
             * we try to render with those instead.
             */
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
                        componentsToFetchUpstream.push_back( std::make_pair( *it, found->second.lock() ) );
                    }
                }
            }
        }
        //Render planes that we are not able to render on this node from upstream
        for (ComponentsAvailableList::iterator it = componentsToFetchUpstream.begin(); it != componentsToFetchUpstream.end(); ++it) {
            NodePtr node = it->second.lock();
            if (node) {
                boost::scoped_ptr<RenderRoIArgs> inArgs (new RenderRoIArgs(args));
                inArgs->preComputedRoD.clear();
                inArgs->components.clear();
                inArgs->components.push_back(it->first);
                ImageList inputPlanes;
                RenderRoIRetCode inputRetCode = node->getLiveInstance()->renderRoI(*inArgs, &inputPlanes);
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
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End pass-through for planes //////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Check if effect is identity ///////////////////////////////////////////////////////////////
    {
        double inputTimeIdentity = 0.;
        int inputNbIdentity;

        assert( !( (supportsRS == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );
        bool identity;
        RectI pixelRod;
        rod.toPixelEnclosing(args.mipMapLevel, par, &pixelRod);
        ViewInvarianceLevel viewInvariance = isViewInvariant();

        if ( (args.view != 0) && (viewInvariance == eViewInvarianceAllViewsInvariant) ) {
            identity = true;
            inputNbIdentity = -2;
            inputTimeIdentity = args.time;
        } else {
            try {
                if (requestPassData) {
                    inputTimeIdentity = requestPassData->globalData.inputIdentityTime;
                    inputNbIdentity = requestPassData->globalData.identityInputNb;
                    identity = requestPassData->globalData.isIdentity;
                } else {
                    identity = isIdentity_public(true, nodeHash, args.time, renderMappedScale, pixelRod, args.view, &inputTimeIdentity, &inputNbIdentity);
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
                    boost::scoped_ptr<RenderRoIArgs> argCpy (new RenderRoIArgs(args));
                    argCpy->time = inputTimeIdentity;

                    if (viewInvariance == eViewInvarianceAllViewsInvariant) {
                        argCpy->view = 0;
                    }

                    argCpy->preComputedRoD.clear(); //< clear as the RoD of the identity input might not be the same (reproducible with Blur)

                    return renderRoI(*argCpy, outputPlanes);
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
                if ( tls->frameArgs.stats && tls->frameArgs.stats->isInDepthProfilingEnabled() ) {
                    tls->frameArgs.stats->setNodeIdentity( getNode(), inputEffectIdentity->getNode() );
                }


                boost::scoped_ptr<RenderRoIArgs> inputArgs (new RenderRoIArgs(args));
                inputArgs->time = inputTimeIdentity;

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
                
                bool fetchUserSelectedComponentsUpstream = getNode()->getChannelSelectorKnob(inputNbIdentity).get() != 0;
                
                if (fetchUserSelectedComponentsUpstream) {
                    /// This corresponds to choice B)
                   EffectInstance::ComponentsNeededMap::const_iterator foundCompsNeeded = neededComps->find(inputNbIdentity);
                    if (foundCompsNeeded != neededComps->end()) {
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
                
                
                
                
                ImageList identityPlanes;
                RenderRoIRetCode ret =  inputEffectIdentity->renderRoI(*inputArgs, &identityPlanes);
                if (ret == eRenderRoIRetCodeOk) {
                    outputPlanes->insert(outputPlanes->end(), identityPlanes.begin(),identityPlanes.end());
                    ImageList convertedPlanes;
                    AppInstance* app = getApp();
                    assert(args.components.size() == outputPlanes->size() || outputPlanes->empty());
                    bool useAlpha0ForRGBToRGBAConversion = args.caller ? args.caller->getNode()->usesAlpha0ToConvertFromRGBToRGBA() : false;
                    
                    std::list<Natron::ImageComponents>::const_iterator compIt = args.components.begin();
                    
                    for (ImageList::iterator it = outputPlanes->begin(); it!=outputPlanes->end(); ++it,++compIt) {
                        
                        ImagePremultiplicationEnum premult;
                        const ImageComponents & outComp = outputComponents.front();
                        if ( outComp.isColorPlane() ) {
                            premult = getOutputPremultiplication();
                        } else {
                            premult = eImagePremultiplicationOpaque;
                        }
                        
                        ImagePtr tmp = convertPlanesFormatsIfNeeded(app, *it, args.roi, *compIt, inputArgs->bitdepth, useAlpha0ForRGBToRGBAConversion, premult, -1);
                        assert(tmp);
                        convertedPlanes.push_back(tmp);
                    }
                    *outputPlanes = convertedPlanes;
                } else {
                    return ret;
                }
            } else {
                assert( outputPlanes->empty() );
            }

            return eRenderRoIRetCodeOk;
        } // if (identity)

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////// End identity check ///////////////////////////////////////////////////////////////



        // At this point, if only the pass through planes are view variant and the rendered view is different than 0,
        // just call renderRoI again for the components left to render on the view 0.
        if ( (args.view != 0) && (viewInvariance == eViewInvarianceOnlyPassThroughPlanesVariant) ) {
            boost::scoped_ptr<RenderRoIArgs> argCpy(new RenderRoIArgs(args));
            argCpy->view = 0;
            argCpy->preComputedRoD.clear();

            return renderRoI(*argCpy, outputPlanes);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Transform concatenations ///////////////////////////////////////////////////////////////
    ///Try to concatenate transform effects
    bool useTransforms;
    
    if (requestPassData) {
        tls->currentRenderArgs.transformRedirections.reset(new InputMatrixMap);
        tls->currentRenderArgs.transformRedirections = requestPassData->globalData.transforms;
        useTransforms = !tls->currentRenderArgs.transformRedirections->empty();
    } else {
        useTransforms = appPTR->getCurrentSettings()->isTransformConcatenationEnabled();
        if (useTransforms) {
            tls->currentRenderArgs.transformRedirections.reset(new InputMatrixMap);
            tryConcatenateTransforms(args.time, args.view, args.scale, tls->currentRenderArgs.transformRedirections.get());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////End transform concatenations//////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Compute RoI depending on render scale ///////////////////////////////////////////////////


    RectI downscaledImageBoundsNc;
    RectI upscaledImageBoundsNc;
    {
        rod.toPixelEnclosing(args.mipMapLevel, par, &downscaledImageBoundsNc);
        rod.toPixelEnclosing(0, par, &upscaledImageBoundsNc);


        ///Make sure the RoI falls within the image bounds
        ///Intersection will be in pixel coordinates
        if (tls->frameArgs.tilesSupported) {
            if (renderFullScaleThenDownscale) {
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
            upscaledImageBoundsNc.intersect(roi, &upscaledImageBoundsNc);
            downscaledImageBoundsNc.intersect(args.roi, &downscaledImageBoundsNc);
#endif
        }
    }
    
    /*
     * Keep in memory what the user as requested, and change the roi to the full bounds if the effect doesn't support tiles
     */
    const RectI originalRoI = roi;
    if (!tls->frameArgs.tilesSupported) {
        roi = renderFullScaleThenDownscale ? upscaledImageBoundsNc : downscaledImageBoundsNc;
    }
    
    const RectI & downscaledImageBounds = downscaledImageBoundsNc;
    const RectI & upscaledImageBounds = upscaledImageBoundsNc;
    RectD canonicalRoI;
    {
        if (renderFullScaleThenDownscale) {
            roi.toCanonical(0, par, rod, &canonicalRoI);
        } else {
            roi.toCanonical(args.mipMapLevel, par, rod, &canonicalRoI);
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End Compute RoI /////////////////////////////////////////////////////////////////////////
    const bool draftModeSupported = getNode()->isDraftModeUsed();

    const bool isFrameVaryingOrAnimated = isFrameVaryingOrAnimated_Recursive();
    const bool createInCache = shouldCacheOutput(isFrameVaryingOrAnimated, args.time, args.view);
    ///Do we want to render the graph upstream at scale 1 or at the requested render scale ? (user setting)
    bool renderScaleOneUpstreamIfRenderScaleSupportDisabled = false;
    if (renderFullScaleThenDownscale) {
        renderScaleOneUpstreamIfRenderScaleSupportDisabled = getNode()->useScaleOneImagesWhenRenderScaleSupportIsDisabled();

        ///For multi-resolution we want input images with exactly the same size as the output image
        if ( !renderScaleOneUpstreamIfRenderScaleSupportDisabled && !supportsMultiResolution() ) {
            renderScaleOneUpstreamIfRenderScaleSupportDisabled = true;
        }
    }
    Natron::ImageKey key(getNode().get(),
                         nodeHash,
                         isFrameVaryingOrAnimated,
                         args.time,
                         args.view,
                         1.,
                         draftModeSupported && tls->frameArgs.draftMode,
                         renderMappedMipMapLevel == 0 && args.mipMapLevel != 0 && !renderScaleOneUpstreamIfRenderScaleSupportDisabled);
    Natron::ImageKey nonDraftKey(getNode().get(),
                         nodeHash,
                         isFrameVaryingOrAnimated,
                         args.time,
                         args.view,
                         1.,
                         false,
                         renderMappedMipMapLevel == 0 && args.mipMapLevel != 0 && !renderScaleOneUpstreamIfRenderScaleSupportDisabled);

    bool useDiskCacheNode = dynamic_cast<DiskCacheNode*>(this) != NULL;


    /*
     * Get the bitdepth and output components that the plug-in expects to render. The cached image does not necesserarily has the bitdepth
     * that the plug-in expects.
     */
    Natron::ImageBitDepthEnum outputDepth;
    std::list<Natron::ImageComponents> outputClipPrefComps;
    getPreferredDepthAndComponents(-1, &outputClipPrefComps, &outputDepth);
    assert( !outputClipPrefComps.empty() );


    boost::shared_ptr<ImagePlanesToRender> planesToRender(new ImagePlanesToRender);
    boost::shared_ptr<FramesNeededMap> framesNeeded(new FramesNeededMap);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Look-up the cache ///////////////////////////////////////////////////////////////

    {
        //If one plane is missing from cache, we will have to render it all. For all other planes, either they have nothing
        //left to render, otherwise we render them for all the roi again.
        bool missingPlane = false;

        for (std::list<ImageComponents>::iterator it = requestedComponents.begin(); it != requestedComponents.end(); ++it) {
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
                for (std::vector<Natron::ImageComponents>::const_iterator it2 = outputComponents.begin(); it2 != outputComponents.end(); ++it2) {
                    if ( it2->isColorPlane() ) {
                        components = &(*it2);
                        break;
                    }
                }
            }
            assert(components);
            
            //For writers, we always want to call the render action when doing a sequential render, but we still want to use the cache for nodes upstream
            bool doCacheLookup = !isWriter() || !tls->frameArgs.isSequentialRender;
            if (doCacheLookup) {
     
                int nLookups = draftModeSupported && tls->frameArgs.draftMode ? 2 : 1;
                
                for (int n = 0; n < nLookups; ++n) {
                    getImageFromCacheAndConvertIfNeeded(createInCache, useDiskCacheNode, n == 0 ? nonDraftKey : key, renderMappedMipMapLevel,
                                                        renderFullScaleThenDownscale ? &upscaledImageBounds : &downscaledImageBounds,
                                                        &rod,
                                                        args.bitdepth, *it,
                                                        outputDepth,
                                                        *components,
                                                        args.inputImagesList,
                                                        tls->frameArgs.stats,
                                                        &plane.fullscaleImage);
                    if (plane.fullscaleImage) {
                        break;
                    }
                }
            }
            
            if (byPassCache) {
                if (plane.fullscaleImage) {
                    appPTR->removeFromNodeCache( key.getHash() );
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
                        outputPlanes->push_back(plane.fullscaleImage);
                        continue;
                    }
                } else {
                    *framesNeeded = plane.fullscaleImage->getParams()->getFramesNeeded();
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
                                outputPlanes->push_back(it2->second.fullscaleImage);
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

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////End cache lookup//////////////////////////////////////////////////////////

    if ( framesNeeded->empty() ) {
        if (requestPassData) {
            *framesNeeded = requestPassData->globalData.frameViewsNeeded;
        } else {
            *framesNeeded = getFramesNeeded_public(nodeHash, args.time, args.view, renderMappedMipMapLevel);
        }
    }


    ///In the event where we had the image from the cache, but it wasn't completly rendered over the RoI but the cache was almost full,
    ///we don't hold a pointer to it, allowing the cache to free it.
    ///Hence after rendering all the input images, we redo a cache look-up to check whether the image is still here
    bool redoCacheLookup = false;
    bool cacheAlmostFull = appPTR->isNodeCacheAlmostFull();
    ImagePtr isPlaneCached;

    if ( !planesToRender->planes.empty() ) {
        isPlaneCached = planesToRender->planes.begin()->second.fullscaleImage;
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
    
    //While painting, clear only the needed portion of the bitmap
    RectI lastStrokePixelRoD;
    if ( isDuringPaintStroke && args.inputImagesList.empty() ) {
        RectD lastStrokeRoD;
        NodePtr node = getNode();
        if ( !node->isLastPaintStrokeBitmapCleared() ) {
            lastStrokeRoD = getApp()->getLastPaintStrokeBbox();
            node->clearLastPaintStrokeRoD();
            // qDebug() << getScriptName_mt_safe().c_str() << "last stroke RoD: " << lastStrokeRoD.x1 << lastStrokeRoD.y1 << lastStrokeRoD.x2 << lastStrokeRoD.y2;
            lastStrokeRoD.toPixelEnclosing(mipMapLevel, par, &lastStrokePixelRoD);
        }
    }

    if (isPlaneCached) {
        if ( isDuringPaintStroke && !lastStrokePixelRoD.isNull() ) {
            fillGrownBoundsWithZeroes = true;
            //Clear the bitmap of the cached image in the portion of the last stroke to only recompute what's needed
            for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it2 = planesToRender->planes.begin();
                 it2 != planesToRender->planes.end(); ++it2) {
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
        if (!tls->frameArgs.canAbort && tls->frameArgs.isRenderResponseToUserInteraction) {
#ifndef DEBUG
            isPlaneCached->getRestToRender_trimap(roi, rectsLeftToRender, &planesToRender->isBeingRenderedElsewhere);
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
            for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it2 = planesToRender->planes.begin(); it2 != planesToRender->planes.end(); ++it2) {
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
        if (!tls->frameArgs.tilesSupported && !rectsLeftToRender.empty() && isPlaneCached) {
            ///if the effect doesn't support tiles, just render the whole rod again even though
            rectsLeftToRender.clear();
            rectsLeftToRender.push_back(renderFullScaleThenDownscale ? upscaledImageBounds : downscaledImageBounds);
        }
    } else { // !isPlaneCached
        if (tls->frameArgs.tilesSupported) {
            rectsLeftToRender.push_back(roi);
        } else {
            rectsLeftToRender.push_back(renderFullScaleThenDownscale ? upscaledImageBounds : downscaledImageBounds);
        }
    } //  // !isPlaneCached

    /*
     * If the effect has multiple inputs (such as masks) try to call isIdentity if the RoDs do not intersect the RoI
     */
    bool tryIdentityOptim = false;
    RectI inputsRoDIntersectionPixel;
    if ( tls->frameArgs.tilesSupported && !rectsLeftToRender.empty()
         && (tls->frameArgs.viewerProgressReportEnabled || isDuringPaintStroke) ) {
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
        optimizeRectsToRender(this, inputsRoDIntersectionPixel, rectsLeftToRender, args.time, args.view, renderMappedScale, &planesToRender->rectsToRender);
    } else {
        for (std::list<RectI>::iterator it = rectsLeftToRender.begin(); it != rectsLeftToRender.end(); ++it) {
            RectToRender r;
            r.rect = *it;
            r.identityInput = 0;
            r.isIdentity = false;
            planesToRender->rectsToRender.push_back(r);
        }
    }

    bool hasSomethingToRender = !planesToRender->rectsToRender.empty();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End Determine rectangles left to render /////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Pre-render input images ////////////////////////////////////////////////////////////////

    ///Pre-render input images before allocating the image if we need to render
    {
        const ImageComponents & outComp = outputComponents.front();
        if ( outComp.isColorPlane() ) {
            planesToRender->outputPremult = getOutputPremultiplication();
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

            inputCode = renderInputImagesForRoI(requestPassData,
                                                useTransforms,
                                                args.time,
                                                args.view,
                                                rod,
                                                canonicalRoI,
                                                tls->currentRenderArgs.transformRedirections,
                                                args.mipMapLevel,
                                                renderMappedScale,
                                                renderScaleOneUpstreamIfRenderScaleSupportDisabled,
                                                byPassCache,
                                                *framesNeeded,
                                                *neededComps,
                                                &it->imgs,
                                                &it->inputRois);
        }
        if ( planesToRender->inputPremult.empty() ) {
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

                    planesToRender->inputPremult[it2->first] = inputPremult;
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
                for (std::vector<Natron::ImageComponents>::const_iterator it = outputComponents.begin(); it != outputComponents.end(); ++it) {
                    if ( it->isColorPlane() ) {
                        components = &(*it);
                        break;
                    }
                }
            }

            assert(components);
            getImageFromCacheAndConvertIfNeeded(createInCache, useDiskCacheNode, key, renderMappedMipMapLevel,
                                                renderFullScaleThenDownscale ? &upscaledImageBounds : &downscaledImageBounds,
                                                &rod,
                                                args.bitdepth, it->first,
                                                outputDepth, *components,
                                                args.inputImagesList, tls->frameArgs.stats, &it->second.fullscaleImage);

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

        isPlaneCached = planesToRender->planes.begin()->second.fullscaleImage;

        if (!isPlaneCached) {
            planesToRender->rectsToRender.clear();
            rectsLeftToRender.clear();
            if (tls->frameArgs.tilesSupported) {
                rectsLeftToRender.push_back(roi);
            } else {
                rectsLeftToRender.push_back(renderFullScaleThenDownscale ? upscaledImageBounds : downscaledImageBounds);
            }


            if ( tryIdentityOptim && !rectsLeftToRender.empty() ) {
                optimizeRectsToRender(this, inputsRoDIntersectionPixel, rectsLeftToRender, args.time, args.view, renderMappedScale, &planesToRender->rectsToRender);
            } else {
                for (std::list<RectI>::iterator it = rectsLeftToRender.begin(); it != rectsLeftToRender.end(); ++it) {
                    RectToRender r;
                    r.rect = *it;
                    r.identityTime = 0;
                    r.identityInput = 0;
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

                RenderRoIRetCode inputRetCode = renderInputImagesForRoI(requestPassData,
                                                                        useTransforms,
                                                                        args.time,
                                                                        args.view,
                                                                        rod,
                                                                        canonicalRoI,
                                                                        tls->currentRenderArgs.transformRedirections,
                                                                        args.mipMapLevel,
                                                                        renderMappedScale,
                                                                        renderScaleOneUpstreamIfRenderScaleSupportDisabled,
                                                                        byPassCache,
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
    } // if (redoCacheLookup) {

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End 2nd cache lookup ///////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Allocate planes in the cache ////////////////////////////////////////////////////////////

    ///For all planes, if needed allocate the associated image
    if (hasSomethingToRender) {
        for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planesToRender->planes.begin();
             it != planesToRender->planes.end(); ++it) {
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
            if (!components) {
                continue;
            }

            if (!it->second.fullscaleImage) {
                ///The image is not cached
                allocateImagePlane(key, rod, downscaledImageBounds, upscaledImageBounds, isProjectFormat, *framesNeeded, *components, args.bitdepth, par, args.mipMapLevel, renderFullScaleThenDownscale, useDiskCacheNode, createInCache, &it->second.fullscaleImage, &it->second.downscaleImage);
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
                
                if (args.calledFromGetImage) {
                    /*
                     * When called from EffectInstance::getImage() we must prevent from taking any write lock because
                     * this image probably already has a lock for read on it. To overcome the write lock, we resize in a
                     * separate image and then we swap the images in the cache directly, without taking the image write lock.
                     */
                    
                    hasResized = it->second.fullscaleImage->copyAndResizeIfNeeded(renderFullScaleThenDownscale ? upscaledImageBounds : downscaledImageBounds,fillGrownBoundsWithZeroes, fillGrownBoundsWithZeroes,&it->second.cacheSwapImage);
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
                    hasResized = it->second.fullscaleImage->ensureBounds(renderFullScaleThenDownscale ? upscaledImageBounds : downscaledImageBounds,
                                                                         fillGrownBoundsWithZeroes, fillGrownBoundsWithZeroes);
                }
                
                
                /*
                 * Note that the image has been resized and the bitmap explicitly set to 1 in the newly allocated portions (for rotopaint purpose).
                 * We must reset it back to 0 in the last stroke tick RoD.
                 */
                if (hasResized && fillGrownBoundsWithZeroes) {
                    it->second.fullscaleImage->clearBitmap(lastStrokePixelRoD);
                }

                if ( renderFullScaleThenDownscale && (it->second.fullscaleImage->getMipMapLevel() == 0) ) {
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
    assert( !planesToRender->planes.empty() );

    ///If we reach here, it can be either because the planes are cached or not, either way
    ///the planes are NOT a total identity, and they may have some content left to render.
    EffectInstance::RenderRoIStatusEnum renderRetCode = eRenderRoIStatusImageAlreadyRendered;
    bool renderAborted;

    if (!hasSomethingToRender && !planesToRender->isBeingRenderedElsewhere) {
        renderAborted = _imp->aborted(tls);
    } else {
#if NATRON_ENABLE_TRIMAP
        ///Only use trimap system if the render cannot be aborted.
        if (!tls->frameArgs.canAbort && tls->frameArgs.isRenderResponseToUserInteraction) {
            for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
                _imp->markImageAsBeingRendered(renderFullScaleThenDownscale ? it->second.fullscaleImage : it->second.downscaleImage);
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
            Natron::RenderSafetyEnum safety = tls->frameArgs.currentThreadSafety;
            if (safety == eRenderSafetyInstanceSafe) {
                locker.reset( new QMutexLocker( &getNode()->getRenderInstancesSharedMutex() ) );
            } else if (safety == eRenderSafetyUnsafe) {
                const Natron::Plugin* p = getNode()->getPlugin();
                assert(p);
                locker.reset( new QMutexLocker( p->getPluginLock() ) );
            }
            ///For eRenderSafetyFullySafe, don't take any lock, the image already has a lock on itself so we're sure it can't be written to by 2 different threads.

            if ( tls->frameArgs.stats && tls->frameArgs.stats->isInDepthProfilingEnabled() ) {
                tls->frameArgs.stats->setGlobalRenderInfosForNode(getNode(), rod, planesToRender->outputPremult, processChannels, tls->frameArgs.tilesSupported, !renderFullScaleThenDownscale, renderMappedMipMapLevel);
            }

# ifdef DEBUG

            /*{
                const std::list<RectToRender>& rectsToRender = planesToRender->rectsToRender;
                qDebug() <<'('<<QThread::currentThread()<<")--> "<< getNode()->getScriptName_mt_safe().c_str() << ": render view: " << args.view << ", time: " << args.time << " No. tiles: " << rectsToRender.size() << " rectangles";
                for (std::list<RectToRender>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
                    qDebug() << "rect: " << "x1= " <<  it->rect.x1 << " , y1= " << it->rect.y1 << " , x2= " << it->rect.x2 << " , y2= " << it->rect.y2 << "(identity:" << it->isIdentity << ")";
                }
                for (std::map<Natron::ImageComponents, PlaneToRender> ::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
                    qDebug() << "plane: " <<  it->second.downscaleImage.get() << it->first.getLayerName().c_str();
                }
                qDebug() << "Cached:" << (isPlaneCached.get() != 0) << "Rendered elsewhere:" << planesToRender->isBeingRenderedElsewhere;

               }*/
# endif
            renderRetCode = renderRoIInternal(args.time,
                                              tls->frameArgs,
                                              safety,
                                              args.mipMapLevel,
                                              args.view,
                                              rod,
                                              par,
                                              planesToRender,
                                              tls->frameArgs.isSequentialRender,
                                              tls->frameArgs.isRenderResponseToUserInteraction,
                                              nodeHash,
                                              renderFullScaleThenDownscale,
                                              byPassCache,
                                              outputDepth,
                                              outputClipPrefComps,
                                              neededComps,
                                              processChannels);
        } // if (hasSomethingToRender) {

        renderAborted = _imp->aborted(tls);
#if NATRON_ENABLE_TRIMAP

        if (!tls->frameArgs.canAbort && tls->frameArgs.isRenderResponseToUserInteraction) {
            ///Only use trimap system if the render cannot be aborted.
            ///If we were aborted after all (because the node got deleted) then return a NULL image and empty the cache
            ///of this image
            for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
                if (!renderAborted) {
                    if ( (renderRetCode == eRenderRoIStatusRenderFailed) || !planesToRender->isBeingRenderedElsewhere ) {
                        _imp->unmarkImageAsBeingRendered(renderFullScaleThenDownscale ? it->second.fullscaleImage : it->second.downscaleImage,
                                                         renderRetCode == eRenderRoIStatusRenderFailed);
                    } else {
                        if ( !_imp->waitForImageBeingRenderedElsewhereAndUnmark(roi,
                                                                                renderFullScaleThenDownscale ? it->second.fullscaleImage : it->second.downscaleImage) ) {
                            renderAborted = true;
                        }
                    }
                } else {
                    appPTR->removeFromNodeCache(renderFullScaleThenDownscale ? it->second.fullscaleImage : it->second.downscaleImage);
                    _imp->unmarkImageAsBeingRendered(renderFullScaleThenDownscale ? it->second.fullscaleImage : it->second.downscaleImage, true);

                    return eRenderRoIRetCodeAborted;
                }
            }
        }
#endif
    } // if (!hasSomethingToRender && !planesToRender->isBeingRenderedElsewhere) {


    if ( renderAborted && (renderRetCode != eRenderRoIStatusImageAlreadyRendered) ) {
        ///Return a NULL image

        if (isDuringPaintStroke) {
            //We know the image will never be used ever again
            getNode()->removeAllImagesFromCache(false);
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

        for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
            if (!tls->frameArgs.tilesSupported) {
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
             If crashing on this assert this is likely due to a bug of the Trimap system. 
             Most likely another thread started rendering the portion that is in restToRender but did not fill the bitmap with 1
             yet. Do not remove this assert, there should never be 2 threads running concurrently renderHandler for the same roi 
             on the same image.
             */
            if (!restToRender.empty()) {
                it->second.downscaleImage->printUnrenderedPixels(roi);
            }
            assert( restToRender.empty() );
        }
    }
#endif

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////// Make sure all planes rendered have the requested mipmap level and format ///////////////////////////

    bool useAlpha0ForRGBToRGBAConversion = args.caller ? args.caller->getNode()->usesAlpha0ToConvertFromRGBToRGBA() : false;
    //bool multiplanar = isMultiPlanar();
    for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
        
        //If we have worked on a local swaped image, swap it in the cache
        if (it->second.cacheSwapImage) {
            const CacheAPI* cache = it->second.cacheSwapImage->getCacheAPI();
            const Cache<Image>* imgCache = dynamic_cast<const Cache<Image>*>(cache);
            if (imgCache) {
                Cache<Image>* ccImgCache = const_cast<Cache<Image>*>(imgCache);
                assert(ccImgCache);
                ccImgCache->swapOrInsert(it->second.cacheSwapImage, it->second.fullscaleImage);
            }
        }
        
        //We have to return the downscale image, so make sure it has been computed
        if ( (renderRetCode != eRenderRoIStatusRenderFailed) &&
            renderFullScaleThenDownscale &&
            it->second.fullscaleImage->getMipMapLevel() != mipMapLevel &&
            !hasSomethingToRender) {
            assert(it->second.fullscaleImage->getMipMapLevel() == 0);
            if (it->second.downscaleImage == it->second.fullscaleImage) {
                it->second.downscaleImage.reset( new Image(it->second.fullscaleImage->getComponents(),
                                                           it->second.fullscaleImage->getRoD(),
                                                           downscaledImageBounds,
                                                           args.mipMapLevel,
                                                           it->second.fullscaleImage->getPixelAspectRatio(),
                                                           it->second.fullscaleImage->getBitDepth(),
                                                           false) );
            }

            it->second.fullscaleImage->downscaleMipMap( it->second.fullscaleImage->getRoD(), originalRoI, 0, args.mipMapLevel, false, it->second.downscaleImage.get() );
        }
        
        ///The image might need to be converted to fit the original requested format
        it->second.downscaleImage = convertPlanesFormatsIfNeeded(getApp(), it->second.downscaleImage, originalRoI, it->first, args.bitdepth, useAlpha0ForRGBToRGBAConversion, planesToRender->outputPremult, -1);
        
        assert(it->second.downscaleImage->getComponents() == it->first && it->second.downscaleImage->getBitDepth() == args.bitdepth);
        outputPlanes->push_back(it->second.downscaleImage);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////// End requested format convertion ////////////////////////////////////////////////////////////////////


    ///// Termination
#ifdef DEBUG
    if (outputPlanes->size() != args.components.size()) {
        qDebug() << "Requested:";
        for (std::list<ImageComponents>::const_iterator it = args.components.begin(); it!=args.components.end(); ++it) {
            qDebug() << it->getLayerName().c_str();
        }
        qDebug() << "But rendered:";
        for (ImageList::iterator it = outputPlanes->begin(); it!=outputPlanes->end(); ++it) {
            if (*it) {
                qDebug() << (*it)->getComponents().getLayerName().c_str();
            }
        }
    }
#endif
    assert(outputPlanes->size() == args.components.size());
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
                                  const boost::shared_ptr<ImagePlanesToRender> & planesToRender,
                                  bool isSequentialRender,
                                  bool isRenderMadeInResponseToUserInteraction,
                                  U64 nodeHash,
                                  bool renderFullScaleThenDownscale,
                                  bool byPassCache,
                                  Natron::ImageBitDepthEnum outputClipPrefDepth,
                                  const std::list<Natron::ImageComponents> & outputClipPrefsComps,
                                  const boost::shared_ptr<ComponentsNeededMap> & compsNeeded,
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
    if ( isReader() && ( QThread::currentThread() != qApp->thread() ) ) {
        Format frmt;
        RectI pixelRoD;
        rod.toPixelEnclosing(0, par, &pixelRoD);
        frmt.set(pixelRoD);
        frmt.setPixelAspectRatio(par);
        getApp()->getProject()->setOrAddProjectFormat(frmt);
    }

    unsigned int renderMappedMipMapLevel = 0;

    for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
        it->second.renderMappedImage = renderFullScaleThenDownscale ? it->second.fullscaleImage : it->second.downscaleImage;
        if ( it == planesToRender->planes.begin() ) {
            renderMappedMipMapLevel = it->second.renderMappedImage->getMipMapLevel();
        }
    }

    RenderScale renderMappedScale(Image::getScaleFromMipMapLevel(renderMappedMipMapLevel));


    RenderingFunctorRetEnum renderStatus = eRenderingFunctorRetOK;
    if ( planesToRender->rectsToRender.empty() ) {
        retCode = EffectInstance::eRenderRoIStatusImageAlreadyRendered;
    } else {
        retCode = EffectInstance::eRenderRoIStatusImageRendered;
    }


    ///Notify the gui we're rendering
    boost::shared_ptr<NotifyRenderingStarted_RAII> renderingNotifier;
    if ( !planesToRender->rectsToRender.empty() ) {
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


    boost::shared_ptr<std::map<boost::shared_ptr<Natron::Node>, ParallelRenderArgs > > tlsCopy;
    if (safety == eRenderSafetyFullySafeFrame) {
        tlsCopy.reset(new std::map<boost::shared_ptr<Natron::Node>, ParallelRenderArgs >);
        /*
         * Since we're about to start new threads potentially, copy all the thread local storage on all nodes (any node may be involved in
         * expressions, and we need to retrieve the exact local time of render).
         */
        getApp()->getProject()->getParallelRenderArgs(*tlsCopy);
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


    if (renderStatus != eRenderingFunctorRetFailed) {
        if (safety == eRenderSafetyFullySafeFrame && planesToRender->rectsToRender.size() > 1) {

            
            const QThread* currentThread = QThread::currentThread();

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


#ifdef NATRON_HOSTFRAMETHREADING_SEQUENTIAL
            std::vector<EffectInstance::RenderingFunctorRetEnum> ret( tiledData.size() );
            int i = 0;
            for (std::list<RectToRender>::const_iterator it = planesToRender->rectsToRender.begin(); it != planesToRender->rectsToRender.end(); ++it, ++i) {
                ret[i] = tiledRenderingFunctor(tiledArgs,
                                               *it,
                                               currentThread);
            }
            std::vector<EffectInstance::RenderingFunctorRetEnum>::const_iterator it2;

#else


            QFuture<RenderingFunctorRetEnum> ret = QtConcurrent::mapped( planesToRender->rectsToRender,
                                                                        boost::bind(&EffectInstance::Implementation::tiledRenderingFunctor,
                                                                                    _imp.get(),
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
                }
            }
        } else {
            for (std::list<RectToRender>::const_iterator it = planesToRender->rectsToRender.begin(); it != planesToRender->rectsToRender.end(); ++it) {
                RenderingFunctorRetEnum functorRet = _imp->tiledRenderingFunctor(*it,  renderFullScaleThenDownscale, isSequentialRender, isRenderMadeInResponseToUserInteraction, firstFrame, lastFrame, preferredInput, mipMapLevel, renderMappedMipMapLevel, rod, time, view, par, byPassCache, outputClipPrefDepth, outputClipPrefsComps, compsNeeded, processChannels, planesToRender);

                if ( (functorRet == eRenderingFunctorRetFailed) || (functorRet == eRenderingFunctorRetAborted) ) {
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
