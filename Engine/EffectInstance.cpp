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
#include <bitset>

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


NATRON_NAMESPACE_ENTER;


class KnobFile;
class KnobOutputFile;


void
EffectInstance::addThreadLocalInputImageTempPointer(int inputNb,
                                                    const boost::shared_ptr<Image> & img)
{
    _imp->addInputImageTempPointer(inputNb, img);
}

EffectInstance::EffectInstance(NodePtr node)
    : NamedKnobHolder(node ? node->getApp() : NULL)
    , _node(node)
    , _imp( new Implementation(this) )
{
}

EffectInstance::~EffectInstance()
{
    clearPluginMemoryChunks();
}

void
EffectInstance::resetTotalTimeSpentRendering()
{
    resetTimeSpentRenderingInfos();
}

void
EffectInstance::lock(const boost::shared_ptr<Image> & entry)
{
    NodePtr n = _node.lock();

    n->lock(entry);
}

bool
EffectInstance::tryLock(const boost::shared_ptr<Image> & entry)
{
    NodePtr n = _node.lock();

    return n->tryLock(entry);
}

void
EffectInstance::unlock(const boost::shared_ptr<Image> & entry)
{
    NodePtr n = _node.lock();

    n->unlock(entry);
}

void
EffectInstance::clearPluginMemoryChunks()
{
    int toRemove;
    {
        QMutexLocker l(&_imp->pluginMemoryChunksMutex);
        toRemove = (int)_imp->pluginMemoryChunks.size();
    }

    while (toRemove > 0) {
        PluginMemory* mem;
        {
            QMutexLocker l(&_imp->pluginMemoryChunksMutex);
            mem = ( *_imp->pluginMemoryChunks.begin() );
        }
        delete mem;
        --toRemove;
    }
}

#ifdef DEBUG
void
EffectInstance::setCanSetValue(bool can)
{
    _imp->tlsData->getOrCreateTLSData()->canSetValue.push_back(can);
}

void
EffectInstance::invalidateCanSetValueFlag()
{
    EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
    assert(tls);
    assert(!tls->canSetValue.empty());
    tls->canSetValue.pop_back();
}


bool
EffectInstance::isDuringActionThatCanSetValue() const
{
    EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
    if (!tls) {
        return true;
    }
    if (tls->canSetValue.empty()) {
        return true;
    }
    return tls->canSetValue.back();
}
#endif //DEBUG

void
EffectInstance::setParallelRenderArgsTLS(double time,
                                         int view,
                                         bool isRenderUserInteraction,
                                         bool isSequential,
                                         bool canAbort,
                                         U64 nodeHash,
                                         U64 renderAge,
                                         const NodePtr & treeRoot,
                                         const boost::shared_ptr<NodeFrameRequest> & nodeRequest,
                                         int textureIndex,
                                         const TimeLine* timeline,
                                         bool isAnalysis,
                                         bool isDuringPaintStrokeCreation,
                                         const NodesList & rotoPaintNodes,
                                         RenderSafetyEnum currentThreadSafety,
                                         bool doNanHandling,
                                         bool draftMode,
                                         bool viewerProgressReportEnabled,
                                         const boost::shared_ptr<RenderStats> & stats)
{
    EffectDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
    std::list<boost::shared_ptr<ParallelRenderArgs> >& argsList = tls->frameArgs;

    boost::shared_ptr<ParallelRenderArgs> args(new ParallelRenderArgs);
    
    args->time = time;
    args->timeline = timeline;
    args->view = view;
    args->isRenderResponseToUserInteraction = isRenderUserInteraction;
    args->isSequentialRender = isSequential;
    args->request = nodeRequest;
    if (nodeRequest) {
        args->nodeHash = nodeRequest->nodeHash;
    } else {
        args->nodeHash = nodeHash;
    }
    args->canAbort = canAbort;
    args->renderAge = renderAge;
    args->treeRoot = treeRoot;
    args->textureIndex = textureIndex;
    args->isAnalysis = isAnalysis;
    args->isDuringPaintStrokeCreation = isDuringPaintStrokeCreation;
    args->currentThreadSafety = currentThreadSafety;
    args->rotoPaintNodes = rotoPaintNodes;
    args->doNansHandling = doNanHandling;
    args->draftMode = draftMode;
    args->tilesSupported = getNode()->getCurrentSupportTiles();
    args->viewerProgressReportEnabled = viewerProgressReportEnabled;
    args->stats = stats;
    argsList.push_back(args);
}

bool
EffectInstance::getThreadLocalRotoPaintTreeNodes(NodesList* nodes) const
{
    EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
    if (!tls) {
        return false;
    }
    if (tls->frameArgs.empty()) {
        return false;
    }
    *nodes = tls->frameArgs.back()->rotoPaintNodes;
    return true;
}

void
EffectInstance::setDuringPaintStrokeCreationThreadLocal(bool duringPaintStroke)
{
    EffectDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
    tls->frameArgs.back()->isDuringPaintStrokeCreation = duringPaintStroke;
}

void
EffectInstance::setParallelRenderArgsTLS(const boost::shared_ptr<ParallelRenderArgs> & args)
{
    EffectDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
    tls->frameArgs.push_back(args);
}

void
EffectInstance::invalidateParallelRenderArgsTLS()
{
    EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
    if (!tls) {
        return;
    }
    
    assert(!tls->frameArgs.empty());
    const boost::shared_ptr<ParallelRenderArgs>& back = tls->frameArgs.back();
    for (NodesList::iterator it = back->rotoPaintNodes.begin(); it != back->rotoPaintNodes.end(); ++it) {
        (*it)->getEffectInstance()->invalidateParallelRenderArgsTLS();
    }
    tls->frameArgs.pop_back();
   
}

boost::shared_ptr<ParallelRenderArgs>
EffectInstance::getParallelRenderArgsTLS() const
{
    EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
    if (!tls || tls->frameArgs.empty()) {
        return boost::shared_ptr<ParallelRenderArgs>();
    }
    return tls->frameArgs.back();
}

U64
EffectInstance::getHash() const
{
    NodePtr n = _node.lock();

    return n->getHashValue();
}


U64
EffectInstance::getRenderHash() const
{
    EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
    if (!tls || tls->frameArgs.empty()) {
        //No tls: get the GUI hash
        return getHash();
    }
    
    const boost::shared_ptr<ParallelRenderArgs> &args = tls->frameArgs.back();

    if (args->request) {
        //A request pass was made, Hash for this thread was already computed, use it
        return args->request->nodeHash;
    }
    //Use the hash that was computed when we set the ParallelRenderArgs TLS
    return args->nodeHash;
}

bool
EffectInstance::Implementation::aborted(const EffectDataTLSPtr& tls) const
{
    if (tls->frameArgs.empty()) {
        return false;
    }
    const boost::shared_ptr<ParallelRenderArgs> & args = tls->frameArgs.back();
    
    if (args->isRenderResponseToUserInteraction) {
        //Don't allow the plug-in to abort while in analysis.
        //Need to work on this for the tracker in order to be abortable.
        if (args->isAnalysis) {
            return false;
        }
        ViewerInstance* isViewer  = 0;
        if (args->treeRoot) {
            isViewer = args->treeRoot->isEffectViewer();
            //If the viewer is already doing a sequential render, abort
            if ( isViewer && isViewer->isDoingSequentialRender() ) {
                return true;
            }
        }
        
        if (args->canAbort) {
            if ( isViewer && isViewer->isRenderAbortable(args->textureIndex, args->renderAge) ) {
                return true;
            }
            
            if (args->draftMode && args->timeline && args->time != args->timeline->currentFrame()) {
                return true;
            }
            
            ///Rendering issued by RenderEngine::renderCurrentFrame, if time or hash changed, abort
            bool ret = !_publicInterface->getNode()->isActivated();
            
            return ret;
        } else {
            bool deactivated = !_publicInterface->getNode()->isActivated();
            
            return deactivated;
        }
    } else {
        ///Rendering is playback or render on disk, we rely on the flag set on the node that requested the render
        if (args->treeRoot) {
            OutputEffectInstance* effect = dynamic_cast<OutputEffectInstance*>(args->treeRoot->getEffectInstance().get());
            assert(effect);
            
            return effect ? effect->isSequentialRenderBeingAborted() : false;
        } else {
            return false;
        }
    }
    
    
}

bool
EffectInstance::aborted() const
{
    EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
    if (!tls) {
        return false;
    }
    return _imp->aborted(tls);
    
} // EffectInstance::aborted

bool
EffectInstance::shouldCacheOutput(bool isFrameVaryingOrAnimated,double time, int view) const
{
    NodePtr n = _node.lock();

    return n->shouldCacheOutput(isFrameVaryingOrAnimated, time ,view);
}

U64
EffectInstance::getKnobsAge() const
{
    return getNode()->getKnobsAge();
}

void
EffectInstance::setKnobsAge(U64 age)
{
    getNode()->setKnobsAge(age);
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

void
EffectInstance::getRenderFormat(Format *f) const
{
    assert(f);
    getApp()->getProject()->getProjectDefaultFormat(f);
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

EffectInstPtr
EffectInstance::getInput(int n) const
{
    NodePtr inputNode = getNode()->getInput(n);

    if (inputNode) {
        return inputNode->getEffectInstance();
    }

    return EffectInstPtr();
}

std::string
EffectInstance::getInputLabel(int inputNb) const
{
    std::string out;

    out.append( 1, (char)(inputNb + 65) );

    return out;
}

bool
EffectInstance::retrieveGetImageDataUponFailure(const double time,
                                                const int view,
                                                const RenderScale & scale,
                                                const RectD* optionalBoundsParam,
                                                U64* nodeHash_p,
                                                bool* isIdentity_p,
                                                double* identityTime,
                                                EffectInstPtr* identityInput_p,
                                                bool* duringPaintStroke_p,
                                                RectD* rod_p,
                                                RoIMap* inputRois_p, //!< output, only set if optionalBoundsParam != NULL
                                                RectD* optionalBounds_p) //!< output, only set if optionalBoundsParam != NULL
{
    /////Update 09/02/14
    /// We now AUTHORIZE GetRegionOfDefinition and isIdentity and getRegionsOfInterest to be called recursively.
    /// It didn't make much sense to forbid them from being recursive.

//#ifdef DEBUG
//    if (QThread::currentThread() != qApp->thread()) {
//        ///This is a bad plug-in
//        qDebug() << getNode()->getScriptName_mt_safe().c_str() << " is trying to call clipGetImage during an unauthorized time. "
//        "Developers of that plug-in should fix it. \n Reminder from the OpenFX spec: \n "
//        "Images may be fetched from an attached clip in the following situations... \n"
//        "- in the kOfxImageEffectActionRender action\n"
//        "- in the kOfxActionInstanceChanged and kOfxActionEndInstanceChanged actions with a kOfxPropChangeReason or kOfxChangeUserEdited";
//    }
//#endif

    ///Try to compensate for the mistake

    *nodeHash_p = getHash();
    *duringPaintStroke_p = getNode()->isDuringPaintStrokeCreation();
    const U64 & nodeHash = *nodeHash_p;

    {
        RECURSIVE_ACTION();
        StatusEnum stat = getRegionOfDefinition(nodeHash, time, scale, view, rod_p);
        if (stat == eStatusFailed) {
            return false;
        }
    }
    const RectD & rod = *rod_p;

    ///OptionalBoundsParam is the optional rectangle passed to getImage which may be NULL, in which case we use the RoD.
    if (!optionalBoundsParam) {
        ///// We cannot recover the RoI, we just assume the plug-in wants to render the full RoD.
        *optionalBounds_p = rod;
        ifInfiniteApplyHeuristic(nodeHash, time, scale, view, optionalBounds_p);
        const RectD & optionalBounds = *optionalBounds_p;

        /// If the region parameter is not set to NULL, then it will be clipped to the clip's
        /// Region of Definition for the given time. The returned image will be m at m least as big as this region.
        /// If the region parameter is not set, then the region fetched will be at least the Region of Interest
        /// the effect has previously specified, clipped the clip's Region of Definition.
        /// (renderRoI will do the clipping for us).


        ///// This code is wrong but executed ONLY IF THE PLUG-IN DOESN'T RESPECT THE SPECIFICATIONS. Recursive actions
        ///// should never happen.
        getRegionsOfInterest(time, scale, optionalBounds, optionalBounds, 0, inputRois_p);
    }

    assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(scale.x == 1. && scale.y == 1.) ) );
    RectI pixelRod;
    rod.toPixelEnclosing(scale, getPreferredAspectRatio(), &pixelRod);
    try {
        int identityInputNb;
        *isIdentity_p = isIdentity_public(true, nodeHash, time, scale, pixelRod, view, identityTime, &identityInputNb);
        if (*isIdentity_p) {
            if (identityInputNb >= 0) {
                *identityInput_p = getInput(identityInputNb);
            } else if (identityInputNb == -2) {
                *identityInput_p = shared_from_this();
            }
        }
    } catch (...) {
        return false;
    }

    return true;
} // EffectInstance::retrieveGetImageDataUponFailure

void
EffectInstance::getThreadLocalInputImages(InputImagesMap* images) const
{
    EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
    if (!tls) {
        return;
    }
    *images = tls->currentRenderArgs.inputImages;
}

bool
EffectInstance::getThreadLocalRegionsOfInterests(RoIMap & roiMap) const
{
    EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
    if (!tls) {
        return false;
    }
    roiMap = tls->currentRenderArgs.regionOfInterestResults;
    return true;
}

