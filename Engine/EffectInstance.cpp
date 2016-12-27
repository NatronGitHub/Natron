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
#include <bitset>
#include <cassert>
#include <stdexcept>

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/scoped_ptr.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include <QtCore/QReadWriteLock>
#include <QtCore/QCoreApplication>
#include <QtConcurrentMap> // QtCore on Qt4, QtConcurrent on Qt5
#include <QtConcurrentRun> // QtCore on Qt4, QtConcurrent on Qt5

#include "Global/MemoryInfo.h"
#include "Global/QtCompat.h"

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/BlockingBackgroundRender.h"
#include "Engine/DiskCacheNode.h"
#include "Engine/EffectOpenGLContextData.h"
#include "Engine/Cache.h"
#include "Engine/EffectInstanceActionResults.h"
#include "Engine/EffectInstanceTLSData.h"
#include "Engine/Image.h"
#include "Engine/Hash64.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/Log.h"
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxOverlayInteract.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/GPUContextPool.h"
#include "Engine/OSGLContext.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/PluginMemory.h"
#include "Engine/Project.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/RenderValuesCache.h"
#include "Engine/ReadNode.h"
#include "Engine/Settings.h"
#include "Engine/Timer.h"
#include "Engine/TreeRender.h"
#include "Engine/TreeRenderNodeArgs.h"
#include "Engine/Transform.h"
#include "Engine/UndoCommand.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"


NATRON_NAMESPACE_ENTER;


EffectInstance::EffectInstance(const NodePtr& node)
    : NamedKnobHolder( node ? node->getApp() : AppInstancePtr() )
    , _node(node)
    , _imp( new Implementation(this) )
{
  
}

EffectInstance::EffectInstance(const EffectInstance& other)
    : NamedKnobHolder(other)
    , _node( other.getNode() )
    , _imp( new Implementation(*other._imp) )
{
    _imp->_publicInterface = this;
}

EffectInstance::~EffectInstance()
{
}




void
EffectInstance::appendToHash(const ComputeHashArgs& args, Hash64* hash)
{
    NodePtr node = getNode();

    assert(hash->isEmpty());

    // Append the plug-in ID in case for there is a coincidence of all parameter values (and ordering!) between 2 plug-ins
    Hash64::appendQString(QString::fromUtf8(node->getPluginID().c_str()), hash);

    if (args.hashType == eComputeHashTypeTimeViewVariant) {
        bool frameVarying = isFrameVaryingOrAnimated();

        // If the node is frame varying, append the time to its hash.
        // Do so as well if it is view varying
        if (frameVarying) {
            // Make sure the time is rounded to the image equality epsilon to account for double precision if we want to reproduce the
            // same hash
            hash->append(roundImageTimeToEpsilon(args.time));
        }

        if (isViewInvariant() == eViewInvarianceAllViewsVariant) {
            hash->append((int)view);
        }
    }


    
    // Also append the project knobs to the hash. Their hash will only change when the project properties have been invalidated
    U64 projectHash = getApp()->getProject()->computeHash(args.time, args.view);
    hash->append(projectHash);

    // Append all knobs hash
    KnobHolder::appendToHash(args, hash);

    GetFramesNeededResultsPtr framesNeededResults;


    // Append input hash for each frames needed.
    if (!args.appendTimeAndViewToHashIfFrameVarying) {
        // We don't need to be frame varying for the hash, just append the hash of the inputs at the current time
        int nInputs = getMaxInputCount();
        for (int i = 0; i < nInputs; ++i) {
            EffectInstancePtr input = getInput(i);
            if (!input) {
                ComputeHashArgs inputArgs = args;
                if (args.render) {
                    inputArgs.render = args.render->getInputRenderArgs(i);
                }
                U64 inputHash = input->computeHash(inputArgs);
                hash->append(inputHash);
            } else {
                hash->append(0);
            }
        }
    } else {
        // We must add the input hash at the frames needed because a node may depend on a value at a different frame


        StatusEnum stat = getFramesNeeded_public(args.time, args.view, args.render, &framesNeededResults);

        FramesNeededMap framesNeeded;
        if (stat != eStatusFailed) {
            framesNeededResults->getFramesNeeded(&framesNeeded);
        }
        for (FramesNeededMap::const_iterator it = framesNeeded.begin(); it != framesNeeded.end(); ++it) {

            EffectInstancePtr inputEffect = resolveInputEffectForFrameNeeded(it->first, this);
            if (!inputEffect) {
                continue;
            }

            // If during a render we must also find the render args for the input node
            TreeRenderNodeArgsPtr inputRenderArgs;
            if (render) {
                inputRenderArgs = render->getInputRenderArgs(it->first);
            }

            // For all views requested in input
            for (FrameRangesMap::const_iterator viewIt = it->second.begin(); viewIt != it->second.end(); ++viewIt) {

                // For all ranges in this view
                for (U32 range = 0; range < viewIt->second.size(); ++range) {

                    // For all frames in the range
                    for (double f = viewIt->second[range].min; f <= viewIt->second[range].max; f += 1.) {

                        ComputeHashArgs inputArgs = args;
                        if (args.render) {
                            inputArgs.render = args.render->getInputRenderArgs(i);
                        }
                        inputArgs.time = TimeValue(f);
                        inputArgs.view = viewIt->first;
                        U64 inputHash = inputEffect->computeHash(inputArgs);
                        
                        // Append the input hash
                        hashObj->append(inputHash);
                    }
                }
                
            }
        }
    }

    // Also append the disabled state of the node. This is useful because the knob disabled itself is not enough: if the node is not disabled
    // but inside a disabled group, it is considered disabled but yet has the same hash than when not disabled.
    bool disabled = node->isNodeDisabledForFrame(args.time, args.view);
    hash->append(disabled);

    hash->computeHash();

    U64 hashValue = hash->value();

    // If we used getFramesNeeded, cache it now if possible
    if (framesNeededResults) {
        GetFramesNeededKeyPtr cacheKey(hashValue, getNode()->getPluginID());
        CacheFetcher cacheAccess(cacheKey);
        if (!cacheAccess.isCached()) {
            cacheAccess.setEntry(framesNeededResults);
        }
    }

    if (args.render) {
        switch (args.hashType) {
            case HashableObject::eComputeHashTypeTimeViewVariant: {
                FrameViewRequest* fvRequest = args.render->getOrCreateFrameViewRequest(args.time, args.view);
                assert(fvRequest);
                fvRequest->setHash(hashValue);
            }   break;
            case HashableObject::eComputeHashTypeTimeViewInvariant:
                args.render->setTimeViewInvariantHash(hashValue);
                break;
            case HashableObject::eComputeHashTypeOnlyMetadataSlaves:
                args.render->setTimeInvariantMetadataHash(hashValue);
                break;
        }
    }

} // appendToHash

bool
EffectInstance::invalidateHashCacheImplementation(const bool recurse, std::set<HashableObject*>* invalidatedObjects)
{
    // Clear hash on this node
    if (!HashableObject::invalidateHashCacheInternal(invalidatedObjects)) {
        return false;
    }

    // If any knob has an expression, we must clear its results and clear its hash cache
    // because the result of the expression might depend on the state of the node
    KnobsVec knobs = getKnobs_mt_safe();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        for (int i = 0; i < (*it)->getNDimensions(); ++i) {
            std::string expr = (*it)->getExpression(DimIdx(i));
            if (!expr.empty()) {
                (*it)->clearExpressionsResults(DimSpec(i), ViewSetSpec::all());
                (*it)->invalidateHashCacheInternal(invalidatedObjects);
            }
        }
    }

    // For a group, also invalidate the hash of all its nodes
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(this);
    if (isGroup) {
        NodesList groupNodes = isGroup->getNodes();
        for (NodesList::const_iterator it = groupNodes.begin(); it!=groupNodes.end(); ++it) {
            EffectInstancePtr subNodeEffect = (*it)->getEffectInstance();
            if (!subNodeEffect) {
                continue;
            }
            // Do not recurse on outputs, since we iterate on all nodes in the group
            subNodeEffect->invalidateHashCacheImplementation(false /*recurse*/, invalidatedObjects);
        }
    }

    if (recurse) {
        NodesList outputs;
        getNode()->getOutputsWithGroupRedirection(outputs);
        for (NodesList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
            (*it)->getEffectInstance()->invalidateHashCacheImplementation(recurse, invalidatedObjects);
        }
    }
    return true;
} // invalidateHashCacheImplementation

bool
EffectInstance::invalidateHashCacheInternal(std::set<HashableObject*>* invalidatedObjects)
{
    return invalidateHashCacheImplementation(true /*recurse*/, invalidatedObjects);
}


bool
EffectInstance::shouldCacheOutput(bool isFrameVaryingOrAnimated,
                                  TimeValue time,
                                  ViewIdx view,
                                  int visitsCount) const
{
    NodePtr n = _node.lock();

    return n->shouldCacheOutput(isFrameVaryingOrAnimated, time, view, visitsCount);
}

const std::string &
EffectInstance::getScriptName() const
{
    return getNode()->getScriptName();
}

std::string
EffectInstance::getScriptName_mt_safe() const
{
    return getNode()->getScriptName_mt_safe();
}

int
EffectInstance::getRenderViewsCount() const
{
    return getApp()->getProject()->getProjectViewsCount();
}

bool
EffectInstance::hasOutputConnected() const
{
    return getNode()->hasOutputConnected();
}

EffectInstancePtr
EffectInstance::getInput(int n) const
{
    NodePtr inputNode = getNode()->getInput(n);

    if (inputNode) {
        return inputNode->getEffectInstance();
    }

    return EffectInstancePtr();
}

std::string
EffectInstance::getInputLabel(int inputNb) const
{
    std::string out;

    out.append( 1, (char)(inputNb + 65) );

    return out;
}

std::string
EffectInstance::getInputHint(int /*inputNb*/) const
{
    return std::string();
}


