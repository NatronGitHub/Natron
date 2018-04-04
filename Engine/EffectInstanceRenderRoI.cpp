/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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
#include "Engine/ThreadPool.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

//#define NATRON_ALWAYS_ALLOCATE_FULL_IMAGE_BOUNDS


NATRON_NAMESPACE_ENTER

/*
 * @brief Split all rects to render in smaller rects and check if each one of them is identity.
 * For identity rectangles, we just call renderRoI again on the identity input in the tiledRenderingFunctor.
 * For non-identity rectangles, compute the bounding box of them and render it
 */
static void
optimizeRectsToRender(EffectInstance* self,
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
                                             const ImagePlaneDesc& targetComponents,
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
#if 0 //def BOOST_NO_CXX11_VARIADIC_TEMPLATES
       ImagePtr tmp( new Image(targetComponents,
                                inputImage->getRoD(),
                                bounds,
                                inputImage->getMipMapLevel(),
                                inputImage->getPixelAspectRatio(),
                                targetDepth,
                                inputImage->getPremultiplication(),
                                inputImage->getFieldingOrder(),
                                false) );
#else
        ImagePtr tmp = boost::make_shared<Image>(targetComponents,
                                                 inputImage->getRoD(),
                                                 bounds,
                                                 inputImage->getMipMapLevel(),
                                                 inputImage->getPixelAspectRatio(),
                                                 targetDepth,
                                                 inputImage->getPremultiplication(),
                                                 inputImage->getFieldingOrder(),
                                                 false);

#endif
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
}

#if NATRON_ENABLE_TRIMAP
class ImageBitMapMarker_RAII
{

    std::map<ImagePlaneDesc, EffectInstance::PlaneToRender> _image;
    RectI _roi;
    EffectInstance* _effect;
    std::list<RectI> _rectsToRender;
    bool _isBeingRenderedElseWhere;
    bool _isValid;
    bool _renderFullScale;

public:

    ImageBitMapMarker_RAII(const std::map<ImagePlaneDesc, EffectInstance::PlaneToRender>& image,
                           bool renderFullScale,
                           const RectI& roi,
                           EffectInstance* effect)
    : _image(image)
    , _roi(roi)
    , _effect(effect)
    , _rectsToRender()
    , _isBeingRenderedElseWhere(false)
    , _isValid(true)
    , _renderFullScale(renderFullScale)
    {
        for (std::map<ImagePlaneDesc,EffectInstance::PlaneToRender>::const_iterator it = _image.begin(); it != _image.end(); ++it) {
            ImagePtr cacheImage;
            if (!renderFullScale) {
                cacheImage = it->second.downscaleImage;
            } else {
                cacheImage = it->second.fullscaleImage;
            }
            if (cacheImage && cacheImage->usesBitMap()) {
                _effect->_imp->markImageAsBeingRendered(cacheImage, roi, &_rectsToRender, &_isBeingRenderedElseWhere);
            }
        }

    }

    const std::list<RectI>& getRectsToRender() const
    {
        return _rectsToRender;
    }

    void invalidate()
    {
        _isValid = false;
    }

    void waitForPendingRegions()
    {
        if (!_isBeingRenderedElseWhere || !_isValid) {
            return;
        }
        for (std::map<ImagePlaneDesc,EffectInstance::PlaneToRender>::const_iterator it = _image.begin(); it != _image.end(); ++it) {
            ImagePtr cacheImage;
            if (!_renderFullScale) {
                cacheImage = it->second.downscaleImage;
            } else {
                cacheImage = it->second.fullscaleImage;
            }
            if (cacheImage && cacheImage->usesBitMap()) {
                if (!_effect->_imp->waitForImageBeingRenderedElsewhere(_roi, cacheImage)) {
                    _isValid = false;
                }
            }
        }
    }

    ~ImageBitMapMarker_RAII()
    {
        for (std::map<ImagePlaneDesc,EffectInstance::PlaneToRender>::const_iterator it = _image.begin(); it != _image.end(); ++it) {
            ImagePtr cacheImage;
            if (!_renderFullScale) {
                cacheImage = it->second.downscaleImage;
            } else {
                cacheImage = it->second.fullscaleImage;
            }
            if (cacheImage && cacheImage->usesBitMap()) {
                _effect->_imp->unmarkImageAsBeingRendered(cacheImage, _rectsToRender, !_isValid);
            }
        }
 
    }
};
#endif // #if NATRON_ENABLE_TRIMAP