ImagePtr
EffectInstance::getImage(int inputNb,
                         const double time,
                         const RenderScale & scale,
                         const int view,
                         const RectD *optionalBoundsParam, //!< optional region in canonical coordinates
                         const ImageComponents & requestComps,
                         const ImageBitDepthEnum depth,
                         const double par,
                         const bool dontUpscale,
                         const bool mapImageToPreferredComps,
                         RectI* roiPixel,
                         boost::shared_ptr<Transform::Matrix3x3>* transform)
{
    if (time != time) {
        // time is NaN
        return ImagePtr();
    }
    
    ///The input we want the image from
    EffectInstPtr inputEffect;
    
    //Check for transform redirections
    boost::shared_ptr<InputMatrixMap> transformRedirections;
    EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
    if (tls && tls->currentRenderArgs.validArgs) {
        transformRedirections = tls->currentRenderArgs.transformRedirections;
        if (transformRedirections) {
            InputMatrixMap::const_iterator foundRedirection = transformRedirections->find(inputNb);
            if (foundRedirection != transformRedirections->end() && foundRedirection->second.newInputEffect) {
                inputEffect = foundRedirection->second.newInputEffect->getInput(foundRedirection->second.newInputNbToFetchFrom);
                if (transform) {
                    *transform = foundRedirection->second.cat;
                }
            }
        }
    }

    if (!inputEffect) {
        inputEffect = getInput(inputNb);
    }

    ///Is this input a mask or not
    bool isMask = isInputMask(inputNb);

    ///If the input is a mask, this is the channel index in the layer of the mask channel
    int channelForMask = -1;

    ///Is this node a roto node or not. If so, find out if this input is the roto-brush
    boost::shared_ptr<RotoContext> roto;
    boost::shared_ptr<RotoDrawableItem> attachedStroke = getNode()->getAttachedRotoItem();

    if (attachedStroke) {
        roto = attachedStroke->getContext();
    }
    bool useRotoInput = false;
    if (roto) {
        useRotoInput = isMask || isInputRotoBrush(inputNb);
    }

    ///This is the actual layer that we are fetching in input
    ImageComponents maskComps;
    if (!isMaskEnabled(inputNb)) {
        return ImagePtr();
    }
    
    ///If this is a mask, fetch the image from the effect indicated by the mask channel
    NodePtr maskInput;
    if (isMask) {
        if (!useRotoInput) {
            channelForMask = getMaskChannel(inputNb, &maskComps, &maskInput);
        } else {
            channelForMask = 3; // default to alpha channel
            maskInput = roto->getNode(); // set it to the RotoPaint node
            maskComps = ImageComponents::getAlphaComponents();
        }
    }
    if (maskInput && (channelForMask != -1)) {
        inputEffect = maskInput->getEffectInstance();
    }

    //Invalid mask
    if (isMask && ((channelForMask == -1) || (maskComps.getNumComponents() == 0))) {
        return ImagePtr();
    }


    if ((!roto || (roto && !useRotoInput)) && !inputEffect) {
        //Disconnected input
        return ImagePtr();
    }

    ///If optionalBounds have been set, use this for the RoI instead of the data int the TLS
    RectD optionalBounds;
    if (optionalBoundsParam) {
        optionalBounds = *optionalBoundsParam;
    }

    /*
     * These are the data fields stored in the TLS from the on-going render action or instance changed action
     */
    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    RoIMap inputsRoI;
    RectD rod;
    bool isIdentity = false;
    EffectInstPtr identityInput;
    double inputIdentityTime = 0.;
    U64 nodeHash;
    bool duringPaintStroke;
    /// Never by-pass the cache here because we already computed the image in renderRoI and by-passing the cache again can lead to
    /// re-computing of the same image many many times
    bool byPassCache = false;

    ///The caller thread MUST be a thread owned by Natron. It cannot be a thread from the multi-thread suite.
    ///A call to getImage is forbidden outside an action running in a thread launched by Natron.

    /// From http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ImageEffectsImagesAndClipsUsingClips
    //    Images may be fetched from an attached clip in the following situations...
    //    in the kOfxImageEffectActionRender action
    //    in the kOfxActionInstanceChanged and kOfxActionEndInstanceChanged actions with a kOfxPropChangeReason of kOfxChangeUserEdited
    RectD roi;
    bool roiWasInRequestPass = false;
    
    ///Try to find in the input images thread local storage if we already pre-computed the image
    EffectInstance::InputImagesMap inputImagesThreadLocal;
    
    if (!tls || !tls->currentRenderArgs.validArgs || tls->frameArgs.empty()) {
        if (!retrieveGetImageDataUponFailure(time, view, scale, optionalBoundsParam, &nodeHash, &isIdentity, &inputIdentityTime, &identityInput, &duringPaintStroke, &rod, &inputsRoI, &optionalBounds)) {
            return ImagePtr();
        }
    } else {
        const RenderArgs& renderArgs = tls->currentRenderArgs;
        const boost::shared_ptr<ParallelRenderArgs>& frameRenderArgs = tls->frameArgs.back();
        
        if (inputEffect) {
            boost::shared_ptr<ParallelRenderArgs> inputFrameArgs = inputEffect->getParallelRenderArgsTLS();
            const FrameViewRequest* request = 0;
            if (inputFrameArgs && inputFrameArgs->request) {
                request = inputFrameArgs->request->getFrameViewRequest(time, view);
            }
            if (request) {
                roiWasInRequestPass = true;
                roi = request->finalData.finalRoi;
            }
        }
        if (!roiWasInRequestPass) {
            inputsRoI = renderArgs.regionOfInterestResults;
        }
        rod = renderArgs.rod;
        isIdentity = renderArgs.isIdentity;
        inputIdentityTime = renderArgs.identityTime;
        identityInput = renderArgs.identityInput;
        inputImagesThreadLocal = renderArgs.inputImages;
        
        nodeHash = frameRenderArgs->nodeHash;
        duringPaintStroke = frameRenderArgs->isDuringPaintStrokeCreation;
    }

    if (optionalBoundsParam) {
        roi = optionalBounds;
    } else if (!roiWasInRequestPass) {
        EffectInstPtr inputToFind;
        if (useRotoInput) {
            if (getNode()->getRotoContext()) {
                inputToFind = shared_from_this();
            } else {
                assert(attachedStroke);
                inputToFind = attachedStroke->getContext()->getNode()->getEffectInstance();
            }
        } else {
            inputToFind = inputEffect;
        }
        RoIMap::iterator found = inputsRoI.find(inputToFind);
        if (found != inputsRoI.end()) {
            ///RoI is in canonical coordinates since the results of getRegionsOfInterest is in canonical coords.
            roi = found->second;
        } else {
            ///Oops, we didn't find the roi in the thread-storage... use  the RoD instead...
            if (inputEffect) {
                qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "[Bug] RoI not found in TLS...falling back on RoD when calling getImage() on" <<
                inputEffect->getScriptName_mt_safe().c_str();
            }
            roi = rod;
        }
    }

    if (roi.isNull()) {
        return ImagePtr();
    }


    if (isIdentity) {
        assert(identityInput.get() != this);
        ///If the effect is an identity but it didn't ask for the effect's image of which it is identity
        ///return a null image
        if (identityInput != inputEffect) {
            return ImagePtr();
        }
    }


    ///Does this node supports images at a scale different than 1
    bool renderFullScaleThenDownscale = (!supportsRenderScale() && mipMapLevel != 0);

    ///Do we want to render the graph upstream at scale 1 or at the requested render scale ? (user setting)
    bool renderScaleOneUpstreamIfRenderScaleSupportDisabled = false;
    unsigned int renderMappedMipMapLevel = mipMapLevel;
    if (renderFullScaleThenDownscale) {
        renderScaleOneUpstreamIfRenderScaleSupportDisabled = getNode()->useScaleOneImagesWhenRenderScaleSupportIsDisabled();
        if (renderScaleOneUpstreamIfRenderScaleSupportDisabled) {
            renderMappedMipMapLevel = 0;
        }
    }

    ///Both the result of getRegionsOfInterest and optionalBounds are in canonical coordinates, we have to convert in both cases
    ///Convert to pixel coordinates
    RectI pixelRoI;
    roi.toPixelEnclosing(renderScaleOneUpstreamIfRenderScaleSupportDisabled ? 0 : mipMapLevel, par, &pixelRoI);

    ImagePtr inputImg;

    ///For the roto brush, we do things separatly and render the mask with the RotoContext.
    if (useRotoInput) {
        ///Usage of roto outside of the rotopaint node is no longer handled
        assert(attachedStroke);
        if (attachedStroke) {
            if (duringPaintStroke) {
                inputImg = getNode()->getOrRenderLastStrokeImage(mipMapLevel, pixelRoI, par, requestComps, depth);
            } else {
                inputImg = roto->renderMaskFromStroke(attachedStroke, pixelRoI, requestComps,
                                                      time, view, depth, mipMapLevel);
                if ( roto->isDoingNeatRender() ) {
                    getApp()->updateStrokeImage(inputImg, 0, false);
                }
            }
        }
        if (roiPixel) {
            *roiPixel = pixelRoI;
        }

        if ( inputImg && !pixelRoI.intersects( inputImg->getBounds() ) ) {
            //The RoI requested does not intersect with the bounds of the input image, return a NULL image.
#ifdef DEBUG
            RectI inputBounds = inputImg->getBounds();
            qDebug() << getNode()->getScriptName_mt_safe().c_str() << ": The RoI requested to the roto mask does not intersect with the bounds of the input image: Pixel RoI x1=" << pixelRoI.x1 << "y1=" << pixelRoI.y1 << "x2=" << pixelRoI.x2 << "y2=" << pixelRoI.y2 <<
            "Bounds x1=" << inputBounds.x1 << "y1=" << inputBounds.y1 << "x2=" << inputBounds.x2 << "y2=" << inputBounds.y2;
#endif

            return ImagePtr();
        }

        if (inputImg && inputImagesThreadLocal.empty()) {
            ///If the effect is analysis (e.g: Tracker) there's no input images in the tread local storage, hence add it
            tls->currentRenderArgs.inputImages[inputNb].push_back(inputImg);
        }

        return inputImg;
    }


    /// The node is connected.
    assert(inputEffect);

    std::list<ImageComponents> requestedComps;
    requestedComps.push_back(isMask ? maskComps : requestComps);
    ImageList inputImages;
    RenderRoIRetCode retCode = inputEffect->renderRoI(RenderRoIArgs(time,
                                                                    scale,
                                                                    renderMappedMipMapLevel,
                                                                    view,
                                                                    byPassCache,
                                                                    pixelRoI,
                                                                    RectD(),
                                                                    requestedComps,
                                                                    depth,
                                                                    true,
                                                                    this,
                                                                    inputImagesThreadLocal), &inputImages);
    
    if (inputImages.empty() || (retCode != eRenderRoIRetCodeOk)) {
        return ImagePtr();
    }
    assert(inputImages.size() == 1);

    inputImg = inputImages.front();

    if (!pixelRoI.intersects( inputImg->getBounds())) {
        //The RoI requested does not intersect with the bounds of the input image, return a NULL image.
#ifdef DEBUG
        qDebug() << getNode()->getScriptName_mt_safe().c_str() << ": The RoI requested to" << inputEffect->getScriptName_mt_safe().c_str() << "does not intersect with the bounds of the input image";
#endif

        return ImagePtr();
    }

    /*
     * From now on this is the generic part. We first call renderRoI and then convert to the appropriate scale/components if needed.
     * Note that since the image has been pre-rendered before by the recursive nature of the algorithm, the call to renderRoI will be
     * instantaneous thanks to the image cache.
     */

#ifdef DEBUG
    ///Check that the rendered image contains what we requested.
    if ((!isMask && inputImg->getComponents() != requestComps) || (isMask && inputImg->getComponents() != maskComps)) {
        ImageComponents cc;
        if (isMask) {
            cc = maskComps;
        } else {
            cc = requestComps;
        }
        qDebug() << "WARNING:"<< getNode()->getScriptName_mt_safe().c_str() << "requested" << cc.getComponentsGlobalName().c_str() << "but" << inputEffect->getScriptName_mt_safe().c_str() << "returned an image with"
        << inputImg->getComponents().getComponentsGlobalName().c_str();
        std::list<ImageComponents> prefComps;
        ImageBitDepthEnum depth;
        inputEffect->getPreferredDepthAndComponents(-1, &prefComps, &depth);
        assert(!prefComps.empty());
        qDebug() << inputEffect->getScriptName_mt_safe().c_str() << "output clip preference is" << prefComps.front().getComponentsGlobalName().c_str();
    }

#endif
    
    if (roiPixel) {
        *roiPixel = pixelRoI;
    }
    unsigned int inputImgMipMapLevel = inputImg->getMipMapLevel();

    ///If the plug-in doesn't support the render scale, but the image is downscaled, up-scale it.
    ///Note that we do NOT cache it because it is really low def!
    if ( !dontUpscale  && renderFullScaleThenDownscale && (inputImgMipMapLevel != 0) ) {
        assert(inputImgMipMapLevel != 0);
        ///Resize the image according to the requested scale
        ImageBitDepthEnum bitdepth = inputImg->getBitDepth();
        RectI bounds;
        inputImg->getRoD().toPixelEnclosing(0, par, &bounds);
        ImagePtr rescaledImg( new Image(inputImg->getComponents(), inputImg->getRoD(),
                                                bounds, 0, par, bitdepth) );
        inputImg->upscaleMipMap( inputImg->getBounds(), inputImgMipMapLevel, 0, rescaledImg.get() );
        if (roiPixel) {
            RectD canonicalPixelRoI;
            pixelRoI.toCanonical(inputImgMipMapLevel, par, rod, &canonicalPixelRoI);
            canonicalPixelRoI.toPixelEnclosing(0, par, roiPixel);
            pixelRoI = *roiPixel;
        }

        inputImg = rescaledImg;
    }
    
    
    //Remap if needed
    ImagePremultiplicationEnum outputPremult;
    if (requestComps.isColorPlane()) {
        outputPremult = inputEffect->getOutputPremultiplication();
    } else {
        outputPremult = eImagePremultiplicationOpaque;
    }
    

    
    if (mapImageToPreferredComps) {
        std::list<ImageComponents> prefComps;
        ImageBitDepthEnum prefDepth;
        getPreferredDepthAndComponents(inputNb, &prefComps, &prefDepth);
        assert(!prefComps.empty());
        
        inputImg = convertPlanesFormatsIfNeeded(getApp(), inputImg, pixelRoI, prefComps.front(), prefDepth, getNode()->usesAlpha0ToConvertFromRGBToRGBA(), outputPremult, channelForMask);
    }
    
    if (inputImagesThreadLocal.empty()) {
        ///If the effect is analysis (e.g: Tracker) there's no input images in the tread local storage, hence add it
        tls->currentRenderArgs.inputImages[inputNb].push_back(inputImg);
    }

    return inputImg;
} // getImage

void
EffectInstance::calcDefaultRegionOfDefinition(U64 /*hash*/,
                                              double /*time*/,
                                              const RenderScale & /*scale*/,
                                              int /*view*/,
                                              RectD *rod)
{
    Format projectDefault;

    getRenderFormat(&projectDefault);
    *rod = RectD( projectDefault.left(), projectDefault.bottom(), projectDefault.right(), projectDefault.top() );
}

StatusEnum
EffectInstance::getRegionOfDefinition(U64 hash,
                                      double time,
                                      const RenderScale & scale,
                                      int view,
                                      RectD* rod) //!< rod is in canonical coordinates
{
    bool firstInput = true;
    RenderScale renderMappedScale = scale;

    assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(scale.x == 1. && scale.y == 1.) ) );

    for (int i = 0; i < getMaxInputCount(); ++i) {
        if ( isInputMask(i) ) {
            continue;
        }
        EffectInstPtr input = getInput(i);
        if (input) {
            RectD inputRod;
            bool isProjectFormat;
            StatusEnum st = input->getRegionOfDefinition_public(hash, time, renderMappedScale, view, &inputRod, &isProjectFormat);
            assert(inputRod.x2 >= inputRod.x1 && inputRod.y2 >= inputRod.y1);
            if (st == eStatusFailed) {
                return st;
            }

            if (firstInput) {
                *rod = inputRod;
                firstInput = false;
            } else {
                rod->merge(inputRod);
            }
            assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);
        }
    }

    // if rod was not set, return default, else return OK
    return firstInput ? eStatusReplyDefault : eStatusOK;
}

bool
EffectInstance::ifInfiniteApplyHeuristic(U64 hash,
                                         double time,
                                         const RenderScale & scale,
                                         int view,
                                         RectD* rod) //!< input/output
{
    /*If the rod is infinite clip it to the project's default*/

    Format projectFormat;

    getRenderFormat(&projectFormat);
    RectD projectDefault = projectFormat.toCanonicalFormat();
    /// FIXME: before removing the assert() (I know you are tempted) please explain (here: document!) if the format rectangle can be empty and in what situation(s)
    assert( !projectDefault.isNull() );

    assert(rod);
    assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);
    bool x1Infinite = rod->x1 <= kOfxFlagInfiniteMin;
    bool y1Infinite = rod->y1 <= kOfxFlagInfiniteMin;
    bool x2Infinite = rod->x2 >= kOfxFlagInfiniteMax;
    bool y2Infinite = rod->y2 >= kOfxFlagInfiniteMax;

    ///Get the union of the inputs.
    RectD inputsUnion;

    ///Do the following only if one coordinate is infinite otherwise we wont need the RoD of the input
    if (x1Infinite || y1Infinite || x2Infinite || y2Infinite) {
        // initialize with the effect's default RoD, because inputs may not be connected to other effects (e.g. Roto)
        calcDefaultRegionOfDefinition(hash, time, scale, view, &inputsUnion);
        bool firstInput = true;
        for (int i = 0; i < getMaxInputCount(); ++i) {
            EffectInstPtr input = getInput(i);
            if (input) {
                RectD inputRod;
                bool isProjectFormat;
                RenderScale inputScale = scale;
                if (input->supportsRenderScaleMaybe() == eSupportsNo) {
                    inputScale.x = inputScale.y = 1.;
                }
                StatusEnum st = input->getRegionOfDefinition_public(hash, time, inputScale, view, &inputRod, &isProjectFormat);
                if (st != eStatusFailed) {
                    if (firstInput) {
                        inputsUnion = inputRod;
                        firstInput = false;
                    } else {
                        inputsUnion.merge(inputRod);
                    }
                }
            }
        }
    }
    ///If infinite : clip to inputsUnion if not null, otherwise to project default

    // BE CAREFUL:
    // std::numeric_limits<int>::infinity() does not exist (check std::numeric_limits<int>::has_infinity)
    bool isProjectFormat = false;
    if (x1Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->x1 = std::min(inputsUnion.x1, projectDefault.x1);
        } else {
            rod->x1 = projectDefault.x1;
            isProjectFormat = true;
        }
        rod->x2 = std::max(rod->x1, rod->x2);
    }
    if (y1Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->y1 = std::min(inputsUnion.y1, projectDefault.y1);
        } else {
            rod->y1 = projectDefault.y1;
            isProjectFormat = true;
        }
        rod->y2 = std::max(rod->y1, rod->y2);
    }
    if (x2Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->x2 = std::max(inputsUnion.x2, projectDefault.x2);
        } else {
            rod->x2 = projectDefault.x2;
            isProjectFormat = true;
        }
        rod->x1 = std::min(rod->x1, rod->x2);
    }
    if (y2Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->y2 = std::max(inputsUnion.y2, projectDefault.y2);
        } else {
            rod->y2 = projectDefault.y2;
            isProjectFormat = true;
        }
        rod->y1 = std::min(rod->y1, rod->y2);
    }
    if ( isProjectFormat && !isGenerator() ) {
        isProjectFormat = false;
    }
    assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);

    return isProjectFormat;
} // ifInfiniteApplyHeuristic

void
EffectInstance::getRegionsOfInterest(double time,
                                     const RenderScale & scale,
                                     const RectD & /*outputRoD*/, //!< the RoD of the effect, in canonical coordinates
                                     const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                     int view,
                                     RoIMap* ret)
{
    bool tilesSupported = supportsTiles();
    for (int i = 0; i < getMaxInputCount(); ++i) {
        EffectInstPtr input = getInput(i);
        if (input) {
            if (tilesSupported) {
                ret->insert( std::make_pair(input, renderWindow) );
            } else {
                //Tiles not supported: get the RoD as RoI
                RectD rod;
                bool isPF;
                RenderScale inpScale(input->supportsRenderScale() ? scale.x : 1.);
                StatusEnum stat = input->getRegionOfDefinition_public(input->getRenderHash(), time, inpScale, view, &rod, &isPF);
                if (stat == eStatusFailed) {
                    return;
                }
                ret->insert( std::make_pair(input, rod) );
            }
        }
    }
}

FramesNeededMap
EffectInstance::getFramesNeeded(double time,
                                int view)
{
    FramesNeededMap ret;
    RangeD defaultRange;

    defaultRange.min = defaultRange.max = time;
    std::vector<RangeD> ranges;
    ranges.push_back(defaultRange);
    std::map<int, std::vector<RangeD> > defViewRange;
    defViewRange.insert( std::make_pair(view, ranges) );
    for (int i = 0; i < getMaxInputCount(); ++i) {
        if ( isInputRotoBrush(i) ) {
            ret.insert( std::make_pair(i, defViewRange) );
        } else {
            EffectInstPtr input = getInput(i);
            if (input) {
                ret.insert( std::make_pair(i, defViewRange) );
            }
        }
    }

    return ret;
}