bool
EffectInstance::getImage(const GetImageInArgs& inArgs, GetImageOutArgs* outArgs)
{
    if (inArgs.time != inArgs.time) {
        // time is NaN
#ifdef DEBUG
        qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inArgs.inputNb << "failing because time is NaN";
#endif
        return false;
    }

    // The input to fetch images from
    EffectInstancePtr inputEffect = getInput(inArgs.inputNb);

    // Is this input a mask or not
    bool isMask = isInputMask(inArgs.inputNb);

    // If the input is a mask, this is the channel index in the layer of the mask channel
    int channelForMask = -1;

    // This is the actual layer that we are fetching in input for a mask
    ImageComponents maskComps;

    // If the mask checbkox is disabled, don't fetch a mask
    if ( !isMaskEnabled(inArgs.inputNb) ) {
        return false;
    }

    // If this is a mask, fetch the image from the effect indicated by the mask channel

    if (isMask) {
        NodePtr maskInput;
        channelForMask = getMaskChannel(inArgs.inputNb, &maskComps, &maskInput);
        // Invalid mask
        if ((channelForMask == -1) || (maskComps.getNumComponents() == 0)) {
#ifdef DEBUG
            qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inArgs.inputNb << "failing because the mask is not connected";
#endif
            return false;
        }
        if ( maskInput && (channelForMask != -1) ) {
            inputEffect = maskInput->getEffectInstance();
        }
    }


    if (!inputEffect) {
        // Disconnected input
        // No need to display a warning, the effect can perfectly call this function even if it did not check isConnected() before
        //qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inputNb << "failing because the input is not connected";
        return ImagePtr();
    }

    // If this effect is not continuous, no need to ask for a floating point time upstream
    TimeValue time = inArgs.time;
    {
        int roundedTime = std::floor(time + 0.5);
        if (roundedTime != time && !inputEffect->canRenderContinuously(inArgs.renderArgs)) {
            time = TimeValue(roundedTime);
        }
    }

    if (!inArgs.renderArgs) {

        // We were not during a render, create one and render the tree upstream.
        TreeRender::CtorArgsPtr rargs(new TreeRender::CtorArgs());
        rargs->time = time;
        rargs->view = inArgs.view;
        rargs->treeRoot = inputEffect->getNode();
        rargs->canonicalRoi = inArgs.optionalBounds;
        rargs->mipMapLevel = Image::getLevelFromScale(inArgs.scale.x);
        rargs->layer = inArgs.layer;
        rargs->draftMode = false;
        rargs->playback = false;
        rargs->byPassCache = false;

        ImagePtr renderedImage;
        RenderRoIRetCode status;
        renderedImage = TreeRender::renderImage(rargs, &status);
        if (status != eRenderRoIRetCodeOk) {
            return ImagePtr();
        }
        return renderedImage;
    }

    // Ok now we are during a render action.
    // Determine if the frame is "known" (i.e: it was requested from getFramesNeeded) or not.
    //
    // For a known frame we know the exact final RoI to ask for on the effect and we can guarantee
    // that the effect will be rendered once. We cache the resulting image only during the render of this node
    // frame, then we can throw it away.
    //
    // For an unknown frame, we don't know if the frame will be asked for another time hence we lock
    // it in the cache throughout the lifetime of the render.

    // Get the RoI from what was computed in the request pass on the input.
    RectD roiCanonical;
    {

        bool gotRoIFromRequestPass = false;
        if (inputFrameArgs->request) {
            // The roi is the optimized roi: the bounding rect of all RoI from different branches leading to that node.
            gotRoIFromRequestPass = inputFrameArgs->request->getFrameViewCanonicalRoI(time, view, &roiCanonical);
        }
        if (!gotRoIFromRequestPass) {

            // If we were in analysis, the ParallelRenderArgsSetter that were created just above should have created the request for the exact RoI, so we should never
            // enter this condition.
            // This condition can occur only in the render action:
            // This node did not request anything on the input in getFramesNeeded action: it did a raw call to fetchImage without advertising us.
            // Don't fail:
            // If we are in a current render, (tls->currentRenderArgs.validArgs==true)  call getRegionsOfInterest on the current render window
            // If roi is not advertised, well fall back onto rendering the whole image (RoD).
            assert(tls);
            double thisEffectRenderTime;
            ViewIdx thisEffectRenderView;
            RectI thisEffectRenderWindowPixels;

            {
                bool gotRenderActionTLSData = false;
                if (tls) {
                    gotRenderActionTLSData = tls->getCurrentRenderActionArgs(&thisEffectRenderTime, &thisEffectRenderView, 0, &thisEffectRenderWindowPixels, 0, 0, 0);
                }
                assert(gotRenderActionTLSData);
                if (!gotRenderActionTLSData) {
#ifdef DEBUG
                    qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inputNb << "failing because of a bug in the thread local storage!";
#endif
                    return ImagePtr();
                }
            }


            bool gotInputRoI = false;
#ifdef DEBUG
            qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inputNb << "at time" << time << "and view" << view <<
            ":The frame was not requested properly in the getFramesNeeded action. This is a bug either in this plug-in or a plug-in downstream (which should also have this warning displayed)";
#endif

            // We are in the render action, the frame/view hash was computed in the ParallelRenderArgs ctor.
            // Get this node rod
            RectD outputRod;
            StatusEnum stat = getRegionOfDefinition_public(0, thisEffectRenderTime, scale, thisEffectRenderView, &outputRod);
            if (stat == eStatusFailed) {
                return ImagePtr();
            }

            RectD thisEffectCurrentRenderWindowCanonical;
            thisEffectRenderWindowPixels.toCanonical(mipMapLevel, getAspectRatio(-1), outputRod, &thisEffectCurrentRenderWindowCanonical);

            // Get the roi for the current render window
            RoIMap inputRoisMap;
            inputEffect->getRegionsOfInterest_public(thisEffectRenderTime, scale, outputRod, thisEffectCurrentRenderWindowCanonical, thisEffectRenderView, &inputRoisMap);
            RoIMap::iterator foundInputEffectRoI = inputRoisMap.find(inputEffect);
            if (foundInputEffectRoI != inputRoisMap.end()) {
                roiCanonical = foundInputEffectRoI->second;
                gotInputRoI = true;
            }


            if (!gotInputRoI) {
                // Fallback on RoD. There may be no hash after all because the frame was not registered in ParallelRenderArgs.

                StatusEnum stat = inputEffect->getRegionOfDefinition_public(0, time, scale, view, &roiCanonical);
                if (stat == eStatusFailed) {
                    return ImagePtr();
                }
            }


            // There is no request pass as-well, but renderRoI needs it, comptue it
            stat = EffectInstance::optimizeRoI(time, view, mipMapLevel, roiCanonical, inputEffect->getNode());
            if (stat != eStatusOK) {
                return ImagePtr();
            }


        } // inputFrameViewRequest
    }

    if (roiCanonical.isNull()) {
        // No RoI
#ifdef DEBUG
        qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inputNb << "failing because the region of interest is empty";
#endif
        return ImagePtr();
    }

    // This is the time of this effect current action
    double thisEffectRenderTime = time;

    // If within a render action, the input images have been pre-fetched and set on the currentRenderArgs TLS
    InputImagesMap inputImagesThreadLocal;

    if (tls) {
        // When rendering, retrieve the time and input images that were set within the renderRoI call
        tls->getCurrentRenderActionArgs(&thisEffectRenderTime, 0, 0, 0, &inputImagesThreadLocal, 0, 0);
    }


    // Determine at which mipmap level we should fetch the input image
    unsigned int renderMappedMipMapLevel = mipMapLevel;

    // Do we want to render the graph upstream at scale 1 or at the requested render scale ? (user setting)
    bool renderScaleOneUpstreamIfRenderScaleSupportDisabled = false;

    // Does this node supports images at a scale different than 1
    bool renderFullScaleThenDownscale = (!supportsRenderScale() && mipMapLevel != 0 && returnStorage == eStorageModeRAM);


    if (renderFullScaleThenDownscale) {
        renderScaleOneUpstreamIfRenderScaleSupportDisabled = getNode()->useScaleOneImagesWhenRenderScaleSupportIsDisabled();
        if (renderScaleOneUpstreamIfRenderScaleSupportDisabled) {
            renderMappedMipMapLevel = 0;
        }
    }


    // Retrieve an image with the format given by this node preferences
    const double par = getAspectRatio(inputNb);
    ImageBitDepthEnum depth = getBitDepth(inputNb);
    ImageComponents components;
    ImageComponents clipPrefComps = getComponents(inputNb);

    // If there's a layer argument, use it, otherwise use the components meta-data.
    if (layer) {
        components = *layer;
    } else {
        components = clipPrefComps;
    }

    // Convert the roi to pixel coordinates
    RectI pixelRoI;
    roiCanonical.toPixelEnclosing(renderScaleOneUpstreamIfRenderScaleSupportDisabled ? 0 : mipMapLevel, par, &pixelRoI);

    ImagePtr inputImg;

    {
        // If input is a mask, request the maskComps otherwise use components
        std::list<ImageComponents> requestedComps;
        requestedComps.push_back(isMask ? maskComps : components);


        RenderRoIResults inputRenderResults;
        // Never by-pass the cache here because we already computed the image in renderRoI and by-passing the cache again can lead to
        // re-computing of the same image many many times
        RenderRoIRetCode retCode = inputEffect->renderRoI(RenderRoIArgs(time,
                                                                        scale,
                                                                        renderMappedMipMapLevel,
                                                                        view,
                                                                        false /*byPassCache*/,
                                                                        pixelRoI,
                                                                        requestedComps,
                                                                        depth,
                                                                        true,
                                                                        shared_from_this(),
                                                                        inputNb,
                                                                        returnStorage,
                                                                        thisEffectRenderTime,
                                                                        inputImagesThreadLocal), &inputRenderResults);

        if ( inputRenderResults.outputPlanes.empty() || (retCode != eRenderRoIRetCodeOk) ) {
#ifdef DEBUG
            //qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inputNb << "failing because the call to renderRoI was aborted or failed";
#endif
            return ImagePtr();
        }

        // We asked for a single plane, we should get a single image!
        assert(inputRenderResults.outputPlanes.size() == 1);
        inputImg = inputRenderResults.outputPlanes.begin()->second;
    }

    if ( !pixelRoI.intersects( inputImg->getBounds() ) ) {
        // The RoI requested does not intersect with the bounds of the input image computed, return a NULL image.
#ifdef DEBUG
        qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inputNb << "failing because the RoI requested does not intersect the bounds of the input image fetched. This is a bug, please investigate!";
#endif
        return ImagePtr();
    }

    if (roiPixel) {
        *roiPixel = pixelRoI;
    }


    unsigned int inputImgMipMapLevel = inputImg->getMipMapLevel();

    // If the plug-in doesn't support the render scale, but the image is downscaled, up-scale it.
    // Note that we do NOT cache it because it is really low def!
    // For OpenGL textures, we do not do it because GL_TEXTURE_2D uses normalized texture coordinates anyway, so any OpenGL plug-in should support render scale.

    if (!dontUpscale && renderFullScaleThenDownscale && (inputImgMipMapLevel != 0) && returnStorage == eStorageModeRAM) {

        // Resize the image according to the requested scale
        RectI boundsLevel0;
        inputImg->getRoD().toPixelEnclosing(0, par, &boundsLevel0);
        ImagePtr rescaledImg( new Image(inputImg->getComponents(),
                                        inputImg->getRoD(),
                                        boundsLevel0, // bounds
                                        0 /*mipMapLevel*/,
                                        par,
                                        inputImg->getBitDepth(),
                                        inputImg->getPremultiplication(),
                                        inputImg->getFieldingOrder(),
                                        true,
                                        eStorageModeRAM,
                                        OSGLContextPtr(),
                                        GL_TEXTURE_2D,
                                        true) );

        inputImg->upscaleMipMap( inputImg->getBounds(), inputImgMipMapLevel, 0, rescaledImg.get() );

        if (roiPixel) {
            roiCanonical.toPixelEnclosing(0, par, roiPixel);
            pixelRoI = *roiPixel;
        }

        inputImg = rescaledImg;
    }


    // Remap if needed
    if (mapToClipPrefs) {

        // The image premult is of the effect output premult except for non color planes
        ImagePremultiplicationEnum outputPremult;
        if ( components.isColorPlane() ) {
            outputPremult = inputEffect->getPremult();
        } else {
            outputPremult = eImagePremultiplicationOpaque;
        }


        inputImg = convertPlanesFormatsIfNeeded(getApp(), inputImg, pixelRoI, clipPrefComps, depth, false /*useAlpha0ForRGBToRGBAConversion*/, outputPremult, channelForMask);
    }

#ifdef DEBUG
    ///Check that the rendered image contains what we requested.
    if ( !mapToClipPrefs && ( ( !isMask && (inputImg->getComponents() != components) ) || ( isMask && (inputImg->getComponents() != maskComps) ) ) ) {
        ImageComponents cc;
        if (isMask) {
            cc = maskComps;
        } else {
            cc = components;
        }
        qDebug() << "WARNING:" << getNode()->getScriptName_mt_safe().c_str() << "requested" << cc.getComponentsGlobalName().c_str() << "but" << inputEffect->getScriptName_mt_safe().c_str() << "returned an image with"
                 << inputImg->getComponents().getComponentsGlobalName().c_str();
        qDebug() << inputEffect->getScriptName_mt_safe().c_str() << "output clip preference is" << inputEffect->getComponents(-1).getComponentsGlobalName().c_str();
    }

#endif
    
    return inputImg;
} // getImage



EffectInstance::NotifyRenderingStarted_RAII::NotifyRenderingStarted_RAII(Node* node)
    : _node(node)
    , _didGroupEmit(false)
{
    _didEmit = node->notifyRenderingStarted();

    // If the node is in a group, notify also the group
    NodeCollectionPtr group = node->getGroup();
    if (group) {
        NodeGroupPtr isGroupNode = toNodeGroup(group);
        if (isGroupNode) {
            _didGroupEmit = isGroupNode->getNode()->notifyRenderingStarted();
        }
    }
}

EffectInstance::NotifyRenderingStarted_RAII::~NotifyRenderingStarted_RAII()
{
    if (_didEmit) {
        _node->notifyRenderingEnded();
    }
    if (_didGroupEmit) {
        NodeCollectionPtr group = _node->getGroup();
        if (group) {
            NodeGroupPtr isGroupNode = toNodeGroup(group);
            if (isGroupNode) {
                isGroupNode->getNode()->notifyRenderingEnded();
            }
        }
    }
}

EffectInstance::NotifyInputNRenderingStarted_RAII::NotifyInputNRenderingStarted_RAII(Node* node,
                                                                                     int inputNumber)
    : _node(node)
    , _inputNumber(inputNumber)
{
    _didEmit = node->notifyInputNIsRendering(inputNumber);
}

EffectInstance::NotifyInputNRenderingStarted_RAII::~NotifyInputNRenderingStarted_RAII()
{
    if (_didEmit) {
        _node->notifyInputNIsFinishedRendering(_inputNumber);
    }
}

static void
getOrCreateFromCacheInternal(const ImageKey & key,
                             const ImageParamsPtr & params,
                             const OSGLContextPtr& glContext,
                             bool useCache,
                             ImagePtr* image)
{
    if (!useCache) {
        image->reset( new Image(key, params) );
    } else {
        if (params->getStorageInfo().mode == eStorageModeRAM || params->getStorageInfo().mode == eStorageModeGLTex) {
            appPTR->getImageOrCreate(key, params, 0, image);
        } else if (params->getStorageInfo().mode == eStorageModeDisk) {
            appPTR->getImageOrCreate_diskCache(key, params, image);
        }

        if (!*image) {
            std::stringstream ss;
            ss << "Failed to allocate an image of ";
            const CacheEntryStorageInfo& info = params->getStorageInfo();
            std::size_t size = info.dataTypeSize * info.numComponents * info.bounds.area();
            ss << printAsRAM(size).toStdString();
            Dialogs::errorDialog( QCoreApplication::translate("EffectInstance", "Out of memory").toStdString(), ss.str() );

            return;
        }

        /*
         * Note that at this point the image is already exposed to other threads and another one might already have allocated it.
         * This function does nothing if it has been reallocated already.
         */
        (*image)->allocateMemory();


        /*
         * Another thread might have allocated the same image in the cache but with another RoI, make sure
         * it is big enough for us, or resize it to our needs.
         */


        (*image)->ensureBounds( glContext, params->getBounds() );
    }
}



static ImagePtr ensureImageScale(unsigned int mipMapLevel,
                                 const ImagePtr& image,
                                 const ImageKey & key,
                                 const RectI* boundsParam,
                                 const RectD* rodParam,
                                 const OSGLContextAttacherPtr& glContextAttacher)
{
    if (image->getMipMapLevel() == mipMapLevel) {
        return image;
    }

    ImagePtr imageToConvert = image;

    ImageParamsPtr oldParams = imageToConvert->getParams();

    if (imageToConvert->getMipMapLevel() < mipMapLevel) {

        //This is the bounds of the upscaled image
        RectI imgToConvertBounds = imageToConvert->getBounds();

        //The rodParam might be different of oldParams->getRoD() simply because the RoD is dependent on the mipmap level
        const RectD & rod = rodParam ? *rodParam : oldParams->getRoD();

        RectI downscaledBounds;
        rod.toPixelEnclosing(mipMapLevel, imageToConvert->getPixelAspectRatio(), &downscaledBounds);

        if (boundsParam) {
            downscaledBounds.merge(*boundsParam);
        }
        ImageParamsPtr imageParams = Image::makeParams(rod,
                                                       downscaledBounds,
                                                       oldParams->getPixelAspectRatio(),
                                                       mipMapLevel,
                                                       oldParams->getComponents(),
                                                       oldParams->getBitDepth(),
                                                       oldParams->getPremultiplication(),
                                                       oldParams->getFieldingOrder(),
                                                       glContextAttacher ? glContextAttacher->getContext() : OSGLContextPtr(),
                                                       eStorageModeRAM,
                                                       GL_TEXTURE_2D);



        imageParams->setMipMapLevel(mipMapLevel);


        ImagePtr img;
        getOrCreateFromCacheInternal(key, imageParams, glContextAttacher ? glContextAttacher->getContext() : OSGLContextPtr(), imageToConvert->usesBitMap(), &img);
        if (!img) {
            return img;
        }


        /*
         Since the RoDs of the 2 mipmaplevels are different, their bounds do not match exactly as po2
         To determine which portion we downscale, we downscale the initial image bounds to the mipmap level
         of the downscale image, clip it against the bounds of the downscale image, re-upscale it to the
         original mipmap level and ensure that it lies into the original image bounds
         */
        int downscaleLevels = img->getMipMapLevel() - imageToConvert->getMipMapLevel();
        RectI dstRoi = imgToConvertBounds.downscalePowerOfTwoSmallestEnclosing(downscaleLevels);
        dstRoi.intersect(downscaledBounds, &dstRoi);
        dstRoi = dstRoi.upscalePowerOfTwo(downscaleLevels);
        dstRoi.intersect(imgToConvertBounds, &dstRoi);

        if (imgToConvertBounds.area() > 1) {
            imageToConvert->downscaleMipMap( rod,
                                            dstRoi,
                                            imageToConvert->getMipMapLevel(), img->getMipMapLevel(),
                                            imageToConvert->usesBitMap(),
                                            img.get() );
        } else {
            img->pasteFrom(*imageToConvert, imgToConvertBounds);
        }

        imageToConvert = img;
    } else {

        //This is the bounds of the downscaled image
        RectI upscaledImgBounds;
        //The rodParam might be different of oldParams->getRoD() simply because the RoD is dependent on the mipmap level
        const RectD & rod = rodParam ? *rodParam : oldParams->getRoD();
        rod.toPixelEnclosing(mipMapLevel, imageToConvert->getPixelAspectRatio(), &upscaledImgBounds);

        ImageParamsPtr imageParams = Image::makeParams(rod,
                                                       upscaledImgBounds,
                                                       oldParams->getPixelAspectRatio(),
                                                       mipMapLevel,
                                                       oldParams->getComponents(),
                                                       oldParams->getBitDepth(),
                                                       oldParams->getPremultiplication(),
                                                       oldParams->getFieldingOrder(),
                                                       glContextAttacher ? glContextAttacher->getContext() : OSGLContextPtr(),
                                                       eStorageModeRAM,
                                                       GL_TEXTURE_2D);



        imageParams->setMipMapLevel(mipMapLevel);


        ImagePtr img;
        getOrCreateFromCacheInternal(key, imageParams, glContextAttacher ? glContextAttacher->getContext() : OSGLContextPtr(), imageToConvert->usesBitMap(), &img);
        if (!img) {
            return img;
        }

        imageToConvert->upscaleMipMap( imageToConvert->getBounds(), imageToConvert->getMipMapLevel(), mipMapLevel, img.get() );
        imageToConvert = img;
    }
    return imageToConvert;
}