EffectInstance::RenderRoIRetCode
EffectInstance::renderRoI(const RenderRoIArgs & args,
                          std::map<ImagePlaneDesc, ImagePtr>* outputPlanes)
{
    //Do nothing if no components were requested
    if ( args.components.empty() ) {
        qDebug() << getScriptName_mt_safe().c_str() << "renderRoi: Early bail-out components requested empty";

        return eRenderRoIRetCodeOk;
    }
    if ( args.roi.isNull() ) {
        qDebug() << getScriptName_mt_safe().c_str() << "renderRoi: Early bail-out ROI requested empty ";

        return eRenderRoIRetCodeOk;
    }

    // Make sure this call is not made recursively from getImage on a render clone on which we are already calling renderRoI.
    // If so, forward the call to the main instance
    if (_imp->mainInstance) {
        return _imp->mainInstance->renderRoI(args, outputPlanes);
    }

    //Create the TLS data for this node if it did not exist yet
    EffectTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
    assert(tls);
    OSGLContextPtr glContext;
    AbortableRenderInfoPtr abortInfo;
    ParallelRenderArgsPtr  frameArgs;
    if ( tls->frameArgs.empty() ) {
        qDebug() << QThread::currentThread() << "[BUG]:" << getScriptName_mt_safe().c_str() <<  "Thread-storage for the render of the frame was not set.";

        frameArgs = boost::make_shared<ParallelRenderArgs>();
        {
            NodesWList outputs;
            getNode()->getOutputs_mt_safe(outputs);
            frameArgs->visitsCount = (int)outputs.size();
        }
        frameArgs->time = args.time;
        frameArgs->nodeHash = getHash();
        frameArgs->view = args.view;
        frameArgs->isSequentialRender = false;
        frameArgs->isRenderResponseToUserInteraction = true;
        tls->frameArgs.push_back(frameArgs);
    } else {
        //The hash must not have changed if we did a pre-pass.
        frameArgs = tls->frameArgs.back();
        glContext = frameArgs->openGLContext.lock();
        abortInfo = frameArgs->abortInfo.lock();
        if (!abortInfo) {
            // If we don't have info to identify the render, we cannot manage the OpenGL context properly, so don't try to render with OpenGL.
            glContext.reset();
        }
        assert(!frameArgs->request || frameArgs->nodeHash == frameArgs->request->nodeHash);
    }

    ///For writer we never want to cache otherwise the next time we want to render it will skip writing the image on disk!
    bool byPassCache = args.byPassCache;

    ///Use the hash at this time, and then copy it to the clips in the thread local storage to use the same value
    ///through all the rendering of this frame.
    U64 nodeHash = frameArgs->nodeHash;
    const double par = getAspectRatio(-1);
    const ImageFieldingOrderEnum fieldingOrder = getFieldingOrder();
    const ImagePremultiplicationEnum thisEffectOutputPremult = getPremult();
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
    RenderScale renderMappedScale( Image::getScaleFromMipMapLevel(renderMappedMipMapLevel) );
    assert( !( (supportsRS == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );


    const FrameViewRequest* requestPassData = 0;
    if (frameArgs->request) {
        requestPassData = frameArgs->request->getFrameViewRequest(args.time, args.view);
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
    ComponentsNeededMapPtr neededComps = boost::make_shared<ComponentsNeededMap>();
    ComponentsNeededMap::iterator foundOutputNeededComps;
    std::bitset<4> processChannels;
    std::list<ImagePlaneDesc> passThroughPlanes;
    int ptInputNb;
    double ptTime;
    int ptView;
    {
        bool processAllComponentsRequested;

        {

            getComponentsNeededAndProduced_public(nodeHash, args.time, args.view, neededComps.get(), &passThroughPlanes, &processAllComponentsRequested, &ptTime, &ptView, &processChannels, &ptInputNb);


            foundOutputNeededComps = neededComps->find(-1);
            if ( foundOutputNeededComps == neededComps->end() ) {
                return eRenderRoIRetCodeOk;
            }
        }
        if (processAllComponentsRequested) {
            std::list<ImagePlaneDesc> compVec;
            for (std::list<ImagePlaneDesc>::const_iterator it = args.components.begin(); it != args.components.end(); ++it) {
                bool found = false;
                //Change all needed comps in output to the requested components
                for (std::list<ImagePlaneDesc>::const_iterator it2 = foundOutputNeededComps->second.begin(); it2 != foundOutputNeededComps->second.end(); ++it2) {
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
    const std::list<ImagePlaneDesc> & outputComponents = foundOutputNeededComps->second;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Handle pass-through for planes //////////////////////////////////////////////////////////
    std::list<ImagePlaneDesc> requestedComponents;
    {
        {

            EffectInstancePtr passThroughInput = getInput(ptInputNb);
            /*
             * For all requested planes, check which components can be produced in output by this node.
             * If the components are from the color plane, if another set of components of the color plane is present
             * we try to render with those instead.
             */
            for (std::list<ImagePlaneDesc>::const_iterator it = args.components.begin(); it != args.components.end(); ++it) {
                // We may not request paired layers
                assert(it->getNumComponents() > 0);


                std::list<ImagePlaneDesc>::const_iterator foundInOutputComps = ImagePlaneDesc::findEquivalentLayer(*it, outputComponents.begin(), outputComponents.end());
                if (foundInOutputComps != outputComponents.end()) {
                    requestedComponents.push_back(*foundInOutputComps);
                } else {
                    std::list<ImagePlaneDesc>::iterator foundEquivalent = ImagePlaneDesc::findEquivalentLayer(*it, passThroughPlanes.begin(), passThroughPlanes.end());


                    // If  the requested component is not present, then it will just return black and transparent to the plug-in.
                    if (foundEquivalent != passThroughPlanes.end()) {
                        boost::scoped_ptr<RenderRoIArgs> inArgs ( new RenderRoIArgs(args) );
                        inArgs->preComputedRoD.clear();
                        inArgs->components.clear();
                        inArgs->components.push_back(*foundEquivalent);
                        inArgs->time = ptTime;
                        inArgs->view = ViewIdx(ptView);

                        if (!passThroughInput) {
                            return eRenderRoIRetCodeFailed;
                        }

                        std::map<ImagePlaneDesc, ImagePtr> inputPlanes;
                        RenderRoIRetCode inputRetCode = passThroughInput->renderRoI(*inArgs, &inputPlanes);
                        assert( inputPlanes.size() == 1 || inputPlanes.empty() );
                        if ( (inputRetCode == eRenderRoIRetCodeAborted) || (inputRetCode == eRenderRoIRetCodeFailed) || inputPlanes.empty() ) {
                            return inputRetCode;
                        }
                        outputPlanes->insert( std::make_pair(*it, inputPlanes.begin()->second) );
                    }
                }
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
        ViewIdx inputIdentityView(args.view);
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
                    inputIdentityView = requestPassData->globalData.identityView;
                } else {
                    identity = isIdentity_public(true, nodeHash, args.time, renderMappedScale, pixelRod, args.view, &inputTimeIdentity, &inputIdentityView, &inputNbIdentity);
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
                    boost::scoped_ptr<RenderRoIArgs> argCpy ( new RenderRoIArgs(args) );
                    argCpy->time = inputTimeIdentity;

                    if (viewInvariance == eViewInvarianceAllViewsInvariant) {
                        argCpy->view = ViewIdx(0);
                    } else {
                        argCpy->view = inputIdentityView;
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

            EffectInstancePtr inputEffectIdentity = getInput(inputNbIdentity);
            if (inputEffectIdentity) {
                if ( frameArgs->stats && frameArgs->stats->isInDepthProfilingEnabled() ) {
                    frameArgs->stats->setNodeIdentity( getNode(), inputEffectIdentity->getNode() );
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

                bool fetchUserSelectedComponentsUpstream = getNode()->getChannelSelectorKnob(inputNbIdentity).get() != 0;

                if (fetchUserSelectedComponentsUpstream) {
                    /// This corresponds to choice B)
                    EffectInstance::ComponentsNeededMap::const_iterator foundCompsNeeded = neededComps->find(inputNbIdentity);
                    if ( foundCompsNeeded != neededComps->end() ) {
                        inputArgs->components.clear();
                        for (std::list<ImagePlaneDesc>::const_iterator it = foundCompsNeeded->second.begin(); it != foundCompsNeeded->second.end(); ++it) {
                            if (it->getNumComponents() != 0) {
                                inputArgs->components.push_back(*it);
                            }
                        }
                    }
                } else {
                    /// This corresponds to choice A)
                    inputArgs->components = requestedComponents;
                }


                std::map<ImagePlaneDesc, ImagePtr> identityPlanes;
                RenderRoIRetCode ret =  inputEffectIdentity->renderRoI(*inputArgs, &identityPlanes);
                if (ret == eRenderRoIRetCodeOk) {
                    outputPlanes->insert( identityPlanes.begin(), identityPlanes.end() );

                    if (fetchUserSelectedComponentsUpstream) {
                        // We fetched potentially different components, so convert them to the format requested
                        std::map<ImagePlaneDesc, ImagePtr> convertedPlanes;
                        AppInstancePtr app = getApp();
                        bool useAlpha0ForRGBToRGBAConversion = args.caller ? args.caller->getNode()->usesAlpha0ToConvertFromRGBToRGBA() : false;
                        std::list<ImagePlaneDesc>::const_iterator compIt = args.components.begin();

                        for (std::map<ImagePlaneDesc, ImagePtr>::iterator it = outputPlanes->begin(); it != outputPlanes->end(); ++it, ++compIt) {
                            ImagePremultiplicationEnum premult;
                            const ImagePlaneDesc & outComp = outputComponents.front();
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

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////// End identity check ///////////////////////////////////////////////////////////////


        // At this point, if only the pass through planes are view variant and the rendered view is different than 0,
        // just call renderRoI again for the components left to render on the view 0.
        if ( (args.view != 0) && (viewInvariance == eViewInvarianceOnlyPassThroughPlanesVariant) ) {
            boost::scoped_ptr<RenderRoIArgs> argCpy( new RenderRoIArgs(args) );
            argCpy->view = ViewIdx(0);
            argCpy->preComputedRoD.clear();

            return renderRoI(*argCpy, outputPlanes);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Transform concatenations ///////////////////////////////////////////////////////////////
    ///Try to concatenate transform effects
    bool useTransforms;

    if (requestPassData) {
        tls->currentRenderArgs.transformRedirections = requestPassData->globalData.transforms;
        useTransforms = tls->currentRenderArgs.transformRedirections && !tls->currentRenderArgs.transformRedirections->empty();
    } else {
        SettingsPtr settings = appPTR->getCurrentSettings();
        useTransforms = settings && settings->isTransformConcatenationEnabled();
        if (useTransforms) {
            tls->currentRenderArgs.transformRedirections = boost::make_shared<InputMatrixMap>();
            tryConcatenateTransforms( args.time, frameArgs->draftMode, args.view, args.scale, tls->currentRenderArgs.transformRedirections.get() );
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
        if (frameArgs->tilesSupported) {
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
    if (!frameArgs->tilesSupported) {
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
    const PluginOpenGLRenderSupport openGLSupport = frameArgs->currentOpenglSupport;
    StorageModeEnum storage = eStorageModeRAM;
    OSGLContextAttacherPtr glContextLocker;

    if ( dynamic_cast<DiskCacheNode*>(this) ) {
        storage = eStorageModeDisk;
    } else if ( glContext && ( (openGLSupport == ePluginOpenGLRenderSupportNeeded) ||
                             ( ( openGLSupport == ePluginOpenGLRenderSupportYes) && args.allowGPURendering) ) ) {
        // Enable GPU render if the plug-in cannot render another way or if all conditions are met

        if (openGLSupport == ePluginOpenGLRenderSupportNeeded && !getNode()->getPlugin()->isOpenGLEnabled()) {
            QString message = tr("OpenGL render is required for  %1 but was disabled in the Preferences for this plug-in, please enable it and restart %2").arg(QString::fromUtf8(getNode()->getLabel().c_str())).arg(QString::fromUtf8(NATRON_APPLICATION_NAME));
            setPersistentMessage(eMessageTypeError, message.toStdString());
            return eRenderRoIRetCodeFailed;
        }

        /*
           We only render using OpenGL if this effect is the preferred input of the calling node (to avoid recursions in the graph
           since we do not use the cache for textures)
         */
        // Make the OpenGL context current to this thread
        glContextLocker = boost::make_shared<OSGLContextAttacher>(glContext, abortInfo
#ifdef DEBUG
                                                                  , frameArgs->time
#endif
                                                                  );
        storage = eStorageModeGLTex;

        // If the plug-in knows how to render on CPU, check if we actually should not render on CPU instead.
        if (openGLSupport == ePluginOpenGLRenderSupportYes) {
            // User want to force caching of this node but we cannot cache OpenGL renders, so fallback on CPU.
            if ( getNode()->isForceCachingEnabled() ) {
                storage = eStorageModeRAM;
                glContextLocker.reset();
            }

            // If a node has multiple outputs, do not render it on OpenGL since we do not use the cache. We could end-up with this render being executed multiple times.
            // Also, if the render time is different from the caller render time, don't render using OpenGL otherwise we could computed this render multiple times.

            if (storage == eStorageModeGLTex) {
                if ( (frameArgs->visitsCount > 1) ||
                     ( args.time != args.callerRenderTime) ) {
                    storage = eStorageModeRAM;
                    glContextLocker.reset();
                }
            }

            // Ensure that the texture will be at least smaller than the maximum OpenGL texture size
            if (storage == eStorageModeGLTex) {
                int maxTextureSize = appPTR->getGPUContextPool()->getCurrentOpenGLRendererMaxTextureSize();
                if ( (roi.width() >= maxTextureSize) ||
                     ( roi.height() >= maxTextureSize) ) {
                    // Fallback on CPU rendering since the image is larger than the maximum allowed OpenGL texture size
                    storage = eStorageModeRAM;
                    glContextLocker.reset();
                }
            }
        }
        if (storage == eStorageModeGLTex) {
            // OpenGL renders always support render scale...
            if (renderFullScaleThenDownscale) {
                renderFullScaleThenDownscale = false;
                renderMappedMipMapLevel = args.mipMapLevel;
                renderMappedScale.x = renderMappedScale.y = Image::getScaleFromMipMapLevel(renderMappedMipMapLevel);
                if (frameArgs->tilesSupported) {
                    roi = args.roi;
                    if ( !roi.intersect(downscaledImageBoundsNc, &roi) ) {
                        return eRenderRoIRetCodeOk;
                    }
                } else {
                    roi = downscaledImageBoundsNc;
                }
            }
        }
    }


    const bool draftModeSupported = getNode()->isDraftModeUsed();
    const bool isFrameVaryingOrAnimated = isFrameVaryingOrAnimated_Recursive();
    bool createInCache;
    // Do not use the cache for OpenGL rendering
    if (storage == eStorageModeGLTex) {
        createInCache = false;
    } else {
        // in Analysis, the node upstream of te analysis node should always cache
        createInCache = (frameArgs->isAnalysis && frameArgs->treeRoot->getEffectInstance().get() == args.caller) ? true : shouldCacheOutput(isFrameVaryingOrAnimated, args.time, args.view, frameArgs->visitsCount);
    }
    ///Do we want to render the graph upstream at scale 1 or at the requested render scale ? (user setting)
    bool renderScaleOneUpstreamIfRenderScaleSupportDisabled = getNode()->useScaleOneImagesWhenRenderScaleSupportIsDisabled();
    ///For multi-resolution we want input images with exactly the same size as the output image
    if ( !renderScaleOneUpstreamIfRenderScaleSupportDisabled && !supportsMultiResolution() ) {
        renderScaleOneUpstreamIfRenderScaleSupportDisabled = true;
    }

    boost::scoped_ptr<ImageKey> key( new ImageKey(getNode().get(),
                                                  nodeHash,
                                                  isFrameVaryingOrAnimated,
                                                  args.time,
                                                  args.view,
                                                  1.,
                                                  draftModeSupported && frameArgs->draftMode,
                                                  renderMappedMipMapLevel == 0 && !renderScaleOneUpstreamIfRenderScaleSupportDisabled) );
    boost::scoped_ptr<ImageKey> nonDraftKey( new ImageKey(getNode().get(),
                                                          nodeHash,
                                                          isFrameVaryingOrAnimated,
                                                          args.time,
                                                          args.view,
                                                          1.,
                                                          false,
                                                          renderMappedMipMapLevel == 0 && !renderScaleOneUpstreamIfRenderScaleSupportDisabled) );

    bool isDuringPaintStroke = isDuringPaintStrokeCreationThreadLocal();

    /*
     * Get the bitdepth and output components that the plug-in expects to render. The cached image does not necesserarily has the bitdepth
     * that the plug-in expects.
     */
    ImageBitDepthEnum outputDepth = getBitDepth(-1);
    ImagePlaneDesc outputClipPrefComps, outputClipPrefCompsPaired;
    getMetadataComponents(-1, &outputClipPrefComps, &outputClipPrefCompsPaired);
    ImagePlanesToRenderPtr planesToRender = boost::make_shared<ImagePlanesToRender>();
    planesToRender->useOpenGL = storage == eStorageModeGLTex;
    FramesNeededMapPtr framesNeeded = boost::make_shared<FramesNeededMap>();
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Look-up the cache ///////////////////////////////////////////////////////////////

    {
        //If one plane is missing from cache, we will have to render it all. For all other planes, either they have nothing
        //left to render, otherwise we render them for all the roi again.
        bool missingPlane = false;

        for (std::list<ImagePlaneDesc>::iterator it = requestedComponents.begin(); it != requestedComponents.end(); ++it) {
            EffectInstance::PlaneToRender plane;

            /*
             * If the plane is the color plane, we might have to convert between components, hence we always
             * try to find in the cache the "preferred" components of this node for the color plane.
             * For all other planes, just consider this set of components, we do not allow conversion.
             */
            const ImagePlaneDesc* components = 0;
            if ( !it->isColorPlane() ) {
                components = &(*it);
            } else {
                for (std::list<ImagePlaneDesc>::const_iterator it2 = outputComponents.begin(); it2 != outputComponents.end(); ++it2) {
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
            bool doCacheLookup = !isWriter() || !frameArgs->isSequentialRender;
            if (doCacheLookup) {
                int nLookups = draftModeSupported && frameArgs->draftMode ? 2 : 1;

                // If the node does'nt support render scale, first lookup the cache with requested level, if not cached then lookup
                // with full scale
                unsigned int lookupMipMapLevel = (renderMappedMipMapLevel != mipMapLevel && !isDuringPaintStroke)? mipMapLevel : renderMappedMipMapLevel;
                for (int n = 0; n < nLookups; ++n) {
                    getImageFromCacheAndConvertIfNeeded(createInCache, storage, args.returnStorage, n == 0 ? *nonDraftKey : *key, lookupMipMapLevel,
                                                        &downscaledImageBounds,
                                                        &rod, args.roi,
                                                        args.bitdepth, *it,
                                                        args.inputImagesList,
                                                        frameArgs->stats,
                                                        glContextLocker,
                                                        &plane.fullscaleImage);
                    if (plane.fullscaleImage) {
                        if (byPassCache) {
                            if (plane.fullscaleImage) {
                                appPTR->removeFromNodeCache( key->getHash() );
                                plane.fullscaleImage.reset();
                            }
                        } else if (renderMappedMipMapLevel != mipMapLevel) {
                            // Only keep the cached image if it covers the roi
                            std::list<RectI> restToRender;
                            plane.fullscaleImage->getRestToRender(args.roi, restToRender);
                            if ( !restToRender.empty() ) {
                                plane.fullscaleImage.reset();
                            } else {
                                renderFullScaleThenDownscale = false;
                                renderMappedMipMapLevel = mipMapLevel;
                                roi = args.roi;
                            }
                        }
                        break;
                    }
                }
                if (!plane.fullscaleImage && renderMappedMipMapLevel != lookupMipMapLevel) {
                    // Not found at requested mipmap level, look at full scale
                    for (int n = 0; n < nLookups; ++n) {
                        getImageFromCacheAndConvertIfNeeded(createInCache, storage, args.returnStorage, n == 0 ? *nonDraftKey : *key, renderMappedMipMapLevel,
                                                            &upscaledImageBounds,
                                                            &rod, roi,
                                                            args.bitdepth, *it,
                                                            args.inputImagesList,
                                                            frameArgs->stats,
                                                            glContextLocker,
                                                            &plane.fullscaleImage);
                        if (plane.fullscaleImage) {
                            if (byPassCache) {
                                if (plane.fullscaleImage) {
                                    appPTR->removeFromNodeCache( key->getHash() );
                                    plane.fullscaleImage.reset();
                                }
                            }
                            break;
                        }

                    }
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
                    std::map<ImagePlaneDesc, EffectInstance::PlaneToRender> newPlanes;
                    for (std::map<ImagePlaneDesc, EffectInstance::PlaneToRender>::iterator it2 = planesToRender->planes.begin();
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

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////End cache lookup//////////////////////////////////////////////////////////

    /// Release the context from this thread as it may have been used  when calling getImageFromCacheAndConvertIfNeeded
    /// This will enable all threads to be concurrent again to render input images
    if (glContextLocker) {
        glContextLocker->dettach();
    }

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

#if NATRON_ENABLE_TRIMAP
    boost::scoped_ptr<ImageBitMapMarker_RAII> guard;
#endif

    if (!isPlaneCached) {
        if (frameArgs->tilesSupported) {
            rectsLeftToRender.push_back(roi);
        } else {
            rectsLeftToRender.push_back(renderFullScaleThenDownscale ? upscaledImageBounds : downscaledImageBounds);
        }
    } else {
        if ( isDuringPaintStroke && !lastStrokePixelRoD.isNull() ) {
            fillGrownBoundsWithZeroes = true;
            //Clear the bitmap of the cached image in the portion of the last stroke to only recompute what's needed
            for (std::map<ImagePlaneDesc, EffectInstance::PlaneToRender>::iterator it2 = planesToRender->planes.begin();
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
        // scoped_ptr
        guard.reset(new ImageBitMapMarker_RAII(planesToRender->planes, renderFullScaleThenDownscale, roi, this));
        rectsLeftToRender = guard->getRectsToRender();
#else // !NATRON_ENABLE_TRIMAP
        isPlaneCached->getRestToRender(roi, rectsLeftToRender);
#endif // NATRON_ENABLE_TRIMAP



        if ( isDuringPaintStroke && !rectsLeftToRender.empty() && !lastStrokePixelRoD.isNull() ) {
            rectsLeftToRender.clear();
            RectI intersection;
            if ( downscaledImageBounds.intersect(lastStrokePixelRoD, &intersection) ) {
                rectsLeftToRender.push_back(intersection);
            }
        }

        // If doing opengl renders, we don't allow retrieving partial images from the cache
        if ( !rectsLeftToRender.empty() && (planesToRender->useOpenGL) ) {
            ///The node cache is almost full and we need to render  something in the image, if we hold a pointer to this image here
            ///we might recursively end-up in this same situation at each level of the render tree, ending with all images of each level
            ///being held in memory.
            ///Our strategy here is to clear the pointer, hence allowing the cache to remove the image, and ask the inputs to render the full RoI
            ///instead of the rest to render. This way, even if the image is cleared from the cache we already have rendered the full RoI anyway.
            rectsLeftToRender.clear();
            rectsLeftToRender.push_back(roi);
            for (std::map<ImagePlaneDesc, EffectInstance::PlaneToRender>::iterator it2 = planesToRender->planes.begin(); it2 != planesToRender->planes.end(); ++it2) {
                //Keep track of the original cached image for the re-lookup afterward, if the pointer doesn't match the first look-up, don't consider
                //the image because the region to render might have changed and we might have to re-trigger a render on inputs again.

                ///Make sure to never dereference originalCachedImage! We only compare it (that's why it s a void*)
                it2->second.originalCachedImage = it2->second.fullscaleImage.get();
                it2->second.fullscaleImage.reset();
                it2->second.downscaleImage.reset();
            }
            isPlaneCached.reset();
        }


        ///If the effect doesn't support tiles and it has something left to render, just render the bounds again
        ///Note that it should NEVER happen because if it doesn't support tiles in the first place, it would
        ///have rendered the rod already.
        if (!frameArgs->tilesSupported && !rectsLeftToRender.empty() && isPlaneCached) {
            ///if the effect doesn't support tiles, just render the whole rod again even though
            rectsLeftToRender.clear();
            rectsLeftToRender.push_back(renderFullScaleThenDownscale ? upscaledImageBounds : downscaledImageBounds);
        }
    } // isPlaneCached

    /*
     * If the effect has multiple inputs (such as masks) try to call isIdentity if the RoDs do not intersect the RoI
     */
    bool tryIdentityOptim = false;
    RectI inputsRoDIntersectionPixel;
    if (frameArgs->tilesSupported && !rectsLeftToRender.empty() && isDuringPaintStroke) {
        RectD inputsIntersection;
        bool inputsIntersectionSet = false;
        bool hasDifferentRods = false;
        int maxInput = getNInputs();
        bool hasMask = false;
        RotoDrawableItemPtr attachedStroke = getNode()->getAttachedRotoItem();
        for (int i = 0; i < maxInput; ++i) {
            bool isMask = isInputMask(i) || isInputRotoBrush(i);
            RectD inputRod;
            if (attachedStroke && isMask) {
                getNode()->getPaintStrokeRoD(args.time, &inputRod);
                hasMask = true;
            } else {
                EffectInstancePtr input = getInput(i);
                if (!input) {
                    continue;
                }
                bool isProjectFormat;
                ParallelRenderArgsPtr inputFrameArgs = input->getParallelRenderArgsTLS();
                U64 inputHash = (inputFrameArgs) ? inputFrameArgs->nodeHash : input->getHash();
                StatusEnum stat = input->getRegionOfDefinition_public(inputHash, args.time, args.scale, args.view, &inputRod, &isProjectFormat);
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

    RenderSafetyEnum safety = frameArgs->currentThreadSafety;
    if (safety == eRenderSafetyFullySafeFrame) {
        int nbThreads = appPTR->getCurrentSettings()->getNumberOfThreads();
        // If the plug-in is eRenderSafetyFullySafeFrame that means it wants the host to perform SMP aka slice up the RoI into chunks
        // but if the effect doesn't support tiles it won't work.
        // Also check that the number of threads indicating by the settings are appropriate for this render mode.
        if ( !frameArgs->tilesSupported || (nbThreads == -1) || (nbThreads == 1) ||
            ( (nbThreads == 0) && (appPTR->getHardwareIdealThreadCount() == 1) ) ||
            ( QThreadPool::globalInstance()->activeThreadCount() >= QThreadPool::globalInstance()->maxThreadCount() )) {
            safety = eRenderSafetyFullySafe;
        }
    }


    if (tryIdentityOptim) {
        optimizeRectsToRender(this, inputsRoDIntersectionPixel, rectsLeftToRender, args.time, args.view, renderMappedScale, &planesToRender->rectsToRender);
    } else {
        // If plug-in wants host frame threading and there is only 1 rect to render, split it
        /*if (safety == eRenderSafetyFullySafeFrame && rectsLeftToRender.size() == 1 && frameArgs->tilesSupported) {
            QThreadPool* tp = QThreadPool::globalInstance();
            int nThreads = (tp->maxThreadCount() - tp->activeThreadCount());

            std::vector<RectI> splits;
            if (nThreads > 1) {
                splits = rectsLeftToRender.front().splitIntoSmallerRects(nThreads);
            } else {
                splits.push_back(rectsLeftToRender.front());
            }
            for (std::vector<RectI>::iterator it = splits.begin(); it != splits.end(); ++it) {
                RectToRender r;
                r.rect = *it;
                r.isIdentity = false;
                planesToRender->rectsToRender.push_back(r);
            }
        }*/
        for (std::list<RectI>::iterator it = rectsLeftToRender.begin(); it != rectsLeftToRender.end(); ++it) {
            RectToRender r;
            r.rect = *it;
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

            inputCode = renderInputImagesForRoI(requestPassData,
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
                                                byPassCache,
                                                *framesNeeded,
                                                *neededComps,
                                                &it->imgs,
                                                &it->inputRois);
        }
        if ( planesToRender->inputPremult.empty() ) {
            for (InputImagesMap::iterator it2 = it->imgs.begin(); it2 != it->imgs.end(); ++it2) {
                EffectInstancePtr input = getInput(it2->first);
                if (input) {
                    ImagePremultiplicationEnum inputPremult = input->getPremult();
                    if ( !it2->second.empty() ) {
                        const ImagePlaneDesc & comps = it2->second.front()->getComponents();
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
#if NATRON_ENABLE_TRIMAP
            if (guard) {
                guard->invalidate();
            }
#endif
            return inputCode;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End Pre-render input images ////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Allocate planes in the cache ////////////////////////////////////////////////////////////

    ///For all planes, if needed allocate the associated image
    if (hasSomethingToRender) {

        if (glContextLocker) {
            glContextLocker->attach();
        }
        for (std::map<ImagePlaneDesc, EffectInstance::PlaneToRender>::iterator it = planesToRender->planes.begin();
             it != planesToRender->planes.end(); ++it) {
            const ImagePlaneDesc *components = 0;

            if ( !it->first.isColorPlane() ) {
                //This plane is not color, there can only be a single set of components
                components = &(it->first);
            } else {
                //Find color plane from clip preferences
                for (std::list<ImagePlaneDesc>::const_iterator it = outputComponents.begin(); it != outputComponents.end(); ++it) {
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
                allocateImagePlane(*key,
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
                                   storage,
                                   createInCache,
                                   &it->second.fullscaleImage,
                                   &it->second.downscaleImage);
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

                    hasResized = it->second.fullscaleImage->copyAndResizeIfNeeded(renderFullScaleThenDownscale ? upscaledImageBounds : downscaledImageBounds, fillGrownBoundsWithZeroes, fillGrownBoundsWithZeroes, &it->second.cacheSwapImage);
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
                    it->second.downscaleImage = boost::make_shared<Image>(*components,
                                                                          rod,
                                                                          downscaledImageBounds,
                                                                          args.mipMapLevel,
                                                                          it->second.fullscaleImage->getPixelAspectRatio(),
                                                                          outputDepth,
                                                                          planesToRender->outputPremult,
                                                                          fieldingOrder,
                                                                          true);

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
        } // for each plane

#if NATRON_ENABLE_TRIMAP
        if (!guard) {
            // scoped_ptr
            guard.reset(new ImageBitMapMarker_RAII(planesToRender->planes, renderFullScaleThenDownscale, roi, this));
        }
#endif // NATRON_ENABLE_TRIMAP
    } // hasSomethingToRender
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      ////////////////////////////// End allocation of planes ///////////////////////////////////////////////////////////////


    //There should always be at least 1 plane to render (The color plane)
    assert( !planesToRender->planes.empty() );

    ///If we reach here, it can be either because the planes are cached or not, either way
    ///the planes are NOT a total identity, and they may have some content left to render.
    EffectInstance::RenderRoIStatusEnum renderRetCode = hasSomethingToRender ? eRenderRoIStatusImageRendered : eRenderRoIStatusImageAlreadyRendered;
    bool renderAborted;

    renderAborted = aborted();
#if NATRON_ENABLE_TRIMAP
    if (renderAborted) {
        assert(guard);
        guard->invalidate();
    }
#endif // NATRON_ENABLE_TRIMAP
    if (!renderAborted && hasSomethingToRender) {

        // eRenderSafetyInstanceSafe means that there is at most one render per instance
        // NOTE: the per-instance lock should probably be shared between
        // all clones of the same instance, because an InstanceSafe plugin may assume it is the sole owner of the output image,
        // and read-write on it.
        // It is probably safer to assume that several clones may write to the same output image only in the eRenderSafetyFullySafe case.

        // eRenderSafetyFullySafe means that there is only one render per FRAME : the lock is by image and handled in Node.cpp
        ///locks belongs to an instance)


        boost::scoped_ptr<QMutexLocker> locker;


        EffectInstancePtr renderInstance;
        /**
         * Figure out If this node should use a render clone rather than execute renderRoIInternal on the main (this) instance.
         * Reasons to use a render clone is be because a plug-in is eRenderSafetyInstanceSafe or does not support
         * concurrent GL renders.
         **/
        bool useRenderClone = safety == eRenderSafetyInstanceSafe || (safety != eRenderSafetyUnsafe && storage == eStorageModeGLTex && !supportsConcurrentOpenGLRenders());
        if (useRenderClone) {
            renderInstance = getOrCreateRenderInstance();
        } else {
            renderInstance = shared_from_this();
        }
        assert(renderInstance);

        if (safety == eRenderSafetyInstanceSafe) {
            locker.reset( new QMutexLocker( &getNode()->getRenderInstancesSharedMutex() ) );
        } else if (safety == eRenderSafetyUnsafe) {
            const Plugin* p = getNode()->getPlugin();
            assert(p);
            locker.reset( new QMutexLocker( p->getPluginLock() ) );
        } else {
            // no need to lock
            Q_UNUSED(locker);
        }


        ///For eRenderSafetyFullySafe, don't take any lock, the image already has a lock on itself so we're sure it can't be written to by 2 different threads.

        if ( frameArgs->stats && frameArgs->stats->isInDepthProfilingEnabled() ) {
            frameArgs->stats->setGlobalRenderInfosForNode(getNode(), rod, planesToRender->outputPremult, processChannels, frameArgs->tilesSupported, !renderFullScaleThenDownscale, renderMappedMipMapLevel);
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
        if (storage == eStorageModeGLTex) {
            assert(glContext);
            Natron::StatusEnum stat = renderInstance->attachOpenGLContext_public(glContext, &planesToRender->glContextData);
            if (stat == eStatusOutOfMemory) {
                renderRetCode = eRenderRoIStatusRenderOutOfGPUMemory;
                attachGLOK = false;
            } else if (stat == eStatusFailed) {
                renderRetCode = eRenderRoIStatusRenderFailed;
                attachGLOK = false;
            }
        }
        if (attachGLOK) {
            renderRetCode = renderRoIInternal(renderInstance.get(),
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
                                              byPassCache,
                                              outputDepth,
                                              outputClipPrefComps,
                                              neededComps,
                                              processChannels);
            if (storage == eStorageModeGLTex) {
                // If the plug-in doesn't support concurrent OpenGL renders, release the lock that was taken in the call to attachOpenGLContext_public() above.
                // For safe plug-ins, we call dettachOpenGLContext_public when the effect is destroyed in Node::deactivate() with the function EffectInstance::dettachAllOpenGLContexts().
                // If we were the last render to use this context, clear the data now
                if ( planesToRender->glContextData->getHasTakenLock() || !supportsConcurrentOpenGLRenders() || planesToRender->glContextData.use_count() == 1) {
                    renderInstance->dettachOpenGLContext_public(glContext, planesToRender->glContextData);
                }
            }
        }
        if (useRenderClone) {
            releaseRenderInstance(renderInstance);
        }


        renderAborted = aborted();

    } // if (!hasSomethingToRender) {

#if NATRON_ENABLE_TRIMAP
    assert(guard);
    if (renderAborted && renderRetCode != EffectInstance::eRenderRoIStatusImageRendered  && renderRetCode != EffectInstance::eRenderRoIStatusImageAlreadyRendered) {
        guard->invalidate();
    } else {
        guard->waitForPendingRegions();
    }
#endif // NATRON_ENABLE_TRIMAP

#if NATRON_ENABLE_TRIMAP
    guard.reset();
#endif

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
    } else if (renderRetCode == eRenderRoIStatusRenderOutOfGPUMemory) {
        /// Recall renderRoI on this node, but don't use GPU this time if possible
        if (openGLSupport != ePluginOpenGLRenderSupportYes) {
            // The plug-in can only use GPU or doesn't support GPU
            throw std::runtime_error("Rendering Failed");
        }
        boost::scoped_ptr<RenderRoIArgs> newArgs( new RenderRoIArgs(args) );
        newArgs->allowGPURendering = false;

        return renderRoI(*newArgs, outputPlanes);
    }


#if DEBUG
    if (hasSomethingToRender && (renderRetCode != eRenderRoIStatusRenderFailed) && !renderAborted) {
        // Kindly check that everything we asked for is rendered!

        for (std::map<ImagePlaneDesc, EffectInstance::PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
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

            //if ( frameArgs->isCurrentFrameRenderNotAbortable() ) {
                if ( !restToRender.empty() ) {
                    it->second.downscaleImage->printUnrenderedPixels(roi);
                }
                /*
                   If crashing on this assert this is likely due to a bug of the Trimap system.
                   Most likely another thread started rendering the portion that is in restToRender but did not fill the bitmap with 1
                   yet. Do not remove this assert, there should never be 2 threads running concurrently renderHandler for the same roi
                   on the same image.
                 */
                //assert( restToRender.empty() );
            //}
        }
    }
#endif

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////// Make sure all planes rendered have the requested  format ///////////////////////////

    bool useAlpha0ForRGBToRGBAConversion = args.caller ? args.caller->getNode()->usesAlpha0ToConvertFromRGBToRGBA() : false;

    // If the caller is not multiplanar, for the color plane we remap it to the components metadata obtained from the metadata pass, otherwise we stick to returning
    //bool callerIsMultiplanar = args.caller ? args.caller->isMultiPlanar() : false;

    //bool multiplanar = isMultiPlanar();
    for (std::map<ImagePlaneDesc, EffectInstance::PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
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
             ( it->second.fullscaleImage->getMipMapLevel() != mipMapLevel && it->second.fullscaleImage->getStorageMode() != eStorageModeGLTex) &&
             !hasSomethingToRender ) {
            assert(it->second.fullscaleImage->getMipMapLevel() == 0);
            if (it->second.downscaleImage == it->second.fullscaleImage) {
                it->second.downscaleImage = boost::make_shared<Image>(it->second.fullscaleImage->getComponents(),
                                                                      it->second.fullscaleImage->getRoD(),
                                                                      downscaledImageBounds,
                                                                      args.mipMapLevel,
                                                                      it->second.fullscaleImage->getPixelAspectRatio(),
                                                                      it->second.fullscaleImage->getBitDepth(),
                                                                      it->second.fullscaleImage->getPremultiplication(),
                                                                      it->second.fullscaleImage->getFieldingOrder(),
                                                                      false);
                it->second.downscaleImage->setKey(it->second.fullscaleImage->getKey());
            }

            it->second.fullscaleImage->downscaleMipMap( it->second.fullscaleImage->getRoD(), originalRoI, 0, args.mipMapLevel, false, it->second.downscaleImage.get() );
        }

        const ImagePlaneDesc* comp = 0;
        if ( !it->first.isColorPlane() ) {
            comp = &it->first;
        } else {
            // If we were requested the color plane, we rendered what the node's metadata is for the color plane. Map it to what was requested
            for (std::list<ImagePlaneDesc>::const_iterator it2 = args.components.begin(); it2 != args.components.end(); ++it2) {
                if ( it2->isColorPlane() ) {
                    comp = &(*it2);
                    break;
                }
            }
        }
        assert(comp);
        ///The image might need to be converted to fit the original requested format
        if (comp) {
            it->second.downscaleImage = convertPlanesFormatsIfNeeded(getApp(), it->second.downscaleImage, originalRoI, *comp, args.bitdepth, useAlpha0ForRGBToRGBAConversion, planesToRender->outputPremult, -1);
            assert(it->second.downscaleImage->getComponents() == *comp && it->second.downscaleImage->getBitDepth() == args.bitdepth);

            StorageModeEnum imageStorage = it->second.downscaleImage->getStorageMode();
            if ( args.returnStorage == eStorageModeGLTex && (imageStorage != eStorageModeGLTex) ) {
                if (!glContextLocker) {
                    // Make the OpenGL context current to this thread since we may use it for convertRAMImageToOpenGLTexture
                    glContextLocker = boost::make_shared<OSGLContextAttacher>(glContext, abortInfo
#ifdef DEBUG
                                                                              , frameArgs->time
#endif
                                                                              );
                }
                glContextLocker->attach();
                it->second.downscaleImage = convertRAMImageToOpenGLTexture(it->second.downscaleImage);
            } else if ( args.returnStorage != eStorageModeGLTex && (imageStorage == eStorageModeGLTex) ) {
                assert(args.returnStorage == eStorageModeRAM);
                assert(glContextLocker);
                if (glContextLocker) {
                    glContextLocker->attach();
                }
                it->second.downscaleImage = convertOpenGLTextureToCachedRAMImage(it->second.downscaleImage);
            }

            outputPlanes->insert( std::make_pair(*comp, it->second.downscaleImage) );
        }

#ifdef DEBUG
        RectI renderedImageBounds;
        rod.toPixelEnclosing(args.mipMapLevel, par, &renderedImageBounds);
        RectI expectedContainedRoI;
        args.roi.intersect(renderedImageBounds, &expectedContainedRoI);
        if ( !it->second.downscaleImage->getBounds().contains(expectedContainedRoI) ) {
            qDebug() << "[WARNING]:" << getScriptName_mt_safe().c_str() << "rendered an image with an RoI that fell outside its bounds.";
        }
#endif
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////// End requested format convertion ////////////////////////////////////////////////////////////////////


    ///// Termination
#ifdef DEBUG
    if ( outputPlanes->size() != args.components.size() ) {
        qDebug() << "Requested:";
        for (std::list<ImagePlaneDesc>::const_iterator it = args.components.begin(); it != args.components.end(); ++it) {
            qDebug() << it->getPlaneID().c_str();
        }
        qDebug() << "But rendered:";
        for (std::map<ImagePlaneDesc, ImagePtr>::iterator it = outputPlanes->begin(); it != outputPlanes->end(); ++it) {
            if (it->second) {
                qDebug() << it->first.getPlaneID().c_str();
            }
        }
    }
#endif

    assert( !outputPlanes->empty() );

    return eRenderRoIRetCodeOk;
} // renderRoI

EffectInstance::RenderRoIStatusEnum
EffectInstance::renderRoIInternal(EffectInstance* self,
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
                                  const ImagePlaneDesc& outputClipPrefsComps,
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
        RectI formatRect = self->getOutputFormat();
        frmt.set(formatRect);
        frmt.setPixelAspectRatio(par);
        self->getApp()->getProject()->setOrAddProjectFormat(frmt);
    }

    unsigned int renderMappedMipMapLevel = 0;

    for (std::map<ImagePlaneDesc, EffectInstance::PlaneToRender>::iterator it = planesToRender->planes.begin(); it != planesToRender->planes.end(); ++it) {
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
    NotifyRenderingStarted_RAIIPtr renderingNotifier;
    if ( !planesToRender->rectsToRender.empty() ) {
        renderingNotifier = boost::make_shared<NotifyRenderingStarted_RAII>( self->getNode().get() );
    }



    boost::shared_ptr<std::map<NodePtr, ParallelRenderArgsPtr> > tlsCopy;
    if (safety == eRenderSafetyFullySafeFrame) {
        tlsCopy = boost::make_shared<std::map<NodePtr, ParallelRenderArgsPtr> >();
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
                } if ( (*it2) == EffectInstance::eRenderingFunctorRetAborted ) {
                    renderStatus = eRenderingFunctorRetFailed;
                    break;
                } else if ( (*it2) == EffectInstance::eRenderingFunctorRetOutOfGPUMemory ) {
                    renderStatus = eRenderingFunctorRetOutOfGPUMemory;
                    break;
                }
            }
        } else {
            for (std::list<RectToRender>::const_iterator it = planesToRender->rectsToRender.begin(); it != planesToRender->rectsToRender.end(); ++it) {
                RenderingFunctorRetEnum functorRet = self->_imp->tiledRenderingFunctor(*it,  renderFullScaleThenDownscale, isSequentialRender, isRenderMadeInResponseToUserInteraction, firstFrame, lastFrame, preferredInput, mipMapLevel, renderMappedMipMapLevel, rod, time, view, par, byPassCache, outputClipPrefDepth, outputClipPrefsComps, compsNeeded, processChannels, planesToRender);

                if ( (functorRet == eRenderingFunctorRetFailed) || (functorRet == eRenderingFunctorRetAborted) || (functorRet == eRenderingFunctorRetOutOfGPUMemory) ) {
                    renderStatus = functorRet;
                    break;
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

NATRON_NAMESPACE_EXIT