void
EffectInstance::getFrameRange(double *first,
                              double *last)
{
    // default is infinite if there are no non optional input clips
    *first = INT_MIN;
    *last = INT_MAX;
    for (int i = 0; i < getMaxInputCount(); ++i) {
        EffectInstPtr input = getInput(i);
        if (input) {
            double inpFirst, inpLast;
            input->getFrameRange(&inpFirst, &inpLast);
            if (i == 0) {
                *first = inpFirst;
                *last = inpLast;
            } else {
                if (inpFirst < *first) {
                    *first = inpFirst;
                }
                if (inpLast > *last) {
                    *last = inpLast;
                }
            }
        }
    }
}

EffectInstance::NotifyRenderingStarted_RAII::NotifyRenderingStarted_RAII(Node* node)
    : _node(node)
{
    _didEmit = node->notifyRenderingStarted();
}

EffectInstance::NotifyRenderingStarted_RAII::~NotifyRenderingStarted_RAII()
{
    if (_didEmit) {
        _node->notifyRenderingEnded();
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
                             const boost::shared_ptr<ImageParams> & params,
                             bool useCache,
                             bool useDiskCache,
                             ImagePtr* image)
{
    if (useCache) {
        if (!useDiskCache) {
            AppManager::getImageFromCacheOrCreate(key, params, image);
        } else {
            AppManager::getImageFromDiskCacheOrCreate(key, params, image);
        }

        if (!*image) {
            std::stringstream ss;
            ss << "Failed to allocate an image of ";
            ss << printAsRAM( params->getElementsCount() * sizeof(Image::data_t) ).toStdString();
            Dialogs::errorDialog( QObject::tr("Out of memory").toStdString(), ss.str() );

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


        (*image)->ensureBounds( params->getBounds() );
    } else {
        image->reset( new Image(key, params) );
    }
}

void
EffectInstance::getImageFromCacheAndConvertIfNeeded(bool useCache,
                                                    bool useDiskCache,
                                                    const ImageKey & key,
                                                    unsigned int mipMapLevel,
                                                    const RectI* boundsParam,
                                                    const RectD* rodParam,
                                                    ImageBitDepthEnum bitdepth,
                                                    const ImageComponents & components,
                                                    ImageBitDepthEnum nodePrefDepth,
                                                    const ImageComponents & nodePrefComps,
                                                    const EffectInstance::InputImagesMap & inputImages,
                                                    const boost::shared_ptr<RenderStats> & stats,
                                                    boost::shared_ptr<Image>* image)
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

    if (!isCached) {
        isCached = !useDiskCache ? AppManager::getImageFromCache(key, &cachedImages) : AppManager::getImageFromDiskCache(key, &cachedImages);
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

            if ( (*it)->getParams()->isRodProjectFormat() ) {
                ////If the image was cached with a RoD dependent on the project format, but the project format changed,
                ////just discard this entry
                Format projectFormat;
                getRenderFormat(&projectFormat);
                RectD canonicalProject = projectFormat.toCanonicalFormat();
                if ( canonicalProject != (*it)->getRoD() ) {
                    appPTR->removeFromNodeCache(*it);
                    continue;
                }
            }

            ///Throw away images that are not even what the node want to render
            if ( ( imgComps.isColorPlane() && nodePrefComps.isColorPlane() && (imgComps != nodePrefComps) ) || (imgDepth != nodePrefDepth) ) {
                appPTR->removeFromNodeCache(*it);
                continue;
            }

            bool convertible = imgComps.isConvertibleTo(components);
            if ( (imgMMlevel == mipMapLevel) && convertible &&
                 ( getSizeOfForBitDepth(imgDepth) >= getSizeOfForBitDepth(bitdepth) ) /* && imgComps == components && imgDepth == bitdepth*/ ) {
                ///We found  a matching image

                *image = *it;
                break;
            } else {
                if ( (imgMMlevel >= mipMapLevel) || !convertible ||
                     ( getSizeOfForBitDepth(imgDepth) < getSizeOfForBitDepth(bitdepth) ) ) {
                    ///Either smaller resolution or not enough components or bit-depth is not as deep, don't use the image
                    continue;
                }

                assert(imgMMlevel < mipMapLevel);

                if (!imageToConvert) {
                    imageToConvert = *it;
                } else {
                    ///We found an image which scale is closer to the requested mipmap level we want, use it instead
                    if ( imgMMlevel > imageToConvert->getMipMapLevel() ) {
                        imageToConvert = *it;
                    }
                }
            }
        } //end for

        if (imageToConvert && !*image) {
            ///Ensure the image is allocated
            (imageToConvert)->allocateMemory();


            if (imageToConvert->getMipMapLevel() != mipMapLevel) {
                boost::shared_ptr<ImageParams> oldParams = imageToConvert->getParams();

                assert(imageToConvert->getMipMapLevel() < mipMapLevel);
                
                //This is the bounds of the upscaled image
                RectI imgToConvertBounds = imageToConvert->getBounds();
                
                //The rodParam might be different of oldParams->getRoD() simply because the RoD is dependent on the mipmap level
                const RectD & rod = rodParam ? *rodParam : oldParams->getRoD();
                
                
                //RectD imgToConvertCanonical;
                //imgToConvertBounds.toCanonical(imageToConvert->getMipMapLevel(), imageToConvert->getPixelAspectRatio(), rod, &imgToConvertCanonical);
                RectI downscaledBounds;
                rod.toPixelEnclosing(mipMapLevel, imageToConvert->getPixelAspectRatio(), &downscaledBounds);
                //imgToConvertCanonical.toPixelEnclosing(imageToConvert->getMipMapLevel(), imageToConvert->getPixelAspectRatio(), &imgToConvertBounds);
                //imgToConvertCanonical.toPixelEnclosing(mipMapLevel, imageToConvert->getPixelAspectRatio(), &downscaledBounds);

                if (boundsParam) {
                    downscaledBounds.merge(*boundsParam);
                }

                //RectI pixelRoD;
                //rod.toPixelEnclosing(mipMapLevel, oldParams->getPixelAspectRatio(), &pixelRoD);
                //downscaledBounds.intersect(pixelRoD, &downscaledBounds);

                boost::shared_ptr<ImageParams> imageParams = Image::makeParams( oldParams->getCost(),
                                                                                rod,
                                                                                downscaledBounds,
                                                                                oldParams->getPixelAspectRatio(),
                                                                                mipMapLevel,
                                                                                oldParams->isRodProjectFormat(),
                                                                                oldParams->getComponents(),
                                                                                oldParams->getBitDepth(),
                                                                                oldParams->getFramesNeeded() );


                imageParams->setMipMapLevel(mipMapLevel);


                boost::shared_ptr<Image> img;
                getOrCreateFromCacheInternal(key, imageParams, useCache, useDiskCache, &img);
                if (!img) {
                    return;
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
                                                     useCache && imageToConvert->usesBitMap(),
                                                     img.get() );
                } else {
                    img->pasteFrom(*imageToConvert, imgToConvertBounds);
                }

                imageToConvert = img;
            }

            *image = imageToConvert;
            //assert(imageToConvert->getBounds().contains(bounds));
            if ( stats && stats->isInDepthProfilingEnabled() ) {
                stats->addCacheInfosForNode(getNode(), false, true);
            }
        } else if (*image) { //  else if (imageToConvert && !*image)
            ///Ensure the image is allocated
            (*image)->allocateMemory();

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
EffectInstance::tryConcatenateTransforms(double time,
                                         int view,
                                         const RenderScale & scale,
                                         InputMatrixMap* inputTransforms)
{
    bool canTransform = getNode()->getCurrentCanTransform();

    //An effect might not be able to concatenate transforms but can still apply a transform (e.g CornerPinMasked)
    std::list<int> inputHoldingTransforms;
    bool canApplyTransform = getInputsHoldingTransform(&inputHoldingTransforms);

    assert(inputHoldingTransforms.empty() || canApplyTransform);

    Transform::Matrix3x3 thisNodeTransform;
    EffectInstPtr inputToTransform;
    bool getTransformSucceeded = false;

    if (canTransform) {
        /*
         * If getting the transform does not succeed, then this effect is treated as any other ones.
         */
        StatusEnum stat = getTransform_public(time, scale, view, &inputToTransform, &thisNodeTransform);
        if (stat == eStatusOK) {
            getTransformSucceeded = true;
        }
    }


    if ( (canTransform && getTransformSucceeded) || ( !canTransform && canApplyTransform && !inputHoldingTransforms.empty() ) ) {
        for (std::list<int>::iterator it = inputHoldingTransforms.begin(); it != inputHoldingTransforms.end(); ++it) {
            EffectInstPtr input = getInput(*it);
            if (!input) {
                continue;
            }
            std::list<Transform::Matrix3x3> matricesByOrder; // from downstream to upstream
            InputMatrix im;
            im.newInputEffect = input;
            im.newInputNbToFetchFrom = *it;


            // recursion upstream
            bool inputCanTransform = false;
            bool inputIsDisabled  =  input->getNode()->isNodeDisabled();

            if (!inputIsDisabled) {
                inputCanTransform = input->getNode()->getCurrentCanTransform();
            }


            while ( input && (inputCanTransform || inputIsDisabled) ) {
                //input is either disabled, or identity or can concatenate a transform too
                if (inputIsDisabled) {
                    int prefInput;
                    input = input->getNearestNonDisabled();
                    prefInput = input ? input->getNode()->getPreferredInput() : -1;
                    if (prefInput == -1) {
                        break;
                    }

                    if (input) {
                        im.newInputNbToFetchFrom = prefInput;
                        im.newInputEffect = input;
                    }
                } else if (inputCanTransform) {
                    Transform::Matrix3x3 m;
                    inputToTransform.reset();
                    StatusEnum stat = input->getTransform_public(time, scale, view, &inputToTransform, &m);
                    if (stat == eStatusOK) {
                        matricesByOrder.push_back(m);
                        if (inputToTransform) {
                            im.newInputNbToFetchFrom = input->getInputNumber(inputToTransform.get());
                            im.newInputEffect = input;
                            input = inputToTransform;
                        }
                    } else {
                        break;
                    }
                } else {
                    assert(false);
                }

                if (input) {
                    inputIsDisabled = input->getNode()->isNodeDisabled();
                    if (!inputIsDisabled) {
                        inputCanTransform = input->getNode()->getCurrentCanTransform();
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
    } // if ((canTransform && getTransformSucceeded) || (canApplyTransform && !inputHoldingTransforms.empty()))
} // EffectInstance::tryConcatenateTransforms


bool
EffectInstance::allocateImagePlane(const ImageKey & key,
                                   const RectD & rod,
                                   const RectI & downscaleImageBounds,
                                   const RectI & fullScaleImageBounds,
                                   bool isProjectFormat,
                                   const FramesNeededMap & framesNeeded,
                                   const ImageComponents & components,
                                   ImageBitDepthEnum depth,
                                   double par,
                                   unsigned int mipmapLevel,
                                   bool renderFullScaleThenDownscale,
                                   bool useDiskCache,
                                   bool createInCache,
                                   boost::shared_ptr<Image>* fullScaleImage,
                                   boost::shared_ptr<Image>* downscaleImage)
{
    //Controls whether images are stored on disk or in RAM, 0 = RAM, 1 = mmap
    int cost = useDiskCache ? 1 : 0;

    //If we're rendering full scale and with input images at full scale, don't cache the downscale image since it is cheap to
    //recreate, instead cache the full-scale image
    if (renderFullScaleThenDownscale) {
        downscaleImage->reset( new Image(components, rod, downscaleImageBounds, mipmapLevel, par, depth, true) );
        boost::shared_ptr<ImageParams> upscaledImageParams = Image::makeParams(cost,
                                                                                               rod,
                                                                                               fullScaleImageBounds,
                                                                                               par,
                                                                                               0,
                                                                                               isProjectFormat,
                                                                                               components,
                                                                                               depth,
                                                                                               framesNeeded);
        
        //The upscaled image will be rendered with input images at full def, it is then the best possibly rendered image so cache it!
        
        fullScaleImage->reset();
        getOrCreateFromCacheInternal(key, upscaledImageParams, createInCache, useDiskCache, fullScaleImage);
        
        if (!*fullScaleImage) {
            return false;
        }

    } else {
        
        ///Cache the image with the requested components instead of the remapped ones
        boost::shared_ptr<ImageParams> cachedImgParams = Image::makeParams(cost,
                                                                                           rod,
                                                                                           downscaleImageBounds,
                                                                                           par,
                                                                                           mipmapLevel,
                                                                                           isProjectFormat,
                                                                                           components,
                                                                                           depth,
                                                                                           framesNeeded);

        //Take the lock after getting the image from the cache or while allocating it
        ///to make sure a thread will not attempt to write to the image while its being allocated.
        ///When calling allocateMemory() on the image, the cache already has the lock since it added it
        ///so taking this lock now ensures the image will be allocated completetly

        getOrCreateFromCacheInternal(key, cachedImgParams, createInCache, useDiskCache, downscaleImage);
        if (!*downscaleImage) {
            return false;
        }
        *fullScaleImage = *downscaleImage;
    }

    return true;
} // EffectInstance::allocateImagePlane


void
EffectInstance::transformInputRois(const EffectInstance* self,
                                   const boost::shared_ptr<InputMatrixMap> & inputTransforms,
                                   double par,
                                   const RenderScale & scale,
                                   RoIMap* inputsRoi,
                                   std::map<int, EffectInstPtr>* reroutesMap)
{
    if (!inputTransforms) {
        return;
    }
    //Transform the RoIs by the inverse of the transform matrix (which is in pixel coordinates)
    for (InputMatrixMap::const_iterator it = inputTransforms->begin(); it != inputTransforms->end(); ++it) {
        RectD transformedRenderWindow;
        EffectInstPtr effectInTransformInput = self->getInput(it->first);
        assert(effectInTransformInput);


        RoIMap::iterator foundRoI = inputsRoi->find(effectInTransformInput);
        if ( foundRoI == inputsRoi->end() ) {
            //There might be no RoI because it was null
            continue;
        }

        // invert it
        Transform::Matrix3x3 invertTransform;
        double det = Transform::matDeterminant(*it->second.cat);
        if (det != 0.) {
            invertTransform = Transform::matInverse(*it->second.cat, det);
        }

        Transform::Matrix3x3 canonicalToPixel = Transform::matCanonicalToPixel(par, scale.x,
                                                                               scale.y, false);
        Transform::Matrix3x3 pixelToCanonical = Transform::matPixelToCanonical(par,  scale.x,
                                                                               scale.y, false);

        invertTransform = Transform::matMul(Transform::matMul(pixelToCanonical, invertTransform), canonicalToPixel);
        Transform::transformRegionFromRoD(foundRoI->second, invertTransform, transformedRenderWindow);

        //Replace the original RoI by the transformed RoI
        inputsRoi->erase(foundRoI);
        inputsRoi->insert( std::make_pair(it->second.newInputEffect->getInput(it->second.newInputNbToFetchFrom), transformedRenderWindow) );
        reroutesMap->insert( std::make_pair(it->first, it->second.newInputEffect) );
    }
}

EffectInstance::RenderRoIRetCode
EffectInstance::renderInputImagesForRoI(const FrameViewRequest* request,
                                        bool useTransforms,
                                        double time,
                                        int view,
                                        const RectD & rod,
                                        const RectD & canonicalRenderWindow,
                                        const boost::shared_ptr<InputMatrixMap>& inputTransforms,
                                        unsigned int mipMapLevel,
                                        const RenderScale & renderMappedScale,
                                        bool useScaleOneInputImages,
                                        bool byPassCache,
                                        const FramesNeededMap & framesNeeded,
                                        const EffectInstance::ComponentsNeededMap & neededComps,
                                        EffectInstance::InputImagesMap *inputImages,
                                        RoIMap* inputsRoi)
{
    if (!request) {
        getRegionsOfInterest_public(time, renderMappedScale, rod, canonicalRenderWindow, view, inputsRoi);
    }
#ifdef DEBUG
    if ( !inputsRoi->empty() && framesNeeded.empty() && !isReader() && !isRotoPaintNode() ) {
        qDebug() << getNode()->getScriptName_mt_safe().c_str() << ": getRegionsOfInterestAction returned 1 or multiple input RoI(s) but returned "
                 << "an empty list with getFramesNeededAction";
    }
#endif


    return treeRecurseFunctor(true,
                              getNode(),
                              framesNeeded,
                              *inputsRoi,
                              inputTransforms,
                              useTransforms,
                              mipMapLevel,
                              time,
                              view,
                              NodePtr(),
                              0,
                              inputImages,
                              &neededComps,
                              useScaleOneInputImages,
                              byPassCache);
}


EffectInstance::RenderingFunctorRetEnum
EffectInstance::Implementation::tiledRenderingFunctor(EffectInstance::Implementation::TiledRenderingFunctorArgs & args,
                                       const RectToRender & specificData,
                                       const QThread* callingThread)
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
                                 args.renderFullScaleThenDownscale,
                                 args.isSequentialRender,
                                 args.isRenderResponseToUserInteraction,
                                 args.firstFrame,
                                 args.lastFrame,
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
    appPTR->getAppTLS()->cleanupTLSForThread();
    
    return ret;
}

EffectInstance::RenderingFunctorRetEnum
EffectInstance::Implementation::tiledRenderingFunctor(const RectToRender & rectToRender,
                                                      const bool renderFullScaleThenDownscale,
                                                      const bool isSequentialRender,
                                                      const bool isRenderResponseToUserInteraction,
                                                      const int firstFrame,
                                                      const int lastFrame,
                                                      const int preferredInput,
                                                      const unsigned int mipMapLevel,
                                                      const unsigned int renderMappedMipMapLevel,
                                                      const RectD & rod,
                                                      const double time,
                                                      const int view,
                                                      const double par,
                                                      const bool byPassCache,
                                                      const ImageBitDepthEnum outputClipPrefDepth,
                                                      const std::list<ImageComponents> & outputClipPrefsComps,
                                                      const boost::shared_ptr<ComponentsNeededMap> & compsNeeded,
                                                      const std::bitset<4>& processChannels,
                                                      const boost::shared_ptr<ImagePlanesToRender> & planes) // when MT, planes is a copy so there's is no data race
{

    ///There cannot be the same thread running 2 concurrent instances of renderRoI on the same effect.
#ifdef DEBUG
    {
        EffectDataTLSPtr tls = tlsData->getTLSData();
        assert(!tls || !tls->currentRenderArgs.validArgs);
    }
#endif
    EffectDataTLSPtr tls = tlsData->getOrCreateTLSData();
    
    assert(!rectToRender.rect.isNull());

    /*
     * renderMappedRectToRender is in the mapped mipmap level, i.e the expected mipmap level of the render action of the plug-in
     */
    RectI renderMappedRectToRender = rectToRender.rect;
    
    /*
     * downscaledRectToRender is in the mipMapLevel
     */
    RectI downscaledRectToRender = renderMappedRectToRender;
    

    ///Upscale the RoI to a region in the full scale image so it is in canonical coordinates
    RectD canonicalRectToRender;
    renderMappedRectToRender.toCanonical(renderMappedMipMapLevel, par, rod, &canonicalRectToRender);
    if (renderFullScaleThenDownscale) {
        assert(mipMapLevel > 0 && renderMappedMipMapLevel != mipMapLevel);
        canonicalRectToRender.toPixelEnclosing(mipMapLevel, par, &downscaledRectToRender);
    }

    const EffectInstance::PlaneToRender & firstPlaneToRender = planes->planes.begin()->second;
    // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
# ifndef NDEBUG
    RectI renderBounds = firstPlaneToRender.renderMappedImage->getBounds();
    assert(renderBounds.x1 <= renderMappedRectToRender.x1 && renderMappedRectToRender.x2 <= renderBounds.x2 &&
           renderBounds.y1 <= renderMappedRectToRender.y1 && renderMappedRectToRender.y2 <= renderBounds.y2);
# endif

    bool isBeingRenderedElseWhere = false;
    ///At this point if we're in eRenderSafetyFullySafeFrame mode, we are a thread that might have been launched way after
    ///the time renderRectToRender was computed. We recompute it to update the portion to render.
    ///Note that if it is bigger than the initial rectangle, we don't render the bigger rectangle since we cannot
    ///now make the preliminaries call to handle that region (getRoI etc...) so just stick with the old rect to render

    // check the bitmap!
    
    bool bitmapMarkedForRendering = false;
    
    const boost::shared_ptr<ParallelRenderArgs>& frameArgs = tls->frameArgs.back();
    if (frameArgs->tilesSupported) {
        if (renderFullScaleThenDownscale) {
           
            RectI initialRenderRect = renderMappedRectToRender;

#if NATRON_ENABLE_TRIMAP
            if (!frameArgs->canAbort && frameArgs->isRenderResponseToUserInteraction) {
                bitmapMarkedForRendering = true;
                renderMappedRectToRender = firstPlaneToRender.renderMappedImage->getMinimalRectAndMarkForRendering_trimap(renderMappedRectToRender, &isBeingRenderedElseWhere);
            } else {
                renderMappedRectToRender = firstPlaneToRender.renderMappedImage->getMinimalRect(renderMappedRectToRender);
            }
#else
            renderMappedRectToRender = renderMappedImage->getMinimalRect(renderMappedRectToRender);
#endif

            ///If the new rect after getMinimalRect is bigger (maybe because another thread as grown the image)
            ///we stick to what was requested
            if (!initialRenderRect.contains(renderMappedRectToRender)) {
                renderMappedRectToRender = initialRenderRect;
            }
            
            RectD canonicalReducedRectToRender;
            renderMappedRectToRender.toCanonical(renderMappedMipMapLevel, par, rod, &canonicalReducedRectToRender);
            canonicalReducedRectToRender.toPixelEnclosing(mipMapLevel, par, &downscaledRectToRender);
            
            
            assert(renderMappedRectToRender.isNull() ||
                   (renderBounds.x1 <= renderMappedRectToRender.x1 && renderMappedRectToRender.x2 <= renderBounds.x2 && renderBounds.y1 <= renderMappedRectToRender.y1 && renderMappedRectToRender.y2 <= renderBounds.y2));
        } else {
            //The downscaled image is cached, read bitmap from it
#if NATRON_ENABLE_TRIMAP
            RectI rectToRenderMinimal;
            if (!frameArgs->canAbort && frameArgs->isRenderResponseToUserInteraction) {
                bitmapMarkedForRendering = true;
                rectToRenderMinimal = firstPlaneToRender.downscaleImage->getMinimalRectAndMarkForRendering_trimap(renderMappedRectToRender, &isBeingRenderedElseWhere);
            } else {
                rectToRenderMinimal = firstPlaneToRender.downscaleImage->getMinimalRect(renderMappedRectToRender);
            }
#else
            const RectI rectToRenderMinimal = downscaledImage->getMinimalRect(renderMappedRectToRender);
#endif

            assert(renderMappedRectToRender.isNull() ||
                   (renderBounds.x1 <= rectToRenderMinimal.x1 && rectToRenderMinimal.x2 <= renderBounds.x2 && renderBounds.y1 <= rectToRenderMinimal.y1 && rectToRenderMinimal.y2 <= renderBounds.y2) );
            
            
            ///If the new rect after getMinimalRect is bigger (maybe because another thread as grown the image)
            ///we stick to what was requested
            if (!renderMappedRectToRender.contains(rectToRenderMinimal)) {
                renderMappedRectToRender = rectToRenderMinimal;
            }
            downscaledRectToRender = renderMappedRectToRender;
        }
    } // tilesSupported
      ///It might have been already rendered now
    if (renderMappedRectToRender.isNull()) {
        return isBeingRenderedElseWhere ? eRenderingFunctorRetTakeImageLock : eRenderingFunctorRetOK;
    }

  

    ///This RAII struct controls the lifetime of the validArgs Flag in tls->currentRenderArgs
    Implementation::ScopedRenderArgs scopedArgs(tls,
                                                rod,
                                                renderMappedRectToRender,
                                                time,
                                                view,
                                                rectToRender.isIdentity,
                                                rectToRender.identityTime,
                                                rectToRender.identityInput,
                                                compsNeeded,
                                                rectToRender.imgs,
                                                rectToRender.inputRois, firstFrame, lastFrame);


    ImagePtr originalInputImage, maskImage;
    ImagePremultiplicationEnum originalImagePremultiplication;
    EffectInstance::InputImagesMap::const_iterator foundPrefInput = rectToRender.imgs.find(preferredInput);
    EffectInstance::InputImagesMap::const_iterator foundMaskInput = rectToRender.imgs.end();

    if ( _publicInterface->isHostMaskingEnabled() ) {
        foundMaskInput = rectToRender.imgs.find(_publicInterface->getMaxInputCount() - 1);
    }
    if ( ( foundPrefInput != rectToRender.imgs.end() ) && !foundPrefInput->second.empty() ) {
        originalInputImage = foundPrefInput->second.front();
    }
    std::map<int, ImagePremultiplicationEnum>::const_iterator foundPrefPremult = planes->inputPremult.find(preferredInput);
    if ( ( foundPrefPremult != planes->inputPremult.end() ) && originalInputImage ) {
        originalImagePremultiplication = foundPrefPremult->second;
    } else {
        originalImagePremultiplication = eImagePremultiplicationOpaque;
    }


    if ( ( foundMaskInput != rectToRender.imgs.end() ) && !foundMaskInput->second.empty() ) {
        maskImage = foundMaskInput->second.front();
    }

#ifndef NDEBUG
    RenderScale scale(Image::getScaleFromMipMapLevel(mipMapLevel));
    // check the dimensions of all input and output images
    const RectD & dstRodCanonical = firstPlaneToRender.renderMappedImage->getRoD();
    RectI dstBounds;
    dstRodCanonical.toPixelEnclosing(firstPlaneToRender.renderMappedImage->getMipMapLevel(), par, &dstBounds); // compute dstRod at level 0
    RectI dstRealBounds = firstPlaneToRender.renderMappedImage->getBounds();
    if (!frameArgs->tilesSupported) {
        assert(dstRealBounds.x1 == dstBounds.x1);
        assert(dstRealBounds.x2 == dstBounds.x2);
        assert(dstRealBounds.y1 == dstBounds.y1);
        assert(dstRealBounds.y2 == dstBounds.y2);
    }
    
    for (InputImagesMap::const_iterator it = rectToRender.imgs.begin();
         it != rectToRender.imgs.end();
         ++it) {
        for (ImageList::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            const RectD & srcRodCanonical = (*it2)->getRoD();
            RectI srcBounds;
            srcRodCanonical.toPixelEnclosing( (*it2)->getMipMapLevel(), (*it2)->getPixelAspectRatio(), &srcBounds ); // compute srcRod at level 0

            if (!frameArgs->tilesSupported) {
                // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsTiles
                //  If a clip or plugin does not support tiled images, then the host should supply full RoD images to the effect whenever it fetches one.

                /*
                 * The following asserts do not hold true: In the following graph example: Viewer-->Writer-->Blur-->Read
                 * The Writer does not support tiles. However Blur produces 2 distinct RoD depending on the render mipmap level
                 * If a Blur image was produced at mipmaplevel 0, and then we render in the Viewer with a mipmap level of 1, the
                 * Blur will actually retrieve the image from the cache and downscale it rather than recompute it.
                 * Since the Writer does not support tiles, the Blur image is the full image and not a tile, which can be veryfied by
                 *
                 * blurCachedImage->getRod().toPixelEnclosing(blurCachedImage->getMipMapLevel(), blurCachedImage->getPixelAspectRatio(), &bounds)
                 *
                 * Since the Blur RoD changed (the RoD at mmlevel 0 is different than the ROD at mmlevel 1),
                 * the resulting bounds of the downscaled image are not necessarily exactly result of the new downscaled RoD to the enclosing pixel
                 * bounds, i.e: the bounds of the downscaled image may be contained in the bounds computed
                 * by the line of code above (replacing blurCachedImage by the downscaledBlurCachedImage).
                 */
                /*
                assert(srcRealBounds.x1 == srcBounds.x1);
                assert(srcRealBounds.x2 == srcBounds.x2);
                assert(srcRealBounds.y1 == srcBounds.y1);
                assert(srcRealBounds.y2 == srcBounds.y2);*/
 
            }
            if ( !_publicInterface->supportsMultiResolution() ) {
                // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsMultiResolution
                //   Multiple resolution images mean...
                //    input and output images can be of any size
                //    input and output images can be offset from the origin
                assert(srcBounds.x1 == 0);
                assert(srcBounds.y1 == 0);
                assert(srcBounds.x1 == dstBounds.x1);
                assert(srcBounds.x2 == dstBounds.x2);
                assert(srcBounds.y1 == dstBounds.y1);
                assert(srcBounds.y2 == dstBounds.y2);
            }
        } // end for
    } //end for

    if (_publicInterface->supportsRenderScaleMaybe() == eSupportsNo) {
        assert(firstPlaneToRender.renderMappedImage->getMipMapLevel() == 0);
        assert(renderMappedMipMapLevel == 0);
    }
#     endif // DEBUG

    RenderingFunctorRetEnum handlerRet =  renderHandler(tls,
                                                        mipMapLevel,
                                                        renderFullScaleThenDownscale,
                                                        isSequentialRender,
                                                        isRenderResponseToUserInteraction,
                                                        renderMappedRectToRender,
                                                        downscaledRectToRender,
                                                        byPassCache,
                                                        bitmapMarkedForRendering,
                                                        outputClipPrefDepth,
                                                        outputClipPrefsComps,
                                                        processChannels,
                                                        originalInputImage,
                                                        maskImage,
                                                        originalImagePremultiplication,
                                                        *planes);
    if (handlerRet == eRenderingFunctorRetOK) {
        if (isBeingRenderedElseWhere) {
            return eRenderingFunctorRetTakeImageLock;
        } else {
            return eRenderingFunctorRetOK;
        }
    } else {
        return handlerRet;
    }
} // EffectInstance::tiledRenderingFunctor

EffectInstance::RenderingFunctorRetEnum
EffectInstance::Implementation::renderHandler(const EffectDataTLSPtr& tls,
                                              const unsigned int mipMapLevel,
                                              const bool renderFullScaleThenDownscale,
                                              const bool isSequentialRender,
                                              const bool isRenderResponseToUserInteraction,
                                              const RectI & renderMappedRectToRender,
                                              const RectI & downscaledRectToRender,
                                              const bool byPassCache,
                                              const bool bitmapMarkedForRendering,
                                              const ImageBitDepthEnum outputClipPrefDepth,
                                              const std::list<ImageComponents> & outputClipPrefsComps,
                                              const std::bitset<4>& processChannels,
                                              const boost::shared_ptr<Image> & originalInputImage,
                                              const boost::shared_ptr<Image> & maskImage,
                                              const ImagePremultiplicationEnum originalImagePremultiplication,
                                              ImagePlanesToRender & planes)
{
    boost::shared_ptr<TimeLapse> timeRecorder;

    const boost::shared_ptr<ParallelRenderArgs>& frameArgs = tls->frameArgs.back();
    if (frameArgs->stats) {
        timeRecorder.reset( new TimeLapse() );
    }

    const EffectInstance::PlaneToRender & firstPlane = planes.planes.begin()->second;
    const double time = tls->currentRenderArgs.time;
    const int view = tls->currentRenderArgs.view;

    // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
# ifndef NDEBUG
    RectI renderBounds = firstPlane.renderMappedImage->getBounds();
    assert(renderBounds.x1 <= renderMappedRectToRender.x1 && renderMappedRectToRender.x2 <= renderBounds.x2 &&
           renderBounds.y1 <= renderMappedRectToRender.y1 && renderMappedRectToRender.y2 <= renderBounds.y2);
# endif

    RenderActionArgs actionArgs;
    actionArgs.byPassCache = byPassCache;
    actionArgs.processChannels = processChannels;
    actionArgs.mappedScale.x = actionArgs.mappedScale.y = Image::getScaleFromMipMapLevel( firstPlane.renderMappedImage->getMipMapLevel() );
    assert( !( (_publicInterface->supportsRenderScaleMaybe() == eSupportsNo) && !(actionArgs.mappedScale.x == 1. && actionArgs.mappedScale.y == 1.) ) );
    actionArgs.originalScale.x = Image::getScaleFromMipMapLevel(mipMapLevel);
    actionArgs.originalScale.y = actionArgs.originalScale.x;
    actionArgs.draftMode = frameArgs->draftMode;

    std::list<std::pair<ImageComponents, ImagePtr> > tmpPlanes;
    bool multiPlanar = _publicInterface->isMultiPlanar();

    actionArgs.roi = renderMappedRectToRender;

    assert( !outputClipPrefsComps.empty() );

    if (tls->currentRenderArgs.isIdentity) {
        std::list<ImageComponents> comps;
        
        for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it) {
            //If color plane, request the preferred comp of the identity input
            if (tls->currentRenderArgs.identityInput && it->second.renderMappedImage->getComponents().isColorPlane()) {
                std::list<ImageComponents> prefInputComps;
                ImageBitDepthEnum prefInputDepth;
                tls->currentRenderArgs.identityInput->getPreferredDepthAndComponents(-1, &prefInputComps, &prefInputDepth);
                assert(!prefInputComps.empty());
                comps.push_back(prefInputComps.front());
            } else {
                comps.push_back( it->second.renderMappedImage->getComponents() );
            }
        }
        assert( !comps.empty() );
        ImageList identityPlanes;
        EffectInstance::RenderRoIArgs renderArgs(tls->currentRenderArgs.identityTime,
                                                 actionArgs.originalScale,
                                                 mipMapLevel,
                                                 view,
                                                 false,
                                                 downscaledRectToRender,
                                                 RectD(),
                                                 comps,
                                                 outputClipPrefDepth,
                                                 false,
                                                 _publicInterface);
        if (!tls->currentRenderArgs.identityInput) {
            for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it) {
                it->second.renderMappedImage->fillZero(renderMappedRectToRender);
                it->second.renderMappedImage->markForRendered(renderMappedRectToRender);
                
                if ( frameArgs->stats && frameArgs->stats->isInDepthProfilingEnabled() ) {
                    frameArgs->stats->addRenderInfosForNode( _publicInterface->getNode(),  NodePtr(), it->first.getComponentsGlobalName(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation() );
                }
            }

            return eRenderingFunctorRetOK;
        } else {
            EffectInstance::RenderRoIRetCode renderOk = tls->currentRenderArgs.identityInput->renderRoI(renderArgs, &identityPlanes);
            if (renderOk == eRenderRoIRetCodeAborted) {
                return eRenderingFunctorRetAborted;
            } else if (renderOk == eRenderRoIRetCodeFailed) {
                return eRenderingFunctorRetFailed;
            } else if ( identityPlanes.empty() ) {
                for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it) {
                    it->second.renderMappedImage->fillZero(renderMappedRectToRender);
                    it->second.renderMappedImage->markForRendered(renderMappedRectToRender);
                    
                    if ( frameArgs->stats && frameArgs->stats->isInDepthProfilingEnabled() ) {
                        frameArgs->stats->addRenderInfosForNode( _publicInterface->getNode(),  tls->currentRenderArgs.identityInput->getNode(), it->first.getComponentsGlobalName(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation() );
                    }
                }

                return eRenderingFunctorRetOK;
            } else {
                assert( identityPlanes.size() == planes.planes.size() );

                ImageList::iterator idIt = identityPlanes.begin();
                for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it, ++idIt) {
                    if ( renderFullScaleThenDownscale && ( (*idIt)->getMipMapLevel() > it->second.fullscaleImage->getMipMapLevel() ) ) {
                        if ( !(*idIt)->getBounds().contains(renderMappedRectToRender) ) {
                            ///Fill the RoI with 0's as the identity input image might have bounds contained into the RoI
                            it->second.fullscaleImage->fillZero(renderMappedRectToRender);
                        }

                        ///Convert format first if needed
                        ImagePtr sourceImage;
                        if ( ( it->second.fullscaleImage->getComponents() != (*idIt)->getComponents() ) || ( it->second.fullscaleImage->getBitDepth() != (*idIt)->getBitDepth() ) ) {
                            sourceImage.reset( new Image(it->second.fullscaleImage->getComponents(), (*idIt)->getRoD(), (*idIt)->getBounds(),
                                                         (*idIt)->getMipMapLevel(), (*idIt)->getPixelAspectRatio(), it->second.fullscaleImage->getBitDepth(), false) );
                            ViewerColorSpaceEnum colorspace = _publicInterface->getApp()->getDefaultColorSpaceForBitDepth( (*idIt)->getBitDepth() );
                            ViewerColorSpaceEnum dstColorspace = _publicInterface->getApp()->getDefaultColorSpaceForBitDepth( it->second.fullscaleImage->getBitDepth() );
                            (*idIt)->convertToFormat( (*idIt)->getBounds(), colorspace, dstColorspace, 3, false, false, sourceImage.get() );
                        } else {
                            sourceImage = *idIt;
                        }

                        ///then upscale
                        const RectD & rod = sourceImage->getRoD();
                        RectI bounds;
                        rod.toPixelEnclosing(it->second.renderMappedImage->getMipMapLevel(), it->second.renderMappedImage->getPixelAspectRatio(), &bounds);
                        ImagePtr inputPlane( new Image(it->first, rod, bounds, it->second.renderMappedImage->getMipMapLevel(),
                                                       it->second.renderMappedImage->getPixelAspectRatio(), it->second.renderMappedImage->getBitDepth(), false) );
                        sourceImage->upscaleMipMap( sourceImage->getBounds(), sourceImage->getMipMapLevel(), inputPlane->getMipMapLevel(), inputPlane.get() );
                        it->second.fullscaleImage->pasteFrom(*inputPlane, renderMappedRectToRender, false);
                        it->second.fullscaleImage->markForRendered(renderMappedRectToRender);
                    } else {
                        if ( !(*idIt)->getBounds().contains(downscaledRectToRender) ) {
                            ///Fill the RoI with 0's as the identity input image might have bounds contained into the RoI
                            it->second.downscaleImage->fillZero(downscaledRectToRender);
                        }

                        ///Convert format if needed or copy
                        if ( ( it->second.downscaleImage->getComponents() != (*idIt)->getComponents() ) || ( it->second.downscaleImage->getBitDepth() != (*idIt)->getBitDepth() ) ) {
                            ViewerColorSpaceEnum colorspace = _publicInterface->getApp()->getDefaultColorSpaceForBitDepth( (*idIt)->getBitDepth() );
                            ViewerColorSpaceEnum dstColorspace = _publicInterface->getApp()->getDefaultColorSpaceForBitDepth( it->second.fullscaleImage->getBitDepth() );
                            RectI convertWindow;
                            (*idIt)->getBounds().intersect(downscaledRectToRender, &convertWindow);
                            (*idIt)->convertToFormat( convertWindow, colorspace, dstColorspace, 3, false, false, it->second.downscaleImage.get() );
                        } else {
                            it->second.downscaleImage->pasteFrom(**idIt, downscaledRectToRender, false);
                        }
                        it->second.downscaleImage->markForRendered(downscaledRectToRender);
                    }

                    if ( frameArgs->stats && frameArgs->stats->isInDepthProfilingEnabled() ) {
                        frameArgs->stats->addRenderInfosForNode( _publicInterface->getNode(),  tls->currentRenderArgs.identityInput->getNode(), it->first.getComponentsGlobalName(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation() );
                    }
                }

                return eRenderingFunctorRetOK;
            } // if (renderOk == eRenderRoIRetCodeAborted) {
        }  //  if (!identityInput) {
    } // if (identity) {

    tls->currentRenderArgs.outputPlanes = planes.planes;
    for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = tls->currentRenderArgs.outputPlanes.begin(); it != tls->currentRenderArgs.outputPlanes.end(); ++it) {
        /*
         * When using the cache, allocate a local temporary buffer onto which the plug-in will render, and then safely
         * copy this buffer to the shared (among threads) image.
         * This is also needed if the plug-in does not support the number of components of the renderMappedImage
         */
        ImageComponents prefComp;
        if (multiPlanar) {
            prefComp = _publicInterface->getNode()->findClosestSupportedComponents( -1, it->second.renderMappedImage->getComponents() );
        } else {
            prefComp = Node::findClosestInList(it->second.renderMappedImage->getComponents(), outputClipPrefsComps, multiPlanar);
        }

        if ( ( it->second.renderMappedImage->usesBitMap() || ( prefComp != it->second.renderMappedImage->getComponents() ) ||
               ( outputClipPrefDepth != it->second.renderMappedImage->getBitDepth() ) ) && !_publicInterface->isPaintingOverItselfEnabled() ) {
            it->second.tmpImage.reset( new Image(prefComp,
                                                 it->second.renderMappedImage->getRoD(),
                                                 actionArgs.roi,
                                                 it->second.renderMappedImage->getMipMapLevel(),
                                                 it->second.renderMappedImage->getPixelAspectRatio(),
                                                 outputClipPrefDepth,
                                                 false) ); //< no bitmap
        } else {
            it->second.tmpImage = it->second.renderMappedImage;
        }
        tmpPlanes.push_back( std::make_pair(it->second.renderMappedImage->getComponents(), it->second.tmpImage) );
    }


#if NATRON_ENABLE_TRIMAP
    if (!bitmapMarkedForRendering && !frameArgs->canAbort && frameArgs->isRenderResponseToUserInteraction) {
        for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = tls->currentRenderArgs.outputPlanes.begin(); it != tls->currentRenderArgs.outputPlanes.end(); ++it) {
            it->second.renderMappedImage->markForRendering(renderMappedRectToRender);
        }
    }
#endif


    /// Render in the temporary image


    actionArgs.time = time;
    actionArgs.view = view;
    actionArgs.isSequentialRender = isSequentialRender;
    actionArgs.isRenderResponseToUserInteraction = isRenderResponseToUserInteraction;
    actionArgs.inputImages = tls->currentRenderArgs.inputImages;

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
    std::map<ImageComponents, EffectInstance::PlaneToRender> outputPlanes;
    for (std::list<std::list<std::pair<ImageComponents, ImagePtr> > >::iterator it = planesLists.begin(); it != planesLists.end(); ++it) {
        if (!multiPlanar) {
            assert( !it->empty() );
            tls->currentRenderArgs.outputPlaneBeingRendered = it->front().first;
        }
        actionArgs.outputPlanes = *it;

        StatusEnum st = _publicInterface->render_public(actionArgs);

        renderAborted = aborted(tls);

        /*
         * Since new planes can have been allocated on the fly by allocateImagePlaneAndSetInThreadLocalStorage(), refresh
         * the planes map from the thread local storage once the render action is finished
         */
        if ( it == planesLists.begin() ) {
            outputPlanes = tls->currentRenderArgs.outputPlanes;
            assert( !outputPlanes.empty() );
        }

        if ( (st != eStatusOK) || renderAborted ) {
#if NATRON_ENABLE_TRIMAP
            if (!frameArgs->canAbort && frameArgs->isRenderResponseToUserInteraction) {
                /*
                   At this point, another thread might have already gotten this image from the cache and could end-up
                   using it while it has still pixels marked to PIXEL_UNAVAILABLE, hence clear the bitmap
                 */
                for (std::map<ImageComponents, EffectInstance::PlaneToRender>::const_iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {
                    it->second.renderMappedImage->clearBitmap(renderMappedRectToRender);
                }
            }
#endif

            return st != eStatusOK ? eRenderingFunctorRetFailed : eRenderingFunctorRetAborted;
        } // if (st != eStatusOK || renderAborted) {
    } // for (std::list<std::list<std::pair<ImageComponents,ImagePtr> > >::iterator it = planesLists.begin(); it != planesLists.end(); ++it)

    assert(!renderAborted);

    bool unPremultIfNeeded = planes.outputPremult == eImagePremultiplicationPremultiplied;
    bool useMaskMix = _publicInterface->isHostMaskingEnabled() || _publicInterface->isHostMixingEnabled();
    double mix = useMaskMix ? _publicInterface->getNode()->getHostMixingValue(time) : 1.;
    bool doMask = useMaskMix ? _publicInterface->getNode()->isMaskEnabled(_publicInterface->getMaxInputCount() - 1) : false;

    
    //Check for NaNs, copy to output image and mark for rendered
    for (std::map<ImageComponents, EffectInstance::PlaneToRender>::const_iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {
        bool unPremultRequired = unPremultIfNeeded && it->second.tmpImage->getComponentsCount() == 4 && it->second.renderMappedImage->getComponentsCount() == 3;

        if ( frameArgs->doNansHandling && it->second.tmpImage->checkForNaNs(actionArgs.roi) ) {
            QString warning( _publicInterface->getNode()->getScriptName_mt_safe().c_str() );
            warning.append(": ");
            warning.append( tr("rendered rectangle (") );
            warning.append( QString::number(actionArgs.roi.x1) );
            warning.append(',');
            warning.append( QString::number(actionArgs.roi.y1) );
            warning.append(")-(");
            warning.append( QString::number(actionArgs.roi.x2) );
            warning.append(',');
            warning.append( QString::number(actionArgs.roi.y2) );
            warning.append(") ");
            warning.append( tr("contains NaN values. They have been converted to 1.") );
            _publicInterface->setPersistentMessage( eMessageTypeWarning, warning.toStdString() );
        }
        if (it->second.isAllocatedOnTheFly) {
            ///Plane allocated on the fly only have a temp image if using the cache and it is defined over the render window only
            if (it->second.tmpImage != it->second.renderMappedImage) {
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
                ///copy the rectangle rendered in the full scale image to the downscaled output
                assert(mipMapLevel != 0);
                
                assert(it->second.fullscaleImage != it->second.downscaleImage && it->second.renderMappedImage == it->second.fullscaleImage);
                
                ImagePtr mappedOriginalInputImage = originalInputImage;
                
                if (originalInputImage && originalInputImage->getMipMapLevel() != 0) {
                    
                    bool mustCopyUnprocessedChannels = it->second.tmpImage->canCallCopyUnProcessedChannels(processChannels);
                    if (mustCopyUnprocessedChannels || useMaskMix) {
                        ///there is some processing to be done by copyUnProcessedChannels or applyMaskMix
                        ///but originalInputImage is not in the correct mipMapLevel, upscale it
                        assert(originalInputImage->getMipMapLevel() > it->second.tmpImage->getMipMapLevel() &&
                               originalInputImage->getMipMapLevel() == mipMapLevel);
                        ImagePtr tmp(new Image(it->second.tmpImage->getComponents(), it->second.tmpImage->getRoD(), renderMappedRectToRender, mipMapLevel, it->second.tmpImage->getPixelAspectRatio(), it->second.tmpImage->getBitDepth(), false));
                        it->second.tmpImage->upscaleMipMap(downscaledRectToRender, originalInputImage->getMipMapLevel(), 0, tmp.get());
                        mappedOriginalInputImage = tmp;
                    }
                }
                
                if (mappedOriginalInputImage) {
                    it->second.tmpImage->copyUnProcessedChannels(renderMappedRectToRender, planes.outputPremult, originalImagePremultiplication, processChannels, mappedOriginalInputImage);
                    if (useMaskMix) {
                        it->second.tmpImage->applyMaskMix(renderMappedRectToRender, maskImage.get(), mappedOriginalInputImage.get(), doMask, false, mix);
                    }
                }
                if ( ( it->second.fullscaleImage->getComponents() != it->second.tmpImage->getComponents() ) ||
                    ( it->second.fullscaleImage->getBitDepth() != it->second.tmpImage->getBitDepth() ) ) {
                    /*
                     * BitDepth/Components conversion required as well as downscaling, do conversion to a tmp buffer
                     */
                    ImagePtr tmp( new Image(it->second.fullscaleImage->getComponents(), it->second.tmpImage->getRoD(), renderMappedRectToRender, mipMapLevel, it->second.tmpImage->getPixelAspectRatio(), it->second.fullscaleImage->getBitDepth(), false) );
                    
                    it->second.tmpImage->convertToFormat( renderMappedRectToRender,
                                                         _publicInterface->getApp()->getDefaultColorSpaceForBitDepth( it->second.tmpImage->getBitDepth() ),
                                                         _publicInterface->getApp()->getDefaultColorSpaceForBitDepth( it->second.fullscaleImage->getBitDepth() ),
                                                         -1, false, unPremultRequired, tmp.get() );
                    tmp->downscaleMipMap( it->second.tmpImage->getRoD(),
                                         renderMappedRectToRender, 0, mipMapLevel, false, it->second.downscaleImage.get() );
                    it->second.fullscaleImage->pasteFrom(*tmp, renderMappedRectToRender, false);
                } else {
                    /*
                     *  Downscaling required only
                     */
                    it->second.tmpImage->downscaleMipMap( it->second.tmpImage->getRoD(),
                                                         actionArgs.roi, 0, mipMapLevel, false, it->second.downscaleImage.get() );
                    it->second.fullscaleImage->pasteFrom(*(it->second.tmpImage), renderMappedRectToRender, false);
                }
                
                
                it->second.fullscaleImage->markForRendered(renderMappedRectToRender);
                
            } else { // if (renderFullScaleThenDownscale) {
                
                
                ///Copy the rectangle rendered in the downscaled image
                if (it->second.tmpImage != it->second.downscaleImage) {
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

                it->second.downscaleImage->copyUnProcessedChannels(actionArgs.roi, planes.outputPremult, originalImagePremultiplication, processChannels, originalInputImage);
                if (useMaskMix) {
                    it->second.downscaleImage->applyMaskMix(actionArgs.roi, maskImage.get(), originalInputImage.get(), doMask, false, mix);
                }
                it->second.downscaleImage->markForRendered(downscaledRectToRender);
                
                
            } // if (renderFullScaleThenDownscale) {
        } // if (it->second.isAllocatedOnTheFly) {

        if ( frameArgs->stats && frameArgs->stats->isInDepthProfilingEnabled() ) {
            frameArgs->stats->addRenderInfosForNode( _publicInterface->getNode(),  NodePtr(), it->first.getComponentsGlobalName(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation() );
        }
    } // for (std::map<ImageComponents,PlaneToRender>::const_iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {


    return eRenderingFunctorRetOK;
} // tiledRenderingFunctor

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
    EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
    if (!tls || !tls->currentRenderArgs.validArgs) {
        return ImagePtr();
    }
    
    assert( !tls->currentRenderArgs.outputPlanes.empty() );
    
    const EffectInstance::PlaneToRender & firstPlane = tls->currentRenderArgs.outputPlanes.begin()->second;
    bool useCache = firstPlane.fullscaleImage->usesBitMap() || firstPlane.downscaleImage->usesBitMap();
    if (getNode()->getPluginID().find("uk.co.thefoundry.furnace") != std::string::npos) {
        //Furnace plug-ins are bugged and do not render properly both planes, just wipe the image.
        useCache = false;
    }
    const ImagePtr & img = firstPlane.fullscaleImage->usesBitMap() ? firstPlane.fullscaleImage : firstPlane.downscaleImage;
    boost::shared_ptr<ImageParams> params = img->getParams();
    EffectInstance::PlaneToRender p;
    bool ok = allocateImagePlane(img->getKey(), tls->currentRenderArgs.rod, tls->currentRenderArgs.renderWindowPixel, tls->currentRenderArgs.renderWindowPixel, false, params->getFramesNeeded(), plane, img->getBitDepth(), img->getPixelAspectRatio(), img->getMipMapLevel(), false, false, useCache, &p.fullscaleImage, &p.downscaleImage);
    if (!ok) {
        return ImagePtr();
    } else {
        p.renderMappedImage = p.downscaleImage;
        p.isAllocatedOnTheFly = true;
        
        /*
         * Allocate a temporary image for rendering only if using cache
         */
        if (useCache) {
            p.tmpImage.reset( new Image(p.renderMappedImage->getComponents(),
                                        p.renderMappedImage->getRoD(),
                                        tls->currentRenderArgs.renderWindowPixel,
                                        p.renderMappedImage->getMipMapLevel(),
                                        p.renderMappedImage->getPixelAspectRatio(),
                                        p.renderMappedImage->getBitDepth(),
                                        false) );
        } else {
            p.tmpImage = p.renderMappedImage;
        }
        tls->currentRenderArgs.outputPlanes.insert( std::make_pair(plane, p) );
        
        return p.downscaleImage;
    }

  
} // allocateImagePlaneAndSetInThreadLocalStorage

void
EffectInstance::openImageFileKnob()
{
    const std::vector< KnobPtr > & knobs = getKnobs();

    for (U32 i = 0; i < knobs.size(); ++i) {
        if ( knobs[i]->typeName() == KnobFile::typeNameStatic() ) {
            boost::shared_ptr<KnobFile> fk = boost::dynamic_pointer_cast<KnobFile>(knobs[i]);
            assert(fk);
            if ( fk->isInputImageFile() ) {
                std::string file = fk->getValue();
                if ( file.empty() ) {
                    fk->open_file();
                }
                break;
            }
        } else if ( knobs[i]->typeName() == KnobOutputFile::typeNameStatic() ) {
            boost::shared_ptr<KnobOutputFile> fk = boost::dynamic_pointer_cast<KnobOutputFile>(knobs[i]);
            assert(fk);
            if ( fk->isOutputImageFile() ) {
                std::string file = fk->getValue();
                if ( file.empty() ) {
                    fk->open_file();
                }
                break;
            }
        }
    }
}

void
EffectInstance::evaluate(KnobI* knob,
                         bool isSignificant,
                         ValueChangedReasonEnum /*reason*/)
{
    KnobPage* isPage = dynamic_cast<KnobPage*>(knob);
    KnobGroup* isGrp = dynamic_cast<KnobGroup*>(knob);
    if (isGrp || isPage) {
        return;
    }
    
    ////If the node is currently modifying its input, to ask for a render
    ////because at then end of the inputChanged handler, it will ask for a refresh
    ////and a rebuild of the inputs tree.
    NodePtr node = getNode();

    if ( node->duringInputChangedAction() ) {
        return;
    }

    if ( getApp()->getProject()->isLoadingProject() ) {
        return;
    }
    
    
   
    KnobButton* button = dynamic_cast<KnobButton*>(knob);

    /*if this is a writer (openfx or built-in writer)*/
    if ( isWriter() ) {
        /*if this is a button and it is a render button,we're safe to assume the plug-ins wants to start rendering.*/
        if (button) {
            if ( button->isRenderButton() ) {
                
                AppInstance::RenderWork w;
                w.writer = dynamic_cast<OutputEffectInstance*>(this);
                w.firstFrame = INT_MIN;
                w.lastFrame = INT_MAX;
                w.frameStep = INT_MIN;
                std::list<AppInstance::RenderWork> works;
                works.push_back(w);
                getApp()->startWritersRendering(getApp()->isRenderStatsActionChecked(), false, works);

                return;
            }
        }
    }

    ///increments the knobs age following a change
    if (!button && isSignificant) {
        //We changed, abort any ongoing current render to refresh them with a newer version
        abortAnyEvaluation();
        node->incrementKnobsAge();
        node->refreshIdentityState();
        
        //Clear any persitent message, the user might have unlocked the situation 
        //node->clearPersistentMessage(false);
    }

    
    /*
     We always have to trigger a render because this might be a tree not connected via a link to the knob who changed
     but just an expression
     
     if (reason == eValueChangedReasonSlaveRefresh) {
        //do not trigger a render, the master will do it already
        return;
    }*/
    

    double time = getCurrentTime();
    std::list<ViewerInstance* > viewers;
    node->hasViewersConnected(&viewers);
    for (std::list<ViewerInstance* >::iterator it = viewers.begin();
         it != viewers.end();
         ++it) {
        if (isSignificant) {
            (*it)->renderCurrentFrame(true);
        } else {
            (*it)->redrawViewer();
        }
    }

    node->refreshPreviewsRecursivelyDownstream(time);
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
    getNode()->clearPersistentMessage(recurse);
}

int
EffectInstance::getInputNumber(const EffectInstance* inputEffect) const
{
    for (int i = 0; i < getMaxInputCount(); ++i) {
        if (getInput(i).get() == inputEffect) {
            return i;
        }
    }

    return -1;
}

/**
 * @brief Does this effect supports rendering at a different scale than 1 ?
 * There is no OFX property for this purpose. The only solution found for OFX is that if a isIdentity
 * with renderscale != 1 fails, the host retries with renderscale = 1 (and upscaled images).
 * If the renderScale support was not set, this throws an exception.
 **/
bool
EffectInstance::supportsRenderScale() const
{
    if (_imp->supportsRenderScale == eSupportsMaybe) {
        qDebug() << "EffectInstance::supportsRenderScale should be set before calling supportsRenderScale(), or use supportsRenderScaleMaybe() instead";
        throw std::runtime_error("supportsRenderScale not set");
    }

    return _imp->supportsRenderScale == eSupportsYes;
}

EffectInstance::SupportsEnum
EffectInstance::supportsRenderScaleMaybe() const
{
    QMutexLocker l(&_imp->supportsRenderScaleMutex);

    return _imp->supportsRenderScale;
}

/// should be set during effect initialization, but may also be set by the first getRegionOfDefinition that succeeds
void
EffectInstance::setSupportsRenderScaleMaybe(EffectInstance::SupportsEnum s) const
{
    {
        QMutexLocker l(&_imp->supportsRenderScaleMutex);

        _imp->supportsRenderScale = s;
    }
    NodePtr node = getNode();

    if (node) {
        node->onSetSupportRenderScaleMaybeSet( (int)s );
    }
}

void
EffectInstance::setOutputFilesForWriter(const std::string & pattern)
{
    if ( !isWriter() ) {
        return;
    }

    const KnobsVec & knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if ( knobs[i]->typeName() == KnobOutputFile::typeNameStatic() ) {
            boost::shared_ptr<KnobOutputFile> fk = boost::dynamic_pointer_cast<KnobOutputFile>(knobs[i]);
            assert(fk);
            if ( fk->isOutputImageFile() ) {
                fk->setValue(pattern, 0);
                break;
            }
        }
    }
}

PluginMemory*
EffectInstance::newMemoryInstance(size_t nBytes)
{
    PluginMemory* ret = new PluginMemory( getNode()->getEffectInstance() ); //< hack to get "this" as a shared ptr
    bool wasntLocked = ret->alloc(nBytes);

    assert(wasntLocked);
    Q_UNUSED(wasntLocked);

    return ret;
}

void
EffectInstance::addPluginMemoryPointer(PluginMemory* mem)
{
    QMutexLocker l(&_imp->pluginMemoryChunksMutex);

    _imp->pluginMemoryChunks.push_back(mem);
}

void
EffectInstance::removePluginMemoryPointer(PluginMemory* mem)
{
    QMutexLocker l(&_imp->pluginMemoryChunksMutex);
    std::list<PluginMemory*>::iterator it = std::find(_imp->pluginMemoryChunks.begin(), _imp->pluginMemoryChunks.end(), mem);

    if ( it != _imp->pluginMemoryChunks.end() ) {
        _imp->pluginMemoryChunks.erase(it);
    }
}

void
EffectInstance::registerPluginMemory(size_t nBytes)
{
    getNode()->registerPluginMemory(nBytes);
}

void
EffectInstance::unregisterPluginMemory(size_t nBytes)
{
    getNode()->unregisterPluginMemory(nBytes);
}

void
EffectInstance::onAllKnobsSlaved(bool isSlave,
                                 KnobHolder* master)
{
    getNode()->onAllKnobsSlaved(isSlave, master);
}

void
EffectInstance::onKnobSlaved(KnobI* slave,
                             KnobI* master,
                             int dimension,
                             bool isSlave)
{
    getNode()->onKnobSlaved(slave, master, dimension, isSlave);
}

void
EffectInstance::setCurrentViewportForOverlays_public(OverlaySupport* viewport)
{
    getNode()->setCurrentViewportForHostOverlays(viewport);
    setCurrentViewportForOverlays(viewport);
}

void
EffectInstance::drawOverlay_public(double time,
                                   const RenderScale & renderScale,
                                   int view)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() && !getNode()->hasHostOverlay() ) {
        return;
    }

    RECURSIVE_ACTION();

    _imp->setDuringInteractAction(true);
    drawOverlay(time, renderScale, view);
    getNode()->drawHostOverlay(time, renderScale);
    _imp->setDuringInteractAction(false);
}

bool
EffectInstance::onOverlayPenDown_public(double time,
                                        const RenderScale & renderScale,
                                        int view,
                                        const QPointF & viewportPos,
                                        const QPointF & pos,
                                        double pressure)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasHostOverlay() ) {
        return false;
    }

    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayPenDown(time, renderScale, view, viewportPos, pos, pressure);
        if (!ret) {
            ret |= getNode()->onOverlayPenDownDefault(renderScale, viewportPos, pos, pressure);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayPenMotion_public(double time,
                                          const RenderScale & renderScale,
                                          int view,
                                          const QPointF & viewportPos,
                                          const QPointF & pos,
                                          double pressure)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasHostOverlay() ) {
        return false;
    }


    NON_RECURSIVE_ACTION();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayPenMotion(time, renderScale, view, viewportPos, pos, pressure);
    if (!ret) {
        ret |= getNode()->onOverlayPenMotionDefault(renderScale, viewportPos, pos, pressure);
    }
    _imp->setDuringInteractAction(false);
    //Don't chek if render is needed on pen motion, wait for the pen up

    //checkIfRenderNeeded();
    return ret;
}