void
EffectInstance::getImageFromCacheAndConvertIfNeeded(bool /*useCache*/,
                                                    bool isDuringPaintStroke,
                                                    StorageModeEnum storage,
                                                    StorageModeEnum returnStorage,
                                                    const ImageKey & key,
                                                    unsigned int mipMapLevel,
                                                    const RectI* boundsParam,
                                                    const RectD* rodParam,
                                                    const RectI& roi,
                                                    ImageBitDepthEnum bitdepth,
                                                    const ImageComponents & components,
                                                    const InputImagesMap & inputImages,
                                                    const RenderStatsPtr & stats,
                                                    const OSGLContextAttacherPtr& glContextAttacher,
                                                    ImagePtr* image)
{
    ImageList cachedImages;
    bool isCached = false;

    ///Find first something in the input images list
    if ( !inputImages.empty() ) {
        for (InputImagesMap::const_iterator it = inputImages.begin(); it != inputImages.end(); ++it) {
            for (ImageList::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                if ( !it2->get() ) {
                    continue;
                }
                const ImageKey & imgKey = (*it2)->getKey();
                if (imgKey == key) {
                    cachedImages.push_back(*it2);
                    isCached = true;
                }
            }
        }
    }

    // If an effect accumulates, make sure we use the last rendered buffer.
    if (isDuringPaintStroke || isPaintingOverItselfEnabled()) {
        ImagePtr buffer = getNode()->getLastRenderedImage();
        if (buffer) {
            // If the last rendered image is on GPU, ensure we are using the same OpenGL context.
            if (buffer->getStorageMode() != eStorageModeGLTex ||
                (glContextAttacher && glContextAttacher->getContext() == buffer->getParams()->getStorageInfo().glContext.lock())) {
                cachedImages.push_back(buffer);
                isCached = true;
            }

        }
    }

    if (!isCached) {
        // For textures, we lookup for a RAM image, if found we convert it to a texture
        if ( (storage == eStorageModeRAM) || (storage == eStorageModeGLTex) ) {
            isCached = appPTR->getImage(key, &cachedImages);
        } else if (storage == eStorageModeDisk) {
            isCached = appPTR->getImage_diskCache(key, &cachedImages);
        }
    }

    if (stats && stats->isInDepthProfilingEnabled() && !isCached) {
        stats->addCacheInfosForNode(getNode(), true, false);
    }

    if (isCached) {
        ///A ptr to a higher resolution of the image or an image with different comps/bitdepth
        ImagePtr imageToConvert;

        for (ImageList::iterator it = cachedImages.begin(); it != cachedImages.end(); ++it) {
            unsigned int imgMMlevel = (*it)->getMipMapLevel();
            const ImageComponents & imgComps = (*it)->getComponents();
            ImageBitDepthEnum imgDepth = (*it)->getBitDepth();

            bool convertible = imgComps.isConvertibleTo(components);
            if ( (imgMMlevel == mipMapLevel) && convertible &&
                 ( getSizeOfForBitDepth(imgDepth) >= getSizeOfForBitDepth(bitdepth) ) /* && imgComps == components && imgDepth == bitdepth*/ ) {
                ///We found  a matching image

                *image = *it;
                break;
            } else {
                if ( !convertible || ( getSizeOfForBitDepth(imgDepth) < getSizeOfForBitDepth(bitdepth) ) ) {
                    // not enough components or bit-depth is not as deep, don't use the image
                    continue;
                }

                if (imgMMlevel > mipMapLevel) {
                    if (!isDuringPaintStroke) {
                        // mipmap level is higher, use it only if plug-in is painting over itself and absolutely requires the data
                        continue;
                    }
                    if (imageToConvert) {
                        ///We found an image which scale is closer to the requested mipmap level we want, use it instead
                        if ( imgMMlevel < imageToConvert->getMipMapLevel() ) {
                            imageToConvert = *it;
                        }
                    } else {
                        imageToConvert = *it;
                    }
                } else if (imgMMlevel < mipMapLevel) {
                    if (imageToConvert) {
                        // We found an image which scale is closer to the requested mipmap level we want, use it instead
                        if ( imgMMlevel > imageToConvert->getMipMapLevel() ) {
                            imageToConvert = *it;
                        }
                    } else {
                        imageToConvert = *it;
                    }
                } else {
                    imageToConvert = *it;
                }

            }
        } //end for

        if (imageToConvert && !*image) {
            ///Ensure the image is allocated
            (imageToConvert)->allocateMemory();


            if (imageToConvert->getMipMapLevel() != mipMapLevel) {
                imageToConvert = ensureImageScale(mipMapLevel, imageToConvert, key, boundsParam, rodParam, glContextAttacher);
                if (!imageToConvert) {
                    return;
                }
            }

            if (storage == eStorageModeGLTex) {

                // When using the GPU, we dont want to retrieve partially rendered image because rendering the portion
                // needed then reading it back to put it in the CPU image would take much more effort than just computing
                // the GPU image.
                std::list<RectI> restToRender;
                imageToConvert->getRestToRender(roi, restToRender);
                if ( restToRender.empty() ) {
                    if (returnStorage == eStorageModeGLTex) {
                        assert(glContextAttacher);
                        glContextAttacher->attach();
                        *image = convertRAMImageToOpenGLTexture(imageToConvert, glContextAttacher ? glContextAttacher->getContext() : OSGLContextPtr());
                    } else {
                        assert(returnStorage == eStorageModeRAM && (imageToConvert->getStorageMode() == eStorageModeRAM || imageToConvert->getStorageMode() == eStorageModeDisk));
                        // If renderRoI must return a RAM image, don't convert it back again!
                        *image = imageToConvert;
                    }
                }
            } else {
                *image = imageToConvert;
            }
            //assert(imageToConvert->getBounds().contains(bounds));
            if ( stats && stats->isInDepthProfilingEnabled() ) {
                stats->addCacheInfosForNode(getNode(), false, true);
            }
        } else if (*image) { //  else if (imageToConvert && !*image)
            ///Ensure the image is allocated
            if ( (*image)->getStorageMode() != eStorageModeGLTex ) {
                (*image)->allocateMemory();

                if (storage == eStorageModeGLTex) {

                    // When using the GPU, we dont want to retrieve partially rendered image because rendering the portion
                    // needed then reading it back to put it in the CPU image would take much more effort than just computing
                    // the GPU image.
                    std::list<RectI> restToRender;
                    (*image)->getRestToRender(roi, restToRender);
                    if ( restToRender.empty() ) {
                        // If renderRoI must return a RAM image, don't convert it back again!
                        if (returnStorage == eStorageModeGLTex) {
                            assert(glContextAttacher);
                            glContextAttacher->attach();
                            *image = convertRAMImageToOpenGLTexture(*image, glContextAttacher ? glContextAttacher->getContext() : OSGLContextPtr());
                        }
                    } else {
                        image->reset();
                        return;
                    }
                }
            }

            if ( stats && stats->isInDepthProfilingEnabled() ) {
                stats->addCacheInfosForNode(getNode(), false, false);
            }
        } else {
            if ( stats && stats->isInDepthProfilingEnabled() ) {
                stats->addCacheInfosForNode(getNode(), true, false);
            }
        }
    } // isCached
} // EffectInstance::getImageFromCacheAndConvertIfNeeded

void
EffectInstance::tryConcatenateTransforms(TimeValue time,
                                         ViewIdx view,
                                         const RenderScale & scale,
                                         U64 hash,
                                         InputMatrixMap* inputTransforms)
{
    bool canTransform = getNode()->getCurrentCanTransform();

    //An effect might not be able to concatenate transforms but can still apply a transform (e.g CornerPinMasked)
    std::list<int> inputHoldingTransforms;
    getInputsHoldingTransform(&inputHoldingTransforms);

    Transform::Matrix3x3 thisNodeTransform;
    bool getTransformSucceeded = false;

    if (canTransform) {
        EffectInstancePtr inputToTransform;
        // If getting the transform does not succeed, then this effect is treated as any other ones.
        StatusEnum stat = getTransform_public(time, scale, view, hash, &inputToTransform, &thisNodeTransform);
        if (stat == eStatusOK) {
            getTransformSucceeded = true;
        }
    }


    // Nothing to recurse onto
    if ( (canTransform && !getTransformSucceeded) || (!canTransform && inputHoldingTransforms.empty() ) ) {
        return;
    }

    // For each input to transform, recurse
    for (std::list<int>::iterator it = inputHoldingTransforms.begin(); it != inputHoldingTransforms.end(); ++it) {

        EffectInstancePtr input = getInput(*it);
        if (!input) {
            continue;
        }

        std::list<Transform::Matrix3x3> matricesByOrder; // from downstream to upstream
        InputMatrix im;
        im.newInputEffect = input;
        im.newInputNbToFetchFrom = *it;


        // recursion upstream
        bool inputCanTransform = false;


        // If the effect is identity, do not call the getRegionOfDefinition action, instead just return the input identity at the
        // identity time and view.
        double inputIdentityTime;
        ViewIdx inputIdentityView;
        int inputIdentityNb;
        bool isIdentity;
        EffectInstancePtr identityInput;
        {
            // If the effect is identity on the format, that means its bound to be identity anywhere and does not depend on the render window.
            RectI format = input->getOutputFormat();
            RenderScale scale(1.);
            isIdentity = input->isIdentity_public(true, 0, time, scale, format, view, &inputIdentityTime, &inputIdentityView, &inputIdentityNb);
            identityInput = input->getInput(inputIdentityNb);
        }


        if (!isIdentity) {
            inputCanTransform = input->getNode()->getCurrentCanTransform();
        } else {
            inputCanTransform = true;
        }


        while (inputCanTransform) {

            if (isIdentity) {
                // Identity concatenates an identity matrix
                im.newInputNbToFetchFrom = inputIdentityNb;
                im.newInputEffect = input;
                input = identityInput;
            } else if (inputCanTransform) {
                Transform::Matrix3x3 m;

                EffectInstancePtr inputToTransform;
                StatusEnum stat = input->getTransform_public(time, scale, view, 0, &inputToTransform, &m);
                if (stat == eStatusOK) {
                    matricesByOrder.push_back(m);
                    if (inputToTransform) {
                        im.newInputNbToFetchFrom = input->getInputNumber(inputToTransform);
                    }
                    im.newInputEffect = input;
                }
                input = inputToTransform;
            } else {
                assert(false);
            }

            if (!input) {
                inputCanTransform = false;
            } else {
                RectI format = input->getOutputFormat();
                RenderScale scale(1.);
                isIdentity = input->isIdentity_public(true, 0, time, scale, format, view, &inputIdentityTime, &inputIdentityView, &inputIdentityNb);
                if (!isIdentity) {
                    inputCanTransform = input->getNode()->getCurrentCanDistort();
                } else {
                    inputCanTransform = true;
                    identityInput = input->getInput(inputIdentityNb);
                }
            }
        }

        if ( input && !matricesByOrder.empty() ) {
            assert(im.newInputEffect);

            ///Now actually concatenate matrices together
            im.cat.reset(new Transform::Matrix3x3);
            std::list<Transform::Matrix3x3>::iterator it2 = matricesByOrder.begin();
            *im.cat = *it2;
            ++it2;
            while ( it2 != matricesByOrder.end() ) {
                *im.cat = Transform::matMul(*im.cat, *it2);
                ++it2;
            }

            inputTransforms->insert( std::make_pair(*it, im) );
        }
    } //  for (std::list<int>::iterator it = inputHoldingTransforms.begin(); it != inputHoldingTransforms.end(); ++it)
} // EffectInstance::tryConcatenateTransforms

bool
EffectInstance::allocateImagePlane(const ImageKey & key,
                                   const RectD & rod,
                                   const RectI & downscaleImageBounds,
                                   const RectI & fullScaleImageBounds,
                                   const ImageComponents & components,
                                   ImageBitDepthEnum depth,
                                   ImagePremultiplicationEnum premult,
                                   ImageFieldingOrderEnum fielding,
                                   double par,
                                   unsigned int mipmapLevel,
                                   bool renderFullScaleThenDownscale,
                                   const OSGLContextPtr& glContext,
                                   StorageModeEnum storage,
                                   bool createInCache,
                                   ImagePtr* fullScaleImage,
                                   ImagePtr* downscaleImage)
{
    //If we're rendering full scale and with input images at full scale, don't cache the downscale image since it is cheap to
    //recreate, instead cache the full-scale image
    if (renderFullScaleThenDownscale) {
        downscaleImage->reset( new Image(components, rod, downscaleImageBounds, mipmapLevel, par, depth, premult, fielding, true, eStorageModeRAM, OSGLContextPtr(), GL_TEXTURE_2D, true) );
        ImageParamsPtr upscaledImageParams = Image::makeParams(rod,
                                                               fullScaleImageBounds,
                                                               par,
                                                               0,
                                                               components,
                                                               depth,
                                                               premult,
                                                               fielding,
                                                               glContext,
                                                               storage,
                                                               GL_TEXTURE_2D);
        //The upscaled image will be rendered with input images at full def, it is then the best possibly rendered image so cache it!

        fullScaleImage->reset();
        getOrCreateFromCacheInternal(key, upscaledImageParams, glContext, createInCache, fullScaleImage);

        if (!*fullScaleImage) {
            return false;
        }
    } else {
        ///Cache the image with the requested components instead of the remapped ones
        ImageParamsPtr cachedImgParams = Image::makeParams(rod,
                                                           downscaleImageBounds,
                                                           par,
                                                           mipmapLevel,
                                                           components,
                                                           depth,
                                                           premult,
                                                           fielding,
                                                           glContext,
                                                           storage,
                                                           GL_TEXTURE_2D);

        //Take the lock after getting the image from the cache or while allocating it
        ///to make sure a thread will not attempt to write to the image while its being allocated.
        ///When calling allocateMemory() on the image, the cache already has the lock since it added it
        ///so taking this lock now ensures the image will be allocated completetly

        getOrCreateFromCacheInternal(key, cachedImgParams, glContext, createInCache, downscaleImage);
        if (!*downscaleImage) {
            return false;
        }
        *fullScaleImage = *downscaleImage;
    }

    return true;
} // EffectInstance::allocateImagePlane