bool
EffectInstance::onOverlayPenUp_public(double time,
                                      const RenderScale & renderScale,
                                      int view,
                                      const QPointF & viewportPos,
                                      const QPointF & pos,
                                      double pressure)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasHostOverlay() ) {
        return false;
    }
    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayPenUp(time, renderScale, view, viewportPos, pos, pressure);
        if (!ret) {
            ret |= getNode()->onOverlayPenUpDefault(renderScale, viewportPos, pos, pressure);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayKeyDown_public(double time,
                                        const RenderScale & renderScale,
                                        int view,
                                        Key key,
                                        KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasHostOverlay() ) {
        return false;
    }

    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayKeyDown(time, renderScale, view, key, modifiers);
        if (!ret) {
            ret |= getNode()->onOverlayKeyDownDefault(renderScale, key, modifiers);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayKeyUp_public(double time,
                                      const RenderScale & renderScale,
                                      int view,
                                      Key key,
                                      KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasHostOverlay() ) {
        return false;
    }

    bool ret;
    {
        NON_RECURSIVE_ACTION();

        _imp->setDuringInteractAction(true);
        ret = onOverlayKeyUp(time, renderScale, view, key, modifiers);
        if (!ret) {
            ret |= getNode()->onOverlayKeyUpDefault(renderScale, key, modifiers);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayKeyRepeat_public(double time,
                                          const RenderScale & renderScale,
                                          int view,
                                          Key key,
                                          KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasHostOverlay() ) {
        return false;
    }

    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayKeyRepeat(time, renderScale, view, key, modifiers);
        if (!ret) {
            ret |= getNode()->onOverlayKeyRepeatDefault(renderScale, key, modifiers);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayFocusGained_public(double time,
                                            const RenderScale & renderScale,
                                            int view)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() && !getNode()->hasHostOverlay() ) {
        return false;
    }

    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayFocusGained(time, renderScale, view);
        if (!ret) {
            ret |= getNode()->onOverlayFocusGainedDefault(renderScale);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayFocusLost_public(double time,
                                          const RenderScale & renderScale,
                                          int view)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() && !getNode()->hasHostOverlay() ) {
        return false;
    }
    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayFocusLost(time, renderScale, view);
        if (!ret) {
            ret |= getNode()->onOverlayFocusLostDefault(renderScale);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::isDoingInteractAction() const
{
    QReadLocker l(&_imp->duringInteractActionMutex);

    return _imp->duringInteractAction;
}

StatusEnum
EffectInstance::render_public(const RenderActionArgs & args)
{
    NON_RECURSIVE_ACTION();
    return render(args);
}

StatusEnum
EffectInstance::getTransform_public(double time,
                                    const RenderScale & renderScale,
                                    int view,
                                    EffectInstPtr* inputToTransform,
                                    Transform::Matrix3x3* transform)
{
    RECURSIVE_ACTION();
    assert( getNode()->getCurrentCanTransform() );

    return getTransform(time, renderScale, view, inputToTransform, transform);
}

bool
EffectInstance::isIdentity_public(bool useIdentityCache, // only set to true when calling for the whole image (not for a subrect)
                                  U64 hash,
                                  double time,
                                  const RenderScale & scale,
                                  const RectI & renderWindow,
                                  int view,
                                  double* inputTime,
                                  int* inputNb)
{
    assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(scale.x == 1. && scale.y == 1.) ) );

    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);

    if (useIdentityCache) {
        double timeF = 0.;
        bool foundInCache = _imp->actionsCache.getIdentityResult(hash, time, view, mipMapLevel, inputNb, &timeF);
        if (foundInCache) {
            *inputTime = timeF;

            return *inputNb >= 0 || *inputNb == -2;
        }
    }
   

    ///EDIT: We now allow isIdentity to be called recursively.
    RECURSIVE_ACTION();


    bool ret = false;
    boost::shared_ptr<RotoDrawableItem> rotoItem = getNode()->getAttachedRotoItem();
    if ((rotoItem && !rotoItem->isActivated(time)) || getNode()->isNodeDisabled() || !getNode()->hasAtLeastOneChannelToProcess() ) {
        ret = true;
        *inputNb = getNode()->getPreferredInput();
        *inputTime = time;
    } else if ( appPTR->isBackground() && (dynamic_cast<DiskCacheNode*>(this) != NULL) ) {
        ret = true;
        *inputNb = 0;
        *inputTime = time;
    } else {
        /// Don't call isIdentity if plugin is sequential only.
        if (getSequentialPreference() != eSequentialPreferenceOnlySequential) {
            try {
                ret = isIdentity(time, scale, renderWindow, view, inputTime, inputNb);
            } catch (...) {
                throw;
            }
        }
    }
    if (!ret) {
        *inputNb = -1;
        *inputTime = time;
    }

    if (useIdentityCache) {
        _imp->actionsCache.setIdentityResult(hash, time, view, mipMapLevel, *inputNb, *inputTime);
    }

    return ret;
} // EffectInstance::isIdentity_public

void
EffectInstance::onInputChanged(int /*inputNo*/)
{

}

StatusEnum
EffectInstance::getRegionOfDefinition_publicInternal(U64 hash,
                                                     double time,
                                                     const RenderScale & scale,
                                                     const RectI & renderWindow,
                                                     bool useRenderWindow,
                                                     int view,
                                                     RectD* rod,
                                                     bool* isProjectFormat)
{
    if ( !isEffectCreated() ) {
        return eStatusFailed;
    }

    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);

    if (useRenderWindow) {
        double inputTimeIdentity;
        int inputNbIdentity;
        bool isIdentity = isIdentity_public(true, hash, time, scale, renderWindow, view, &inputTimeIdentity, &inputNbIdentity);
        if (isIdentity) {
            if (inputNbIdentity >= 0) {
                EffectInstPtr input = getInput(inputNbIdentity);
                if (input) {
                    return input->getRegionOfDefinition_public(input->getRenderHash(), inputTimeIdentity, scale, view, rod, isProjectFormat);
                }
            } else if (inputNbIdentity == -2) {
                return getRegionOfDefinition_public(hash, inputTimeIdentity, scale, view, rod, isProjectFormat);
            } else {
                return eStatusFailed;
            }
        }
    }

    bool foundInCache = _imp->actionsCache.getRoDResult(hash, time, view, mipMapLevel, rod);
    if (foundInCache) {
        *isProjectFormat = false;
        if ( rod->isNull() ) {
            return eStatusFailed;
        }

        return eStatusOK;
    } else {
        ///If this is running on a render thread, attempt to find the RoD in the thread local storage.
        
        if (QThread::currentThread() != qApp->thread()) {
            
            EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
            if (tls && tls->currentRenderArgs.validArgs) {
                *rod = tls->currentRenderArgs.rod;
                *isProjectFormat = false;
                return eStatusOK;
            }
        }

        StatusEnum ret;
        RenderScale scaleOne(1.);
        {
            RECURSIVE_ACTION();


            ret = getRegionOfDefinition(hash, time, supportsRenderScaleMaybe() == eSupportsNo ? scaleOne : scale, view, rod);

            if ( (ret != eStatusOK) && (ret != eStatusReplyDefault) ) {
                // rod is not valid
                //if (!isDuringStrokeCreation) {
                _imp->actionsCache.invalidateAll(hash);
                _imp->actionsCache.setRoDResult( hash, time, view, mipMapLevel, RectD() );

                // }
                return ret;
            }

            if ( rod->isNull() ) {
                //if (!isDuringStrokeCreation) {
                _imp->actionsCache.invalidateAll(hash);
                _imp->actionsCache.setRoDResult( hash, time, view, mipMapLevel, RectD() );

                //}
                return eStatusFailed;
            }

            assert( (ret == eStatusOK || ret == eStatusReplyDefault) && (rod->x1 <= rod->x2 && rod->y1 <= rod->y2) );
        }
        *isProjectFormat = ifInfiniteApplyHeuristic(hash, time, scale, view, rod);
        assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);

        //if (!isDuringStrokeCreation) {
        _imp->actionsCache.setRoDResult(hash, time, view,  mipMapLevel, *rod);

        //}
        return ret;
    }
} // EffectInstance::getRegionOfDefinition_publicInternal

StatusEnum
EffectInstance::getRegionOfDefinitionWithIdentityCheck_public(U64 hash,
                                                              double time,
                                                              const RenderScale & scale,
                                                              const RectI & renderWindow,
                                                              int view,
                                                              RectD* rod,
                                                              bool* isProjectFormat)
{
    return getRegionOfDefinition_publicInternal(hash, time, scale, renderWindow, true, view, rod, isProjectFormat);
}

StatusEnum
EffectInstance::getRegionOfDefinition_public(U64 hash,
                                             double time,
                                             const RenderScale & scale,
                                             int view,
                                             RectD* rod,
                                             bool* isProjectFormat)
{
    return getRegionOfDefinition_publicInternal(hash, time, scale, RectI(), false, view, rod, isProjectFormat);
}

void
EffectInstance::getRegionsOfInterest_public(double time,
                                            const RenderScale & scale,
                                            const RectD & outputRoD, //!< effect RoD in canonical coordinates
                                            const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                            int view,
                                            RoIMap* ret)
{
    NON_RECURSIVE_ACTION();
    assert(outputRoD.x2 >= outputRoD.x1 && outputRoD.y2 >= outputRoD.y1);
    assert(renderWindow.x2 >= renderWindow.x1 && renderWindow.y2 >= renderWindow.y1);

    getRegionsOfInterest(time, scale, outputRoD, renderWindow, view, ret);
}