RenderRoIRetCode
EffectInstance::renderInputImagesForRoI(StorageModeEnum renderStorageMode,
                                        TimeValue time,
                                        ViewIdx view,
                                        unsigned int mipMapLevel,
                                        bool useScaleOneInputImages,
                                        bool byPassCache,
                                        const FramesNeededMap & framesNeeded,
                                        const ComponentsNeededMap & neededComps,
                                        InputImagesMap *inputImages)
{



    return treeRecurseFunctor(true,
                              getNode(),
                              framesNeeded,
                              0,
                              renderStorageMode,
                              mipMapLevel,
                              time,
                              view,
                              NodePtr(),
                              inputImages,
                              &neededComps,
                              useScaleOneInputImages,
                              byPassCache);
}

EffectInstance::RenderingFunctorRetEnum
EffectInstance::Implementation::tiledRenderingFunctor(EffectInstance::Implementation::TiledRenderingFunctorArgs & args,
                                                      const RectToRender & specificData,
                                                      QThread* callingThread)
{
    ///Make the thread-storage live as long as the render action is called if we're in a newly launched thread in eRenderSafetyFullySafeFrame mode
    QThread* curThread = QThread::currentThread();

    if (callingThread != curThread) {
        ///We are in the case of host frame threading, see kOfxImageEffectPluginPropHostFrameThreading
        ///We know that in the renderAction, TLS will be needed, so we do a deep copy of the TLS from the caller thread
        ///to this thread
        appPTR->getAppTLS()->copyTLS(callingThread, curThread);
    }


    EffectInstance::RenderingFunctorRetEnum ret = tiledRenderingFunctor(specificData,
                                                                        args.glContext,
                                                                        args.renderFullScaleThenDownscale,
                                                                        args.isSequentialRender,
                                                                        args.isRenderResponseToUserInteraction,
                                                                        args.preferredInput,
                                                                        args.mipMapLevel,
                                                                        args.renderMappedMipMapLevel,
                                                                        args.rod,
                                                                        args.time,
                                                                        args.view,
                                                                        args.par,
                                                                        args.byPassCache,
                                                                        args.outputClipPrefDepth,
                                                                        args.outputClipPrefsComps,
                                                                        args.compsNeeded,
                                                                        args.processChannels,
                                                                        args.planes);

    //Exit of the host frame threading thread
    if (callingThread != curThread) {
        appPTR->getAppTLS()->cleanupTLSForThread();
    }

    return ret;
}

void
EffectInstance::Implementation::tryShrinkRenderWindow(const EffectTLSDataPtr &tls,
                                                    const EffectInstance::RectToRender & rectToRender,
                                                    const PlaneToRender & firstPlaneToRender,
                                                    bool renderFullScaleThenDownscale,
                                                    unsigned int renderMappedMipMapLevel,
                                                    unsigned int mipMapLevel,
                                                    double par,
                                                    const RectD& rod,
                                                    RectI &renderMappedRectToRender,
                                                    RectI &downscaledRectToRender,
                                                    bool *isBeingRenderedElseWhere,
                                                    bool *bitmapMarkedForRendering)
{
    
    renderMappedRectToRender = rectToRender.rect;
    downscaledRectToRender = renderMappedRectToRender;
    
    
    {
        RectD canonicalRectToRender;
        renderMappedRectToRender.toCanonical(renderMappedMipMapLevel, par, rod, &canonicalRectToRender);
        if (renderFullScaleThenDownscale) {
            assert(mipMapLevel > 0 && renderMappedMipMapLevel != mipMapLevel);
            canonicalRectToRender.toPixelEnclosing(mipMapLevel, par, &downscaledRectToRender);
        }
    }

    // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
# ifndef NDEBUG

    RectI renderBounds = firstPlaneToRender.renderMappedImage->getBounds();
    assert(renderBounds.x1 <= renderMappedRectToRender.x1 && renderMappedRectToRender.x2 <= renderBounds.x2 &&
           renderBounds.y1 <= renderMappedRectToRender.y1 && renderMappedRectToRender.y2 <= renderBounds.y2);

# endif

    *isBeingRenderedElseWhere = false;
    ///At this point if we're in eRenderSafetyFullySafeFrame mode, we are a thread that might have been launched way after
    ///the time renderRectToRender was computed. We recompute it to update the portion to render.
    ///Note that if it is bigger than the initial rectangle, we don't render the bigger rectangle since we cannot
    ///now make the preliminaries call to handle that region (getRoI etc...) so just stick with the old rect to render

    // check the bitmap!
    *bitmapMarkedForRendering = false;
    ParallelRenderArgsPtr frameArgs = tls->getParallelRenderArgs();
    if (frameArgs->tilesSupported) {
        if (renderFullScaleThenDownscale) {

            RectI initialRenderRect = renderMappedRectToRender;

#if NATRON_ENABLE_TRIMAP
            if ( frameArgs->isCurrentFrameRenderNotAbortable() ) {
                *bitmapMarkedForRendering = true;
                renderMappedRectToRender = firstPlaneToRender.renderMappedImage->getMinimalRectAndMarkForRendering_trimap(renderMappedRectToRender, isBeingRenderedElseWhere);
            } else {
                renderMappedRectToRender = firstPlaneToRender.renderMappedImage->getMinimalRect(renderMappedRectToRender);
            }
#else
            renderMappedRectToRender = renderMappedImage->getMinimalRect(renderMappedRectToRender);
#endif

            ///If the new rect after getMinimalRect is bigger (maybe because another thread as grown the image)
            ///we stick to what was requested
            if ( !initialRenderRect.contains(renderMappedRectToRender) ) {
                renderMappedRectToRender = initialRenderRect;
            }

            RectD canonicalReducedRectToRender;
            renderMappedRectToRender.toCanonical(renderMappedMipMapLevel, par, rod, &canonicalReducedRectToRender);
            canonicalReducedRectToRender.toPixelEnclosing(mipMapLevel, par, &downscaledRectToRender);


            assert( renderMappedRectToRender.isNull() ||
                   (renderBounds.x1 <= renderMappedRectToRender.x1 && renderMappedRectToRender.x2 <= renderBounds.x2 && renderBounds.y1 <= renderMappedRectToRender.y1 && renderMappedRectToRender.y2 <= renderBounds.y2) );
        } else {
            //The downscaled image is cached, read bitmap from it
#if NATRON_ENABLE_TRIMAP
            RectI rectToRenderMinimal;
            if ( frameArgs->isCurrentFrameRenderNotAbortable() ) {
                *bitmapMarkedForRendering = true;
                rectToRenderMinimal = firstPlaneToRender.downscaleImage->getMinimalRectAndMarkForRendering_trimap(renderMappedRectToRender, isBeingRenderedElseWhere);
            } else {
                rectToRenderMinimal = firstPlaneToRender.downscaleImage->getMinimalRect(renderMappedRectToRender);
            }
#else
            const RectI rectToRenderMinimal = downscaledImage->getMinimalRect(renderMappedRectToRender);
#endif

            assert( renderMappedRectToRender.isNull() ||
                   (renderBounds.x1 <= rectToRenderMinimal.x1 && rectToRenderMinimal.x2 <= renderBounds.x2 && renderBounds.y1 <= rectToRenderMinimal.y1 && rectToRenderMinimal.y2 <= renderBounds.y2) );


            ///If the new rect after getMinimalRect is bigger (maybe because another thread as grown the image)
            ///we stick to what was requested
            if ( !renderMappedRectToRender.contains(rectToRenderMinimal) ) {
                renderMappedRectToRender = rectToRenderMinimal;
            }
            downscaledRectToRender = renderMappedRectToRender;
        }
    } // tilesSupported

#ifndef NDEBUG
    {
        RenderScale scale( Image::getScaleFromMipMapLevel(mipMapLevel) );
        // check the dimensions of all input and output images
        const RectD & dstRodCanonical = firstPlaneToRender.renderMappedImage->getRoD();
        RectI dstBounds;
        dstRodCanonical.toPixelEnclosing(firstPlaneToRender.renderMappedImage->getMipMapLevel(), par, &dstBounds); // compute dstRod at level 0
        RectI dstRealBounds = firstPlaneToRender.renderMappedImage->getBounds();
        if (!frameArgs->tilesSupported && !_publicInterface->getNode()->isDuringPaintStrokeCreation()) {
            assert(dstRealBounds.x1 == dstBounds.x1);
            assert(dstRealBounds.x2 == dstBounds.x2);
            assert(dstRealBounds.y1 == dstBounds.y1);
            assert(dstRealBounds.y2 == dstBounds.y2);
        }

        if (renderFullScaleThenDownscale) {
            assert(firstPlaneToRender.renderMappedImage->getMipMapLevel() == 0);
            assert(renderMappedMipMapLevel == 0);
        }
    }
#     endif // DEBUG
}

EffectInstance::RenderingFunctorRetEnum
EffectInstance::Implementation::tiledRenderingFunctor(const RectToRender & rectToRender,
                                                      const OSGLContextPtr& glContext,
                                                      const bool renderFullScaleThenDownscale,
                                                      const bool isSequentialRender,
                                                      const bool isRenderResponseToUserInteraction,
                                                      const int preferredInput,
                                                      const unsigned int mipMapLevel,
                                                      const unsigned int renderMappedMipMapLevel,
                                                      const RectD & rod,
                                                      const TimeValue time,
                                                      const ViewIdx view,
                                                      const double par,
                                                      const bool byPassCache,
                                                      const ImageBitDepthEnum outputClipPrefDepth,
                                                      const ImageComponents & outputClipPrefsComps,
                                                      const ComponentsNeededMapPtr & compsNeeded,
                                                      const std::bitset<4>& processChannels,
                                                      const ImagePlanesToRenderPtr & planes) // when MT, planes is a copy so there's is no data race
{
    // There cannot be the same thread running 2 concurrent instances of renderRoI on the same effect.
    // The code below was commented-out: Since in the tiledRenderingFunctor() function calling this one we do a deep copy of the thread local storage of the spawning thread,
    // we also copy the currentRenderArgs which may already have their validArgs flag set to true, so remove the assert.
/*#ifdef DEBUG
    {
        EffectTLSDataPtr tls = tlsData->getTLSData();
        assert(!tls || !tls->currentRenderArgs.validArgs);
    }
#endif*/

    EffectTLSDataPtr tls = tlsData->getOrCreateTLSData();

    // We may have copied the TLS from a thread that spawned us. The other thread might have already started the render actino:
    // ensure that we start this thread with a clean state.
    tls->ensureLastActionInStackIsNotRender();

    // renderMappedRectToRender is in the mapped mipmap level, i.e the expected mipmap level of the render action of the plug-in
    // downscaledRectToRender is in the mipMapLevel
    RectI renderMappedRectToRender, downscaledRectToRender;
    const PlaneToRender & firstPlaneToRender = planes->planes.begin()->second;
    bool isBeingRenderedElseWhere,bitmapMarkedForRendering;
    tryShrinkRenderWindow(tls, rectToRender, firstPlaneToRender, renderFullScaleThenDownscale, renderMappedMipMapLevel, mipMapLevel, par, rod, renderMappedRectToRender, downscaledRectToRender, &isBeingRenderedElseWhere, &bitmapMarkedForRendering);

    // It might have been already rendered now
    if ( renderMappedRectToRender.isNull() ) {
        return isBeingRenderedElseWhere ? eRenderingFunctorRetTakeImageLock : eRenderingFunctorRetOK;
    }



    TimeLapsePtr timeRecorder;
    RenderActionArgs actionArgs;
    boost::shared_ptr<OSGLContextAttacher> glContextAttacher;
    setupRenderArgs(tls, glContext, time, view, mipMapLevel, isSequentialRender, isRenderResponseToUserInteraction, byPassCache, *planes, renderMappedRectToRender, processChannels, planes->inputImages, actionArgs, &glContextAttacher, &timeRecorder);



    // If this tile is identity, copy input image instead
    if (rectToRender.isIdentity) {
        return renderHandlerIdentity(tls, rectToRender, glContext, renderFullScaleThenDownscale, renderMappedRectToRender, downscaledRectToRender, outputClipPrefDepth, actionArgs.time, actionArgs.view, mipMapLevel, timeRecorder, *planes);
    }

    // Call render
    std::map<ImageComponents, PlaneToRender> outputPlanes;
    bool multiPlanar = _publicInterface->isMultiPlanar();
    {
        RenderingFunctorRetEnum internalRet = renderHandlerInternal(tls, glContext, actionArgs, planes, multiPlanar, bitmapMarkedForRendering, outputClipPrefsComps, outputClipPrefDepth, compsNeeded, outputPlanes, &glContextAttacher);
        if (internalRet != eRenderingFunctorRetOK) {
            return internalRet;
        }
    }

    // Apply post-processing
    renderHandlerPostProcess(tls, preferredInput, glContext, actionArgs, *planes, downscaledRectToRender, timeRecorder, renderFullScaleThenDownscale, mipMapLevel, outputPlanes, processChannels);

    if (isBeingRenderedElseWhere) {
        return eRenderingFunctorRetTakeImageLock;
    } else {
        return eRenderingFunctorRetOK;
    }

} // EffectInstance::tiledRenderingFunctor