FramesNeededMap
EffectInstance::getFramesNeeded_public(U64 hash,
                                       double time,
                                       int view,
                                       unsigned int mipMapLevel)
{
    NON_RECURSIVE_ACTION();
    FramesNeededMap framesNeeded;
    bool foundInCache = _imp->actionsCache.getFramesNeededResult(hash, time, view, mipMapLevel, &framesNeeded);
    if (foundInCache) {
        return framesNeeded;
    }

    try {
        framesNeeded = getFramesNeeded(time, view);
    } catch (std::exception &e) {
        if (!hasPersistentMessage()) { // plugin may already have set a message
            setPersistentMessage(eMessageTypeError, e.what());
        }
    }
    _imp->actionsCache.setFramesNeededResult(hash, time, view, mipMapLevel, framesNeeded);

    return framesNeeded;
}

void
EffectInstance::getFrameRange_public(U64 hash,
                                     double *first,
                                     double *last,
                                     bool bypasscache)
{
    double fFirst = 0., fLast = 0.;
    bool foundInCache = false;

    if (!bypasscache) {
        foundInCache = _imp->actionsCache.getTimeDomainResult(hash, &fFirst, &fLast);
    }
    if (foundInCache) {
        *first = std::floor(fFirst + 0.5);
        *last = std::floor(fLast + 0.5);
    } else {
        ///If this is running on a render thread, attempt to find the info in the thread local storage.
        if (QThread::currentThread() != qApp->thread()) {
            EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
            if (tls && tls->currentRenderArgs.validArgs) {
                *first = tls->currentRenderArgs.firstFrame;
                *last = tls->currentRenderArgs.lastFrame;

                return;
            }
        }

        NON_RECURSIVE_ACTION();
        getFrameRange(first, last);
        _imp->actionsCache.setTimeDomainResult(hash, *first, *last);
    }
}

StatusEnum
EffectInstance::beginSequenceRender_public(double first,
                                           double last,
                                           double step,
                                           bool interactive,
                                           const RenderScale & scale,
                                           bool isSequentialRender,
                                           bool isRenderResponseToUserInteraction,
                                           bool draftMode,
                                           int view)
{
    NON_RECURSIVE_ACTION();
    EffectDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
    assert(tls);
    ++tls->beginEndRenderCount;
    return beginSequenceRender(first, last, step, interactive, scale,
                               isSequentialRender, isRenderResponseToUserInteraction, draftMode, view);
}

StatusEnum
EffectInstance::endSequenceRender_public(double first,
                                         double last,
                                         double step,
                                         bool interactive,
                                         const RenderScale & scale,
                                         bool isSequentialRender,
                                         bool isRenderResponseToUserInteraction,
                                         bool draftMode,
                                         int view)
{
    NON_RECURSIVE_ACTION();
    EffectDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
    assert(tls);
    --tls->beginEndRenderCount;
    assert(tls->beginEndRenderCount >= 0);
    return endSequenceRender(first, last, step, interactive, scale, isSequentialRender, isRenderResponseToUserInteraction, draftMode, view);
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
EffectInstance::clearActionsCache()
{
    _imp->actionsCache.clearAll();
}

void
EffectInstance::setComponentsAvailableDirty(bool dirty)
{
    QMutexLocker k(&_imp->componentsAvailableMutex);

    _imp->componentsAvailableDirty = dirty;
}


void
EffectInstance::getComponentsAvailableRecursive(bool useLayerChoice,
                                                bool useThisNodeComponentsNeeded,
                                                double time,
                                                int view,
                                                ComponentsAvailableMap* comps,
                                                std::list<EffectInstance*>* markedNodes)
{
    if ( std::find(markedNodes->begin(), markedNodes->end(), this) != markedNodes->end() ) {
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
    EffectInstance::ComponentsNeededMap neededComps;
    SequenceTime ptTime;
    int ptView;
    NodePtr ptInput;
    bool processAll;
    std::bitset<4> processChannels;
    getComponentsNeededAndProduced_public(useLayerChoice, useThisNodeComponentsNeeded, time, view, &neededComps, &processAll, &ptTime, &ptView, &processChannels, &ptInput);


    ///If the plug-in is not pass-through, only consider the components processed by the plug-in in output,
    ///so we do not need to recurse.
    PassThroughEnum passThrough = isPassThroughForNonRenderedPlanes();
    if (passThrough == ePassThroughPassThroughNonRenderedPlanes ||
        passThrough == ePassThroughRenderAllRequestedPlanes) {

        if (!isMultiPlanar() || !ptInput) {
            ptInput = node->getInput(node->getPreferredInput());
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


   EffectInstance::ComponentsNeededMap::iterator foundOutput = neededComps.find(-1);
    if ( foundOutput != neededComps.end() ) {
        ///Foreach component produced by the node at the given (view,time),  try
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
        ///Foreach user component, add it as an available component, but use this node only if it is also
        ///in the "needed components" list
        for (std::list<ImageComponents>::iterator it = userComps.begin(); it != userComps.end(); ++it) {
            
            
            ///If this is a user comp and used by the node it will be in the needed output components
            bool found = false;
            if (foundOutput != neededComps.end()) {
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
    
    markedNodes->push_back(this);

    
    if (useLayerChoice && useThisNodeComponentsNeeded) {
        QMutexLocker k(&_imp->componentsAvailableMutex);
        _imp->componentsAvailableDirty = false;
        _imp->outputComponentsAvailable = *comps;
    }
} // EffectInstance::getComponentsAvailableRecursive

void
EffectInstance::getComponentsAvailable(bool useLayerChoice,
                                       bool useThisNodeComponentsNeeded,
                                       double time,
                                       ComponentsAvailableMap* comps,
                                       std::list<EffectInstance*>* markedNodes)
{
    getComponentsAvailableRecursive(useLayerChoice, useThisNodeComponentsNeeded, time, 0, comps, markedNodes);
}

void
EffectInstance::getComponentsAvailable(bool useLayerChoice,
                                       bool useThisNodeComponentsNeeded,
                                       double time,
                                       ComponentsAvailableMap* comps)
{
    //int nViews = getApp()->getProject()->getProjectViewsCount();

    ///Union components over all views
    //for (int view = 0; view < nViews; ++view) {
    ///Edit: Just call for 1 view, it should not matter as this should be view agnostic.
    std::list<EffectInstance*> marks;

    getComponentsAvailableRecursive(useLayerChoice, useThisNodeComponentsNeeded, time, 0, comps, &marks);

    //}
}

void
EffectInstance::getComponentsNeededAndProduced(double time,
                                               int view,
                                              EffectInstance::ComponentsNeededMap* comps,
                                               SequenceTime* passThroughTime,
                                               int* passThroughView,
                                               NodePtr* passThroughInput)
{
    *passThroughTime = time;
    *passThroughView = view;

    std::list<ImageComponents> outputComp;
    ImageBitDepthEnum outputDepth;
    getPreferredDepthAndComponents(-1, &outputComp, &outputDepth);

    std::vector<ImageComponents> outputCompVec;
    for (std::list<ImageComponents>::iterator it = outputComp.begin(); it != outputComp.end(); ++it) {
        outputCompVec.push_back(*it);
    }


    comps->insert( std::make_pair(-1, outputCompVec) );

    NodePtr firstConnectedOptional;
    for (int i = 0; i < getMaxInputCount(); ++i) {
        NodePtr node = getNode()->getInput(i);
        if (!node) {
            continue;
        }
        if ( isInputRotoBrush(i) ) {
            continue;
        }

        std::list<ImageComponents> comp;
        ImageBitDepthEnum depth;
        getPreferredDepthAndComponents(-1, &comp, &depth);

        std::vector<ImageComponents> compVect;
        for (std::list<ImageComponents>::iterator it = comp.begin(); it != comp.end(); ++it) {
            compVect.push_back(*it);
        }
        comps->insert( std::make_pair(i, compVect) );

        if ( !isInputOptional(i) ) {
            *passThroughInput = node;
        } else {
            firstConnectedOptional = node;
        }
    }
    if (!*passThroughInput) {
        *passThroughInput = firstConnectedOptional;
    }
}

void
EffectInstance::getComponentsNeededAndProduced_public(bool useLayerChoice,
                                                      bool useThisNodeComponentsNeeded,
                                                      double time,
                                                      int view,
                                                      EffectInstance::ComponentsNeededMap* comps,
                                                      bool* processAllRequested,
                                                      SequenceTime* passThroughTime,
                                                      int* passThroughView,
                                                      std::bitset<4> *processChannels,
                                                      NodePtr* passThroughInput)

{
    RECURSIVE_ACTION();

    if ( isMultiPlanar() ) {
        (*processChannels)[0] = (*processChannels)[1] = (*processChannels)[2] = (*processChannels)[3] = true;
        if (useThisNodeComponentsNeeded) {
            getComponentsNeededAndProduced(time, view, comps, passThroughTime, passThroughView, passThroughInput);
        }
        *processAllRequested = false;
        return;
    }
    
    
    *passThroughTime = time;
    *passThroughView = view;
    int idx = getNode()->getPreferredInput();
    *passThroughInput = getNode()->getInput(idx);
    
    if (!useThisNodeComponentsNeeded) {
        return;
    }
        
    ///Get the output needed components
    {
        ImageComponents layer;
        std::vector<ImageComponents> compVec;
        bool ok = false;
        if (useLayerChoice) {
            ok = getNode()->getSelectedLayer(-1, processChannels, processAllRequested, &layer);
        }
        ImageBitDepthEnum depth;
        std::list<ImageComponents> clipPrefsComps;
        getPreferredDepthAndComponents(-1, &clipPrefsComps, &depth);
        
        if (ok && layer.getNumComponents() != 0 && !layer.isColorPlane()) {
            
            compVec.push_back(layer);
            
            for (std::list<ImageComponents>::iterator it = clipPrefsComps.begin(); it != clipPrefsComps.end(); ++it) {
                if (!(*it).isColorPlane()) {
                    compVec.push_back(*it);
                }
            }
            
        } else {
            for (std::list<ImageComponents>::iterator it = clipPrefsComps.begin(); it != clipPrefsComps.end(); ++it) {
                compVec.push_back(*it);
            }
        }
        
        comps->insert( std::make_pair(-1, compVec) );
    }
    
    ///For each input get their needed components
    int maxInput = getMaxInputCount();
    for (int i = 0; i < maxInput; ++i) {
        EffectInstPtr input = getInput(i);
        bool isRotoInput = isInputRotoBrush(i);
        if (isRotoInput) {
            ///Copy for the roto input the output needed comps
            assert(comps->find(-1) != comps->end());
            comps->insert(std::make_pair(i, (*comps)[-1])); // note: map::at() does not exist in C++98
        } else if (input) {
            std::vector<ImageComponents> compVec;
            std::bitset<4> inputProcChannels;
            ImageComponents layer;
            bool isAll;
            bool ok = getNode()->getSelectedLayer(i, &inputProcChannels, &isAll, &layer);
            ImageComponents maskComp;
            NodePtr maskInput;
            int channelMask = getNode()->getMaskChannel(i, &maskComp, &maskInput);
            
            if (ok && !isAll) {
                if ( !layer.isColorPlane() ) {
                    compVec.push_back(layer);
                } else {
                    //Use regular clip preferences
                    ImageBitDepthEnum depth;
                    std::list<ImageComponents> components;
                    getPreferredDepthAndComponents(i, &components, &depth);
                    for (std::list<ImageComponents>::iterator it = components.begin(); it != components.end(); ++it) {
                        //if (it->isColorPlane()) {
                        compVec.push_back(*it);
                        //}
                    }
                }
            } else if ( (channelMask != -1) && (maskComp.getNumComponents() > 0) ) {
                std::vector<ImageComponents> compVec;
                compVec.push_back(maskComp);
                comps->insert( std::make_pair(i, compVec) );
            } else {
                //Use regular clip preferences
                ImageBitDepthEnum depth;
                std::list<ImageComponents> components;
                getPreferredDepthAndComponents(i, &components, &depth);
                for (std::list<ImageComponents>::iterator it = components.begin(); it != components.end(); ++it) {
                    //if (it->isColorPlane()) {
                    compVec.push_back(*it);
                    //}
                }
            }
            comps->insert( std::make_pair(i, compVec) );
        }
    }
    
} // EffectInstance::getComponentsNeededAndProduced_public

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

void
EffectInstance::onKnobValueChanged(KnobI* /*k*/,
                                   ValueChangedReasonEnum /*reason*/,
                                   double /*time*/,
                                   bool /*originatedFromMainThread*/)
{
}


bool
EffectInstance::getThreadLocalRenderedPlanes(std::map<ImageComponents, EffectInstance::PlaneToRender> *outputPlanes,
                                             ImageComponents* planeBeingRendered,
                                             RectI* renderWindow) const
{
    EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
    if (tls && tls->currentRenderArgs.validArgs) {
        assert( !tls->currentRenderArgs.outputPlanes.empty() );
        *planeBeingRendered = tls->currentRenderArgs.outputPlaneBeingRendered;
        *outputPlanes = tls->currentRenderArgs.outputPlanes;
        *renderWindow = tls->currentRenderArgs.renderWindowPixel;
        
        return true;
    }

    return false;
}

bool
EffectInstance::getThreadLocalNeededComponents(boost::shared_ptr<ComponentsNeededMap>* neededComps) const
{
    EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
    if (tls && tls->currentRenderArgs.validArgs) {
        assert( !tls->currentRenderArgs.outputPlanes.empty() );
        *neededComps = tls->currentRenderArgs.compsNeeded;
        return true;
    }
    
    return false;
}

void
EffectInstance::updateThreadLocalRenderTime(double time)
{
    if (QThread::currentThread() != qApp->thread()) {
        EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
        if (tls && tls->currentRenderArgs.validArgs) {
            tls->currentRenderArgs.time = time;
        }
    }
}

bool
EffectInstance::isDuringPaintStrokeCreationThreadLocal() const
{
    EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
    if (tls && !tls->frameArgs.empty()) {
        return tls->frameArgs.back()->isDuringPaintStrokeCreation;
    }

    return getNode()->isDuringPaintStrokeCreation();
}

void
EffectInstance::redrawOverlayInteract()
{
    if (isDoingInteractAction()) {
        getApp()->queueRedrawForAllViewers();
    } else {
        getApp()->redrawAllViewers();
    }
    
}

RenderScale
EffectInstance::getOverlayInteractRenderScale() const
{
    RenderScale r(1.);
    return r;
}

void
EffectInstance::onKnobValueChanged_public(KnobI* k,
                                          ValueChangedReasonEnum reason,
                                          double time,
                                          bool originatedFromMainThread)
{
    NodePtr node = getNode();

    ///If the param changed is a button and the node is disabled don't do anything which might
    ///trigger an analysis
    if ((reason == eValueChangedReasonUserEdited) && dynamic_cast<KnobButton*>(k) && node->isNodeDisabled()) {
        return;
    }

    if ( reason != eValueChangedReasonTimeChanged && isReader() && (k->getName() == kOfxImageEffectFileParamName) ) {
        node->computeFrameRangeForReader(k);
    }

    if (node->handleFormatKnob(k)) {
        return;
    }
    

    KnobHelper* kh = dynamic_cast<KnobHelper*>(k);
    assert(kh);
    if (kh && kh->isDeclaredByPlugin()) {
        
        /*
         For now since knobs are not view aware, use view=0 always
         */
        const int view = 0;
        
        ////We set the thread storage render args so that if the instance changed action
        ////tries to call getImage it can render with good parameters.
        boost::shared_ptr<ParallelRenderArgsSetter> setter;
        if (reason != eValueChangedReasonTimeChanged) {
            setter.reset(new ParallelRenderArgsSetter(time,
                                                      view, //view
                                                      true, // isRenderUserInteraction
                                                      false, // isSequential
                                                      false, // canAbort
                                                      0, // renderAge
                                                      node, // treeRoot
                                                      0, // request
                                                      0, //texture index
                                                      getApp()->getTimeLine().get(),
                                                      NodePtr(), // activeRotoPaintNode
                                                      true, // isAnalysis
                                                      false, // draftMode
                                                      false, // viewerProgressReportEnabled
                                                      boost::shared_ptr<RenderStats>()));
        }
        {
            RECURSIVE_ACTION();
            knobChanged(k, reason, view, time, originatedFromMainThread);
        }
        
        
    }
    
    if (kh && QThread::currentThread() == qApp->thread() &&
        originatedFromMainThread && reason != eValueChangedReasonTimeChanged) {
        
        ///Run the following only in the main-thread
        if (k->getIsClipPreferencesSlave()) {
            refreshClipPreferences_public(time, getOverlayInteractRenderScale(), reason,true, true);
        }
        if (hasOverlay() && node->shouldDrawOverlay() && !node->hasHostOverlayForParam(k)) {
            // Some plugins (e.g. by digital film tools) forget to set kOfxInteractPropSlaveToParam.
            // Most hosts trigger a redraw if the plugin has an active overlay.
            incrementRedrawNeededCounter();
            
            if (!isDequeueingValuesSet() && getRecursionLevel() == 0 && checkIfOverlayRedrawNeeded()) {
                redrawOverlayInteract();
            }
        }
    }

    node->onEffectKnobValueChanged(k, reason);

    //Don't call the python callback if the reason is time changed
    if (reason == eValueChangedReasonTimeChanged) {
        return;
    }
    
    ///If there's a knobChanged Python callback, run it
    std::string pythonCB = getNode()->getKnobChangedCallback();

    if ( !pythonCB.empty() ) {
        bool userEdited = reason == eValueChangedReasonNatronGuiEdited ||
                          reason == eValueChangedReasonUserEdited;
        _imp->runChangedParamCallback(k, userEdited, pythonCB);
    }

    ///Refresh the dynamic properties that can be changed during the instanceChanged action
    node->refreshDynamicProperties();

    ///Clear input images pointers that were stored in getImage() for the main-thread.
    ///This is safe to do so because if this is called while in render() it won't clear the input images
    ///pointers for the render thread. This is helpful for analysis effects which call getImage() on the main-thread
    ///and whose render() function is never called.
    _imp->clearInputImagePointers();
} // onKnobValueChanged_public

void
EffectInstance::clearLastRenderedImage()
{
    
}

void
EffectInstance::aboutToRestoreDefaultValues()
{
    ///Invalidate the cache by incrementing the age
    NodePtr node = getNode();

    node->incrementKnobsAge();

    if ( node->areKeyframesVisibleOnTimeline() ) {
        node->hideKeyframesFromTimeline(true);
    }
}

/**
 * @brief Returns a pointer to the first non disabled upstream node.
 * When cycling through the tree, we prefer non optional inputs and we span inputs
 * from last to first.
 **/
EffectInstPtr
EffectInstance::getNearestNonDisabled() const
{
    NodePtr node = getNode();

    if ( !node->isNodeDisabled() ) {
        return node->getEffectInstance();
    } else {
        ///Test all inputs recursively, going from last to first, preferring non optional inputs.
        std::list<EffectInstPtr> nonOptionalInputs;
        std::list<EffectInstPtr> optionalInputs;
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
                EffectInstPtr inp = getInput(i);
                if (inp) {
                    nonOptionalInputs.push_front(inp);
                    break;
                }
            } else if (inputLabel == otherName) {
                foundOther = i;
            }
        }

        if ( (foundOther != -1) && nonOptionalInputs.empty() ) {
            EffectInstPtr inp = getInput(foundOther);
            if (inp) {
                nonOptionalInputs.push_front(inp);
            }
        }

        ///If we found A or B so far, cycle through them
        for (std::list<EffectInstPtr> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
            EffectInstPtr inputRet = (*it)->getNearestNonDisabled();
            if (inputRet) {
                return inputRet;
            }
        }


        ///We cycle in reverse by default. It should be a setting of the application.
        ///In this case it will return input B instead of input A of a merge for example.
        for (int i = 0; i < maxinputs; ++i) {
            EffectInstPtr inp = getInput(i);
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
        for (std::list<EffectInstPtr> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
            EffectInstPtr inputRet = (*it)->getNearestNonDisabled();
            if (inputRet) {
                return inputRet;
            }
        }

        ///Cycle through optional inputs...
        for (std::list<EffectInstPtr> ::iterator it = optionalInputs.begin(); it != optionalInputs.end(); ++it) {
            EffectInstPtr inputRet = (*it)->getNearestNonDisabled();
            if (inputRet) {
                return inputRet;
            }
        }

        ///We didn't find anything upstream, return
        return node->getEffectInstance();
    }
} // EffectInstance::getNearestNonDisabled

EffectInstPtr
EffectInstance::getNearestNonDisabledPrevious(int* inputNb)
{
    assert( getNode()->isNodeDisabled() );

    ///Test all inputs recursively, going from last to first, preferring non optional inputs.
    std::list<EffectInstPtr> nonOptionalInputs;
    std::list<EffectInstPtr> optionalInputs;
    int localPreferredInput = -1;
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
            EffectInstPtr inp = getInput(i);
            if (inp) {
                nonOptionalInputs.push_front(inp);
                localPreferredInput = i;
                break;
            }
        } else if (inputLabel == otherName) {
            foundOther = i;
        }
    }

    if ( (foundOther != -1) && nonOptionalInputs.empty() ) {
        EffectInstPtr inp = getInput(foundOther);
        if (inp) {
            nonOptionalInputs.push_front(inp);
            localPreferredInput = foundOther;
        }
    }

    ///If we found A or B so far, cycle through them
    for (std::list<EffectInstPtr> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
        if ( (*it)->getNode()->isNodeDisabled() ) {
            EffectInstPtr inputRet = (*it)->getNearestNonDisabledPrevious(inputNb);
            if (inputRet) {
                return inputRet;
            }
        }
    }


    ///We cycle in reverse by default. It should be a setting of the application.
    ///In this case it will return input B instead of input A of a merge for example.
    for (int i = 0; i < maxinputs; ++i) {
        EffectInstPtr inp = getInput(i);
        bool optional = isInputOptional(i);
        if (inp) {
            if (optional) {
                if (localPreferredInput == -1) {
                    localPreferredInput = i;
                }
                optionalInputs.push_back(inp);
            } else {
                if (localPreferredInput == -1) {
                    localPreferredInput = i;
                }
                nonOptionalInputs.push_back(inp);
            }
        }
    }


    ///Cycle through all non optional inputs first
    for (std::list<EffectInstPtr> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
        if ( (*it)->getNode()->isNodeDisabled() ) {
            EffectInstPtr inputRet = (*it)->getNearestNonDisabledPrevious(inputNb);
            if (inputRet) {
                return inputRet;
            }
        }
    }

    ///Cycle through optional inputs...
    for (std::list<EffectInstPtr> ::iterator it = optionalInputs.begin(); it != optionalInputs.end(); ++it) {
        if ( (*it)->getNode()->isNodeDisabled() ) {
            EffectInstPtr inputRet = (*it)->getNearestNonDisabledPrevious(inputNb);
            if (inputRet) {
                return inputRet;
            }
        }
    }

    *inputNb = localPreferredInput;

    return shared_from_this();
} // EffectInstance::getNearestNonDisabledPrevious

EffectInstPtr
EffectInstance::getNearestNonIdentity(double time)
{
    U64 hash = getRenderHash();
    RenderScale scale(1.);

    RectD rod;
    bool isProjectFormat;
    StatusEnum stat = getRegionOfDefinition_public(hash, time, scale, 0, &rod, &isProjectFormat);

    ///Ignore the result of getRoD if it failed
    Q_UNUSED(stat);

    double inputTimeIdentity;
    int inputNbIdentity;
    RectI pixelRoi;
    rod.toPixelEnclosing(scale, getPreferredAspectRatio(), &pixelRoi);
    if ( !isIdentity_public(true, hash, time, scale, pixelRoi, 0, &inputTimeIdentity, &inputNbIdentity) ) {
        return shared_from_this();
    } else {
        if (inputNbIdentity < 0) {
            return shared_from_this();
        }
        EffectInstPtr effect = getInput(inputNbIdentity);

        return effect ? effect->getNearestNonIdentity(time) : shared_from_this();
    }
}


void
EffectInstance::onNodeHashChanged(U64 hash)
{
    ///Invalidate actions cache
    _imp->actionsCache.invalidateAll(hash);

    const KnobsVec & knobs = getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        for (int i = 0; i < (*it)->getDimension(); ++i) {
            (*it)->clearExpressionsResults(i);
        }
    }
}

bool
EffectInstance::canSetValue() const
{
    return !getNode()->isNodeRendering() || appPTR->isBackground();
}

void
EffectInstance::abortAnyEvaluation()
{
    /*
     Get recursively downstream all Output nodes and abort any render on them
     If an output node such as a viewer was doing playback, enable it to restart 
     automatically playback when the abort finished
     */
    NodePtr node = getNode();
    assert(node);
    std::list<OutputEffectInstance*> outputNodes;
    
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(this);
    if (isGroup) {
        NodesList inputOutputs;
        isGroup->getInputsOutputs(&inputOutputs, false);
        for (NodesList::iterator it = inputOutputs.begin(); it!=inputOutputs.end();++it) {
            (*it)->hasOutputNodesConnected(&outputNodes);
        }
    } else {
        boost::shared_ptr<RotoDrawableItem> attachedStroke = getNode()->getAttachedRotoItem();
        if (attachedStroke) {
            ///For nodes internal to the rotopaint tree, check outputs of the rotopaint node instead
            attachedStroke->getContext()->getNode()->hasOutputNodesConnected(&outputNodes);
        } else {
            node->hasOutputNodesConnected(&outputNodes);
        }
    }
    for (std::list<OutputEffectInstance*>::const_iterator it = outputNodes.begin(); it != outputNodes.end(); ++it) {
        //Abort and allow playback to restart but do not block, when this function returns any ongoing render may very
        //well not be finished
        (*it)->getRenderEngine()->abortRendering(true,false);
    }
}

double
EffectInstance::getCurrentTime() const
{
    EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
    if (!tls) {
        return getApp()->getTimeLine()->currentFrame();
    }
    if (tls->currentRenderArgs.validArgs) {
        return tls->currentRenderArgs.time;
    }
    

    if (!tls->frameArgs.empty()) {
        return tls->frameArgs.back()->time;
    }
    return getApp()->getTimeLine()->currentFrame();
}

int
EffectInstance::getCurrentView() const
{
    EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
    if (!tls || !tls->currentRenderArgs.validArgs) {
        return 0;
    }
    return tls->currentRenderArgs.view;
    
}

SequenceTime
EffectInstance::getFrameRenderArgsCurrentTime() const
{
    EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
    if (!tls || tls->frameArgs.empty()) {
        return getApp()->getTimeLine()->currentFrame();
    }

    return tls->frameArgs.back()->time;
}

int
EffectInstance::getFrameRenderArgsCurrentView() const
{
    EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
    if (!tls || tls->frameArgs.empty()) {
        return 0;
    }
    
    return tls->frameArgs.back()->view;
}

#ifdef DEBUG
void
EffectInstance::checkCanSetValueAndWarn() const
{
    if ( !checkCanSetValue() ) {
        qDebug() << getScriptName_mt_safe().c_str() << ": setValue()/setValueAtTime() was called during an action that is not allowed to call this function.";
    }
}

#endif

static
void
isFrameVaryingOrAnimated_impl(const EffectInstance* node,
                              bool *ret)
{
    if ( node->isFrameVarying() || node->getHasAnimation() || node->getNode()->getRotoContext() ) {
        *ret = true;
    } else {
        int maxInputs = node->getMaxInputCount();
        for (int i = 0; i < maxInputs; ++i) {
            EffectInstPtr input = node->getInput(i);
            if (input) {
                isFrameVaryingOrAnimated_impl(input.get(), ret);
                if (*ret) {
                    return;
                }
            }
        }
    }
}

bool
EffectInstance::isFrameVaryingOrAnimated_Recursive() const
{
    bool ret = false;

    isFrameVaryingOrAnimated_impl(this, &ret);

    return ret;
}

bool
EffectInstance::isPaintingOverItselfEnabled() const
{
    return isDuringPaintStrokeCreationThreadLocal();
}

void
EffectInstance::getPreferredDepthAndComponents(int inputNb,
                                               std::list<ImageComponents>* comp,
                                               ImageBitDepthEnum* depth) const
{

    if (inputNb != -1) {
        EffectInstPtr inp = getInput(inputNb);
        if (inp) {
            inp->getPreferredDepthAndComponents(-1, comp, depth);
        }
    } else {
        QMutexLocker k(&_imp->defaultClipPreferencesDataMutex);
        *comp = _imp->clipPrefsData.comps;
        *depth = _imp->clipPrefsData.bitdepth;
    }
    if ( comp->empty() ) {
        comp->push_back( ImageComponents::getNoneComponents() );
    }
   
}

double
EffectInstance::getPreferredFrameRate() const
{
    QMutexLocker k(&_imp->defaultClipPreferencesDataMutex);
    return _imp->clipPrefsData.frameRate;
}

double
EffectInstance::getPreferredAspectRatio() const
{
    QMutexLocker k(&_imp->defaultClipPreferencesDataMutex);
    return _imp->clipPrefsData.pixelAspectRatio;
}

ImagePremultiplicationEnum
EffectInstance::getOutputPremultiplication() const
{
    QMutexLocker k(&_imp->defaultClipPreferencesDataMutex);
    return _imp->clipPrefsData.outputPremult;
}

void
EffectInstance::refreshClipPreferences_recursive(double time,
                                                  const RenderScale & scale,
                                                 ValueChangedReasonEnum reason,
                                                  bool forceGetClipPrefAction,
                                                  std::list<Node*> & markedNodes)
{
    NodePtr node = getNode();
    std::list<Node*>::iterator found = std::find( markedNodes.begin(), markedNodes.end(), node.get() );

    if ( found != markedNodes.end() ) {
        return;
    }


    refreshClipPreferences(time, scale, reason, forceGetClipPrefAction);
    node->refreshIdentityState();

    if ( !node->duringInputChangedAction() ) {
        ///The channels selector refreshing is already taken care of in the inputChanged action
        node->refreshChannelSelectors();
    }

    markedNodes.push_back( node.get() );

    NodesList  outputs;
    node->getOutputsWithGroupRedirection(outputs);
    for (NodesList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        (*it)->getEffectInstance()->refreshClipPreferences_recursive(time, scale, reason, forceGetClipPrefAction, markedNodes);
    }
}

static void setComponentsDirty_recursive(const Node* node, std::list<const Node*> & markedNodes)
{
    std::list<const Node*>::iterator found = std::find( markedNodes.begin(), markedNodes.end(), node );
    
    if ( found != markedNodes.end() ) {
        return;
    }
    
    markedNodes.push_back(node);
    
    node->getEffectInstance()->setComponentsAvailableDirty(true);
    
    
    NodesList  outputs;
    node->getOutputsWithGroupRedirection(outputs);
    for (NodesList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        setComponentsDirty_recursive(it->get(),markedNodes);
    }
    
}

void
EffectInstance::refreshClipPreferences_public(double time,
                                               const RenderScale & scale,
                                                ValueChangedReasonEnum reason,
                                               bool forceGetClipPrefAction,
                                               bool recurse)
{
    assert( QThread::currentThread() == qApp->thread() );

    if (recurse) {
        {
            std::list<const Node*> markedNodes;
            setComponentsDirty_recursive(_node.lock().get(),markedNodes);
        }
        {
            std::list<Node*> markedNodes;
            refreshClipPreferences_recursive(time, scale, reason, forceGetClipPrefAction, markedNodes);
        }
    } else {
        refreshClipPreferences(time, scale, reason, forceGetClipPrefAction);
    }
}

bool
EffectInstance::refreshClipPreferences(double /*time*/,
                                        const RenderScale & /*scale*/,
                                       ValueChangedReasonEnum /*reason*/,
                                        bool forceGetClipPrefAction)
{

    if (forceGetClipPrefAction) {
        EffectInstPtr input = getNearestNonDisabled();
        if (input.get() == this) {
            int prefInput = getNode()->getPreferredInput();
            if (prefInput != -1) {
                input = getInput(prefInput);
            }
        }
        if (!input) {
            QMutexLocker k(&_imp->defaultClipPreferencesDataMutex);
            _imp->clipPrefsData.outputPremult = eImagePremultiplicationPremultiplied;
            _imp->clipPrefsData.frameRate = getApp()->getProjectFrameRate();
            _imp->clipPrefsData.pixelAspectRatio = 1.;
            _imp->clipPrefsData.comps.clear();
            _imp->clipPrefsData.bitdepth = getBestSupportedBitDepth();
        } else {
            EffectInstance::DefaultClipPreferencesData data;
            data.outputPremult = input->getOutputPremultiplication();
            data.frameRate = input->getPreferredFrameRate();
            data.pixelAspectRatio = input->getPreferredAspectRatio();
            input->getPreferredDepthAndComponents(-1, &data.comps, &data.bitdepth);
            
            for (std::list<ImageComponents>::iterator it = data.comps.begin(); it != data.comps.end(); ++it) {
                *it = findClosestSupportedComponents(-1, *it);
            }
            
            
            ///find deepest bitdepth
            data.bitdepth = getNode()->getClosestSupportedBitDepth(data.bitdepth);
            
            {
                QMutexLocker k(&_imp->defaultClipPreferencesDataMutex);
                _imp->clipPrefsData = data;
            }
        }
        return true;
    }
    return false;
}

void
EffectInstance::refreshExtraStateAfterTimeChanged(double time)
{
    KnobHolder::refreshExtraStateAfterTimeChanged(time);
    getNode()->refreshIdentityState();
}


void
EffectInstance::assertActionIsNotRecursive() const
{
# ifdef DEBUG
    ///Only check recursions which are on a render threads, because we do authorize recursions in getRegionOfDefinition and such
    if (QThread::currentThread() != qApp->thread()) {
        int recursionLvl = getRecursionLevel();
        if ( getApp() && getApp()->isShowingDialog() ) {
            return;
        }
        if (recursionLvl != 0) {
            qDebug() << "A non-recursive action has been called recursively.";
        }
    }
# endif // DEBUG
}


void
EffectInstance::incrementRecursionLevel()
{
    EffectDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
    assert(tls);
    ++tls->actionRecursionLevel;
}

void
EffectInstance::decrementRecursionLevel()
{
    EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
    assert(tls);
    --tls->actionRecursionLevel;
}

int
EffectInstance::getRecursionLevel() const
{
    EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
    if (!tls) {
        return 0;
    }
    return tls->actionRecursionLevel;
}

NATRON_NAMESPACE_EXIT;