EffectInstance::RenderingFunctorRetEnum
EffectInstance::Implementation::renderHandlerIdentity(const EffectTLSDataPtr& tls,
                                                      const RectToRender & rectToRender,
                                                      const OSGLContextPtr& glContext,
                                                      const bool renderFullScaleThenDownscale,
                                                      const RectI & renderMappedRectToRender,
                                                      const RectI & downscaledRectToRender,
                                                      const ImageBitDepthEnum outputClipPrefDepth,
                                                      const TimeValue time,
                                                      const ViewIdx view,
                                                      const unsigned int mipMapLevel,
                                                      const TimeLapsePtr& timeRecorder,
                                                      EffectInstance::ImagePlanesToRender & planes)
{
    std::list<ImageComponents> comps;
    ParallelRenderArgsPtr frameArgs = tls->getParallelRenderArgs();
    for (std::map<ImageComponents, PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it) {
        //If color plane, request the preferred comp of the identity input
        if ( rectToRender.identityInput && it->second.renderMappedImage->getComponents().isColorPlane() ) {
            ImageComponents prefInputComps = rectToRender.identityInput->getComponents(-1);
            comps.push_back(prefInputComps);
        } else {
            comps.push_back( it->second.renderMappedImage->getComponents() );
        }
    }
    assert( !comps.empty() );
    std::map<ImageComponents, ImagePtr> identityPlanes;
    boost::scoped_ptr<EffectInstance::RenderRoIArgs> renderArgs( new EffectInstance::RenderRoIArgs(rectToRender.identityTime,
                                                                                                   Image::getScaleFromMipMapLevel(mipMapLevel),
                                                                                                   mipMapLevel,
                                                                                                   view,
                                                                                                   false,
                                                                                                   downscaledRectToRender,
                                                                                                   comps,
                                                                                                   outputClipPrefDepth,
                                                                                                   false,
                                                                                                   _publicInterface->shared_from_this(),
                                                                                                   rectToRender.identityInputNumber,
                                                                                                   planes.useOpenGL ? eStorageModeGLTex : eStorageModeRAM,
                                                                                                   time) );
    if (!rectToRender.identityInput) {
        for (std::map<ImageComponents, PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it) {
            it->second.renderMappedImage->fillZero(renderMappedRectToRender, glContext);
            it->second.renderMappedImage->markForRendered(renderMappedRectToRender);

            if ( frameArgs->stats && frameArgs->stats->isInDepthProfilingEnabled() ) {
                frameArgs->stats->addRenderInfosForNode( _publicInterface->getNode(),  NodePtr(), it->first.getComponentsGlobalName(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation() );
            }
        }

        return eRenderingFunctorRetOK;
    } else {
        RenderRoIRetCode renderOk;
        renderOk = rectToRender.identityInput->renderRoI(*renderArgs, &identityPlanes);
        if (renderOk == eRenderRoIRetCodeAborted) {
            return eRenderingFunctorRetAborted;
        } else if (renderOk == eRenderRoIRetCodeFailed) {
            return eRenderingFunctorRetFailed;
        } else if ( identityPlanes.empty() ) {
            for (std::map<ImageComponents, PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it) {
                it->second.renderMappedImage->fillZero(renderMappedRectToRender, glContext);
                it->second.renderMappedImage->markForRendered(renderMappedRectToRender);

                if ( frameArgs->stats && frameArgs->stats->isInDepthProfilingEnabled() ) {
                    frameArgs->stats->addRenderInfosForNode( _publicInterface->getNode(),  rectToRender.identityInput->getNode(), it->first.getComponentsGlobalName(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation() );
                }
            }

            return eRenderingFunctorRetOK;
        } else {
            assert( identityPlanes.size() == planes.planes.size() );

            std::map<ImageComponents, ImagePtr>::iterator idIt = identityPlanes.begin();
            for (std::map<ImageComponents, PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it, ++idIt) {
                if ( renderFullScaleThenDownscale && ( idIt->second->getMipMapLevel() > it->second.fullscaleImage->getMipMapLevel() ) ) {
                    // We cannot be rendering using OpenGL in this case
                    assert(!planes.useOpenGL);


                    if ( !idIt->second->getBounds().contains(renderMappedRectToRender) ) {
                        ///Fill the RoI with 0's as the identity input image might have bounds contained into the RoI
                        it->second.fullscaleImage->fillZero(renderMappedRectToRender, glContext);
                    }

                    ///Convert format first if needed
                    ImagePtr sourceImage;
                    if ( ( it->second.fullscaleImage->getComponents() != idIt->second->getComponents() ) || ( it->second.fullscaleImage->getBitDepth() != idIt->second->getBitDepth() ) ) {
                        sourceImage.reset( new Image(it->second.fullscaleImage->getComponents(),
                                                     idIt->second->getRoD(),
                                                     idIt->second->getBounds(),
                                                     idIt->second->getMipMapLevel(),
                                                     idIt->second->getPixelAspectRatio(),
                                                     it->second.fullscaleImage->getBitDepth(),
                                                     idIt->second->getPremultiplication(),
                                                     idIt->second->getFieldingOrder(),
                                                     false,
                                                     eStorageModeRAM,
                                                     OSGLContextPtr(),
                                                     GL_TEXTURE_2D,
                                                     true) );

                        ViewerColorSpaceEnum colorspace = _publicInterface->getApp()->getDefaultColorSpaceForBitDepth( idIt->second->getBitDepth() );
                        ViewerColorSpaceEnum dstColorspace = _publicInterface->getApp()->getDefaultColorSpaceForBitDepth( it->second.fullscaleImage->getBitDepth() );
                        idIt->second->convertToFormat( idIt->second->getBounds(), colorspace, dstColorspace, 3, false, false, sourceImage.get() );
                    } else {
                        sourceImage = idIt->second;
                    }

                    ///then upscale
                    const RectD & rod = sourceImage->getRoD();
                    RectI bounds;
                    rod.toPixelEnclosing(it->second.renderMappedImage->getMipMapLevel(), it->second.renderMappedImage->getPixelAspectRatio(), &bounds);
                    ImagePtr inputPlane( new Image(it->first,
                                                   rod,
                                                   bounds,
                                                   it->second.renderMappedImage->getMipMapLevel(),
                                                   it->second.renderMappedImage->getPixelAspectRatio(),
                                                   it->second.renderMappedImage->getBitDepth(),
                                                   it->second.renderMappedImage->getPremultiplication(),
                                                   it->second.renderMappedImage->getFieldingOrder(),
                                                   false,
                                                   eStorageModeRAM,
                                                   OSGLContextPtr(), GL_TEXTURE_2D, true) );
                    sourceImage->upscaleMipMap( sourceImage->getBounds(), sourceImage->getMipMapLevel(), inputPlane->getMipMapLevel(), inputPlane.get() );
                    it->second.fullscaleImage->pasteFrom(*inputPlane, renderMappedRectToRender, false);
                    it->second.fullscaleImage->markForRendered(renderMappedRectToRender);
                } else {
                    if ( !idIt->second->getBounds().contains(downscaledRectToRender) ) {
                        ///Fill the RoI with 0's as the identity input image might have bounds contained into the RoI
                        it->second.downscaleImage->fillZero(downscaledRectToRender, glContext);
                    }

                    ///Convert format if needed or copy
                    if ( ( it->second.downscaleImage->getComponents() != idIt->second->getComponents() ) || ( it->second.downscaleImage->getBitDepth() != idIt->second->getBitDepth() ) ) {
                        ViewerColorSpaceEnum colorspace = _publicInterface->getApp()->getDefaultColorSpaceForBitDepth( idIt->second->getBitDepth() );
                        ViewerColorSpaceEnum dstColorspace = _publicInterface->getApp()->getDefaultColorSpaceForBitDepth( it->second.fullscaleImage->getBitDepth() );
                        RectI convertWindow;
                        if (idIt->second->getBounds().intersect(downscaledRectToRender, &convertWindow)) {
                            idIt->second->convertToFormat( convertWindow, colorspace, dstColorspace, 3, false, false, it->second.downscaleImage.get() );
                        }
                    } else {
                        it->second.downscaleImage->pasteFrom(*(idIt->second), downscaledRectToRender, false, glContext);
                    }
                    it->second.downscaleImage->markForRendered(downscaledRectToRender);
                }

                if ( frameArgs->stats && frameArgs->stats->isInDepthProfilingEnabled() ) {
                    frameArgs->stats->addRenderInfosForNode( _publicInterface->getNode(),  rectToRender.identityInput->getNode(), it->first.getComponentsGlobalName(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation() );
                }
            }

            return eRenderingFunctorRetOK;
        } // if (renderOk == eRenderRoIRetCodeAborted) {
    }  //  if (!identityInput) {
} // renderHandlerIdentity

template <typename GL>
static void setupGLForRender(const ImagePtr& image,
                             const OSGLContextPtr& glContext,
                             const RectI& roi,
                             bool callGLFinish,
                             boost::shared_ptr<OSGLContextAttacher>* glContextAttacher)
{
#ifndef DEBUG
    Q_UNUSED(time);
#endif
    RectI imageBounds = image->getBounds();
    RectI viewportBounds;
    if (GL::isGPU()) {
        assert(image->getParams()->getStorageInfo().glContext.lock() == glContext);
        viewportBounds = imageBounds;
        int textureTarget = image->getGLTextureTarget();
        GL::Enable(textureTarget);
        assert(image->getStorageMode() == eStorageModeGLTex);

        GL::ActiveTexture(GL_TEXTURE0);
        GL::BindTexture( textureTarget, image->getGLTextureID() );
        assert(GL::IsTexture(image->getGLTextureID()));
        assert(GL::GetError() == GL_NO_ERROR);
        glCheckError(GL);
        GL::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureTarget, image->getGLTextureID(), 0 /*LoD*/);
        glCheckError(GL);
        assert(GL::CheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        glCheckFramebufferError(GL);
    } else {
        viewportBounds = roi;
        assert(image->getStorageMode() == eStorageModeDisk || image->getStorageMode() == eStorageModeRAM);
        Image::WriteAccess outputWriteAccess(image.get());
        unsigned char* data = outputWriteAccess.pixelAt(roi.x1, roi.y1);
        assert(data);

        // With OSMesa we render directly to the context framebuffer
        *glContextAttacher = OSGLContextAttacher::create(glContext, roi.width(), roi.height(), imageBounds.width() , data);
        (*glContextAttacher)->attach();
    }

    // setup the output viewport
    Image::setupGLViewport<GL>(viewportBounds, roi);

    // Enable scissor to make the plug-in not render outside of the viewport...
    GL::Enable(GL_SCISSOR_TEST);
    GL::Scissor( roi.x1 - viewportBounds.x1, roi.y1 - viewportBounds.y1, roi.width(), roi.height() );

    if (callGLFinish) {
        // Ensure that previous asynchronous operations are done (e.g: glTexImage2D) some plug-ins seem to require it (Hitfilm Ignite plugin-s)
        GL::Finish();
    }

}

template <typename GL>
static void finishGLRender()
{
    GL::Disable(GL_SCISSOR_TEST);
    GL::ActiveTexture(GL_TEXTURE0);
    GL::BindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!GL::isGPU()) {
        GL::Flush(); // waits until commands are submitted but does not wait for the commands to finish executing
        GL::Finish(); // waits for all previously submitted commands to complete executing
    }
    glCheckError(GL);
}

EffectInstance::RenderingFunctorRetEnum
EffectInstance::Implementation::renderHandlerInternal(const EffectTLSDataPtr& tls,
                                                      const OSGLContextPtr& glContext,
                                                      EffectInstance::RenderActionArgs &actionArgs,
                                                      const ImagePlanesToRenderPtr & planes,
                                                      bool multiPlanar,
                                                      bool bitmapMarkedForRendering,
                                                      const ImageComponents & outputClipPrefsComps,
                                                      const ImageBitDepthEnum outputClipPrefDepth,
                                                      const ComponentsNeededMapPtr & compsNeeded,
                                                      std::map<ImageComponents, PlaneToRender>& outputPlanes,
                                                      boost::shared_ptr<OSGLContextAttacher>* glContextAttacher)
{
    ParallelRenderArgsPtr frameArgs = tls->getParallelRenderArgs();
    std::list<std::pair<ImageComponents, ImagePtr> > tmpPlanes;
    for (std::map<ImageComponents, PlaneToRender>::iterator it = planes->planes.begin(); it != planes->planes.end(); ++it) {
        /*
         * When using the cache, allocate a local temporary buffer onto which the plug-in will render, and then safely
         * copy this buffer to the shared (among threads) image.
         * This is also needed if the plug-in does not support the number of components of the renderMappedImage
         */
        ImageComponents prefComp;
        if (multiPlanar) {
            prefComp = _publicInterface->getNode()->findClosestSupportedComponents( -1, it->second.renderMappedImage->getComponents() );
        } else {
            prefComp = outputClipPrefsComps;
        }

        // OpenGL render never use the cache and bitmaps, all images are local to a render.
        if ( ( it->second.renderMappedImage->usesBitMap() || ( prefComp != it->second.renderMappedImage->getComponents() ) ||
              ( outputClipPrefDepth != it->second.renderMappedImage->getBitDepth() ) ) && !_publicInterface->isPaintingOverItselfEnabled() && !planes->useOpenGL ) {
            it->second.tmpImage.reset( new Image(prefComp,
                                                 it->second.renderMappedImage->getRoD(),
                                                 actionArgs.roi,
                                                 it->second.renderMappedImage->getMipMapLevel(),
                                                 it->second.renderMappedImage->getPixelAspectRatio(),
                                                 outputClipPrefDepth,
                                                 it->second.renderMappedImage->getPremultiplication(),
                                                 it->second.renderMappedImage->getFieldingOrder(),
                                                 false,
                                                 eStorageModeRAM,
                                                 OSGLContextPtr(),
                                                 GL_TEXTURE_2D,
                                                 true) ); //< no bitmap
        } else {
            it->second.tmpImage = it->second.renderMappedImage;
        }
        tmpPlanes.push_back( std::make_pair(it->second.renderMappedImage->getComponents(), it->second.tmpImage) );
    }

#if NATRON_ENABLE_TRIMAP
    if ( !bitmapMarkedForRendering && frameArgs->isCurrentFrameRenderNotAbortable() ) {
        for (std::map<ImageComponents, PlaneToRender>::iterator it = planes->planes.begin(); it != planes->planes.end(); ++it) {
            it->second.renderMappedImage->markForRendering(actionArgs.roi);
        }
    }
#endif

    std::list< std::list<std::pair<ImageComponents, ImagePtr> > > planesLists;
    if (!multiPlanar) {
        for (std::list<std::pair<ImageComponents, ImagePtr> >::iterator it = tmpPlanes.begin(); it != tmpPlanes.end(); ++it) {
            std::list<std::pair<ImageComponents, ImagePtr> > tmp;
            tmp.push_back(*it);
            planesLists.push_back(tmp);
        }
    } else {
        planesLists.push_back(tmpPlanes);
    }


    bool renderAborted = false;
    for (std::list<std::list<std::pair<ImageComponents, ImagePtr> > >::iterator it = planesLists.begin(); it != planesLists.end(); ++it) {
   
        actionArgs.outputPlanes = *it;
        const ImagePtr& mainImagePlane = actionArgs.outputPlanes.front().second;
        if (planes->useOpenGL) {
            actionArgs.glContextData = planes->glContextData;

            // Effects that render multiple planes at once are NOT supported by the OpenGL render suite
            // We only bind to the framebuffer color attachment 0 the "main" output image plane
            assert(actionArgs.outputPlanes.size() == 1);
            if (glContext->isGPUContext()) {
                setupGLForRender<GL_GPU>(mainImagePlane, glContext, actionArgs.roi, _publicInterface->getNode()->isGLFinishRequiredBeforeRender(), glContextAttacher);
            } else {
                setupGLForRender<GL_CPU>(mainImagePlane, glContext, actionArgs.roi, _publicInterface->getNode()->isGLFinishRequiredBeforeRender(), glContextAttacher);
            }
        }
        StatusEnum st;
        {

            ///This RAII struct controls the lifetime of the render action arguments
            RenderActionArgsSetter_RAII actionArgsTls(tls,
                                                      actionArgs.time,
                                                      actionArgs.view,
                                                      actionArgs.mappedScale,
                                                      actionArgs.roi,
                                                      planes->inputImages,
                                                      planes->planes,
                                                      compsNeeded);

             st = _publicInterface->render_public(actionArgs);
        }

        if (planes->useOpenGL) {
            if (glContext->isGPUContext()) {
                GL_GPU::BindTexture(mainImagePlane->getGLTextureTarget(), 0);
                finishGLRender<GL_GPU>();
            } else {
                finishGLRender<GL_CPU>();
            }
        }

        renderAborted = _publicInterface->aborted();

        /*
         * Since new planes can have been allocated on the fly by allocateImagePlaneAndSetInThreadLocalStorage(), refresh
         * the planes map from the thread local storage once the render action is finished
         */
        if ( it == planesLists.begin() ) {
            tls->getCurrentRenderActionArgs(0, 0, 0, 0, 0, &outputPlanes, 0);
            assert( !outputPlanes.empty() );
        }

        if ( (st != eStatusOK) || renderAborted ) {
#if NATRON_ENABLE_TRIMAP
            if ( frameArgs->isCurrentFrameRenderNotAbortable() ) {
                /*
                 At this point, another thread might have already gotten this image from the cache and could end-up
                 using it while it has still pixels marked to PIXEL_UNAVAILABLE, hence clear the bitmap
                 */
                for (std::map<ImageComponents, PlaneToRender>::const_iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {
                    it->second.renderMappedImage->clearBitmap(actionArgs.roi);
                }
            }
#endif
            switch (st) {
                case eStatusFailed:

                    return eRenderingFunctorRetFailed;
                case eStatusOutOfMemory:

                    return eRenderingFunctorRetOutOfGPUMemory;
                case eStatusOK:
                default:

                    return eRenderingFunctorRetAborted;
            }
        } // if (st != eStatusOK || renderAborted) {
        
    } // for (std::list<std::list<std::pair<ImageComponents,ImagePtr> > >::iterator it = planesLists.begin(); it != planesLists.end(); ++it)
    
    assert(!renderAborted);
    return eRenderingFunctorRetOK;
} // EffectInstance::Implementation::renderHandlerInternal


void
EffectInstance::Implementation::renderHandlerPostProcess(const EffectTLSDataPtr& tls,
                                                         int preferredInput,
                                                         const OSGLContextPtr& glContext,
                                                         const EffectInstance::RenderActionArgs &actionArgs,
                                                         const ImagePlanesToRender & planes,
                                                         const RectI& downscaledRectToRender,
                                                         const TimeLapsePtr& timeRecorder,
                                                         bool renderFullScaleThenDownscale,
                                                         unsigned int mipMapLevel,
                                                         const std::map<ImageComponents, PlaneToRender>& outputPlanes,
                                                         const std::bitset<4>& processChannels)
{

    ParallelRenderArgsPtr frameArgs = tls->getParallelRenderArgs();

    ImagePtr originalInputImage, maskImage;
    ImagePremultiplicationEnum originalImagePremultiplication;
    InputImagesMap::const_iterator foundPrefInput = planes.inputImages.find(preferredInput);
    InputImagesMap::const_iterator foundMaskInput = planes.inputImages.end();

    bool hostMasking = _publicInterface->isHostMaskingEnabled();
    if ( hostMasking ) {
        foundMaskInput = planes.inputImages.find(_publicInterface->getMaxInputCount() - 1);
    }
    if ( ( foundPrefInput != planes.inputImages.end() ) && !foundPrefInput->second.empty() ) {
        originalInputImage = foundPrefInput->second.front();
    }
    std::map<int, ImagePremultiplicationEnum>::const_iterator foundPrefPremult = planes.inputPremult.find(preferredInput);
    if ( ( foundPrefPremult != planes.inputPremult.end() ) && originalInputImage ) {
        originalImagePremultiplication = foundPrefPremult->second;
    } else {
        originalImagePremultiplication = eImagePremultiplicationOpaque;
    }


    if ( ( foundMaskInput != planes.inputImages.end() ) && !foundMaskInput->second.empty() ) {
        maskImage = foundMaskInput->second.front();
    }

    // A node that is part of a stroke render implementation needs to accumulate so set the last rendered image pointer
    RotoStrokeItemPtr attachedItem = toRotoStrokeItem(_publicInterface->getNode()->getAttachedRotoItem());

    bool unPremultIfNeeded = planes.outputPremult == eImagePremultiplicationPremultiplied;
    bool useMaskMix = hostMasking || _publicInterface->isHostMixingEnabled();
    double mix = useMaskMix ? _publicInterface->getNode()->getHostMixingValue(actionArgs.time, actionArgs.view) : 1.;
    bool doMask = useMaskMix ? _publicInterface->getNode()->isMaskEnabled(_publicInterface->getMaxInputCount() - 1) : false;

    //Check for NaNs, copy to output image and mark for rendered
    for (std::map<ImageComponents, PlaneToRender>::const_iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {
        bool unPremultRequired = unPremultIfNeeded && it->second.tmpImage->getComponentsCount() == 4 && it->second.renderMappedImage->getComponentsCount() == 3;

        if ( frameArgs->doNansHandling && it->second.tmpImage->checkForNaNs(actionArgs.roi) ) {
            QString warning = QString::fromUtf8( _publicInterface->getNode()->getScriptName_mt_safe().c_str() );
            warning.append( QString::fromUtf8(": ") );
            warning.append( tr("rendered rectangle (") );
            warning.append( QString::number(actionArgs.roi.x1) );
            warning.append( QChar::fromLatin1(',') );
            warning.append( QString::number(actionArgs.roi.y1) );
            warning.append( QString::fromUtf8(")-(") );
            warning.append( QString::number(actionArgs.roi.x2) );
            warning.append( QChar::fromLatin1(',') );
            warning.append( QString::number(actionArgs.roi.y2) );
            warning.append( QString::fromUtf8(") ") );
            warning.append( tr("contains NaN values. They have been converted to 1.") );
            _publicInterface->setPersistentMessage( eMessageTypeWarning, warning.toStdString() );
        }
        if (it->second.isAllocatedOnTheFly) {
            ///Plane allocated on the fly only have a temp image if using the cache and it is defined over the render window only
            if (it->second.tmpImage != it->second.renderMappedImage) {
                // We cannot be rendering using OpenGL in this case
                assert(!planes.useOpenGL);

                assert(it->second.tmpImage->getBounds() == actionArgs.roi);

                if ( ( it->second.renderMappedImage->getComponents() != it->second.tmpImage->getComponents() ) ||
                    ( it->second.renderMappedImage->getBitDepth() != it->second.tmpImage->getBitDepth() ) ) {
                    it->second.tmpImage->convertToFormat( it->second.tmpImage->getBounds(),
                                                         _publicInterface->getApp()->getDefaultColorSpaceForBitDepth( it->second.tmpImage->getBitDepth() ),
                                                         _publicInterface->getApp()->getDefaultColorSpaceForBitDepth( it->second.renderMappedImage->getBitDepth() ),
                                                         -1, false, unPremultRequired, it->second.renderMappedImage.get() );
                } else {
                    it->second.renderMappedImage->pasteFrom(*(it->second.tmpImage), it->second.tmpImage->getBounds(), false);
                }
            }
            it->second.renderMappedImage->markForRendered(actionArgs.roi);
        } else {
            if (renderFullScaleThenDownscale) {
                // We cannot be rendering using OpenGL in this case
                assert(!planes.useOpenGL);

                ///copy the rectangle rendered in the full scale image to the downscaled output
                assert(mipMapLevel != 0);

                assert(it->second.fullscaleImage != it->second.downscaleImage && it->second.renderMappedImage == it->second.fullscaleImage);

                ImagePtr mappedOriginalInputImage = originalInputImage;

                if ( originalInputImage && (originalInputImage->getMipMapLevel() != 0) ) {
                    bool mustCopyUnprocessedChannels = it->second.tmpImage->canCallCopyUnProcessedChannels(processChannels);
                    if (mustCopyUnprocessedChannels || useMaskMix) {
                        ///there is some processing to be done by copyUnProcessedChannels or applyMaskMix
                        ///but originalInputImage is not in the correct mipMapLevel, upscale it
                        assert(originalInputImage->getMipMapLevel() > it->second.tmpImage->getMipMapLevel() &&
                               originalInputImage->getMipMapLevel() == mipMapLevel);
                        ImagePtr tmp( new Image(it->second.tmpImage->getComponents(),
                                                it->second.tmpImage->getRoD(),
                                                actionArgs.roi,
                                                0,
                                                it->second.tmpImage->getPixelAspectRatio(),
                                                it->second.tmpImage->getBitDepth(),
                                                it->second.tmpImage->getPremultiplication(),
                                                it->second.tmpImage->getFieldingOrder(),
                                                false,
                                                eStorageModeRAM,
                                                OSGLContextPtr(),
                                                GL_TEXTURE_2D,
                                                true) );
                        originalInputImage->upscaleMipMap( downscaledRectToRender, originalInputImage->getMipMapLevel(), 0, tmp.get() );
                        mappedOriginalInputImage = tmp;
                    }
                }

                if (mappedOriginalInputImage) {
                    it->second.tmpImage->copyUnProcessedChannels(actionArgs.roi, planes.outputPremult, originalImagePremultiplication, processChannels, mappedOriginalInputImage, true);
                    if (useMaskMix) {
                        it->second.tmpImage->applyMaskMix(actionArgs.roi, maskImage.get(), mappedOriginalInputImage.get(), doMask, false, mix);
                    }
                }
                if ( ( it->second.fullscaleImage->getComponents() != it->second.tmpImage->getComponents() ) ||
                    ( it->second.fullscaleImage->getBitDepth() != it->second.tmpImage->getBitDepth() ) ) {
                    /*
                     * BitDepth/Components conversion required as well as downscaling, do conversion to a tmp buffer
                     */
                    ImagePtr tmp( new Image(it->second.fullscaleImage->getComponents(),
                                            it->second.tmpImage->getRoD(),
                                            actionArgs.roi,
                                            mipMapLevel,
                                            it->second.tmpImage->getPixelAspectRatio(),
                                            it->second.fullscaleImage->getBitDepth(),
                                            it->second.fullscaleImage->getPremultiplication(),
                                            it->second.fullscaleImage->getFieldingOrder(),
                                            false,
                                            eStorageModeRAM,
                                            OSGLContextPtr(), GL_TEXTURE_2D, true) );

                    it->second.tmpImage->convertToFormat( actionArgs.roi,
                                                         _publicInterface->getApp()->getDefaultColorSpaceForBitDepth( it->second.tmpImage->getBitDepth() ),
                                                         _publicInterface->getApp()->getDefaultColorSpaceForBitDepth( it->second.fullscaleImage->getBitDepth() ),
                                                         -1, false, unPremultRequired, tmp.get() );
                    tmp->downscaleMipMap( it->second.tmpImage->getRoD(),
                                         actionArgs.roi, 0, mipMapLevel, false, it->second.downscaleImage.get() );
                    it->second.fullscaleImage->pasteFrom(*tmp, actionArgs.roi, false);
                } else {
                    /*
                     *  Downscaling required only
                     */
                    it->second.tmpImage->downscaleMipMap( it->second.tmpImage->getRoD(),
                                                         actionArgs.roi, 0, mipMapLevel, false, it->second.downscaleImage.get() );
                    it->second.fullscaleImage->pasteFrom(*(it->second.tmpImage), actionArgs.roi, false);
                }


                it->second.fullscaleImage->markForRendered(actionArgs.roi);
            } else { // if (renderFullScaleThenDownscale) {
                ///Copy the rectangle rendered in the downscaled image
                if (it->second.tmpImage != it->second.downscaleImage) {
                    // We cannot be rendering using OpenGL in this case
                    assert(!planes.useOpenGL);

                    if ( ( it->second.downscaleImage->getComponents() != it->second.tmpImage->getComponents() ) ||
                        ( it->second.downscaleImage->getBitDepth() != it->second.tmpImage->getBitDepth() ) ) {
                        /*
                         * BitDepth/Components conversion required
                         */


                        it->second.tmpImage->convertToFormat( it->second.tmpImage->getBounds(),
                                                             _publicInterface->getApp()->getDefaultColorSpaceForBitDepth( it->second.tmpImage->getBitDepth() ),
                                                             _publicInterface->getApp()->getDefaultColorSpaceForBitDepth( it->second.downscaleImage->getBitDepth() ),
                                                             -1, false, unPremultRequired, it->second.downscaleImage.get() );
                    } else {
                        /*
                         * No conversion required, copy to output
                         */

                        it->second.downscaleImage->pasteFrom(*(it->second.tmpImage), it->second.downscaleImage->getBounds(), false);
                    }
                }

                it->second.downscaleImage->copyUnProcessedChannels(actionArgs.roi, planes.outputPremult, originalImagePremultiplication, processChannels, originalInputImage, true, glContext);
                if (useMaskMix) {
                    it->second.downscaleImage->applyMaskMix(actionArgs.roi, maskImage.get(), originalInputImage.get(), doMask, false, mix, glContext);
                }
                it->second.downscaleImage->markForRendered(downscaledRectToRender);
            } // if (renderFullScaleThenDownscale) {
        } // if (it->second.isAllocatedOnTheFly) {

        if ( frameArgs->stats && frameArgs->stats->isInDepthProfilingEnabled() ) {
            frameArgs->stats->addRenderInfosForNode( _publicInterface->getNode(),  NodePtr(), it->first.getComponentsGlobalName(), actionArgs.roi, timeRecorder->getTimeSinceCreation() );
        }


        // Set the accumulation buffer for this node if needed
        if (attachedItem) {
            _publicInterface->getNode()->setLastRenderedImage(renderFullScaleThenDownscale ?  it->second.fullscaleImage : it->second.downscaleImage);
        }

    } // for (std::map<ImageComponents,PlaneToRender>::const_iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {

} // EffectInstance::Implementation::renderHandlerPostProcess


void
EffectInstance::Implementation::setupRenderArgs(const EffectTLSDataPtr& tls,
                                                const OSGLContextPtr& glContext,
                                                const TimeValue time,
                                                const ViewIdx view,
                                                unsigned int mipMapLevel,
                                                bool isSequentialRender,
                                                bool isRenderResponseToUserInteraction,
                                                bool byPassCache,
                                                const ImagePlanesToRender & planes,
                                                const RectI & renderMappedRectToRender,
                                                const std::bitset<4>& processChannels,
                                                const InputImagesMap& inputImages,
                                                RenderActionArgs &actionArgs,
                                                boost::shared_ptr<OSGLContextAttacher>* glContextAttacher,
                                                TimeLapsePtr *timeRecorder)
{
    ParallelRenderArgsPtr frameArgs = tls->getParallelRenderArgs();

    if (frameArgs->stats) {
        timeRecorder->reset( new TimeLapse() );
    }

    const PlaneToRender & firstPlane = planes.planes.begin()->second;

    // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
# ifndef NDEBUG
    {
        RectI renderBounds = firstPlane.renderMappedImage->getBounds();
        assert(renderBounds.x1 <= renderMappedRectToRender.x1 && renderMappedRectToRender.x2 <= renderBounds.x2 &&
               renderBounds.y1 <= renderMappedRectToRender.y1 && renderMappedRectToRender.y2 <= renderBounds.y2);
    }
# endif

    std::list<std::pair<ImageComponents, ImagePtr> > tmpPlanes;

    actionArgs.byPassCache = byPassCache;
    actionArgs.processChannels = processChannels;
    actionArgs.mappedScale.x = actionArgs.mappedScale.y = Image::getScaleFromMipMapLevel( firstPlane.renderMappedImage->getMipMapLevel() );
    assert( !( (_publicInterface->supportsRenderScaleMaybe() == eSupportsNo) && !(actionArgs.mappedScale.x == 1. && actionArgs.mappedScale.y == 1.) ) );
    actionArgs.originalScale.x = Image::getScaleFromMipMapLevel(mipMapLevel);
    actionArgs.originalScale.y = actionArgs.originalScale.x;
    actionArgs.draftMode = frameArgs->draftMode;
    actionArgs.useOpenGL = planes.useOpenGL;
    actionArgs.roi = renderMappedRectToRender;
    actionArgs.time = time;
    actionArgs.view = view;
    actionArgs.isSequentialRender = isSequentialRender;
    actionArgs.isRenderResponseToUserInteraction = isRenderResponseToUserInteraction;
    actionArgs.inputImages = inputImages;
    actionArgs.glContext = glContext;

    // Setup the context when rendering using OpenGL
    if (planes.useOpenGL) {
        // Setup the viewport and the framebuffer
        AbortableRenderInfoPtr abortInfo = frameArgs->abortInfo.lock();
        assert(abortInfo);
        assert(glContext);

        // Ensure the context is current
        if (glContext->isGPUContext()) {
            *glContextAttacher = OSGLContextAttacher::create(glContext);
            (*glContextAttacher)->attach();


            GLuint fboID = glContext->getOrCreateFBOId();
            GL_GPU::BindFramebuffer(GL_FRAMEBUFFER, fboID);
            glCheckError(GL_GPU);
        }
    }

}


ImagePtr
EffectInstance::allocateImagePlaneAndSetInThreadLocalStorage(const ImageComponents & plane)
{
    /*
     * The idea here is that we may have asked the plug-in to render say motion.forward, but it can only render both fotward
     * and backward at a time.
     * So it needs to allocate motion.backward and store it in the cache for efficiency.
     * Note that when calling this, the plug-in is already in the render action, hence in case of Host frame threading,
     * this function will be called as many times as there were thread used by the host frame threading.
     * For all other planes, there was a local temporary image, shared among all threads for the calls to render.
     * Since we may be in a thread of the host frame threading, only allocate a temporary image of the size of the rectangle
     * to render and mark that we're a plane allocated on the fly so that the tiledRenderingFunctor can know this is a plane
     * to handle specifically.
     */
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();

    if (!tls) {
        return ImagePtr();
    }

    std::map<ImageComponents, PlaneToRender> curActionOutputPlanes;
    RectI currentRenderWindowPixel;
    tls->getCurrentRenderActionArgs(0, 0, 0, &currentRenderWindowPixel, 0, &curActionOutputPlanes, 0);

    assert( !curActionOutputPlanes.empty() );
    if (curActionOutputPlanes.empty()) {
        return ImagePtr();
    }

    ParallelRenderArgsPtr frameArgs = tls->getParallelRenderArgs();
    if (!frameArgs) {
        return ImagePtr();
    }

    const PlaneToRender & firstPlane = curActionOutputPlanes.begin()->second;
    bool useCache = firstPlane.fullscaleImage->usesBitMap() || firstPlane.downscaleImage->usesBitMap();
    if ( boost::starts_with(getNode()->getPluginID(), "uk.co.thefoundry.furnace") ) {
        //Furnace plug-ins are bugged and do not render properly both planes, just wipe the image.
        useCache = false;
    }
    const ImagePtr & img = firstPlane.fullscaleImage->usesBitMap() ? firstPlane.fullscaleImage : firstPlane.downscaleImage;
    ImageParamsPtr params = img->getParams();
    PlaneToRender p;
    bool ok = allocateImagePlane(img->getKey(),
                                 img->getRoD(),
                                 currentRenderWindowPixel,
                                 currentRenderWindowPixel,
                                 plane,
                                 img->getBitDepth(),
                                 img->getPremultiplication(),
                                 img->getFieldingOrder(),
                                 img->getPixelAspectRatio(),
                                 img->getMipMapLevel(),
                                 false,
                                 frameArgs->openGLContext.lock(),
                                 img->getParams()->getStorageInfo().mode,
                                 useCache,
                                 &p.fullscaleImage,
                                 &p.downscaleImage);
    if (!ok) {
        return ImagePtr();
    }

    p.renderMappedImage = p.downscaleImage;
    p.isAllocatedOnTheFly = true;

    /*
     * Allocate a temporary image for rendering only if using cache
     */
    if (useCache) {
        p.tmpImage.reset( new Image(p.renderMappedImage->getComponents(),
                                    p.renderMappedImage->getRoD(),
                                    currentRenderWindowPixel,
                                    p.renderMappedImage->getMipMapLevel(),
                                    p.renderMappedImage->getPixelAspectRatio(),
                                    p.renderMappedImage->getBitDepth(),
                                    p.renderMappedImage->getPremultiplication(),
                                    p.renderMappedImage->getFieldingOrder(),
                                    false /*useBitmap*/,
                                    img->getParams()->getStorageInfo().mode,
                                    frameArgs->openGLContext.lock(),
                                    GL_TEXTURE_2D, true) );
    } else {
        p.tmpImage = p.renderMappedImage;
    }
    curActionOutputPlanes.insert( std::make_pair(plane, p) );
    tls->updateCurrentRenderActionOutputPlanes(curActionOutputPlanes);

    return p.downscaleImage;

} // allocateImagePlaneAndSetInThreadLocalStorage


void
EffectInstance::evaluate(bool isSignificant,
                         bool refreshMetadatas)
{

    // We changed, abort any ongoing current render to refresh them with a newer version
    if (isSignificant) {
        abortAnyEvaluation();
    }

    NodePtr node = getNode();

    if ( refreshMetadatas && node && node->isNodeCreated() ) {
        
        // Force a re-compute of the meta-data if needed
        GetTimeInvariantMetaDatasResultsPtr results;
        getTimeInvariantMetaDatas_public(TreeRenderNodeArgsPtr(), &results);
        if (results) {
            const NodeMetadata& metadatas = results->getMetadatasResults();

            // Refresh warnings
            _imp->refreshMetadaWarnings(metadatas);

            node->refreshIdentityState();
            node->refreshChannelSelectors();
            node->checkForPremultWarningAndCheckboxes();
            node->refreshLayersSelectorsVisibility();

        }
    }

    if (isSignificant) {
        getApp()->triggerAutoSave();
    }

    node->refreshIdentityState();


    TimeValue time = getCurrentTime();

    // Get the connected viewers downstream and re-render or redraw them.
    std::list<ViewerInstancePtr> viewers;
    node->hasViewersConnected(&viewers);
    for (std::list<ViewerInstancePtr>::iterator it = viewers.begin();
         it != viewers.end();
         ++it) {
        if (isSignificant) {
            (*it)->renderCurrentFrame(true);
        } else {
            (*it)->redrawViewer();
        }
    }

    // If significant, also refresh previews downstream 
    if (isSignificant) {
        node->refreshPreviewsRecursivelyDownstream(time);
    }
} // evaluate

bool
EffectInstance::message(MessageTypeEnum type,
                        const std::string & content) const
{
    return getNode()->message(type, content);
}

void
EffectInstance::setPersistentMessage(MessageTypeEnum type,
                                     const std::string & content)
{
    getNode()->setPersistentMessage(type, content);
}

bool
EffectInstance::hasPersistentMessage()
{
    return getNode()->hasPersistentMessage();
}

void
EffectInstance::clearPersistentMessage(bool recurse)
{
    NodePtr node = getNode();

    if (node) {
        node->clearPersistentMessage(recurse);
    }
}

int
EffectInstance::getInputNumber(const EffectInstancePtr& inputEffect) const
{
    for (int i = 0; i < getMaxInputCount(); ++i) {
        if (getInput(i) == inputEffect) {
            return i;
        }
    }

    return -1;
}


void
EffectInstance::setOutputFilesForWriter(const std::string & pattern)
{
    if ( !isWriter() ) {
        return;
    }

    const KnobsVec & knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        KnobFilePtr fk = toKnobFile(knobs[i]);
        if ( fk && fk->getName() == kOfxImageEffectFileParamName ) {
            fk->setValue(pattern);
            break;
        }

    }
}

PluginMemoryPtr
EffectInstance::newMemoryInstance(size_t nBytes)
{
    PluginMemoryPtr ret( new PluginMemory( shared_from_this() ) ); //< hack to get "this" as a shared ptr

    addPluginMemoryPointer(ret);
    bool wasntLocked = ret->alloc(nBytes);

    assert(wasntLocked);
    Q_UNUSED(wasntLocked);

    return ret;
}


void
EffectInstance::setCurrentViewportForOverlays_public(OverlaySupport* viewport)
{
    assert( QThread::currentThread() == qApp->thread() );
    getNode()->setCurrentViewportForHostOverlays(viewport);
    _imp->overlaysViewport = viewport;
    setCurrentViewportForOverlays(viewport);
}

OverlaySupport*
EffectInstance::getCurrentViewportForOverlays() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->overlaysViewport;
}



void
EffectInstance::setInteractColourPicker_public(const OfxRGBAColourD& color, bool setColor, bool hasColor)
{
    const KnobsVec& knobs = getKnobs();
    for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
        const KnobIPtr& k = *it2;
        if (!k) {
            continue;
        }
        OfxParamOverlayInteractPtr interact = k->getCustomInteract();
        if (!interact) {
            continue;
        }

        if (!interact->isColorPickerRequired()) {
            continue;
        }
        if (!hasColor) {
            interact->setHasColorPicker(false);
        } else {
            if (setColor) {
                interact->setLastColorPickerColor(color);
            }
            interact->setHasColorPicker(true);
        }

        k->redraw();
    }

    setInteractColourPicker(color, setColor, hasColor);

}

bool
EffectInstance::isDoingInteractAction() const
{
  
    return getNode()->isDoingInteractAction();
}


EffectInstancePtr
EffectInstance::getOrCreateRenderInstance()
{
    QMutexLocker k(&_imp->renderClonesMutex);
    if (!_imp->isDoingInstanceSafeRender) {
        // The main instance is not rendering, use it
        _imp->isDoingInstanceSafeRender = true;
        return shared_from_this();
    }
    // Ok get a clone
    if (!_imp->renderClonesPool.empty()) {
        EffectInstancePtr ret =  _imp->renderClonesPool.front();
        _imp->renderClonesPool.pop_front();
        ret->_imp->isDoingInstanceSafeRender = true;
        return ret;
    }

    EffectInstancePtr clone = createRenderClone();
    if (!clone) {
        // We have no way but to use this node since the effect does not support render clones
        _imp->isDoingInstanceSafeRender = true;
        return shared_from_this();
    }
    clone->_imp->isDoingInstanceSafeRender = true;
    return clone;
}

void
EffectInstance::clearRenderInstances()
{
    QMutexLocker k(&_imp->renderClonesMutex);
    _imp->renderClonesPool.clear();
}

void
EffectInstance::releaseRenderInstance(const EffectInstancePtr& instance)
{
    if (!instance) {
        return;
    }
    QMutexLocker k(&_imp->renderClonesMutex);
    instance->_imp->isDoingInstanceSafeRender = false;
    if (instance.get() == this) {
        return;
    }

    // Make this instance available again
    _imp->renderClonesPool.push_back(instance);
}


bool
EffectInstance::isSupportedComponent(int inputNb,
                                     const ImageComponents & comp) const
{
    return getNode()->isSupportedComponent(inputNb, comp);
}

ImageBitDepthEnum
EffectInstance::getBestSupportedBitDepth() const
{
    return getNode()->getBestSupportedBitDepth();
}

bool
EffectInstance::isSupportedBitDepth(ImageBitDepthEnum depth) const
{
    return getNode()->isSupportedBitDepth(depth);
}

ImageComponents
EffectInstance::findClosestSupportedComponents(int inputNb,
                                               const ImageComponents & comp) const
{
    return getNode()->findClosestSupportedComponents(inputNb, comp);
}


void
EffectInstance::getComponentsAvailableRecursive(bool useLayerChoice,
                                                bool useThisNodeComponentsNeeded,
                                                TimeValue time,
                                                ViewIdx view,
                                                ComponentsAvailableMap* comps,
                                                std::list<EffectInstancePtr>* markedNodes)
{
    if ( std::find(markedNodes->begin(), markedNodes->end(), shared_from_this()) != markedNodes->end() ) {
        return;
    }

    if (useLayerChoice && useThisNodeComponentsNeeded) {
        QMutexLocker k(&_imp->componentsAvailableMutex);
        if (!_imp->componentsAvailableDirty) {
            comps->insert( _imp->outputComponentsAvailable.begin(), _imp->outputComponentsAvailable.end() );

            return;
        }
    }


    NodePtr node  = getNode();
    if (!node) {
        return;
    }
    ComponentsNeededMap neededComps;
    SequenceTime ptTime;
    int ptView;
    NodePtr ptInput;
    bool processAll;
    std::bitset<4> processChannels;
    getComponentsNeededAndProduced_public(useLayerChoice, useThisNodeComponentsNeeded, time, view, &neededComps, &processAll, &ptTime, &ptView, &processChannels, &ptInput);


    ///If the plug-in is not pass-through, only consider the components processed by the plug-in in output,
    ///so we do not need to recurse.
    PassThroughEnum passThrough = isPassThroughForNonRenderedPlanes();
    if ( (passThrough == ePassThroughPassThroughNonRenderedPlanes) ||
         ( passThrough == ePassThroughRenderAllRequestedPlanes) ) {
        if (!isMultiPlanar() || !ptInput) {
            ptInput = node->getInput( node->getPreferredInput() );
        }

        if (ptInput) {
            ptInput->getEffectInstance()->getComponentsAvailableRecursive(useLayerChoice, true, time, view, comps, markedNodes);
        }
    }
    if (processAll) {
        //The node makes available everything available upstream
        for (ComponentsAvailableMap::iterator it = comps->begin(); it != comps->end(); ++it) {
            if ( it->second.lock() ) {
                it->second = node;
            }
        }
    }


    ComponentsNeededMap::iterator foundOutput = neededComps.find(-1);
    if ( foundOutput != neededComps.end() ) {
        ///Foreach component produced by the node at the given (view, time),  try
        ///to add it to the components available. Since we already handled upstream nodes, it is probably
        ///already in there, in which case we mark that this node is producing the component instead
        for (std::vector<ImageComponents>::iterator it = foundOutput->second.begin();
             it != foundOutput->second.end(); ++it) {
            ComponentsAvailableMap::iterator alreadyExisting = comps->end();

            if ( it->isColorPlane() ) {
                ComponentsAvailableMap::iterator colorMatch = comps->end();

                for (ComponentsAvailableMap::iterator it2 = comps->begin(); it2 != comps->end(); ++it2) {
                    if (it2->first == *it) {
                        alreadyExisting = it2;
                        break;
                    } else if ( it2->first.isColorPlane() ) {
                        colorMatch = it2;
                    }
                }

                if ( ( alreadyExisting == comps->end() ) && ( colorMatch != comps->end() ) ) {
                    comps->erase(colorMatch);
                }
            } else {
                for (ComponentsAvailableMap::iterator it2 = comps->begin(); it2 != comps->end(); ++it2) {
                    if (it2->first == *it) {
                        alreadyExisting = it2;
                        break;
                    }
                }
            }


            if ( alreadyExisting == comps->end() ) {
                comps->insert( std::make_pair(*it, node) );
            } else {
                //If the component already exists from upstream in the tree, mark that we produce it instead
                alreadyExisting->second = node;
            }
        }
    }

    ///If the user has selected "All", do not add created components as they will not be available
    if (!processAll) {
        std::list<ImageComponents> userComps;
        node->getUserCreatedComponents(&userComps);

        ///Add to the user comps the project components
        std::vector<ImageComponents> projectLayers = getApp()->getProject()->getProjectDefaultLayers();
        userComps.insert( userComps.end(), projectLayers.begin(), projectLayers.end() );

        ///Foreach user component, add it as an available component, but use this node only if it is also
        ///in the "needed components" list
        for (std::list<ImageComponents>::iterator it = userComps.begin(); it != userComps.end(); ++it) {
            ///If this is a user comp and used by the node it will be in the needed output components
            bool found = false;
            if ( foundOutput != neededComps.end() ) {
                for (std::vector<ImageComponents>::iterator it2 = foundOutput->second.begin();
                     it2 != foundOutput->second.end(); ++it2) {
                    if (*it2 == *it) {
                        found = true;
                        break;
                    }
                }
            }


            ComponentsAvailableMap::iterator alreadyExisting = comps->end();

            if ( it->isColorPlane() ) {
                ComponentsAvailableMap::iterator colorMatch = comps->end();

                for (ComponentsAvailableMap::iterator it2 = comps->begin(); it2 != comps->end(); ++it2) {
                    if (it2->first == *it) {
                        alreadyExisting = it2;
                        break;
                    } else if ( it2->first.isColorPlane() ) {
                        colorMatch = it2;
                    }
                }

                if ( ( alreadyExisting == comps->end() ) && ( colorMatch != comps->end() ) ) {
                    comps->erase(colorMatch);
                }
            } else {
                alreadyExisting = comps->find(*it);
            }

            ///If the component already exists from above in the tree, do not add it
            if ( alreadyExisting == comps->end() ) {
                comps->insert( std::make_pair( *it, (found) ? node : NodePtr() ) );
            } else {
                ///The user component may very well have been created on a node upstream
                ///Set the component as available only if the node uses it actively,i.e if
                ///it was found in the needed output components
                if (found) {
                    alreadyExisting->second = node;
                }
            }
        }
    }

    markedNodes->push_back( shared_from_this() );


    if (useLayerChoice && useThisNodeComponentsNeeded) {
        QMutexLocker k(&_imp->componentsAvailableMutex);
        _imp->componentsAvailableDirty = false;
        _imp->outputComponentsAvailable = *comps;
    }
} // EffectInstance::getComponentsAvailableRecursive

void
EffectInstance::getComponentsAvailable(bool useLayerChoice,
                                       bool useThisNodeComponentsNeeded,
                                       TimeValue time,
                                       ComponentsAvailableMap* comps,
                                       std::list<EffectInstancePtr>* markedNodes)
{
    getComponentsAvailableRecursive(useLayerChoice, useThisNodeComponentsNeeded, time, ViewIdx(0), comps, markedNodes);
}

void
EffectInstance::getComponentsAvailable(bool useLayerChoice,
                                       bool useThisNodeComponentsNeeded,
                                       TimeValue time,
                                       ComponentsAvailableMap* comps)
{
    //int nViews = getApp()->getProject()->getProjectViewsCount();

    ///Union components over all views
    //for (int view = 0; view < nViews; ++view) {
    ///Edit: Just call for 1 view, it should not matter as this should be view agnostic.
    std::list<EffectInstancePtr> marks;

    getComponentsAvailableRecursive(useLayerChoice, useThisNodeComponentsNeeded, time, ViewIdx(0), comps, &marks);

    //}
}

bool
EffectInstance::getCreateChannelSelectorKnob() const
{
    return ( !isMultiPlanar() && !isReader() && !isWriter() &&
             !boost::starts_with(getNode()->getPluginID(), "uk.co.thefoundry.furnace") );
}

int
EffectInstance::getMaskChannel(int inputNb,
                               ImageComponents* comps,
                               NodePtr* maskInput) const
{
    return getNode()->getMaskChannel(inputNb, comps, maskInput);
}

bool
EffectInstance::isMaskEnabled(int inputNb) const
{
    return getNode()->isMaskEnabled(inputNb);
}

RenderSafetyEnum
EffectInstance::getCurrentRenderThreadSafety() const
{
    return (RenderSafetyEnum)getNode()->getPlugin()->getProperty<int>(kNatronPluginPropRenderSafety);
}

PluginOpenGLRenderSupport
EffectInstance::getCurrentOpenGLSupport() const
{
    return (PluginOpenGLRenderSupport)getNode()->getPlugin()->getProperty<int>(kNatronPluginPropOpenGLSupport);
}

bool
EffectInstance::onKnobValueChanged(const KnobIPtr& /*k*/,
                                   ValueChangedReasonEnum /*reason*/,
                                   TimeValue /*time*/,
                                   ViewSetSpec /*view*/)
{
    return false;
}



RenderScale
EffectInstance::getOverlayInteractRenderScale() const
{
    RenderScale renderScale(1.);

    if (isDoingInteractAction() && _imp->overlaysViewport) {
        unsigned int mmLevel = _imp->overlaysViewport->getCurrentRenderScale();
        renderScale.x = renderScale.y = 1 << mmLevel;
    }

    return renderScale;
}

void
EffectInstance::pushUndoCommand(UndoCommand* command)
{
    UndoCommandPtr ptr(command);

    getNode()->pushUndoCommand(ptr);
}

void
EffectInstance::pushUndoCommand(const UndoCommandPtr& command)
{
    getNode()->pushUndoCommand(command);
}

bool
EffectInstance::setCurrentCursor(CursorEnum defaultCursor)
{
    if ( !isDoingInteractAction() ) {
        return false;
    }
    getNode()->setCurrentCursor(defaultCursor);

    return true;
}

bool
EffectInstance::setCurrentCursor(const QString& customCursorFilePath)
{
    if ( !isDoingInteractAction() ) {
        return false;
    }

    return getNode()->setCurrentCursor(customCursorFilePath);
}


void
EffectInstance::clearLastRenderedImage()
{
    invalidateHashCache();
}

/**
 * @brief Returns a pointer to the first non disabled upstream node.
 * When cycling through the tree, we prefer non optional inputs and we span inputs
 * from last to first.
 **/
EffectInstancePtr
EffectInstance::getNearestNonDisabled(TimeValue time, ViewIdx view) const
{
    NodePtr node = getNode();

    if ( !node->isNodeDisabledForFrame(time, view) ) {
        return node->getEffectInstance();
    } else {
        ///Test all inputs recursively, going from last to first, preferring non optional inputs.
        std::list<EffectInstancePtr> nonOptionalInputs;
        std::list<EffectInstancePtr> optionalInputs;
        bool useInputA = appPTR->getCurrentSettings()->isMergeAutoConnectingToAInput();

        ///Find an input named A
        std::string inputNameToFind, otherName;
        if (useInputA) {
            inputNameToFind = "A";
            otherName = "B";
        } else {
            inputNameToFind = "B";
            otherName = "A";
        }
        int foundOther = -1;
        int maxinputs = getMaxInputCount();
        for (int i = 0; i < maxinputs; ++i) {
            std::string inputLabel = getInputLabel(i);
            if (inputLabel == inputNameToFind) {
                EffectInstancePtr inp = getInput(i);
                if (inp) {
                    nonOptionalInputs.push_front(inp);
                    break;
                }
            } else if (inputLabel == otherName) {
                foundOther = i;
            }
        }

        if ( (foundOther != -1) && nonOptionalInputs.empty() ) {
            EffectInstancePtr inp = getInput(foundOther);
            if (inp) {
                nonOptionalInputs.push_front(inp);
            }
        }

        ///If we found A or B so far, cycle through them
        for (std::list<EffectInstancePtr> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
            EffectInstancePtr inputRet = (*it)->getNearestNonDisabled(time ,view);
            if (inputRet) {
                return inputRet;
            }
        }


        ///We cycle in reverse by default. It should be a setting of the application.
        ///In this case it will return input B instead of input A of a merge for example.
        for (int i = 0; i < maxinputs; ++i) {
            EffectInstancePtr inp = getInput(i);
            bool optional = isInputOptional(i);
            if (inp) {
                if (optional) {
                    optionalInputs.push_back(inp);
                } else {
                    nonOptionalInputs.push_back(inp);
                }
            }
        }

        ///Cycle through all non optional inputs first
        for (std::list<EffectInstancePtr> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
            EffectInstancePtr inputRet = (*it)->getNearestNonDisabled(time, view);
            if (inputRet) {
                return inputRet;
            }
        }

        ///Cycle through optional inputs...
        for (std::list<EffectInstancePtr> ::iterator it = optionalInputs.begin(); it != optionalInputs.end(); ++it) {
            EffectInstancePtr inputRet = (*it)->getNearestNonDisabled(time, view);
            if (inputRet) {
                return inputRet;
            }
        }

        ///We didn't find anything upstream, return
        return node->getEffectInstance();
    }
} // EffectInstance::getNearestNonDisabled

EffectInstancePtr
EffectInstance::getNearestNonIdentity(TimeValue time)
{

    RenderScale scale(1.);

    RectI format = getOutputFormat();
    double inputTimeIdentity;
    int inputNbIdentity;
    ViewIdx inputView;
    if ( !isIdentity_public(true, 0, time, scale, format, ViewIdx(0), &inputTimeIdentity, &inputView, &inputNbIdentity) ) {
        return shared_from_this();
    } else {
        if (inputNbIdentity < 0) {
            return shared_from_this();
        }
        EffectInstancePtr effect = getInput(inputNbIdentity);

        return effect ? effect->getNearestNonIdentity(time) : shared_from_this();
    }
}

void
EffectInstance::abortAnyEvaluation(bool keepOldestRender)
{
    
    // Abort and allow playback to restart but do not block, when this function returns any ongoing render may very
    // well not be finished
    AppInstancePtr app = getApp();
    if (!app) {
        return;
    }
    app->abortAllViewers(keepOldestRender /*autoRestartPlayback*/);
}



bool
EffectInstance::isPaintingOverItselfEnabled() const
{
    return getNode()->isDuringPaintStrokeCreation();
}

void
EffectInstance::refreshExtraStateAfterTimeChanged(bool isPlayback,
                                                  TimeValue time)
{
    getNode()->refreshIdentityState();
}



bool
EffectInstance::isFrameVarying(const TreeRenderNodeArgsPtr& render) const
{
    GetTimeInvariantMetaDatasResultsPtr results;
    StatusEnum stat = getTimeInvariantMetaDatas_public(render, &results);
    if (stat == eStatusFailed) {
        return true;
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getIsFrameVarying();
    }
}

bool
EffectInstance::isFrameVaryingOrAnimated(const TreeRenderNodeArgsPtr& render) const
{
    if (getHasAnimation()) {
        return true;
    }
    if (isFrameVarying(render)) {
        return true;
    }
    return false;
}

double
EffectInstance::getFrameRate(const TreeRenderNodeArgsPtr& render) const
{
    GetTimeInvariantMetaDatasResultsPtr results;
    StatusEnum stat = getTimeInvariantMetaDatas_public(render, &results);
    if (stat == eStatusFailed) {
        return 24.;
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getOutputFrameRate();
    }

}


ImagePremultiplicationEnum
EffectInstance::getPremult(const TreeRenderNodeArgsPtr& render) const
{
    GetTimeInvariantMetaDatasResultsPtr results;
    StatusEnum stat = getTimeInvariantMetaDatas_public(render, &results);
    if (stat == eStatusFailed) {
        return eImagePremultiplicationPremultiplied;
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getOutputPremult();
    }
}

bool
EffectInstance::canRenderContinuously(const TreeRenderNodeArgsPtr& render) const
{
    GetTimeInvariantMetaDatasResultsPtr results;
    StatusEnum stat = getTimeInvariantMetaDatas_public(render, &results);
    if (stat == eStatusFailed) {
        return true;
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getIsContinuous();
    }
}

ImageFieldingOrderEnum
EffectInstance::getFieldingOrder(const TreeRenderNodeArgsPtr& render) const
{
    GetTimeInvariantMetaDatasResultsPtr results;
    StatusEnum stat = getTimeInvariantMetaDatas_public(render, &results);
    if (stat == eStatusFailed) {
        return eImageFieldingOrderNone;
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getOutputFielding();
    }
}


double
EffectInstance::getAspectRatio(const TreeRenderNodeArgsPtr& render, int inputNb) const
{
    GetTimeInvariantMetaDatasResultsPtr results;
    StatusEnum stat = getTimeInvariantMetaDatas_public(render, &results);
    if (stat == eStatusFailed) {
        return 1.;
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getPixelAspectRatio(inputNb);
    }
}

ImageComponents
EffectInstance::getComponents(const TreeRenderNodeArgsPtr& render, int inputNb) const
{
    GetTimeInvariantMetaDatasResultsPtr results;
    StatusEnum stat = getTimeInvariantMetaDatas_public(render, &results);
    if (stat == eStatusFailed) {
        return ImageComponents::getNoneComponents();
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getImageComponents(inputNb);
    }
}

ImageBitDepthEnum
EffectInstance::getBitDepth(const TreeRenderNodeArgsPtr& render, int inputNb) const
{
    GetTimeInvariantMetaDatasResultsPtr results;
    StatusEnum stat = getTimeInvariantMetaDatas_public(render, &results);
    if (stat == eStatusFailed) {
        return eImageBitDepthFloat;
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getBitDepth(inputNb);
    }
}


NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_EffectInstance.cpp"

