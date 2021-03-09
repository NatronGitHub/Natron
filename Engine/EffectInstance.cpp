/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <sstream> // stringstream
#include <algorithm> // min, max
#include <fstream>
#include <bitset>
#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/make_shared.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include <QtCore/QReadWriteLock>
#include <QtCore/QCoreApplication>
#include <QtConcurrentMap> // QtCore on Qt4, QtConcurrent on Qt5
#include <QtConcurrentRun> // QtCore on Qt4, QtConcurrent on Qt5

#include "Global/QtCompat.h"

#include "Engine/AbortableRenderInfo.h"
#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/BlockingBackgroundRender.h"
#include "Engine/DiskCacheNode.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/Log.h"
#include "Engine/MemoryInfo.h" // printAsRAM
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxOverlayInteract.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/GPUContextPool.h"
#include "Engine/OSGLContext.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/PluginMemory.h"
#include "Engine/Project.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/ReadNode.h"
#include "Engine/Settings.h"
#include "Engine/Timer.h"
#include "Engine/Transform.h"
#include "Engine/UndoCommand.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

//#define NATRON_ALWAYS_ALLOCATE_FULL_IMAGE_BOUNDS


NATRON_NAMESPACE_ENTER


class KnobFile;
class KnobOutputFile;


void
EffectInstance::addThreadLocalInputImageTempPointer(int inputNb,
                                                    const ImagePtr & img)
{
    _imp->addInputImageTempPointer(inputNb, img);
}

EffectInstance::EffectInstance(NodePtr node)
    : NamedKnobHolder( node ? node->getApp() : AppInstancePtr() )
    , _node(node)
    , _imp( new Implementation(this) )
{
    if (node) {
        if ( !node->isRenderScaleSupportEnabledForPlugin() ) {
            setSupportsRenderScaleMaybe(eSupportsNo);
        }
    }
}

EffectInstance::EffectInstance(const EffectInstance& other)
    : NamedKnobHolder(other)
    , LockManagerI<Image>()
    , boost::enable_shared_from_this<EffectInstance>()
    , _node( other.getNode() )
    , _imp( new Implementation(*other._imp) )
{
    _imp->_publicInterface = this;
}

EffectInstance::~EffectInstance()
{
}

void
EffectInstance::lock(const ImagePtr & entry)
{
    NodePtr n = _node.lock();

    n->lock(entry);
}

bool
EffectInstance::tryLock(const ImagePtr & entry)
{
    NodePtr n = _node.lock();

    return n->tryLock(entry);
}

void
EffectInstance::unlock(const ImagePtr & entry)
{
    NodePtr n = _node.lock();

    n->unlock(entry);
}


/**
 * @brief Add the layers from the inputList to the toList if they do not already exist in the list.
 * For the color plane, if it already existed in toList it is replaced by the value in inputList
 **/
static void mergeLayersList(const std::list<ImagePlaneDesc>& inputList,
                            std::list<ImagePlaneDesc>* toList)
{
    for (std::list<ImagePlaneDesc>::const_iterator it = inputList.begin(); it != inputList.end(); ++it) {

        std::list<ImagePlaneDesc>::iterator foundMatch = ImagePlaneDesc::findEquivalentLayer(*it, toList->begin(), toList->end());

        // If we found the color plane, replace it by this color plane which may have changed (e.g: input was Color.RGB but this node Color.RGBA)
        if (foundMatch != toList->end()) {
            toList->erase(foundMatch);
        }
        toList->push_back(*it);

    } // for each input components
} // mergeLayersList

/**
 * @brief Remove any layer from the toRemove list from toList.
 **/
static void removeFromLayersList(const std::list<ImagePlaneDesc>& toRemove,
                                 std::list<ImagePlaneDesc>* toList)
{
    for (std::list<ImagePlaneDesc>::const_iterator it = toRemove.begin(); it != toRemove.end(); ++it) {
        std::list<ImagePlaneDesc>::iterator foundMatch = ImagePlaneDesc::findEquivalentLayer<std::list<ImagePlaneDesc>::iterator>(*it, toList->begin(), toList->end());
        if (foundMatch != toList->end()) {
            toList->erase(foundMatch);
        }
    } // for each input components

} // removeFromLayersList


const std::vector<std::string>&
EffectInstance::getUserPlanes() const
{
    EffectTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
    assert(tls);
    tls->userPlaneStrings.clear();

    std::list<ImagePlaneDesc> projectLayers = getApp()->getProject()->getProjectDefaultLayers();


    std::list<ImagePlaneDesc> userCreatedLayers;
    getNode()->getUserCreatedComponents(&userCreatedLayers);
    mergeLayersList(userCreatedLayers, &projectLayers);


    for (std::list<ImagePlaneDesc>::iterator it = projectLayers.begin(); it != projectLayers.end(); ++it) {
        std::string ofxPlane = ImagePlaneDesc::mapPlaneToOFXPlaneString(*it);
        tls->userPlaneStrings.push_back(ofxPlane);
    }
    return tls->userPlaneStrings;
}

void
EffectInstance::clearPluginMemoryChunks()
{
    // This will remove the mem from the pluginMemoryChunks list
    QMutexLocker l(&_imp->pluginMemoryChunksMutex);
    for (std::list<PluginMemoryWPtr>::iterator it = _imp->pluginMemoryChunks.begin(); it!=_imp->pluginMemoryChunks.end(); ++it) {
        PluginMemoryPtr mem = it->lock();
        if (!mem) {
            continue;
        }
        mem->setUnregisterOnDestructor(false);
    }
    _imp->pluginMemoryChunks.clear();
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
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();

    assert(tls);
    assert( !tls->canSetValue.empty() );
    tls->canSetValue.pop_back();
}

bool
EffectInstance::isDuringActionThatCanSetValue() const
{
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();

    if (!tls) {
        return true;
    }
    if ( tls->canSetValue.empty() ) {
        return true;
    }

    return tls->canSetValue.back();
}

#endif //DEBUG

void
EffectInstance::setNodeRequestThreadLocal(const NodeFrameRequestPtr & nodeRequest)
{
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();

    if (!tls) {
        assert(false);

        return;
    }
    std::list<ParallelRenderArgsPtr>& argsList = tls->frameArgs;
    if ( argsList.empty() ) {
        return;
    }
    argsList.back()->request = nodeRequest;
}

void
EffectInstance::setParallelRenderArgsTLS(double time,
                                         ViewIdx view,
                                         bool isRenderUserInteraction,
                                         bool isSequential,
                                         U64 nodeHash,
                                         const AbortableRenderInfoPtr& abortInfo,
                                         const NodePtr & treeRoot,
                                         int visitsCount,
                                         const NodeFrameRequestPtr & nodeRequest,
                                         const OSGLContextPtr& glContext,
                                         int textureIndex,
                                         const TimeLine* timeline,
                                         bool isAnalysis,
                                         bool isDuringPaintStrokeCreation,
                                         const NodesList & rotoPaintNodes,
                                         RenderSafetyEnum currentThreadSafety,
                                         PluginOpenGLRenderSupport currentOpenGLSupport,
                                         bool doNanHandling,
                                         bool draftMode,
                                         const RenderStatsPtr & stats)
{
    EffectTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
    std::list<ParallelRenderArgsPtr>& argsList = tls->frameArgs;
    ParallelRenderArgsPtr args = boost::make_shared<ParallelRenderArgs>();

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
    assert(abortInfo);
    args->abortInfo = abortInfo;
    args->treeRoot = treeRoot;
    args->visitsCount = visitsCount;
    args->textureIndex = textureIndex;
    args->isAnalysis = isAnalysis;
    args->isDuringPaintStrokeCreation = isDuringPaintStrokeCreation;
    args->currentThreadSafety = currentThreadSafety;
    args->currentOpenglSupport = currentOpenGLSupport;
    args->rotoPaintNodes = rotoPaintNodes;
    args->doNansHandling = isAnalysis ? false : doNanHandling;
    args->draftMode = draftMode;
    args->tilesSupported = getNode()->getCurrentSupportTiles();
    args->stats = stats;
    args->openGLContext = glContext;
    argsList.push_back(args);
}

bool
EffectInstance::getThreadLocalRotoPaintTreeNodes(NodesList* nodes) const
{
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();

    if (!tls) {
        return false;
    }
    if ( tls->frameArgs.empty() ) {
        return false;
    }
    *nodes = tls->frameArgs.back()->rotoPaintNodes;

    return true;
}

void
EffectInstance::setDuringPaintStrokeCreationThreadLocal(bool duringPaintStroke)
{
    EffectTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();

    tls->frameArgs.back()->isDuringPaintStrokeCreation = duringPaintStroke;
}

void
EffectInstance::setParallelRenderArgsTLS(const ParallelRenderArgsPtr & args)
{
    EffectTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();

    assert( args->abortInfo.lock() );
    tls->frameArgs.push_back(args);
}

void
EffectInstance::invalidateParallelRenderArgsTLS()
{
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();

    if (!tls) {
        return;
    }

    assert( !tls->frameArgs.empty() );
    const ParallelRenderArgsPtr& back = tls->frameArgs.back();
    for (NodesList::iterator it = back->rotoPaintNodes.begin(); it != back->rotoPaintNodes.end(); ++it) {
        (*it)->getEffectInstance()->invalidateParallelRenderArgsTLS();
    }
    tls->frameArgs.pop_back();
}

ParallelRenderArgsPtr
EffectInstance::getParallelRenderArgsTLS() const
{
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();

    if ( !tls || tls->frameArgs.empty() ) {
        return ParallelRenderArgsPtr();
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
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();

    if ( !tls || tls->frameArgs.empty() ) {
        //No tls: get the GUI hash
        return getHash();
    }

    const ParallelRenderArgsPtr &args = tls->frameArgs.back();

    if (args->request) {
        //A request pass was made, Hash for this thread was already computed, use it
        return args->request->nodeHash;
    }

    //Use the hash that was computed when we set the ParallelRenderArgs TLS
    return args->nodeHash;
}

bool
EffectInstance::Implementation::aborted(bool isRenderResponseToUserInteraction,
                                        const AbortableRenderInfoPtr& abortInfo,
                                        const EffectInstancePtr& treeRoot)
{
    if (!isRenderResponseToUserInteraction) {
        // Rendering is playback or render on disk

        // If we have abort info, e just peek the atomic int inside the abort info, this is very fast
        if ( abortInfo && abortInfo->isAborted() ) {
            return true;
        }

        // Fallback on the flag set on the node that requested the render in OutputSchedulerThread
        if (treeRoot) {
            OutputEffectInstance* effect = dynamic_cast<OutputEffectInstance*>( treeRoot.get() );
            assert(effect);
            if (effect) {
                return effect->isSequentialRenderBeingAborted();
            }
        }

        // We have no other means to know if abort was called
        return false;
    } else {
        // This is a render issued to refresh the image on the Viewer

        if ( !abortInfo || !abortInfo->canAbort() ) {
            // We do not have any abortInfo set or this render is not abortable. This should be avoided as much as possible!
            return false;
        }

        // This is very fast, we just peek the atomic int inside the abort info
        if ( (int)abortInfo->isAborted() ) {
            return true;
        }

        // If this node can start sequential renders (e.g: start playback like on the viewer or render on disk) and it is already doing a sequential render, abort
        // this render
        OutputEffectInstance* isRenderEffect = dynamic_cast<OutputEffectInstance*>( treeRoot.get() );
        if (isRenderEffect) {
            if ( isRenderEffect->isDoingSequentialRender() ) {
                return true;
            }
        }

        // The render was not aborted
        return false;
    }
}

bool
EffectInstance::aborted() const
{
    QThread* thisThread = QThread::currentThread();

    /* If this thread is an AbortableThread, this function will be extremely fast*/
    AbortableThread* isAbortableThread = dynamic_cast<AbortableThread*>(thisThread);

    /**
       The solution here is to store per-render info on the thread that we retrieve.
       These info contain an atomic integer determining whether this particular render was aborted or not.
       If this thread does not have abort info yet on it, we retrieve them from the thread-local storage of this node
       and set it.
       Threads that start a render generally already have the AbortableThread::setAbortInfo function called on them, but
       threads spawned from the thread pool may not.
     **/
    bool isRenderUserInteraction;
    AbortableRenderInfoPtr abortInfo;
    EffectInstancePtr treeRoot;


    if ( !isAbortableThread || !isAbortableThread->getAbortInfo(&isRenderUserInteraction, &abortInfo, &treeRoot) ) {
        // If this thread is not abortable or we did not set the abort info for this render yet, retrieve them from the TLS of this node.
        EffectTLSDataPtr tls = _imp->tlsData->getTLSData();
        if (!tls) {
            return false;
        }
        if ( tls->frameArgs.empty() ) {
            return false;
        }
        const ParallelRenderArgsPtr & args = tls->frameArgs.back();
        isRenderUserInteraction = args->isRenderResponseToUserInteraction;
        abortInfo = args->abortInfo.lock();
        if (args->treeRoot) {
            treeRoot = args->treeRoot->getEffectInstance();
        }

        if (isAbortableThread) {
            isAbortableThread->setAbortInfo(isRenderUserInteraction, abortInfo, treeRoot);
        }
    }

    // The internal function that given a AbortableRenderInfoPtr determines if a render was aborted or not
    return Implementation::aborted(isRenderUserInteraction,
                                   abortInfo,
                                   treeRoot);
} // EffectInstance::aborted

bool
EffectInstance::shouldCacheOutput(bool isFrameVaryingOrAnimated,
                                  double time,
                                  ViewIdx view,
                                  int visitsCount) const
{
    NodePtr n = _node.lock();

    return n->shouldCacheOutput(isFrameVaryingOrAnimated, time, view, visitsCount);
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
EffectInstance::retrieveGetImageDataUponFailure(const double time,
                                                const ViewIdx view,
                                                const RenderScale & scale,
                                                const RectD* optionalBoundsParam,
                                                U64* nodeHash_p,
                                                bool* isIdentity_p,
                                                EffectInstancePtr* identityInput_p,
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
        getRegionsOfInterest(time, scale, optionalBounds, optionalBounds, ViewIdx(0), inputRois_p);
    }

    assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(scale.x == 1. && scale.y == 1.) ) );
    RectI pixelRod;
    rod.toPixelEnclosing(scale, getAspectRatio(-1), &pixelRod);
    try {
        int identityInputNb;
        double identityTime;
        ViewIdx identityView;
        *isIdentity_p = isIdentity_public(true, nodeHash, time, scale, pixelRod, view, &identityTime, &identityView, &identityInputNb);
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
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();

    if (!tls) {
        return;
    }
    *images = tls->currentRenderArgs.inputImages;
}

bool
EffectInstance::getThreadLocalRegionsOfInterests(RoIMap & roiMap) const
{
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();

    if (!tls) {
        return false;
    }
    roiMap = tls->currentRenderArgs.regionOfInterestResults;

    return true;
}

OSGLContextPtr
EffectInstance::getThreadLocalOpenGLContext() const
{
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();

    if ( !tls || tls->frameArgs.empty() ) {
        return OSGLContextPtr();
    }

    return tls->frameArgs.back()->openGLContext.lock();
}

ImagePtr
EffectInstance::getImage(int inputNb,
                         const double time,
                         const RenderScale & scale,
                         const ViewIdx view,
                         const RectD *optionalBoundsParam, //!< optional region in canonical coordinates
                         const ImagePlaneDesc* layer,
                         const bool mapToClipPrefs,
                         const bool dontUpscale,
                         const StorageModeEnum returnStorage,
                         const ImageBitDepthEnum* /*textureDepth*/, // < ignore requested texture depth because internally we use 32bit fp textures, so we offer the highest possible quality anyway.
                         RectI* roiPixel,
                         Transform::Matrix3x3Ptr* transform)
{
    if (time != time) {
        // time is NaN
        return ImagePtr();
    }

    ///The input we want the image from
    EffectInstancePtr inputEffect;

    //Check for transform redirections
    InputMatrixMapPtr transformRedirections;
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();
    if (tls && tls->currentRenderArgs.validArgs) {
        transformRedirections = tls->currentRenderArgs.transformRedirections;
        if (transformRedirections) {
            InputMatrixMap::const_iterator foundRedirection = transformRedirections->find(inputNb);
            if ( ( foundRedirection != transformRedirections->end() ) && foundRedirection->second.newInputEffect ) {
                inputEffect = foundRedirection->second.newInputEffect->getInput(foundRedirection->second.newInputNbToFetchFrom);
                if (transform) {
                    *transform = foundRedirection->second.cat;
                }
            }
        }
    }

    NodePtr node = getNode();

    if (!inputEffect) {
        inputEffect = getInput(inputNb);
    }

    ///Is this input a mask or not
    bool isMask = isInputMask(inputNb);

    ///If the input is a mask, this is the channel index in the layer of the mask channel
    int channelForMask = -1;

    ///Is this node a roto node or not. If so, find out if this input is the roto-brush
    RotoContextPtr roto;
    RotoDrawableItemPtr attachedStroke = node->getAttachedRotoItem();

    if (attachedStroke) {
        roto = attachedStroke->getContext();
    }
    bool useRotoInput = false;
    bool inputIsRotoBrush = roto && isInputRotoBrush(inputNb);
    bool supportsOnlyAlpha = node->isInputOnlyAlpha(inputNb);
    if (roto) {
        useRotoInput = isMask || inputIsRotoBrush;
    }

    ///This is the actual layer that we are fetching in input
    ImagePlaneDesc maskComps;
    if ( !isMaskEnabled(inputNb) ) {
        return ImagePtr();
    }

    ///If this is a mask, fetch the image from the effect indicated by the mask channel
    if (isMask || supportsOnlyAlpha) {
        if (!useRotoInput) {
            std::list<ImagePlaneDesc> availableLayers;
            getAvailableLayers(time, view, inputNb, &availableLayers);

            channelForMask = getMaskChannel(inputNb, availableLayers, &maskComps);
        } else {
            channelForMask = 3; // default to alpha channel
            maskComps = ImagePlaneDesc::getAlphaComponents();
        }
    }

    //Invalid mask
    if ( isMask && ( (channelForMask == -1) || (maskComps.getNumComponents() == 0) ) ) {
        return ImagePtr();
    }


    if ( ( !roto || (roto && !useRotoInput) ) && !inputEffect ) {
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
    bool isIdentity = false;
    EffectInstancePtr identityInput;
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
    bool isAnalysisPass = false;
    RectD thisRod;
    double thisEffectRenderTime = time;

    ///Try to find in the input images thread-local storage if we already pre-computed the image
    EffectInstance::InputImagesMap inputImagesThreadLocal;
    OSGLContextPtr glContext;
    AbortableRenderInfoPtr renderInfo;
    if ( !tls || ( !tls->currentRenderArgs.validArgs && tls->frameArgs.empty() ) ) {
        /*
           This is either a huge bug or an unknown thread that called clipGetImage from the OpenFX plug-in.
           Make-up some reasonable arguments
         */
        if ( !retrieveGetImageDataUponFailure(time, view, scale, optionalBoundsParam, &nodeHash, &isIdentity, &identityInput, &duringPaintStroke, &thisRod, &inputsRoI, &optionalBounds) ) {
            return ImagePtr();
        }
    } else {
        assert( tls->currentRenderArgs.validArgs || !tls->frameArgs.empty() );

        if (inputEffect) {
            //When analysing we do not compute a request pass so we do not enter this condition
            ParallelRenderArgsPtr inputFrameArgs = inputEffect->getParallelRenderArgsTLS();
            const FrameViewRequest* request = 0;
            if (inputFrameArgs && inputFrameArgs->request) {
                request = inputFrameArgs->request->getFrameViewRequest(time, view);
            }
            if (request) {
                roiWasInRequestPass = true;
                roi = request->finalData.finalRoi;
            }
        }

        if ( !tls->frameArgs.empty() ) {
            const ParallelRenderArgsPtr& frameRenderArgs = tls->frameArgs.back();
            nodeHash = frameRenderArgs->nodeHash;
            duringPaintStroke = frameRenderArgs->isDuringPaintStrokeCreation;
            isAnalysisPass = frameRenderArgs->isAnalysis;
            glContext = frameRenderArgs->openGLContext.lock();
            renderInfo = frameRenderArgs->abortInfo.lock();
        } else {
            //This is a bug, when entering here, frameArgs TLS should always have been set, except for unknown threads.
            nodeHash = getHash();
            duringPaintStroke = false;
        }
        if (tls->currentRenderArgs.validArgs) {
            //This will only be valid for render pass, not analysis
            const RenderArgs& renderArgs = tls->currentRenderArgs;
            if (!roiWasInRequestPass) {
                inputsRoI = renderArgs.regionOfInterestResults;
            }
            thisEffectRenderTime = renderArgs.time;
            isIdentity = renderArgs.isIdentity;
            identityInput = renderArgs.identityInput;
            inputImagesThreadLocal = renderArgs.inputImages;
            thisRod = renderArgs.rod;
        }
    }

    if ( (!glContext || !renderInfo) && returnStorage == eStorageModeGLTex ) {
        qDebug() << "[BUG]: " << getScriptName_mt_safe().c_str() << "is doing an OpenGL render but no context is bound to the current render.";

        return ImagePtr();
    }



    RectD inputRoD;
    bool inputRoDSet = false;
    if (optionalBoundsParam) {
        //Set the RoI from the parameters given to clipGetImage
        roi = optionalBounds;
    } else if (!roiWasInRequestPass) {
        //We did not have a request pass, use if possible the result of getRegionsOfInterest found in the TLS
        //If not, fallback on input RoD
        EffectInstancePtr inputToFind;
        if (useRotoInput) {
            if ( node->getRotoContext() ) {
                inputToFind = shared_from_this();
            } else {
                assert(attachedStroke);
                inputToFind = attachedStroke->getContext()->getNode()->getEffectInstance();
            }
        } else {
            inputToFind = inputEffect;
        }
        RoIMap::iterator found = inputsRoI.find(inputToFind);
        if ( found != inputsRoI.end() ) {
            ///RoI is in canonical coordinates since the results of getRegionsOfInterest is in canonical coords.
            roi = found->second;
        } else {
            ///Oops, we didn't find the roi in the thread-storage... use  the RoD instead...
            if (inputEffect && !isAnalysisPass) {
                qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "[Bug] RoI not found in TLS...falling back on RoD when calling getImage() on" <<
                    inputEffect->getScriptName_mt_safe().c_str();
            }


            //We are either in analysis or in an unknown thread
            //do not set identity flags, request for RoI the full RoD of the input
            if (useRotoInput) {
                assert( !thisRod.isNull() );
                roi = thisRod;
            } else {
                if (inputEffect) {
                    StatusEnum stat = inputEffect->getRegionOfDefinition_public(inputEffect->getRenderHash(), time, scale, view, &inputRoD, 0);
                    if (stat != eStatusFailed) {
                        inputRoDSet = true;
                    }
                }

                roi = inputRoD;
            }
        }
    }

    if ( roi.isNull() ) {
        return ImagePtr();
    }


    if (isIdentity) {
        assert(identityInput.get() != this);
        ///If the effect is an identity but it didn't ask for the effect's image of which it is identity
        ///return a null image (only when non analysis)
        if ( (identityInput != inputEffect) && !isAnalysisPass ) {
            return ImagePtr();
        }
    }


    ///Does this node supports images at a scale different than 1
    bool renderFullScaleThenDownscale = (!supportsRenderScale() && mipMapLevel != 0 && returnStorage == eStorageModeRAM);

    ///Do we want to render the graph upstream at scale 1 or at the requested render scale ? (user setting)
    bool renderScaleOneUpstreamIfRenderScaleSupportDisabled = false;
    unsigned int renderMappedMipMapLevel = mipMapLevel;
    if (renderFullScaleThenDownscale) {
        renderScaleOneUpstreamIfRenderScaleSupportDisabled = node->useScaleOneImagesWhenRenderScaleSupportIsDisabled();
        if (renderScaleOneUpstreamIfRenderScaleSupportDisabled) {
            renderMappedMipMapLevel = 0;
        }
    }

    ///Both the result of getRegionsOfInterest and optionalBounds are in canonical coordinates, we have to convert in both cases
    ///Convert to pixel coordinates
    const double par = getAspectRatio(inputNb);
    ImageBitDepthEnum depth = getBitDepth(inputNb);
    ImagePlaneDesc components;
    ImagePlaneDesc clipPrefComps, clipPrefMappedComps;
    getMetadataComponents(inputNb, &clipPrefComps, &clipPrefMappedComps);

    if (layer) {
        components = *layer;
    } else {
        components = clipPrefComps;
    }


    RectI pixelRoI;
    roi.toPixelEnclosing(renderScaleOneUpstreamIfRenderScaleSupportDisabled ? 0 : mipMapLevel, par, &pixelRoI);

    ImagePtr inputImg;

    ///For the roto brush, we do things separately and render the mask with the RotoContext.
    if (useRotoInput) {
        ///Usage of roto outside of the rotopaint node is no longer handled
        assert(attachedStroke);
        if (attachedStroke) {
            if (duringPaintStroke) {
                inputImg = node->getOrRenderLastStrokeImage(mipMapLevel, par, components, depth);
            } else {
                RectD rotoSrcRod;
                if (inputIsRotoBrush) {
                    //If the roto is inverted, we need to fill the full RoD of the input
                    bool inverted = attachedStroke->getInverted(time);
                    if (inverted) {
                        EffectInstancePtr rotoInput = getInput(0);
                        if (rotoInput) {
                            bool isProjectFormat;
                            StatusEnum st = rotoInput->getRegionOfDefinition_public(rotoInput->getRenderHash(), time, scale, view, &rotoSrcRod, &isProjectFormat);
                            (void)st;
                        }
                    }
                }

                inputImg = attachedStroke->renderMaskFromStroke(components,
                                                                time, view, depth, mipMapLevel, rotoSrcRod);

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
            qDebug() << node->getScriptName_mt_safe().c_str() << ": The RoI requested to the roto mask does not intersect with the bounds of the input image: Pixel RoI x1=" << pixelRoI.x1 << "y1=" << pixelRoI.y1 << "x2=" << pixelRoI.x2 << "y2=" << pixelRoI.y2 <<
                "Bounds x1=" << inputBounds.x1 << "y1=" << inputBounds.y1 << "x2=" << inputBounds.x2 << "y2=" << inputBounds.y2;
#endif

            return ImagePtr();
        }

        if ( inputImg && inputImagesThreadLocal.empty() ) {
            ///If the effect is analysis (e.g: Tracker) there's no input images in the thread-local storage, hence add it
            tls->currentRenderArgs.inputImages[inputNb].push_back(inputImg);
        }

        if ( returnStorage == eStorageModeGLTex && (inputImg->getStorageMode() != eStorageModeGLTex) ) {
            inputImg = convertRAMImageToOpenGLTexture(inputImg);
        }

        if (mapToClipPrefs) {
            inputImg = convertPlanesFormatsIfNeeded(getApp(), inputImg, pixelRoI, clipPrefComps, depth, node->usesAlpha0ToConvertFromRGBToRGBA(), eImagePremultiplicationPremultiplied, channelForMask);
        }

        return inputImg;
    }


    /// The node is connected.
    assert(inputEffect);

    std::list<ImagePlaneDesc> requestedComps;
    requestedComps.push_back(isMask ? maskComps : components);
    std::map<ImagePlaneDesc, ImagePtr> inputImages;
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
                                                                    returnStorage,
                                                                    thisEffectRenderTime,
                                                                    inputImagesThreadLocal), &inputImages);

    if ( inputImages.empty() || (retCode != eRenderRoIRetCodeOk) ) {
        return ImagePtr();
    }
    assert(inputImages.size() == 1);

    inputImg = inputImages.begin()->second;

    if ( !pixelRoI.intersects( inputImg->getBounds() ) ) {
        //The RoI requested does not intersect with the bounds of the input image, return a NULL image.
#ifdef DEBUG
        qDebug() << node->getScriptName_mt_safe().c_str() << ": The RoI requested to" << inputEffect->getScriptName_mt_safe().c_str() << "does not intersect with the bounds of the input image";
#endif

        return ImagePtr();
    }

    /*
     * From now on this is the generic part. We first call renderRoI and then convert to the appropriate scale/components if needed.
     * Note that since the image has been pre-rendered before by the recursive nature of the algorithm, the call to renderRoI will be
     * instantaneous thanks to the image cache.
     */


    if (roiPixel) {
        *roiPixel = pixelRoI;
    }
    unsigned int inputImgMipMapLevel = inputImg->getMipMapLevel();

    ///If the plug-in doesn't support the render scale, but the image is downscaled, up-scale it.
    ///Note that we do NOT cache it because it is really low def!
    ///For OpenGL textures, we do not do it because GL_TEXTURE_2D uses normalized texture coordinates anyway, so any OpenGL plug-in should support render scale.
    if (!dontUpscale  && renderFullScaleThenDownscale && (inputImgMipMapLevel != 0) && returnStorage == eStorageModeRAM) {
        assert(inputImgMipMapLevel != 0);
        ///Resize the image according to the requested scale
        ImageBitDepthEnum bitdepth = inputImg->getBitDepth();
        RectI bounds;
        inputImg->getRoD().toPixelEnclosing(0, par, &bounds);
        ImagePtr rescaledImg = boost::make_shared<Image>( inputImg->getComponents(), inputImg->getRoD(),
                                                         bounds, 0, par, bitdepth, inputImg->getPremultiplication(), inputImg->getFieldingOrder() );
        inputImg->upscaleMipMap( inputImg->getBounds(), inputImgMipMapLevel, 0, rescaledImg.get() );
        if (roiPixel) {
            RectD canonicalPixelRoI;

            if (!inputRoDSet) {
                bool isProjectFormat;
                StatusEnum st = inputEffect->getRegionOfDefinition_public(inputEffect->getRenderHash(), time, scale, view, &inputRoD, &isProjectFormat);
                Q_UNUSED(st);
            }

            pixelRoI.toCanonical(inputImgMipMapLevel, par, inputRoD, &canonicalPixelRoI);
            canonicalPixelRoI.toPixelEnclosing(0, par, roiPixel);
            pixelRoI = *roiPixel;
        }

        inputImg = rescaledImg;
    }


    //Remap if needed
    ImagePremultiplicationEnum outputPremult;
    if ( components.isColorPlane() ) {
        outputPremult = inputEffect->getPremult();
    } else {
        outputPremult = eImagePremultiplicationOpaque;
    }


    if (mapToClipPrefs) {
        inputImg = convertPlanesFormatsIfNeeded(getApp(), inputImg, pixelRoI, clipPrefComps, depth, node->usesAlpha0ToConvertFromRGBToRGBA(), outputPremult, channelForMask);
    }

#ifdef DEBUG
    ///Check that the rendered image contains what we requested.
    if ( !mapToClipPrefs && ( ( !isMask && (inputImg->getComponents() != components) ) || ( isMask && (inputImg->getComponents() != maskComps) ) ) ) {
        ImagePlaneDesc cc;
        if (isMask) {
            cc = maskComps;
        } else {
            cc = components;
        }
        qDebug() << "WARNING:" << node->getScriptName_mt_safe().c_str() << "requested" << cc.getChannelsLabel().c_str() << "but" << inputEffect->getScriptName_mt_safe().c_str() << "returned an image with"
                 << inputImg->getComponents().getChannelsLabel().c_str();
    }

#endif

    if ( inputImagesThreadLocal.empty() ) {
        ///If the effect is analysis (e.g: Tracker) there's no input images in the thread-local storage, hence add it
        tls->currentRenderArgs.inputImages[inputNb].push_back(inputImg);
    }

    return inputImg;
} // getImage

void
EffectInstance::calcDefaultRegionOfDefinition(U64 /*hash*/,
                                              double /*time*/,
                                              const RenderScale & scale,
                                              ViewIdx /*view*/,
                                              RectD *rod)
{

    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    RectI format = getOutputFormat();
    double par = getAspectRatio(-1);
    format.toCanonical_noClipping(mipMapLevel, par, rod);
}

StatusEnum
EffectInstance::getRegionOfDefinition(U64 hash,
                                      double time,
                                      const RenderScale & scale,
                                      ViewIdx view,
                                      RectD* rod) //!< rod is in canonical coordinates
{
    bool firstInput = true;
    RenderScale renderMappedScale = scale;

    assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(scale.x == 1. && scale.y == 1.) ) );

    for (int i = 0; i < getNInputs(); ++i) {
        if ( isInputMask(i) ) {
            continue;
        }
        EffectInstancePtr input = getInput(i);
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
                                         ViewIdx view,
                                         RectD* rod) //!< input/output
{
    /*If the rod is infinite clip it to the format*/


    assert(rod);
    if ( rod->isNull() ) {
        // if the RoD is empty, set it to a "standard" empty RoD (0,0,0,0)
        rod->clear();
    }
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
        for (int i = 0; i < getNInputs(); ++i) {
            EffectInstancePtr input = getInput(i);
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


    RectD canonicalFormat;

    if (x1Infinite || y1Infinite || x2Infinite || y2Infinite) {
        RectI format = getOutputFormat();
        assert(!format.isNull());
        double par = getAspectRatio(-1);
        unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
        format.toCanonical_noClipping(mipMapLevel, par, &canonicalFormat);
    }

    // BE CAREFUL:
    // std::numeric_limits<int>::infinity() does not exist (check std::numeric_limits<int>::has_infinity)
    bool isProjectFormat = false;
    if (x1Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->x1 = std::min(inputsUnion.x1, canonicalFormat.x1);
        } else {
            rod->x1 = canonicalFormat.x1;
            isProjectFormat = true;
        }
        rod->x2 = std::max(rod->x1, rod->x2);
    }
    if (y1Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->y1 = std::min(inputsUnion.y1, canonicalFormat.y1);
        } else {
            rod->y1 = canonicalFormat.y1;
            isProjectFormat = true;
        }
        rod->y2 = std::max(rod->y1, rod->y2);
    }
    if (x2Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->x2 = std::max(inputsUnion.x2, canonicalFormat.x2);
        } else {
            rod->x2 = canonicalFormat.x2;
            isProjectFormat = true;
        }
        rod->x1 = std::min(rod->x1, rod->x2);
    }
    if (y2Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->y2 = std::max(inputsUnion.y2, canonicalFormat.y2);
        } else {
            rod->y2 = canonicalFormat.y2;
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
                                     ViewIdx view,
                                     RoIMap* ret)
{
    bool tilesSupported = supportsTiles();

    for (int i = 0; i < getNInputs(); ++i) {
        EffectInstancePtr input = getInput(i);
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
                                ViewIdx view)
{
    FramesNeededMap ret;
    RangeD defaultRange;

    defaultRange.min = defaultRange.max = time;
    std::vector<RangeD> ranges;
    ranges.push_back(defaultRange);
    FrameRangesMap defViewRange;
    defViewRange.insert( std::make_pair(view, ranges) );
    for (int i = 0; i < getNInputs(); ++i) {
        if ( isInputRotoBrush(i) ) {
            ret.insert( std::make_pair(i, defViewRange) );
        } else {
            EffectInstancePtr input = getInput(i);
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
    for (int i = 0; i < getNInputs(); ++i) {
        EffectInstancePtr input = getInput(i);
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
    , _didGroupEmit(false)
{
    _didEmit = node->notifyRenderingStarted();

    // If the node is in a group, notify also the group
    NodeCollectionPtr group = node->getGroup();
    if (group) {
        NodeGroup* isGroupNode = dynamic_cast<NodeGroup*>(group.get());
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
            NodeGroup* isGroupNode = dynamic_cast<NodeGroup*>(group.get());
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
                             bool useCache,
                             ImagePtr* image)
{
    if (!useCache) {
        *image = boost::make_shared<Image>(key, params);
    } else {
        assert(params->getStorageInfo().mode != eStorageModeGLTex);

        if (params->getStorageInfo().mode == eStorageModeRAM) {
            appPTR->getImageOrCreate(key, params, image);
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


        (*image)->ensureBounds( params->getBounds() );
    }
}

ImagePtr
EffectInstance::convertOpenGLTextureToCachedRAMImage(const ImagePtr& image)
{
    assert(image->getStorageMode() == eStorageModeGLTex);

    ImageParamsPtr params = boost::make_shared<ImageParams>( *image->getParams() );
    CacheEntryStorageInfo& info = params->getStorageInfo();
    info.mode = eStorageModeRAM;

    ImagePtr ramImage;
    getOrCreateFromCacheInternal(image->getKey(), params, true /*useCache*/, &ramImage);
    if (!ramImage) {
        return ramImage;
    }

    OSGLContextPtr context = getThreadLocalOpenGLContext();
    assert(context);
    if (!context) {
        throw std::runtime_error("No OpenGL context attached");
    }

    ramImage->pasteFrom(*image, image->getBounds(), false, context);
    ramImage->markForRendered(image->getBounds());

    return ramImage;
}

ImagePtr
EffectInstance::convertRAMImageToOpenGLTexture(const ImagePtr& image)
{
    assert(image->getStorageMode() != eStorageModeGLTex);

    ImageParamsPtr params = boost::make_shared<ImageParams>( *image->getParams() );
    CacheEntryStorageInfo& info = params->getStorageInfo();
    info.mode = eStorageModeGLTex;
    info.textureTarget = GL_TEXTURE_2D;

    RectI bounds = image->getBounds();
    OSGLContextPtr context = getThreadLocalOpenGLContext();
    assert(context);
    if (!context) {
        throw std::runtime_error("No OpenGL context attached");
    }

    GLuint pboID = context->getPBOId();
    assert(pboID != 0);
    glEnable(GL_TEXTURE_2D);
    // bind PBO to update texture source
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboID);

    std::size_t dataSize = bounds.area() * 4 * info.dataTypeSize;

    // Note that glMapBufferARB() causes sync issue.
    // If GPU is working with this buffer, glMapBufferARB() will wait(stall)
    // until GPU to finish its job. To avoid waiting (idle), you can call
    // first glBufferDataARB() with NULL pointer before glMapBufferARB().
    // If you do that, the previous data in PBO will be discarded and
    // glMapBufferARB() returns a new allocated pointer immediately
    // even if GPU is still working with the previous data.
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, dataSize, 0, GL_DYNAMIC_DRAW_ARB);

    bool useTmpImage = image->getComponentsCount() != 4;
    ImagePtr tmpImg;
    if (useTmpImage) {
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
        tmpImg.reset( new Image( ImagePlaneDesc::getRGBAComponents(), image->getRoD(), bounds, 0, image->getPixelAspectRatio(), image->getBitDepth(), image->getPremultiplication(), image->getFieldingOrder(), false, eStorageModeRAM) );
#else
        tmpImg = boost::make_shared<Image>( ImagePlaneDesc::getRGBAComponents(), image->getRoD(), bounds, 0, image->getPixelAspectRatio(), image->getBitDepth(), image->getPremultiplication(), image->getFieldingOrder(), false, eStorageModeRAM);
#endif
        tmpImg->setKey(image->getKey());
        if (tmpImg->getComponents() == image->getComponents()) {
            tmpImg->pasteFrom(*image, bounds);
        } else {
            image->convertToFormat(bounds, eViewerColorSpaceLinear, eViewerColorSpaceLinear, -1, false, false, tmpImg.get());
        }
    }

    Image::ReadAccess racc( tmpImg ? tmpImg.get() : image.get() );
    const unsigned char* srcdata = racc.pixelAt(bounds.x1, bounds.y1);
    assert(srcdata);

    GLvoid* gpuData = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    if (gpuData) {
            // update data directly on the mapped buffer
        memcpy(gpuData, srcdata, dataSize);
        GLboolean result = glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release the mapped buffer
        assert(result == GL_TRUE);
        Q_UNUSED(result);
    }
    glCheckError();

    // The creation of the image will use glTexImage2D and will get filled with the PBO
    ImagePtr gpuImage;
    getOrCreateFromCacheInternal(image->getKey(), params, false /*useCache*/, &gpuImage);

    // it is good idea to release PBOs with ID 0 after use.
    // Once bound with 0, all pixel operations are back to normal ways.
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    //glBindTexture(GL_TEXTURE_2D, 0); // useless, we didn't bind anything
    glCheckError();


    return gpuImage;
} // convertRAMImageToOpenGLTexture

void
EffectInstance::getImageFromCacheAndConvertIfNeeded(bool /*useCache*/,
                                                    StorageModeEnum storage,
                                                    StorageModeEnum returnStorage,
                                                    const ImageKey & key,
                                                    unsigned int mipMapLevel,
                                                    const RectI* boundsParam,
                                                    const RectD* rodParam,
                                                    const RectI& roi,
                                                    ImageBitDepthEnum bitdepth,
                                                    const ImagePlaneDesc & components,
                                                    const EffectInstance::InputImagesMap & inputImages,
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
            const ImagePlaneDesc & imgComps = (*it)->getComponents();
            ImageBitDepthEnum imgDepth = (*it)->getBitDepth();

            if ( (*it)->getParams()->isRodProjectFormat() ) {
                ////If the image was cached with a RoD dependent on the project format, but the project format changed,
                ////just discard this entry
                Format projectFormat;
                getApp()->getProject()->getProjectDefaultFormat(&projectFormat);
                RectD canonicalProject = projectFormat.toCanonicalFormat();
                if ( canonicalProject != (*it)->getRoD() ) {
                    appPTR->removeFromNodeCache(*it);
                    continue;
                }
            }

            if ((*it)->getBounds().isNull()) {
                continue;
            }

            ///Throw away images that are not even what the node want to render
            /*if ( ( imgComps.isColorPlane() && nodePrefComps.isColorPlane() && (imgComps != nodePrefComps) ) || (imgDepth != nodePrefDepth) ) {
                appPTR->removeFromNodeCache(*it);
                continue;
            }*/

            bool convertible = (imgComps.isColorPlane() && components.isColorPlane()) || (imgComps == components);
            if ( (imgMMlevel == mipMapLevel) && convertible &&
                 ( getSizeOfForBitDepth(imgDepth) >= getSizeOfForBitDepth(bitdepth) ) /* && imgComps == components && imgDepth == bitdepth*/ ) {
                ///We found  a matching image

                *image = *it;
                break;
            } else {
                if ( (*it)->getStorageMode() != eStorageModeRAM || (imgMMlevel >= mipMapLevel) || !convertible ||
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
                ImageParamsPtr oldParams = imageToConvert->getParams();

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

                ImageParamsPtr imageParams = Image::makeParams(rod,
                                                                               downscaledBounds,
                                                                               oldParams->getPixelAspectRatio(),
                                                                               mipMapLevel,
                                                                               oldParams->isRodProjectFormat(),
                                                                               oldParams->getComponents(),
                                                                               oldParams->getBitDepth(),
                                                                               oldParams->getPremultiplication(),
                                                                               oldParams->getFieldingOrder(),
                                                                               eStorageModeRAM);


                imageParams->setMipMapLevel(mipMapLevel);


                ImagePtr img;
                getOrCreateFromCacheInternal(key, imageParams, imageToConvert->usesBitMap(), &img);
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
                                                     imageToConvert->usesBitMap(),
                                                     img.get() );
                } else {
                    img->pasteFrom(*imageToConvert, imgToConvertBounds);
                }

                imageToConvert = img;
            }

            if (storage == eStorageModeGLTex) {

                // When using the GPU, we don't want to retrieve partially rendered image because rendering the portion
                // needed then reading it back to put it in the CPU image would take much more effort than just computing
                // the GPU image.
                std::list<RectI> restToRender;
                imageToConvert->getRestToRender(roi, restToRender);
                if ( restToRender.empty() ) {
                    if (returnStorage == eStorageModeGLTex) {
                        assert(glContextAttacher);
                        glContextAttacher->attach();
                        *image = convertRAMImageToOpenGLTexture(imageToConvert);
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

                    // When using the GPU, we don't want to retrieve partially rendered image because rendering the portion
                    // needed then reading it back to put it in the CPU image would take much more effort than just computing
                    // the GPU image.
                    std::list<RectI> restToRender;
                    (*image)->getRestToRender(roi, restToRender);
                    if ( restToRender.empty() ) {
                        // If renderRoI must return a RAM image, don't convert it back again!
                        if (returnStorage == eStorageModeGLTex) {
                            assert(glContextAttacher);
                            glContextAttacher->attach();
                            *image = convertRAMImageToOpenGLTexture(*image);
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
EffectInstance::tryConcatenateTransforms(double time,
                                         bool draftRender,
                                         ViewIdx view,
                                         const RenderScale & scale,
                                         InputMatrixMap* inputTransforms)
{
    bool canTransform = getNode()->getCurrentCanTransform();

    //An effect might not be able to concatenate transforms but can still apply a transform (e.g CornerPinMasked)
    std::list<int> inputHoldingTransforms;
    bool canApplyTransform = getInputsHoldingTransform(&inputHoldingTransforms);

    assert(inputHoldingTransforms.empty() || canApplyTransform);

    Transform::Matrix3x3 thisNodeTransform;
    EffectInstancePtr inputToTransform;
    bool getTransformSucceeded = false;

    if (canTransform) {
        /*
         * If getting the transform does not succeed, then this effect is treated as any other ones.
         */
        StatusEnum stat = getTransform_public(time, scale, draftRender, view, &inputToTransform, &thisNodeTransform);
        if (stat == eStatusOK) {
            getTransformSucceeded = true;
        }
    }


    if ( (canTransform && getTransformSucceeded) || ( !canTransform && canApplyTransform && !inputHoldingTransforms.empty() ) ) {
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
                    StatusEnum stat = input->getTransform_public(time, scale, draftRender, view, &inputToTransform, &m);
                    if (stat == eStatusOK) {
                        matricesByOrder.push_back(m);
                        if (inputToTransform) {
                            im.newInputNbToFetchFrom = input->getInputNumber( inputToTransform.get() );
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
                im.cat= boost::make_shared<Transform::Matrix3x3>();
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
                                   const ImagePlaneDesc & components,
                                   ImageBitDepthEnum depth,
                                   ImagePremultiplicationEnum premult,
                                   ImageFieldingOrderEnum fielding,
                                   double par,
                                   unsigned int mipmapLevel,
                                   bool renderFullScaleThenDownscale,
                                   StorageModeEnum storage,
                                   bool createInCache,
                                   ImagePtr* fullScaleImage,
                                   ImagePtr* downscaleImage)
{
    //If we're rendering full scale and with input images at full scale, don't cache the downscale image since it is cheap to
    //recreate, instead cache the full-scale image
    if (renderFullScaleThenDownscale) {
        *downscaleImage = boost::make_shared<Image>(components, rod, downscaleImageBounds, mipmapLevel, par, depth, premult, fielding, true);
        ImageParamsPtr upscaledImageParams = Image::makeParams(rod,
                                                                               fullScaleImageBounds,
                                                                               par,
                                                                               0,
                                                                               isProjectFormat,
                                                                               components,
                                                                               depth,
                                                                               premult,
                                                                               fielding,
                                                                               storage,
                                                                               GL_TEXTURE_2D);
        //The upscaled image will be rendered with input images at full def, it is then the best possibly rendered image so cache it!

        fullScaleImage->reset();
        getOrCreateFromCacheInternal(key, upscaledImageParams, createInCache, fullScaleImage);

        if (!*fullScaleImage) {
            return false;
        }
    } else {
        ///Cache the image with the requested components instead of the remapped ones
        ImageParamsPtr cachedImgParams = Image::makeParams(rod,
                                                                           downscaleImageBounds,
                                                                           par,
                                                                           mipmapLevel,
                                                                           isProjectFormat,
                                                                           components,
                                                                           depth,
                                                                           premult,
                                                                           fielding,
                                                                           storage,
                                                                           GL_TEXTURE_2D);

        //Take the lock after getting the image from the cache or while allocating it
        ///to make sure a thread will not attempt to write to the image while its being allocated.
        ///When calling allocateMemory() on the image, the cache already has the lock since it added it
        ///so taking this lock now ensures the image will be allocated completely

        getOrCreateFromCacheInternal(key, cachedImgParams, createInCache, downscaleImage);
        if (!*downscaleImage) {
            return false;
        }
        *fullScaleImage = *downscaleImage;
    }

    return true;
} // EffectInstance::allocateImagePlane

void
EffectInstance::transformInputRois(const EffectInstance* self,
                                   const InputMatrixMapPtr & inputTransforms,
                                   double par,
                                   const RenderScale & scale,
                                   RoIMap* inputsRoi,
                                   std::map<int, EffectInstancePtr>* reroutesMap)
{
    if (!inputTransforms) {
        return;
    }
    //Transform the RoIs by the inverse of the transform matrix (which is in pixel coordinates)
    for (InputMatrixMap::const_iterator it = inputTransforms->begin(); it != inputTransforms->end(); ++it) {
        RectD transformedRenderWindow;
        EffectInstancePtr effectInTransformInput = self->getInput(it->first);
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
                                        StorageModeEnum renderStorageMode,
                                        double time,
                                        ViewIdx view,
                                        const RectD & rod,
                                        const RectD & canonicalRenderWindow,
                                        const InputMatrixMapPtr& inputTransforms,
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
                              renderStorageMode,
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
                                                      const ViewIdx view,
                                                      const double par,
                                                      const bool byPassCache,
                                                      const ImageBitDepthEnum outputClipPrefDepth,
                                                      const ImagePlaneDesc & outputClipPrefsComps,
                                                      const ComponentsNeededMapPtr & compsNeeded,
                                                      const std::bitset<4>& processChannels,
                                                      const ImagePlanesToRenderPtr & planes) // when MT, planes is a copy so there's is no data race
{
    ///There cannot be the same thread running 2 concurrent instances of renderRoI on the same effect.
#ifdef DEBUG
    {
        EffectTLSDataPtr tls = tlsData->getTLSData();
        assert(!tls || !tls->currentRenderArgs.validArgs);
    }
#endif
    EffectTLSDataPtr tls = tlsData->getOrCreateTLSData();

    assert( !rectToRender.rect.isNull() );

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

    // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
# ifndef NDEBUG
    const EffectInstance::PlaneToRender & firstPlaneToRender = planes->planes.begin()->second;
    RectI renderBounds = firstPlaneToRender.renderMappedImage->getBounds();
    assert(renderBounds.x1 <= renderMappedRectToRender.x1 && renderMappedRectToRender.x2 <= renderBounds.x2 &&
           renderBounds.y1 <= renderMappedRectToRender.y1 && renderMappedRectToRender.y2 <= renderBounds.y2);
# endif

#ifndef NDEBUG
    const ParallelRenderArgsPtr& frameArgs = tls->frameArgs.back();
#endif

      ///It might have been already rendered now
    if ( renderMappedRectToRender.isNull() ) {
        return eRenderingFunctorRetOK;
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
                                                rectToRender.inputRois,
                                                firstFrame,
                                                lastFrame,
                                                planes->useOpenGL);
    ImagePtr originalInputImage, maskImage;
    ImagePremultiplicationEnum originalImagePremultiplication;
    EffectInstance::InputImagesMap::const_iterator foundPrefInput = rectToRender.imgs.find(preferredInput);
    EffectInstance::InputImagesMap::const_iterator foundMaskInput = rectToRender.imgs.end();

    if ( _publicInterface->isHostMaskingEnabled() ) {
        foundMaskInput = rectToRender.imgs.find(_publicInterface->getNInputs() - 1);
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
    RenderScale scale( Image::getScaleFromMipMapLevel(mipMapLevel) );
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
                // Commented-out: Some Furnace plug-ins from The Foundry (e.g F_Steadiness) are not supporting multi-resolution but actually produce an output
                // with a RoD different from the input
                /*/assert(srcBounds.x1 == 0);
                   assert(srcBounds.y1 == 0);
                   assert(srcBounds.x1 == dstBounds.x1);
                   assert(srcBounds.x2 == dstBounds.x2);
                   assert(srcBounds.y1 == dstBounds.y1);
                   assert(srcBounds.y2 == dstBounds.y2);*/
            }
        } // end for
    } //end for

    if (_publicInterface->supportsRenderScaleMaybe() == eSupportsNo) {
        assert(firstPlaneToRender.renderMappedImage->getMipMapLevel() == 0);
        assert(renderMappedMipMapLevel == 0);
    }
#endif // !NDEBUG

    RenderingFunctorRetEnum handlerRet =  renderHandler(tls,
                                                        mipMapLevel,
                                                        renderFullScaleThenDownscale,
                                                        isSequentialRender,
                                                        isRenderResponseToUserInteraction,
                                                        renderMappedRectToRender,
                                                        downscaledRectToRender,
                                                        byPassCache,
                                                        outputClipPrefDepth,
                                                        outputClipPrefsComps,
                                                        processChannels,
                                                        originalInputImage,
                                                        maskImage,
                                                        originalImagePremultiplication,
                                                        *planes);
    if (handlerRet == eRenderingFunctorRetOK) {
        return eRenderingFunctorRetOK;
    } else {
        return handlerRet;
    }
} // EffectInstance::tiledRenderingFunctor

EffectInstance::RenderingFunctorRetEnum
EffectInstance::Implementation::renderHandler(const EffectTLSDataPtr& tls,
                                              const unsigned int mipMapLevel,
                                              const bool renderFullScaleThenDownscale,
                                              const bool isSequentialRender,
                                              const bool isRenderResponseToUserInteraction,
                                              const RectI & renderMappedRectToRender,
                                              const RectI & downscaledRectToRender,
                                              const bool byPassCache,
                                              const ImageBitDepthEnum outputClipPrefDepth,
                                              const ImagePlaneDesc & outputClipPrefsComps,
                                              const std::bitset<4>& processChannels,
                                              const ImagePtr & originalInputImage,
                                              const ImagePtr & maskImage,
                                              const ImagePremultiplicationEnum originalImagePremultiplication,
                                              ImagePlanesToRender & planes)
{
    TimeLapsePtr timeRecorder;
    const ParallelRenderArgsPtr& frameArgs = tls->frameArgs.back();

    if (frameArgs->stats) {
        timeRecorder = boost::make_shared<TimeLapse>();
    }

    const EffectInstance::PlaneToRender & firstPlane = planes.planes.begin()->second;
    const double time = tls->currentRenderArgs.time;
    const ViewIdx view = tls->currentRenderArgs.view;

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
    actionArgs.useOpenGL = planes.useOpenGL;

    std::list<std::pair<ImagePlaneDesc, ImagePtr> > tmpPlanes;
    bool multiPlanar = _publicInterface->isMultiPlanar();

    actionArgs.roi = renderMappedRectToRender;


    // Setup the context when rendering using OpenGL
    OSGLContextPtr glContext;
    boost::scoped_ptr<OSGLContextAttacher> glContextAttacher;
    if (planes.useOpenGL) {
        // Setup the viewport and the framebuffer
        glContext = frameArgs->openGLContext.lock();
        AbortableRenderInfoPtr abortInfo = frameArgs->abortInfo.lock();
        assert(abortInfo);
        assert(glContext);

        // Ensure the context is current
        // scoped_ptr
        glContextAttacher.reset( new OSGLContextAttacher(glContext, abortInfo
#ifdef DEBUG
                                                         , frameArgs->time
#endif
                                                         ) );
        glContextAttacher->attach();


        GLuint fboID = glContext->getFBOId();
        glBindFramebuffer(GL_FRAMEBUFFER, fboID);
        glCheckError();
    }


    if (tls->currentRenderArgs.isIdentity) {
        std::list<ImagePlaneDesc> comps;

        for (std::map<ImagePlaneDesc, EffectInstance::PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it) {
            //If color plane, request the preferred comp of the identity input
            if ( tls->currentRenderArgs.identityInput && it->second.renderMappedImage->getComponents().isColorPlane() ) {
                ImagePlaneDesc prefInputComps, prefInputCompsPaired;
                tls->currentRenderArgs.identityInput->getMetadataComponents(-1, &prefInputComps, &prefInputCompsPaired);
                comps.push_back(prefInputComps);
            } else {
                comps.push_back( it->second.renderMappedImage->getComponents() );
            }
        }
        assert( !comps.empty() );
        std::map<ImagePlaneDesc, ImagePtr> identityPlanes;
        // scoped_ptr
        boost::scoped_ptr<EffectInstance::RenderRoIArgs> renderArgs( new EffectInstance::RenderRoIArgs(tls->currentRenderArgs.identityTime,
                                                                                                       actionArgs.originalScale,
                                                                                                       mipMapLevel,
                                                                                                       view,
                                                                                                       false,
                                                                                                       downscaledRectToRender,
                                                                                                       RectD(),
                                                                                                       comps,
                                                                                                       outputClipPrefDepth,
                                                                                                       false,
                                                                                                       _publicInterface,
                                                                                                       planes.useOpenGL ? eStorageModeGLTex : eStorageModeRAM,
                                                                                                       time) );
        if (!tls->currentRenderArgs.identityInput) {
            for (std::map<ImagePlaneDesc, EffectInstance::PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it) {
                it->second.renderMappedImage->fillZero(renderMappedRectToRender, glContext);

                if ( frameArgs->stats && frameArgs->stats->isInDepthProfilingEnabled() ) {
                    frameArgs->stats->addRenderInfosForNode( _publicInterface->getNode(),  NodePtr(), it->first.getChannelsLabel(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation() );
                }
            }

            return eRenderingFunctorRetOK;
        } else {
            EffectInstance::RenderRoIRetCode renderOk;
            renderOk = tls->currentRenderArgs.identityInput->renderRoI(*renderArgs, &identityPlanes);
            if (renderOk == eRenderRoIRetCodeAborted) {
                return eRenderingFunctorRetAborted;
            } else if (renderOk == eRenderRoIRetCodeFailed) {
                return eRenderingFunctorRetFailed;
            } else if ( identityPlanes.empty() ) {
                for (std::map<ImagePlaneDesc, EffectInstance::PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it) {
                    it->second.renderMappedImage->fillZero(renderMappedRectToRender, glContext);

                    if ( frameArgs->stats && frameArgs->stats->isInDepthProfilingEnabled() ) {
                        frameArgs->stats->addRenderInfosForNode( _publicInterface->getNode(),  tls->currentRenderArgs.identityInput->getNode(), it->first.getChannelsLabel(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation() );
                    }
                }

                return eRenderingFunctorRetOK;
            } else {
                assert( identityPlanes.size() == planes.planes.size() );

                std::map<ImagePlaneDesc, ImagePtr>::iterator idIt = identityPlanes.begin();
                for (std::map<ImagePlaneDesc, EffectInstance::PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it, ++idIt) {
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
                            sourceImage = boost::make_shared<Image>(it->second.fullscaleImage->getComponents(),
                                                                    idIt->second->getRoD(),
                                                                    idIt->second->getBounds(),
                                                                    idIt->second->getMipMapLevel(),
                                                                    idIt->second->getPixelAspectRatio(),
                                                                    it->second.fullscaleImage->getBitDepth(),
                                                                    idIt->second->getPremultiplication(),
                                                                    idIt->second->getFieldingOrder(),
                                                                    false);

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
                        ImagePtr inputPlane = boost::make_shared<Image>(it->first,
                                                       rod,
                                                       bounds,
                                                       it->second.renderMappedImage->getMipMapLevel(),
                                                       it->second.renderMappedImage->getPixelAspectRatio(),
                                                       it->second.renderMappedImage->getBitDepth(),
                                                       it->second.renderMappedImage->getPremultiplication(),
                                                       it->second.renderMappedImage->getFieldingOrder(),
                                                       false);
                        sourceImage->upscaleMipMap( sourceImage->getBounds(), sourceImage->getMipMapLevel(), inputPlane->getMipMapLevel(), inputPlane.get() );
                        it->second.fullscaleImage->pasteFrom(*inputPlane, renderMappedRectToRender, false);
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
                            idIt->second->getBounds().intersect(downscaledRectToRender, &convertWindow);
                            idIt->second->convertToFormat( convertWindow, colorspace, dstColorspace, 3, false, false, it->second.downscaleImage.get() );
                        } else {
                            it->second.downscaleImage->pasteFrom(*(idIt->second), downscaledRectToRender, false, glContext);
                        }
                    }

                    if ( frameArgs->stats && frameArgs->stats->isInDepthProfilingEnabled() ) {
                        frameArgs->stats->addRenderInfosForNode( _publicInterface->getNode(),  tls->currentRenderArgs.identityInput->getNode(), it->first.getChannelsLabel(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation() );
                    }
                }

                return eRenderingFunctorRetOK;
            } // if (renderOk == eRenderRoIRetCodeAborted) {
        }  //  if (!identityInput) {
    } // if (identity) {

    tls->currentRenderArgs.outputPlanes = planes.planes;
    for (std::map<ImagePlaneDesc, EffectInstance::PlaneToRender>::iterator it = tls->currentRenderArgs.outputPlanes.begin(); it != tls->currentRenderArgs.outputPlanes.end(); ++it) {
        /*
         * When using the cache, allocate a local temporary buffer onto which the plug-in will render, and then safely
         * copy this buffer to the shared (among threads) image.
         * This is also needed if the plug-in does not support the number of components of the renderMappedImage
         */
        ImagePlaneDesc prefComp;
        if (multiPlanar) {
            prefComp = _publicInterface->getNode()->findClosestSupportedComponents( -1, it->second.renderMappedImage->getComponents() );
        } else {
            prefComp = outputClipPrefsComps;
        }

        // OpenGL render never use the cache and bitmaps, all images are local to a render.
        if ( ( it->second.renderMappedImage->usesBitMap() || ( prefComp != it->second.renderMappedImage->getComponents() ) ||
               ( outputClipPrefDepth != it->second.renderMappedImage->getBitDepth() ) ) && !_publicInterface->isPaintingOverItselfEnabled() && !planes.useOpenGL ) {
            it->second.tmpImage = boost::make_shared<Image>(prefComp,
                                                            it->second.renderMappedImage->getRoD(),
                                                            actionArgs.roi,
                                                            it->second.renderMappedImage->getMipMapLevel(),
                                                            it->second.renderMappedImage->getPixelAspectRatio(),
                                                            outputClipPrefDepth,
                                                            it->second.renderMappedImage->getPremultiplication(),
                                                            it->second.renderMappedImage->getFieldingOrder(),
                                                 false); //< no bitmap
        } else {
            it->second.tmpImage = it->second.renderMappedImage;
        }
        tmpPlanes.push_back( std::make_pair(it->second.renderMappedImage->getComponents(), it->second.tmpImage) );
    }


    /// Render in the temporary image


    actionArgs.time = time;
    actionArgs.view = view;
    actionArgs.isSequentialRender = isSequentialRender;
    actionArgs.isRenderResponseToUserInteraction = isRenderResponseToUserInteraction;
    actionArgs.inputImages = tls->currentRenderArgs.inputImages;

    std::list<std::list<std::pair<ImagePlaneDesc, ImagePtr> > > planesLists;
    if (!multiPlanar) {
        for (std::list<std::pair<ImagePlaneDesc, ImagePtr> >::iterator it = tmpPlanes.begin(); it != tmpPlanes.end(); ++it) {
            std::list<std::pair<ImagePlaneDesc, ImagePtr> > tmp;
            tmp.push_back(*it);
            planesLists.push_back(tmp);
        }
    } else {
        planesLists.push_back(tmpPlanes);
    }

    bool renderAborted = false;
    std::map<ImagePlaneDesc, EffectInstance::PlaneToRender> outputPlanes;
    for (std::list<std::list<std::pair<ImagePlaneDesc, ImagePtr> > >::iterator it = planesLists.begin(); it != planesLists.end(); ++it) {
        if (!multiPlanar) {
            assert( !it->empty() );
            tls->currentRenderArgs.outputPlaneBeingRendered = it->front().first;
        }
        actionArgs.outputPlanes = *it;

        int textureTarget = 0;
        if (planes.useOpenGL) {
            actionArgs.glContextData = planes.glContextData;

            // Effects that render multiple planes at once are NOT supported by the OpenGL render suite
            // We only bind to the framebuffer color attachment 0 the "main" output image plane
            assert(actionArgs.outputPlanes.size() == 1);

            const ImagePtr& mainImagePlane = actionArgs.outputPlanes.front().second;
            assert(mainImagePlane->getStorageMode() == eStorageModeGLTex);
            textureTarget = mainImagePlane->getGLTextureTarget();
            glEnable(textureTarget);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture( textureTarget, mainImagePlane->getGLTextureID() );
            glCheckError();
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureTarget, mainImagePlane->getGLTextureID(), 0 /*LoD*/);
            glCheckError();
            glCheckFramebufferError();

            // setup the output viewport
            RectI imageBounds = mainImagePlane->getBounds();
            glViewport( actionArgs.roi.x1 - imageBounds.x1, actionArgs.roi.y1 - imageBounds.y1, actionArgs.roi.width(), actionArgs.roi.height() );

            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(actionArgs.roi.x1, actionArgs.roi.x2, actionArgs.roi.y1, actionArgs.roi.y2, -1, 1);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();


            glCheckError();

            // Enable scissor to make the plug-in doesn't render outside of the viewport...
            glEnable(GL_SCISSOR_TEST);
            glScissor( actionArgs.roi.x1 - imageBounds.x1, actionArgs.roi.y1 - imageBounds.y1, actionArgs.roi.width(), actionArgs.roi.height() );

            if (_publicInterface->getNode()->isGLFinishRequiredBeforeRender()) {
                // Ensure that previous asynchronous operations are done (e.g: glTexImage2D) some plug-ins seem to require it (Hitfilm Ignite plugin-s)
                glFinish();
            }
        }

        StatusEnum st = _publicInterface->render_public(actionArgs);

        if (planes.useOpenGL) {
            glDisable(GL_SCISSOR_TEST);
            glCheckError();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(textureTarget, 0);
            glCheckError();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glCheckError();
        }

        renderAborted = _publicInterface->aborted();

        /*
         * Since new planes can have been allocated on the fly by allocateImagePlaneAndSetInThreadLocalStorage(), refresh
         * the planes map from the thread-local storage once the render action is finished
         */
        if ( it == planesLists.begin() ) {
            outputPlanes = tls->currentRenderArgs.outputPlanes;
            assert( !outputPlanes.empty() );
        }

        if ( (st != eStatusOK) || renderAborted ) {
#if NATRON_ENABLE_TRIMAP
            //if ( frameArgs->isCurrentFrameRenderNotAbortable() ) {
                /*
                   At this point, another thread might have already gotten this image from the cache and could end-up
                   using it while it has still pixels marked to PIXEL_UNAVAILABLE, hence clear the bitmap
                 */
                for (std::map<ImagePlaneDesc, EffectInstance::PlaneToRender>::const_iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {
                    it->second.renderMappedImage->clearBitmap(renderMappedRectToRender);
                }
            //}
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
    } // for (std::list<std::list<std::pair<ImagePlaneDesc,ImagePtr> > >::iterator it = planesLists.begin(); it != planesLists.end(); ++it)

    assert(!renderAborted);

    bool unPremultIfNeeded = planes.outputPremult == eImagePremultiplicationPremultiplied;
    bool useMaskMix = _publicInterface->isHostMaskingEnabled() || _publicInterface->isHostMixingEnabled();
    double mix = useMaskMix ? _publicInterface->getNode()->getHostMixingValue(time, view) : 1.;
    bool doMask = useMaskMix ? _publicInterface->getNode()->isMaskEnabled(_publicInterface->getNInputs() - 1) : false;

    //Check for NaNs, copy to output image and mark for rendered
    for (std::map<ImagePlaneDesc, EffectInstance::PlaneToRender>::const_iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {
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
                        ImagePtr tmp = boost::make_shared<Image>(it->second.tmpImage->getComponents(),
                                                it->second.tmpImage->getRoD(),
                                                renderMappedRectToRender,
                                                0,
                                                it->second.tmpImage->getPixelAspectRatio(),
                                                it->second.tmpImage->getBitDepth(),
                                                it->second.tmpImage->getPremultiplication(),
                                                it->second.tmpImage->getFieldingOrder(),
                                                false);
                        originalInputImage->upscaleMipMap( downscaledRectToRender, originalInputImage->getMipMapLevel(), 0, tmp.get() );
                        mappedOriginalInputImage = tmp;
                    }
                }

                if (mappedOriginalInputImage) {
                    it->second.tmpImage->copyUnProcessedChannels(renderMappedRectToRender, planes.outputPremult, originalImagePremultiplication, processChannels, mappedOriginalInputImage, true);
                    if (useMaskMix) {
                        it->second.tmpImage->applyMaskMix(renderMappedRectToRender, maskImage.get(), mappedOriginalInputImage.get(), doMask, false, mix);
                    }
                }
                if ( ( it->second.fullscaleImage->getComponents() != it->second.tmpImage->getComponents() ) ||
                     ( it->second.fullscaleImage->getBitDepth() != it->second.tmpImage->getBitDepth() ) ) {
                    /*
                     * BitDepth/Components conversion required as well as downscaling, do conversion to a tmp buffer
                     */
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
                    ImagePtr tmp( new Image(it->second.fullscaleImage->getComponents(),
                                            it->second.tmpImage->getRoD(),
                                            renderMappedRectToRender,
                                            mipMapLevel,
                                            it->second.tmpImage->getPixelAspectRatio(),
                                            it->second.fullscaleImage->getBitDepth(),
                                            it->second.fullscaleImage->getPremultiplication(),
                                            it->second.fullscaleImage->getFieldingOrder(),
                                            false) );
#else
                    ImagePtr tmp = boost::make_shared<Image>(it->second.fullscaleImage->getComponents(),
                                                             it->second.tmpImage->getRoD(),
                                                             renderMappedRectToRender,
                                                             mipMapLevel,
                                                             it->second.tmpImage->getPixelAspectRatio(),
                                                             it->second.fullscaleImage->getBitDepth(),
                                                             it->second.fullscaleImage->getPremultiplication(),
                                                             it->second.fullscaleImage->getFieldingOrder(),
                                                             false);
#endif

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
                    if (it->second.tmpImage != it->second.fullscaleImage) {
                        it->second.fullscaleImage->pasteFrom(*(it->second.tmpImage), renderMappedRectToRender, false);
                    }
                }


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
            } // if (renderFullScaleThenDownscale) {
        } // if (it->second.isAllocatedOnTheFly) {

        if ( frameArgs->stats && frameArgs->stats->isInDepthProfilingEnabled() ) {
            frameArgs->stats->addRenderInfosForNode( _publicInterface->getNode(),  NodePtr(), it->first.getChannelsLabel(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation() );
        }
    } // for (std::map<ImagePlaneDesc,PlaneToRender>::const_iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {


    return eRenderingFunctorRetOK;
} // tiledRenderingFunctor

ImagePtr
EffectInstance::allocateImagePlaneAndSetInThreadLocalStorage(const ImagePlaneDesc & plane)
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

    if (!tls || !tls->currentRenderArgs.validArgs) {
        return ImagePtr();
    }

    assert( !tls->currentRenderArgs.outputPlanes.empty() );

    const EffectInstance::PlaneToRender & firstPlane = tls->currentRenderArgs.outputPlanes.begin()->second;
    bool useCache = firstPlane.fullscaleImage->usesBitMap() || firstPlane.downscaleImage->usesBitMap();
    if ( boost::starts_with(getNode()->getPluginID(), "uk.co.thefoundry.furnace") ) {
        //Furnace plug-ins are bugged and do not render properly both planes, just wipe the image.
        useCache = false;
    }
    const ImagePtr & img = firstPlane.fullscaleImage->usesBitMap() ? firstPlane.fullscaleImage : firstPlane.downscaleImage;
    ImageParamsPtr params = img->getParams();
    EffectInstance::PlaneToRender p;
    bool ok = allocateImagePlane(img->getKey(),
                                 tls->currentRenderArgs.rod,
                                 tls->currentRenderArgs.renderWindowPixel,
                                 tls->currentRenderArgs.renderWindowPixel,
                                 false /*isProjectFormat*/,
                                 plane,
                                 img->getBitDepth(),
                                 img->getPremultiplication(),
                                 img->getFieldingOrder(),
                                 img->getPixelAspectRatio(),
                                 img->getMipMapLevel(),
                                 false,
                                 img->getParams()->getStorageInfo().mode,
                                 useCache,
                                 &p.fullscaleImage,
                                 &p.downscaleImage);
    if (!ok) {
        return ImagePtr();
    } else {
        p.renderMappedImage = p.downscaleImage;
        p.isAllocatedOnTheFly = true;

        /*
         * Allocate a temporary image for rendering only if using cache
         */
        if (useCache) {
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
            p.tmpImage.reset( new Image(p.renderMappedImage->getComponents(),
                                        p.renderMappedImage->getRoD(),
                                        tls->currentRenderArgs.renderWindowPixel,
                                        p.renderMappedImage->getMipMapLevel(),
                                        p.renderMappedImage->getPixelAspectRatio(),
                                        p.renderMappedImage->getBitDepth(),
                                        p.renderMappedImage->getPremultiplication(),
                                        p.renderMappedImage->getFieldingOrder(),
                                        false /*useBitmap*/,
                                        img->getParams()->getStorageInfo().mode) );
#else
            p.tmpImage = boost::make_shared<Image>(p.renderMappedImage->getComponents(),
                                                   p.renderMappedImage->getRoD(),
                                                   tls->currentRenderArgs.renderWindowPixel,
                                                   p.renderMappedImage->getMipMapLevel(),
                                                   p.renderMappedImage->getPixelAspectRatio(),
                                                   p.renderMappedImage->getBitDepth(),
                                                   p.renderMappedImage->getPremultiplication(),
                                                   p.renderMappedImage->getFieldingOrder(),
                                                   false /*useBitmap*/,
                                                   img->getParams()->getStorageInfo().mode);
#endif
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
    const std::vector<KnobIPtr> & knobs = getKnobs();

    for (U32 i = 0; i < knobs.size(); ++i) {
        if ( knobs[i]->typeName() == KnobFile::typeNameStatic() ) {
            KnobFilePtr fk = boost::dynamic_pointer_cast<KnobFile>(knobs[i]);
            assert(fk);
            if ( fk->isInputImageFile() ) {
                std::string file = fk->getValue();
                if ( file.empty() ) {
                    fk->open_file();
                }
                break;
            }
        } else if ( knobs[i]->typeName() == KnobOutputFile::typeNameStatic() ) {
            KnobOutputFilePtr fk = boost::dynamic_pointer_cast<KnobOutputFile>(knobs[i]);
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
EffectInstance::onSignificantEvaluateAboutToBeCalled(KnobI* knob)
{
    //We changed, abort any ongoing current render to refresh them with a newer version
    abortAnyEvaluation();

    NodePtr node = getNode();
    if ( !node->isNodeCreated() ) {
        return;
    }

    bool isMT = QThread::currentThread() == qApp->thread();

    if ( isMT && ( !knob || knob->getEvaluateOnChange() ) ) {
        getApp()->triggerAutoSave();
    }


    if (isMT) {
        node->refreshIdentityState();

        //Increments the knobs age following a change
        node->incrementKnobsAge();
    }
}

void
EffectInstance::evaluate(bool isSignificant,
                         bool refreshMetadatas)
{
    NodePtr node = getNode();

    if ( refreshMetadatas && node->isNodeCreated() ) {
        refreshMetadata_public(true);
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
    if (isSignificant) {
        node->refreshPreviewsRecursivelyDownstream(time);
    }
} // evaluate

bool
EffectInstance::message(MessageTypeEnum type,
                        const std::string & content) const
{
    NodePtr node = getNode();
    assert(node);
    return node ? node->message(type, content) : false;
}

void
EffectInstance::setPersistentMessage(MessageTypeEnum type,
                                     const std::string & content)
{
    NodePtr node = getNode();
    assert(node);
    if (node) {
        node->setPersistentMessage(type, content);
    }
}

bool
EffectInstance::hasPersistentMessage()
{
    NodePtr node = getNode();
    assert(node);
    return node ? node->hasPersistentMessage() : false;
}

void
EffectInstance::clearPersistentMessage(bool recurse)
{
    NodePtr node = getNode();
    assert(node);
    if (node) {
        node->clearPersistentMessage(recurse);
    }
    assert( !hasPersistentMessage() );
}

int
EffectInstance::getInputNumber(const EffectInstance* inputEffect) const
{
    for (int i = 0; i < getNInputs(); ++i) {
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
            KnobOutputFilePtr fk = boost::dynamic_pointer_cast<KnobOutputFile>(knobs[i]);
            assert(fk);
            if ( fk->isOutputImageFile() ) {
                fk->setValue(pattern);
                break;
            }
        }
    }
}

PluginMemoryPtr
EffectInstance::newMemoryInstance(size_t nBytes)
{
    PluginMemoryPtr ret = boost::make_shared<PluginMemory>( shared_from_this() ); //< hack to get "this" as a shared ptr

    addPluginMemoryPointer(ret);
    bool wasntLocked = ret->alloc(nBytes);

    assert(wasntLocked);
    Q_UNUSED(wasntLocked);

    return ret;
}

void
EffectInstance::addPluginMemoryPointer(const PluginMemoryPtr& mem)
{
    QMutexLocker l(&_imp->pluginMemoryChunksMutex);

    _imp->pluginMemoryChunks.push_back(mem);
}

void
EffectInstance::removePluginMemoryPointer(const PluginMemory* mem)
{
    std::list<PluginMemoryPtr> safeCopy;

    {
        QMutexLocker l(&_imp->pluginMemoryChunksMutex);
        // make a copy of the list so that elements don't get deleted while the mutex is held

        for (std::list<PluginMemoryWPtr>::iterator it = _imp->pluginMemoryChunks.begin(); it != _imp->pluginMemoryChunks.end(); ++it) {
            PluginMemoryPtr p = it->lock();
            if (!p) {
                continue;
            }
            safeCopy.push_back(p);
            if (p.get() == mem) {
                _imp->pluginMemoryChunks.erase(it);

                return;
            }
        }
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
EffectInstance::onKnobSlaved(const KnobIPtr& slave,
                             const KnobIPtr& master,
                             int dimension,
                             bool isSlave)
{
    getNode()->onKnobSlaved(slave, master, dimension, isSlave);
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
EffectInstance::setDoingInteractAction(bool doing)
{
    _imp->setDuringInteractAction(doing);
}

void
EffectInstance::drawOverlay_public(double time,
                                   const RenderScale & renderScale,
                                   ViewIdx view)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() && !getNode()->hasHostOverlay() ) {
        return;
    }

    RECURSIVE_ACTION();

    RenderScale actualScale;
    if ( !canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    _imp->setDuringInteractAction(true);
    bool drawHostOverlay = shouldDrawHostOverlay();
    drawOverlay(time, actualScale, view);
    if (drawHostOverlay) {
        getNode()->drawHostOverlay(time, actualScale, view);
    }
    _imp->setDuringInteractAction(false);
}

bool
EffectInstance::onOverlayPenDown_public(double time,
                                        const RenderScale & renderScale,
                                        ViewIdx view,
                                        const QPointF & viewportPos,
                                        const QPointF & pos,
                                        double pressure,
                                        double timestamp,
                                        PenType pen)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasHostOverlay() ) {
        return false;
    }

    RenderScale actualScale;
    if ( !canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        bool drawHostOverlay = shouldDrawHostOverlay();
        if (!shouldPreferPluginOverlayOverHostOverlay()) {
            ret = drawHostOverlay ? getNode()->onOverlayPenDownDefault(time, actualScale, view, viewportPos, pos, pressure) : false;
            if (!ret) {
                ret |= onOverlayPenDown(time, actualScale, view, viewportPos, pos, pressure, timestamp, pen);
            }
        } else {
            ret = onOverlayPenDown(time, actualScale, view, viewportPos, pos, pressure, timestamp, pen);
            if (!ret && drawHostOverlay) {
                ret |= getNode()->onOverlayPenDownDefault(time, actualScale, view, viewportPos, pos, pressure);
            }
        }

        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayPenDoubleClicked_public(double time,
                                                 const RenderScale & renderScale,
                                                 ViewIdx view,
                                                 const QPointF & viewportPos,
                                                 const QPointF & pos)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasHostOverlay() ) {
        return false;
    }

    RenderScale actualScale;
    if ( !canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        bool drawHostOverlay = shouldDrawHostOverlay();
        if (!shouldPreferPluginOverlayOverHostOverlay()) {
            ret = drawHostOverlay ? getNode()->onOverlayPenDoubleClickedDefault(time, actualScale, view, viewportPos, pos) : false;
            if (!ret) {
                ret |= onOverlayPenDoubleClicked(time, actualScale, view, viewportPos, pos);
            }
        } else {
            ret = onOverlayPenDoubleClicked(time, actualScale, view, viewportPos, pos);
            if (!ret && drawHostOverlay) {
                ret |= getNode()->onOverlayPenDoubleClickedDefault(time, actualScale, view, viewportPos, pos);
            }
        }

        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayPenMotion_public(double time,
                                          const RenderScale & renderScale,
                                          ViewIdx view,
                                          const QPointF & viewportPos,
                                          const QPointF & pos,
                                          double pressure,
                                          double timestamp)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasHostOverlay() ) {
        return false;
    }

    RenderScale actualScale;
    if ( !canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }


    NON_RECURSIVE_ACTION();
    _imp->setDuringInteractAction(true);
    bool ret;
    bool drawHostOverlay = shouldDrawHostOverlay();
    if (!shouldPreferPluginOverlayOverHostOverlay()) {
        ret = drawHostOverlay ? getNode()->onOverlayPenMotionDefault(time, actualScale, view, viewportPos, pos, pressure) : false;
        if (!ret) {
            ret |= onOverlayPenMotion(time, actualScale, view, viewportPos, pos, pressure, timestamp);
        }
    } else {
        ret = onOverlayPenMotion(time, actualScale, view, viewportPos, pos, pressure, timestamp);
        if (!ret && drawHostOverlay) {
            ret |= getNode()->onOverlayPenMotionDefault(time, actualScale, view, viewportPos, pos, pressure);
        }
    }

    _imp->setDuringInteractAction(false);
    //Don't check if render is needed on pen motion, wait for the pen up

    //checkIfRenderNeeded();
    return ret;
}

bool
EffectInstance::onOverlayPenUp_public(double time,
                                      const RenderScale & renderScale,
                                      ViewIdx view,
                                      const QPointF & viewportPos,
                                      const QPointF & pos,
                                      double pressure,
                                      double timestamp)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasHostOverlay() ) {
        return false;
    }

    RenderScale actualScale;
    if ( !canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        bool drawHostOverlay = shouldDrawHostOverlay();
        if (!shouldPreferPluginOverlayOverHostOverlay()) {
            ret = drawHostOverlay ? getNode()->onOverlayPenUpDefault(time, actualScale, view, viewportPos, pos, pressure) : false;
            if (!ret) {
                ret |= onOverlayPenUp(time, actualScale, view, viewportPos, pos, pressure, timestamp);
            }
        } else {
            ret = onOverlayPenUp(time, actualScale, view, viewportPos, pos, pressure, timestamp);
            if (!ret && drawHostOverlay) {
                ret |= getNode()->onOverlayPenUpDefault(time, actualScale, view, viewportPos, pos, pressure);
            }
        }

        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayKeyDown_public(double time,
                                        const RenderScale & renderScale,
                                        ViewIdx view,
                                        Key key,
                                        KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasHostOverlay() ) {
        return false;
    }

    RenderScale actualScale;
    if ( !canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }


    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayKeyDown(time, actualScale, view, key, modifiers);
        if (!ret && shouldDrawHostOverlay()) {
            ret |= getNode()->onOverlayKeyDownDefault(time, actualScale, view, key, modifiers);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayKeyUp_public(double time,
                                      const RenderScale & renderScale,
                                      ViewIdx view,
                                      Key key,
                                      KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasHostOverlay() ) {
        return false;
    }

    RenderScale actualScale;
    if ( !canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    bool ret;
    {
        NON_RECURSIVE_ACTION();

        _imp->setDuringInteractAction(true);
        ret = onOverlayKeyUp(time, actualScale, view, key, modifiers);
        if (!ret && shouldDrawHostOverlay()) {
            ret |= getNode()->onOverlayKeyUpDefault(time, actualScale, view, key, modifiers);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayKeyRepeat_public(double time,
                                          const RenderScale & renderScale,
                                          ViewIdx view,
                                          Key key,
                                          KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasHostOverlay() ) {
        return false;
    }

    RenderScale actualScale;
    if ( !canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayKeyRepeat(time, actualScale, view, key, modifiers);
        if (!ret && shouldDrawHostOverlay()) {
            ret |= getNode()->onOverlayKeyRepeatDefault(time, actualScale, view, key, modifiers);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayFocusGained_public(double time,
                                            const RenderScale & renderScale,
                                            ViewIdx view)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() && !getNode()->hasHostOverlay() ) {
        return false;
    }

    RenderScale actualScale;
    if ( !canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayFocusGained(time, actualScale, view);
        if (shouldDrawHostOverlay()) {
            ret |= getNode()->onOverlayFocusGainedDefault(time, actualScale, view);
        }

        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayFocusLost_public(double time,
                                          const RenderScale & renderScale,
                                          ViewIdx view)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() && !getNode()->hasHostOverlay() ) {
        return false;
    }

    RenderScale actualScale;
    if ( !canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }


    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayFocusLost(time, actualScale, view);
        if (shouldDrawHostOverlay()) {
            ret |= getNode()->onOverlayFocusLostDefault(time, actualScale, view);
        }

        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
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
    QReadLocker l(&_imp->duringInteractActionMutex);

    return _imp->duringInteractAction;
}

StatusEnum
EffectInstance::render_public(const RenderActionArgs & args)
{
    NON_RECURSIVE_ACTION();
    REPORT_CURRENT_THREAD_ACTION( kOfxImageEffectActionRender, getNode() );

    return render(args);
}

StatusEnum
EffectInstance::getTransform_public(double time,
                                    const RenderScale & renderScale,
                                    bool draftRender,
                                    ViewIdx view,
                                    EffectInstancePtr* inputToTransform,
                                    Transform::Matrix3x3* transform)
{
    RECURSIVE_ACTION();
    //assert( getNode()->getCurrentCanTransform() ); // called in every case for overlays

    return getTransform(time, renderScale, draftRender, view, inputToTransform, transform);
}

bool
EffectInstance::isIdentity_public(bool useIdentityCache, // only set to true when calling for the whole image (not for a subrect)
                                  U64 hash,
                                  double time,
                                  const RenderScale & scale,
                                  const RectI & renderWindow,
                                  ViewIdx view,
                                  double* inputTime,
                                  ViewIdx* inputView,
                                  int* inputNb)
{
    //assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(scale.x == 1. && scale.y == 1.) ) );

    if (useIdentityCache) {
        double timeF = 0.;
        bool foundInCache = _imp->actionsCache->getIdentityResult(hash, time, view, inputNb, inputView, &timeF);
        if (foundInCache) {
            *inputTime = timeF;

            return *inputNb >= 0 || *inputNb == -2;
        }
    }


    ///EDIT: We now allow isIdentity to be called recursively.
    RECURSIVE_ACTION();


    bool ret = false;
    RotoDrawableItemPtr rotoItem = getNode()->getAttachedRotoItem();
    if ( ( rotoItem && !rotoItem->isActivated(time) ) || getNode()->isNodeDisabled() || !getNode()->hasAtLeastOneChannelToProcess() ) {
        ret = true;
        *inputNb = getNode()->getPreferredInput();
        *inputTime = time;
        *inputView = view;
    } else if ( appPTR->isBackground() && (dynamic_cast<DiskCacheNode*>(this) != NULL) ) {
        ret = true;
        *inputNb = 0;
        *inputTime = time;
        *inputView = view;
    } else {
        /// Don't call isIdentity if plugin is sequential only.
        if (getSequentialPreference() != eSequentialPreferenceOnlySequential) {
            try {
                *inputView = view;
                ret = isIdentity(time, scale, renderWindow, view, inputTime, inputView, inputNb);
            } catch (...) {
                throw;
            }
        }
    }
    if (!ret) {
        *inputNb = -1;
        *inputTime = time;
        *inputView = view;
    }

    if (useIdentityCache) {
        _imp->actionsCache->setIdentityResult(hash, time, view, *inputNb, *inputView, *inputTime);
    }

    return ret;
} // EffectInstance::isIdentity_public

void
EffectInstance::onInputChanged(int /*inputNo*/)
{
}

StatusEnum
EffectInstance::getRegionOfDefinitionFromCache(U64 hash,
                                               double time,
                                               const RenderScale & scale,
                                               ViewIdx view,
                                               RectD* rod,
                                               bool* isProjectFormat)
{
    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    bool foundInCache = _imp->actionsCache->getRoDResult(hash, time, view, mipMapLevel, rod);

    if (foundInCache) {
        if (isProjectFormat) {
            *isProjectFormat = false;
        }
        if ( rod->isNull() ) {
            return eStatusFailed;
        }

        return eStatusOK;
    }

    return eStatusFailed;
}

StatusEnum
EffectInstance::getRegionOfDefinition_public(U64 hash,
                                             double time,
                                             const RenderScale & scale,
                                             ViewIdx view,
                                             RectD* rod,
                                             bool* isProjectFormat)
{
    if ( !isEffectCreated() ) {
        return eStatusFailed;
    }

    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    bool foundInCache = _imp->actionsCache->getRoDResult(hash, time, view, mipMapLevel, rod);
    if (foundInCache) {
        if (isProjectFormat) {
            *isProjectFormat = false;
        }
#pragma message WARN("[FD] why is an empty RoD a failure case? this is ignored in renderRoI, search for 'if getRoD fails, this might be because the RoD is null after all (e.g: an empty Roto node), we don't want the render to fail'")
        if ( rod->isNull() ) {
            return eStatusFailed;
        }

        return eStatusOK;
    } else {
        ///If this is running on a render thread, attempt to find the RoD in the thread-local storage.

        if ( QThread::currentThread() != qApp->thread() ) {
            EffectTLSDataPtr tls = _imp->tlsData->getTLSData();
            if (tls && tls->currentRenderArgs.validArgs) {
                *rod = tls->currentRenderArgs.rod;
                if (isProjectFormat) {
                    *isProjectFormat = false;
                }

                return eStatusOK;
            }
        }

        if ( getNode()->isNodeDisabled() ) {
            NodePtr preferredInput = getNode()->getPreferredInputNode();
            if (!preferredInput) {
                return eStatusFailed;
            }

            return preferredInput->getEffectInstance()->getRegionOfDefinition_public(preferredInput->getEffectInstance()->getRenderHash(), time, scale, view, rod, isProjectFormat);
        }

        StatusEnum ret;
        RenderScale scaleOne(1.);
        {
            RECURSIVE_ACTION();


            ret = getRegionOfDefinition(hash, time, supportsRenderScaleMaybe() == eSupportsNo ? scaleOne : scale, view, rod);

            if ( (ret != eStatusOK) && (ret != eStatusReplyDefault) ) {
                // rod is not valid
                //if (!isDuringStrokeCreation) {
                _imp->actionsCache->invalidateAll(hash);
                _imp->actionsCache->setRoDResult( hash, time, view, mipMapLevel, RectD() );

                // }
                return ret;
            }

            if ( rod->isNull() ) {
                // RoD is empty, which means output is black and transparent
                _imp->actionsCache->setRoDResult( hash, time, view, mipMapLevel, RectD() );

                return ret;
            }

            assert( (ret == eStatusOK || ret == eStatusReplyDefault) && (rod->x1 <= rod->x2 && rod->y1 <= rod->y2) );
        }
        bool isProject = ifInfiniteApplyHeuristic(hash, time, scale, view, rod);
        if (isProjectFormat) {
            *isProjectFormat = isProject;
        }
        assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);

        //if (!isDuringStrokeCreation) {
        _imp->actionsCache->setRoDResult(hash, time, view,  mipMapLevel, *rod);

        //}
        return ret;
    }
} // EffectInstance::getRegionOfDefinition_public

void
EffectInstance::getRegionsOfInterest_public(double time,
                                            const RenderScale & scale,
                                            const RectD & outputRoD, //!< effect RoD in canonical coordinates
                                            const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                            ViewIdx view,
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
                                       ViewIdx view,
                                       unsigned int mipMapLevel)
{
    NON_RECURSIVE_ACTION();
    FramesNeededMap framesNeeded;
    bool foundInCache = _imp->actionsCache->getFramesNeededResult(hash, time, view, mipMapLevel, &framesNeeded);
    if (foundInCache) {
        return framesNeeded;
    }

    try {
        framesNeeded = getFramesNeeded(time, view);
    } catch (std::exception &e) {
        if ( !hasPersistentMessage() ) { // plugin may already have set a message
            setPersistentMessage( eMessageTypeError, e.what() );
        }
    }

    _imp->actionsCache->setFramesNeededResult(hash, time, view, mipMapLevel, framesNeeded);

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
        foundInCache = _imp->actionsCache->getTimeDomainResult(hash, &fFirst, &fLast);
    }
    if (foundInCache) {
        *first = std::floor(fFirst + 0.5);
        *last = std::floor(fLast + 0.5);
    } else {
        ///If this is running on a render thread, attempt to find the info in the thread-local storage.
        if ( QThread::currentThread() != qApp->thread() ) {
            EffectTLSDataPtr tls = _imp->tlsData->getTLSData();
            if (tls && tls->currentRenderArgs.validArgs) {
                *first = tls->currentRenderArgs.firstFrame;
                *last = tls->currentRenderArgs.lastFrame;

                return;
            }
        }

        NON_RECURSIVE_ACTION();
        getFrameRange(first, last);
        _imp->actionsCache->setTimeDomainResult(hash, *first, *last);
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
                                           ViewIdx view,
                                           bool isOpenGLRender,
                                           const EffectInstance::OpenGLContextEffectDataPtr& glContextData)
{
    NON_RECURSIVE_ACTION();
    REPORT_CURRENT_THREAD_ACTION( kOfxImageEffectActionBeginSequenceRender, getNode() );
    EffectTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
    assert(tls);
    ++tls->beginEndRenderCount;

    return beginSequenceRender(first, last, step, interactive, scale,
                               isSequentialRender, isRenderResponseToUserInteraction, draftMode, view, isOpenGLRender, glContextData);
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
                                         ViewIdx view,
                                         bool isOpenGLRender,
                                         const EffectInstance::OpenGLContextEffectDataPtr& glContextData)
{
    NON_RECURSIVE_ACTION();
    REPORT_CURRENT_THREAD_ACTION( kOfxImageEffectActionEndSequenceRender, getNode() );
    EffectTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
    assert(tls);
    --tls->beginEndRenderCount;
    assert(tls->beginEndRenderCount >= 0);

    return endSequenceRender(first, last, step, interactive, scale, isSequentialRender, isRenderResponseToUserInteraction, draftMode, view, isOpenGLRender, glContextData);
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

/**
 * @brief This function calls the implementation specific attachOpenGLContext()
 **/
StatusEnum
EffectInstance::attachOpenGLContext_public(const OSGLContextPtr& glContext,
                                           EffectInstance::OpenGLContextEffectDataPtr* data)
{
    NON_RECURSIVE_ACTION();
    bool concurrentGLRender = supportsConcurrentOpenGLRenders();
    boost::scoped_ptr<QMutexLocker> locker;
    if (concurrentGLRender) {
        locker.reset( new QMutexLocker(&_imp->attachedContextsMutex) );
    } else {
        _imp->attachedContextsMutex.lock();
    }

    std::map<OSGLContextWPtr, EffectInstance::OpenGLContextEffectDataPtr>::iterator found = _imp->attachedContexts.find(glContext);
    if ( found != _imp->attachedContexts.end() ) {
        // The context is already attached
        *data = found->second;

        return eStatusOK;
    }


    StatusEnum ret = attachOpenGLContext(data);

    if ( (ret == eStatusOK) || (ret == eStatusReplyDefault) ) {
        if (!concurrentGLRender) {
            (*data)->setHasTakenLock(true);
        }
        _imp->attachedContexts.insert( std::make_pair(glContext, *data) );
    } else {
        _imp->attachedContextsMutex.unlock();
    }

    // Take the lock until dettach is called for plug-ins that do not support concurrent GL renders
    return ret;
}

void
EffectInstance::dettachAllOpenGLContexts()
{
    QMutexLocker locker(&_imp->attachedContextsMutex);

    for (std::map<OSGLContextWPtr, EffectInstance::OpenGLContextEffectDataPtr>::iterator it = _imp->attachedContexts.begin(); it != _imp->attachedContexts.end(); ++it) {
        OSGLContextPtr context = it->first.lock();
        if (!context) {
            continue;
        }
        context->setContextCurrentNoRender();
        if (it->second.use_count() == 1) {
            // If no render is using it, dettach the context
            dettachOpenGLContext(it->second);
        }
    }
    if ( !_imp->attachedContexts.empty() ) {
        OSGLContext::unsetCurrentContextNoRender();
    }
    _imp->attachedContexts.clear();
}

/**
 * @brief This function calls the implementation specific dettachOpenGLContext()
 **/
StatusEnum
EffectInstance::dettachOpenGLContext_public(const OSGLContextPtr& glContext, const EffectInstance::OpenGLContextEffectDataPtr& data)
{
    NON_RECURSIVE_ACTION();
    bool concurrentGLRender = supportsConcurrentOpenGLRenders();
    boost::scoped_ptr<QMutexLocker> locker;
    if (concurrentGLRender) {
        locker.reset( new QMutexLocker(&_imp->attachedContextsMutex) );
    }


    bool mustUnlock = data->getHasTakenLock();
    std::map<OSGLContextWPtr, EffectInstance::OpenGLContextEffectDataPtr>::iterator found = _imp->attachedContexts.find(glContext);
    if ( found != _imp->attachedContexts.end() ) {
        _imp->attachedContexts.erase(found);
    }

    StatusEnum ret = dettachOpenGLContext(data);
    if (mustUnlock) {
        _imp->attachedContextsMutex.unlock();
    }

    return ret;
}

bool
EffectInstance::isSupportedComponent(int inputNb,
                                     const ImagePlaneDesc & comp) const
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

ImagePlaneDesc
EffectInstance::findClosestSupportedComponents(int inputNb,
                                               const ImagePlaneDesc & comp) const
{
    return getNode()->findClosestSupportedComponents(inputNb, comp);
}

void
EffectInstance::clearActionsCache()
{
    _imp->actionsCache->clearAll();
}



void
EffectInstance::getComponentsNeededAndProduced(double time,
                                               ViewIdx view,
                                               EffectInstance::ComponentsNeededMap* comps,
                                               double* passThroughTime,
                                               int* passThroughView,
                                               int* passThroughInputNb)
{
    bool processAllRequested;
    std::bitset<4> processChannels;
    std::list<ImagePlaneDesc> passThroughPlanes;
    getComponentsNeededDefault(time, view, comps, &passThroughPlanes,  &processAllRequested, passThroughTime, passThroughView, &processChannels, passThroughInputNb);

}

void
EffectInstance::getComponentsNeededDefault(double time, ViewIdx view,
                                           EffectInstance::ComponentsNeededMap* comps,
                                           std::list<ImagePlaneDesc>* passThroughPlanes,
                                           bool* processAllRequested,
                                           double* passThroughTime,
                                           int* passThroughView,
                                           std::bitset<4> *processChannels,
                                           int* passThroughInputNb)
{
    *passThroughTime = time;
    *passThroughView = view;
    *passThroughInputNb = getNode()->getPreferredInput();
    *processAllRequested = false;

    {
        std::list<ImagePlaneDesc> upstreamAvailableLayers;
        if (*passThroughInputNb != -1) {
            getAvailableLayers(time, view, *passThroughInputNb, &upstreamAvailableLayers);
        }

        // upstreamAvailableLayers now contain all available planes in input of this node
        *passThroughPlanes = upstreamAvailableLayers;

    }

 
    // Get the output needed components
    {

        std::vector<ImagePlaneDesc> clipPrefsAllComps;

        // The clipPrefsComps is the number of components desired by the plug-in in the
        // getTimeInvariantMetadatas action (getClipPreferences for OpenFX) mapped to the
        // color-plane.
        //
        // There's a special case for a plug-in that requests a 2 component image:
        // OpenFX does not support 2-component images by default. 2 types of plug-in
        // may request such images:
        // - non multi-planar effect that supports 2 component images, added with the Natron OpenFX extensions
        // - multi-planar effect that supports The Foundry Furnace plug-in suite: the value returned is either
        // disparity components or a motion vector components.
        //
        ImagePlaneDesc metadataPlane, metadataPairedPlane;
        getMetadataComponents(-1, &metadataPlane, &metadataPairedPlane);
        // Some plug-ins, such as The Foundry Furnace set the meta-data to disparity/motion vector, requiring
        // both planes to be computed at once (Forward/Backard for motion vector) (Left/Right for Disparity)
        if (metadataPlane.getNumComponents() > 0) {
            clipPrefsAllComps.push_back(metadataPlane);
        }
        if (metadataPairedPlane.getNumComponents() > 0) {
            clipPrefsAllComps.push_back(metadataPairedPlane);
        }
        if (clipPrefsAllComps.empty()) {
            // If metada are not set yet, at least append RGBA
            clipPrefsAllComps.push_back(ImagePlaneDesc::getRGBAComponents());
        }

        // Natron adds for all non multi-planar effects a default layer selector to emulate
        // multi-plane even if the plug-in is not aware of it. When calling getImagePlanes(), the
        // plug-in will receive this user-selected plane, mapped to the number of components indicated
        // by the plug-in in getTimeInvariantMetadatas
        ImagePlaneDesc layer;
        bool gotUserSelectedPlane;
        {
            // In output, the available layers are those pass-through the input + project layers +
            // layers produced by this node
            std::list<ImagePlaneDesc> availableLayersInOutput = *passThroughPlanes;
            availableLayersInOutput.insert(availableLayersInOutput.end(), clipPrefsAllComps.begin(), clipPrefsAllComps.end());

            {
                std::list<ImagePlaneDesc> projectLayers = getApp()->getProject()->getProjectDefaultLayers();
                mergeLayersList(projectLayers, &availableLayersInOutput);
            }

            {
                std::list<ImagePlaneDesc> userCreatedLayers;
                getNode()->getUserCreatedComponents(&userCreatedLayers);
                mergeLayersList(userCreatedLayers, &availableLayersInOutput);
            }

            gotUserSelectedPlane = getNode()->getSelectedLayer(-1, availableLayersInOutput, processChannels, processAllRequested, &layer);
        }

        // If the user did not select any components or the layer is the color-plane, fallback on
        // meta-data color plane
        if (layer.getNumComponents() == 0 || layer.isColorPlane()) {
            gotUserSelectedPlane = false;
        }

        std::list<ImagePlaneDesc> &componentsSet = (*comps)[-1];

        if (gotUserSelectedPlane) {
            componentsSet.push_back(layer);
        } else {
            componentsSet.insert( componentsSet.end(), clipPrefsAllComps.begin(), clipPrefsAllComps.end() );
        }

    }

    // For each input get their needed components
    int maxInput = getNInputs();
    for (int i = 0; i < maxInput; ++i) {

        std::list<ImagePlaneDesc> upstreamAvailableLayers;
        getAvailableLayers(time, view, i, &upstreamAvailableLayers);


        std::list<ImagePlaneDesc> &componentsSet = (*comps)[i];

        // Get the selected layer from the source channels menu
        std::bitset<4> inputProcChannels;
        ImagePlaneDesc layer;
        bool isAll;
        bool ok = getNode()->getSelectedLayer(i, upstreamAvailableLayers, &inputProcChannels, &isAll, &layer);

        // When color plane or all choice then request the default metadata components
        if (isAll || layer.isColorPlane()) {
            ok = false;
        }

        // For a mask get its selected channel
        ImagePlaneDesc maskComp;
        int channelMask = getNode()->getMaskChannel(i, upstreamAvailableLayers, &maskComp);


        std::vector<ImagePlaneDesc> clipPrefsAllComps;
        {
            ImagePlaneDesc metadataPlane, metadataPairedPlane;
            getMetadataComponents(i, &metadataPlane, &metadataPairedPlane);

            // Some plug-ins, such as The Foundry Furnace set the meta-data to disparity/motion vector, requiring
            // both planes to be computed at once (Forward/Backard for motion vector) (Left/Right for Disparity)
            if (metadataPlane.getNumComponents() > 0) {
                clipPrefsAllComps.push_back(metadataPlane);
            }
            if (metadataPairedPlane.getNumComponents() > 0) {
                clipPrefsAllComps.push_back(metadataPairedPlane);
            }
            if (clipPrefsAllComps.empty()) {
                // If metada are not set yet, at least append RGBA
                clipPrefsAllComps.push_back(ImagePlaneDesc::getRGBAComponents());
            }
        }

        if ( (channelMask != -1) && (maskComp.getNumComponents() > 0) ) {

            // If this is a mask, ask for the selected mask layer
            componentsSet.push_back(maskComp);

        } else if (ok && layer.getNumComponents() > 0) {
            componentsSet.push_back(layer);
        } else {
            //Use regular clip preferences
            componentsSet.insert( componentsSet.end(), clipPrefsAllComps.begin(), clipPrefsAllComps.end() );
        }
        
    } // for each input
}

void
EffectInstance::getComponentsNeededAndProduced_public(U64 hash,
                                                      double time,
                                                      ViewIdx view,
                                                      EffectInstance::ComponentsNeededMap* comps,
                                                      std::list<ImagePlaneDesc>* passThroughPlanes,
                                                      bool* processAllRequested,
                                                      double* passThroughTime,
                                                      int* passThroughView,
                                                      std::bitset<4> *processChannels,
                                                      int* passThroughInputNb)

{
    RECURSIVE_ACTION();

    {
        ViewIdx ptView;
        bool foundInCache = _imp->actionsCache->getComponentsNeededResults(hash, time, view, comps, processChannels, processAllRequested, passThroughPlanes, passThroughInputNb, &ptView, passThroughTime);
        if (foundInCache) {
            *passThroughView = ptView;
            return;
        }
    }
    

    if ( !isMultiPlanar() ) {
        getComponentsNeededDefault(time, view, comps, passThroughPlanes, processAllRequested, passThroughTime, passThroughView, processChannels, passThroughInputNb);
        _imp->actionsCache->setComponentsNeededResults(hash, time, view, *comps, *processChannels, *processAllRequested, *passThroughPlanes, *passThroughInputNb, ViewIdx(*passThroughView), *passThroughTime);
        return;
    }


    // call the getClipComponents action

    getComponentsNeededAndProduced(time, view, comps, passThroughTime, passThroughView, passThroughInputNb);


    // upstreamAvailableLayers now contain all available planes in input of this node
    // Remove from this list all layers produced from this node to get the pass-through planes list
    std::list<ImagePlaneDesc>& outputLayers = (*comps)[-1];

    // Ensure the plug-in made the metadata plane available.
    {
        std::list<ImagePlaneDesc> metadataPlanes;
        ImagePlaneDesc metadataPlane, metadataPairedPlane;
        getMetadataComponents(-1, &metadataPlane, &metadataPairedPlane);
        if (metadataPairedPlane.getNumComponents() > 0) {
            metadataPlanes.push_back(metadataPairedPlane);
        }
        if (metadataPlane.getNumComponents() > 0) {
            metadataPlanes.push_back(metadataPlane);
        }
        mergeLayersList(metadataPlanes, &outputLayers);
    }


    // If the plug-in does not block upstream planes, recurse up-stream on the pass-through input to get available components.
    PassThroughEnum passThrough = isPassThroughForNonRenderedPlanes();
    if (*passThroughInputNb != -1 && ( (passThrough == ePassThroughPassThroughNonRenderedPlanes) ||
        ( passThrough == ePassThroughRenderAllRequestedPlanes)) ) {

        std::list<ImagePlaneDesc> upstreamAvailableLayers;
        getAvailableLayers(time, view, *passThroughInputNb, &upstreamAvailableLayers);


        removeFromLayersList(outputLayers, &upstreamAvailableLayers);

        *passThroughPlanes = upstreamAvailableLayers;

    } // if pass-through for planes



    for (int i = 0; i < 4; ++i) {
        (*processChannels)[i] = getNode()->getProcessChannel(i);
    }
    
    *processAllRequested = false;

    _imp->actionsCache->setComponentsNeededResults(hash, time, view, *comps, *processChannels, *processAllRequested, *passThroughPlanes, *passThroughInputNb, ViewIdx(*passThroughView), *passThroughTime);

} // EffectInstance::getComponentsNeededAndProduced_public


void
EffectInstance::getAvailableLayers(double time, ViewIdx view, int inputNb, std::list<ImagePlaneDesc>* availableLayers)
{

    EffectInstancePtr effect;
    if (inputNb >= 0) {
        effect = getInput(inputNb);
    } else {
        effect = shared_from_this();
    }
    if (!effect) {
        return;
    }


    std::list<ImagePlaneDesc> passThroughLayers;
    {


        EffectInstance::ComponentsNeededMap comps;
        double passThroughTime = 0.;
        int passThroughView = 0;
        int passThroughInputNb = -1; // prevent infinite recursion, because getComponentsNeededAndProduced_public() may call getAvailableLayers()
        std::bitset<4> processChannels;
        bool processAll = false;
        effect->getComponentsNeededAndProduced_public(getRenderHash(), time, view, &comps, &passThroughLayers, &processAll, &passThroughTime, &passThroughView, &processChannels, &passThroughInputNb);

        // Merge pass-through planes produced + pass-through available planes and make it as the pass-through planes for this node
        // if they are not produced by this node
        std::list<ImagePlaneDesc>& outputLayers = (comps)[-1];
        mergeLayersList(outputLayers, &passThroughLayers);
    }

    // Ensure the color layer is always the first one available in the list
    bool hasColorPlane = false;
    for (std::list<ImagePlaneDesc>::iterator it = passThroughLayers.begin(); it != passThroughLayers.end(); ++it) {
        if (it->isColorPlane()) {
            hasColorPlane = true;
            availableLayers->push_front(*it);
            passThroughLayers.erase(it);
            break;
        }
    }

    // In output, also make available the default project layers and the user created components
    if (inputNb == -1) {

        std::list<ImagePlaneDesc> projectLayers = getApp()->getProject()->getProjectDefaultLayers();
        if (hasColorPlane) {
            // Don't add the color plane from the default alyers if already present
            for (std::list<ImagePlaneDesc>::iterator it = projectLayers.begin(); it != projectLayers.end(); ++it) {
                if (it->isColorPlane()) {
                    projectLayers.erase(it);
                    break;
                }
            }
        }
        mergeLayersList(projectLayers, availableLayers);
    }

    mergeLayersList(passThroughLayers, availableLayers);

     {
        std::list<ImagePlaneDesc> userCreatedLayers;
        getNode()->getUserCreatedComponents(&userCreatedLayers);
        mergeLayersList(userCreatedLayers, availableLayers);
    }
    
} // getAvailableLayers

bool
EffectInstance::getCreateChannelSelectorKnob() const
{
    return ( !isMultiPlanar() && !isReader() && !isWriter() && !isTrackerNodePlugin() &&
             !boost::starts_with(getPluginID(), "uk.co.thefoundry.furnace") );
}

int
EffectInstance::getMaskChannel(int inputNb, const std::list<ImagePlaneDesc>& availableLayers, ImagePlaneDesc* comps) const
{
    return getNode()->getMaskChannel(inputNb, availableLayers, comps);
}

bool
EffectInstance::isMaskEnabled(int inputNb) const
{
    return getNode()->isMaskEnabled(inputNb);
}

bool
EffectInstance::onKnobValueChanged(KnobI* /*k*/,
                                   ValueChangedReasonEnum /*reason*/,
                                   double /*time*/,
                                   ViewSpec /*view*/,
                                   bool /*originatedFromMainThread*/)
{
    return false;
}

bool
EffectInstance::getThreadLocalRenderedPlanes(std::map<ImagePlaneDesc, EffectInstance::PlaneToRender> *outputPlanes,
                                             ImagePlaneDesc* planeBeingRendered,
                                             RectI* renderWindow) const
{
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();

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
EffectInstance::getThreadLocalNeededComponents(ComponentsNeededMapPtr* neededComps) const
{
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();

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
    if ( QThread::currentThread() != qApp->thread() ) {
        EffectTLSDataPtr tls = _imp->tlsData->getTLSData();
        if (tls && tls->currentRenderArgs.validArgs) {
            tls->currentRenderArgs.time = time;
        }
    }
}

bool
EffectInstance::isDuringPaintStrokeCreationThreadLocal() const
{
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();

    if ( tls && !tls->frameArgs.empty() ) {
        return tls->frameArgs.back()->isDuringPaintStrokeCreation;
    }

    return getNode()->isDuringPaintStrokeCreation();
}

void
EffectInstance::redrawOverlayInteract()
{
    if ( isDoingInteractAction() ) {
        getApp()->queueRedrawForAllViewers();
    } else {
        getApp()->redrawAllViewers();
    }
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
EffectInstance::addOverlaySlaveParam(const KnobIPtr& knob)
{
    _imp->overlaySlaves.push_back(knob);
}

bool
EffectInstance::isOverlaySlaveParam(const KnobI* knob) const
{
    for (std::list<KnobIWPtr>::const_iterator it = _imp->overlaySlaves.begin(); it != _imp->overlaySlaves.end(); ++it) {
        KnobIPtr k = it->lock();
        if (!k) {
            continue;
        }
        if (k.get() == knob) {
            return true;
        }
    }

    return false;
}

bool
EffectInstance::onKnobValueChanged_public(KnobI* k,
                                          ValueChangedReasonEnum reason,
                                          double time,
                                          ViewSpec view,
                                          bool originatedFromMainThread)
{
    NodePtr node = getNode();

    ///If the param changed is a button and the node is disabled don't do anything which might
    ///trigger an analysis
    if ( (reason == eValueChangedReasonUserEdited) && dynamic_cast<KnobButton*>(k) && node->isNodeDisabled() ) {
        return false;
    }

    // for image readers, image writers, and video writers, frame range must be updated before kOfxActionInstanceChanged is called on kOfxImageEffectFileParamName
    bool mustCallOnFileNameParameterChanged = false;
    if ( (reason != eValueChangedReasonTimeChanged) && ( isReader() || isWriter() ) && k && (k->getName() == kOfxImageEffectFileParamName) ) {
        node->computeFrameRangeForReader(k);
        mustCallOnFileNameParameterChanged = true;
    }


    bool ret = false;

    // assert(!(view.isAll() || view.isCurrent())); // not yet implemented
    const ViewIdx viewIdx( ( view.isAll() || view.isCurrent() ) ? 0 : view );
    bool wasFormatKnobCaught = node->handleFormatKnob(k);
    KnobHelper* kh = dynamic_cast<KnobHelper*>(k);
    assert(kh);
    
    // While creating a PyPlug, never handle changes, but call syncPrivateData before any other action,
    // fixes https://github.com/MrKepzie/Natron/issues/1637
    if (getApp()->isCreatingPythonGroup() && kh && kh->isDeclaredByPlugin() && !wasFormatKnobCaught) {
        // must sync private data in EffectInstance::getPreferredMetadata_public()
        QMutexLocker l(&_imp->mustSyncPrivateDataMutex);
        _imp->mustSyncPrivateData = true;
    } else if (kh && kh->isDeclaredByPlugin() && !wasFormatKnobCaught) {
        ////We set the thread storage render args so that if the instance changed action
        ////tries to call getImage it can render with good parameters.
        ParallelRenderArgsSetterPtr setter;
        if (reason != eValueChangedReasonTimeChanged) {
            AbortableRenderInfoPtr abortInfo = AbortableRenderInfo::create(false, 0);
            const bool isRenderUserInteraction = true;
            const bool isSequentialRender = false;
            AbortableThread* isAbortable = dynamic_cast<AbortableThread*>( QThread::currentThread() );
            if (isAbortable) {
                isAbortable->setAbortInfo( isRenderUserInteraction, abortInfo, node->getEffectInstance() );
            }
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
            setter.reset( new ParallelRenderArgsSetter( time,
                                                        viewIdx, //view
                                                        isRenderUserInteraction, // isRenderUserInteraction
                                                        isSequentialRender, // isSequential
                                                        abortInfo, // abortInfo
                                                        node, // treeRoot
                                                        0, //texture index
                                                        getApp()->getTimeLine().get(),
                                                        NodePtr(), // activeRotoPaintNode
                                                        true, // isAnalysis
                                                        false, // draftMode
                                                        RenderStatsPtr() ) );
#else
            setter = boost::make_shared<ParallelRenderArgsSetter>( time,
                                                                  viewIdx, //view
                                                                  isRenderUserInteraction, // isRenderUserInteraction
                                                                  isSequentialRender, // isSequential
                                                                  abortInfo, // abortInfo
                                                                  node, // treeRoot
                                                                  0, //texture index
                                                                  getApp()->getTimeLine().get(),
                                                                  NodePtr(), // activeRotoPaintNode
                                                                  true, // isAnalysis
                                                                  false, // draftMode
                                                                  RenderStatsPtr() );
#endif
        }
        {
            RECURSIVE_ACTION();
            REPORT_CURRENT_THREAD_ACTION( kOfxActionInstanceChanged, getNode() );
            // Map to a plug-in known reason
            if (reason == eValueChangedReasonNatronGuiEdited) {
                reason = eValueChangedReasonUserEdited;
            } 
            ret |= knobChanged(k, reason, view, time, originatedFromMainThread);
        }
    }

    // for video readers, frame range must be updated after kOfxActionInstanceChanged is called on kOfxImageEffectFileParamName
    if (mustCallOnFileNameParameterChanged) {
        node->onFileNameParameterChanged(k);
    }

    if ( kh && ( QThread::currentThread() == qApp->thread() ) &&
         originatedFromMainThread && ( reason != eValueChangedReasonTimeChanged) ) {
        ///Run the following only in the main-thread
        if ( hasOverlay() && node->shouldDrawOverlay() && !node->hasHostOverlayForParam(k) ) {
            // Some plugins (e.g. by digital film tools) forget to set kOfxInteractPropSlaveToParam.
            // Most hosts trigger a redraw if the plugin has an active overlay.
            incrementRedrawNeededCounter();

            if ( !isDequeueingValuesSet() && (getRecursionLevel() == 0) && checkIfOverlayRedrawNeeded() ) {
                redrawOverlayInteract();
            }
        }
        if (isOverlaySlaveParam(kh)) {
            kh->redraw();
        }
    }

    ret |= node->onEffectKnobValueChanged(k, reason);

    //Don't call the python callback if the reason is time changed
    if (reason == eValueChangedReasonTimeChanged) {
        return false;
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

    // If there are any render clones, kill them as the plug-in might have changed internally
    clearRenderInstances();

    return ret;
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
EffectInstancePtr
EffectInstance::getNearestNonDisabled() const
{
    NodePtr node = getNode();

    if ( !node->isNodeDisabled() ) {
        return node->getEffectInstance();
    } else {
        ///Test all inputs recursively, going from last to first, preferring non optional inputs.
        std::list<EffectInstancePtr> nonOptionalInputs;
        std::list<EffectInstancePtr> optionalInputs;
        // the following is wrong, because the behavior of scripts or PyPlugs when rendering will then depend on the preferences!
        //bool useInputA = appPTR->getCurrentSettings()->useInputAForMergeAutoConnect() || (getPluginID() == PLUGINID_OFX_SHUFFLE && getMajorVersion() < 3);
        const bool useInputA = false || (getPluginID() == PLUGINID_OFX_SHUFFLE && getMajorVersion() < 3);

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
        int maxinputs = getNInputs();
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
            EffectInstancePtr inputRet = (*it)->getNearestNonDisabled();
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
            EffectInstancePtr inputRet = (*it)->getNearestNonDisabled();
            if (inputRet) {
                return inputRet;
            }
        }

        ///Cycle through optional inputs...
        for (std::list<EffectInstancePtr> ::iterator it = optionalInputs.begin(); it != optionalInputs.end(); ++it) {
            EffectInstancePtr inputRet = (*it)->getNearestNonDisabled();
            if (inputRet) {
                return inputRet;
            }
        }

        ///We didn't find anything upstream, return
        return node->getEffectInstance();
    }
} // EffectInstance::getNearestNonDisabled

EffectInstancePtr
EffectInstance::getNearestNonDisabledPrevious(int* inputNb)
{
    assert( getNode()->isNodeDisabled() );

    ///Test all inputs recursively, going from last to first, preferring non optional inputs.
    std::list<EffectInstancePtr> nonOptionalInputs;
    std::list<EffectInstancePtr> optionalInputs;
    int localPreferredInput = -1;
    // the following is wrong, because the behavior of scripts or PyPlugs when rendering will then depend on the preferences!
    //bool useInputA = appPTR->getCurrentSettings()->useInputAForMergeAutoConnect() || (getPluginID() == PLUGINID_OFX_SHUFFLE && getMajorVersion() < 3);
    const bool useInputA = false || (getPluginID() == PLUGINID_OFX_SHUFFLE && getMajorVersion() < 3);
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
    int maxinputs = getNInputs();
    for (int i = 0; i < maxinputs; ++i) {
        std::string inputLabel = getInputLabel(i);
        if (inputLabel == inputNameToFind) {
            EffectInstancePtr inp = getInput(i);
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
        EffectInstancePtr inp = getInput(foundOther);
        if (inp) {
            nonOptionalInputs.push_front(inp);
            localPreferredInput = foundOther;
        }
    }

    ///If we found A or B so far, cycle through them
    for (std::list<EffectInstancePtr> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
        if ( (*it)->getNode()->isNodeDisabled() ) {
            EffectInstancePtr inputRet = (*it)->getNearestNonDisabledPrevious(inputNb);
            if (inputRet) {
                return inputRet;
            }
        }
    }


    ///We cycle in reverse by default. It should be a setting of the application.
    ///In this case it will return input B instead of input A of a merge for example.
    for (int i = 0; i < maxinputs; ++i) {
        EffectInstancePtr inp = getInput(i);
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
    for (std::list<EffectInstancePtr> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
        if ( (*it)->getNode()->isNodeDisabled() ) {
            EffectInstancePtr inputRet = (*it)->getNearestNonDisabledPrevious(inputNb);
            if (inputRet) {
                return inputRet;
            }
        }
    }

    ///Cycle through optional inputs...
    for (std::list<EffectInstancePtr> ::iterator it = optionalInputs.begin(); it != optionalInputs.end(); ++it) {
        if ( (*it)->getNode()->isNodeDisabled() ) {
            EffectInstancePtr inputRet = (*it)->getNearestNonDisabledPrevious(inputNb);
            if (inputRet) {
                return inputRet;
            }
        }
    }

    *inputNb = localPreferredInput;

    return shared_from_this();
} // EffectInstance::getNearestNonDisabledPrevious

EffectInstancePtr
EffectInstance::getNearestNonIdentity(double time)
{
    U64 hash = getRenderHash();
    RenderScale scale(1.);
    Format frmt;

    getApp()->getProject()->getProjectDefaultFormat(&frmt);

    double inputTimeIdentity;
    int inputNbIdentity;
    ViewIdx inputView;
    if ( !isIdentity_public(true, hash, time, scale, frmt, ViewIdx(0), &inputTimeIdentity, &inputView, &inputNbIdentity) ) {
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
EffectInstance::onNodeHashChanged(U64 hash)
{
    ///Invalidate actions cache
    _imp->actionsCache->invalidateAll(hash);

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
EffectInstance::abortAnyEvaluation(bool keepOldestRender)
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
        for (NodesList::iterator it = inputOutputs.begin(); it != inputOutputs.end(); ++it) {
            (*it)->hasOutputNodesConnected(&outputNodes);
        }
    } else {
        RotoDrawableItemPtr attachedStroke = getNode()->getAttachedRotoItem();
        if (attachedStroke) {
            ///For nodes internal to the rotopaint tree, check outputs of the rotopaint node instead
            RotoContextPtr context = attachedStroke->getContext();
            assert(context);
            if (context) {
                NodePtr rotonode = context->getNode();
                if (rotonode) {
                    rotonode->hasOutputNodesConnected(&outputNodes);
                }
            }
        } else {
            node->hasOutputNodesConnected(&outputNodes);
        }
    }
    for (std::list<OutputEffectInstance*>::const_iterator it = outputNodes.begin(); it != outputNodes.end(); ++it) {
        //Abort and allow playback to restart but do not block, when this function returns any ongoing render may very
        //well not be finished
        if (keepOldestRender) {
            (*it)->getRenderEngine()->abortRenderingAutoRestart();
        } else {
            (*it)->getRenderEngine()->abortRenderingNoRestart(keepOldestRender);
        }
    }
}

double
EffectInstance::getCurrentTime() const
{
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();
    AppInstancePtr app = getApp();
    if (!app) {
        return 0.;
    }
    if (!tls) {
        return app->getTimeLine()->currentFrame();
    }
    if (tls->currentRenderArgs.validArgs) {
        return tls->currentRenderArgs.time;
    }


    if ( !tls->frameArgs.empty() ) {
        return tls->frameArgs.back()->time;
    }

    return app->getTimeLine()->currentFrame();
}

ViewIdx
EffectInstance::getCurrentView() const
{
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();

    if (!tls) {
        return ViewIdx(0);
    }
    if (tls->currentRenderArgs.validArgs) {
        return tls->currentRenderArgs.view;
    }
    if ( !tls->frameArgs.empty() ) {
        return tls->frameArgs.back()->view;
    }

    return ViewIdx(0);
}

SequenceTime
EffectInstance::getFrameRenderArgsCurrentTime() const
{
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();

    if ( !tls || tls->frameArgs.empty() ) {
        return getApp()->getTimeLine()->currentFrame();
    }

    return tls->frameArgs.back()->time;
}

ViewIdx
EffectInstance::getFrameRenderArgsCurrentView() const
{
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();

    if ( !tls || tls->frameArgs.empty() ) {
        return ViewIdx(0);
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
        int maxInputs = node->getNInputs();
        for (int i = 0; i < maxInputs; ++i) {
            EffectInstancePtr input = node->getInput(i);
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

StatusEnum
EffectInstance::getPreferredMetadata_public(NodeMetadata& metadata)
{
    StatusEnum stat = getDefaultMetadata(metadata);

    if (stat == eStatusFailed) {
        return stat;
    }
    if (!getNode()->isNodeDisabled()) {
        // call syncPrivateData if necessary
        bool mustSyncPrivateData;
        {
            QMutexLocker l(&_imp->mustSyncPrivateDataMutex);
            mustSyncPrivateData = _imp->mustSyncPrivateData;
            _imp->mustSyncPrivateData = false;
        }
        if (mustSyncPrivateData) {
            // for now, only OfxEffectInstance has syncPrivateData capabilities, but
            // syncPrivateData() could also be a virtual member of EffectInstance
            OfxEffectInstance* effect = dynamic_cast<OfxEffectInstance*>(this);
            if (effect) {
                assert( QThread::currentThread() == qApp->thread() );
                effect->onSyncPrivateDataRequested(); //syncPrivateData_other_thread();
            }
        }
        return getPreferredMetadata(metadata);
    }
    return stat;

}

static int
getUnmappedComponentsForInput(EffectInstance* self,
                              int inputNb,
                              const std::vector<EffectInstancePtr>& inputs,
                              int firstNonOptionalConnectedInputComps)
{
    int rawComps;

    if (inputs[inputNb]) {
        rawComps = inputs[inputNb]->getMetadataNComps(-1);
    } else {
        ///The node is not connected but optional, return the closest supported components
        ///of the first connected non optional input.
        rawComps = firstNonOptionalConnectedInputComps;
    }
    if (rawComps) {
        if (!rawComps) {
            //None comps
            return rawComps;
        } else {
            ImagePlaneDesc supportedComps = self->findClosestSupportedComponents(inputNb, ImagePlaneDesc::mapNCompsToColorPlane(rawComps)); //turn that into a comp the plugin expects on that clip
            rawComps = supportedComps.getNumComponents();
        }
    }
    if (!rawComps) {
        rawComps = 4; // default to RGBA
    }

    return rawComps;
}

StatusEnum
EffectInstance::getDefaultMetadata(NodeMetadata &metadata)
{
    NodePtr node = getNode();

    if (!node) {
        return eStatusFailed;
    }

    const bool multiBitDepth = supportsMultipleClipDepths();
    int nInputs = getNInputs();
    metadata.clearAndResize(nInputs);

    // OK find the deepest chromatic component on our input clips and the one with the
    // most components
    bool hasSetCompsAndDepth = false;
    ImageBitDepthEnum deepestBitDepth = eImageBitDepthNone;
    int mostComponents = 0;

    //Default to the project frame rate
    double frameRate = getApp()->getProjectFrameRate();
    std::vector<EffectInstancePtr> inputs(nInputs);

    // Find the components of the first non optional connected input
    // They will be used for disconnected input
    int firstNonOptionalConnectedInputComps = 0;
    for (std::size_t i = 0; i < inputs.size(); ++i) {
        inputs[i] = getInput(i);
        if ( !firstNonOptionalConnectedInputComps && inputs[i] && !isInputOptional(i) ) {
            firstNonOptionalConnectedInputComps = inputs[i]->getMetadataNComps(-1);
        }
    }

    double inputPar = 1.;
    bool inputParSet = false;
    ImagePremultiplicationEnum premult = eImagePremultiplicationOpaque;
    bool premultSet = false;
    for (int i = 0; i < nInputs; ++i) {
        const EffectInstancePtr& input = inputs[i];
        if (input) {
            frameRate = std::max( frameRate, input->getFrameRate() );
        }


        if (input) {
            if (!inputParSet) {
                inputPar = input->getAspectRatio(-1);
                inputParSet = true;
            }
        }

        int rawComp = getUnmappedComponentsForInput(this, i, inputs, firstNonOptionalConnectedInputComps);
        ImageBitDepthEnum rawDepth = input ? input->getBitDepth(-1) : eImageBitDepthFloat;
        ImagePremultiplicationEnum rawPreMult = input ? input->getPremult() : eImagePremultiplicationPremultiplied;

        // Note: first chromatic input gives the default output premult too, even if not connected
        // (else the output of generators may be opaque even if the host default is premultiplied)
        if ( ( rawComp == 4 ) && (input || !premultSet) ) {
            if (rawPreMult == eImagePremultiplicationPremultiplied) {
                premult = eImagePremultiplicationPremultiplied;
                premultSet = true;
            } else if ( (rawPreMult == eImagePremultiplicationUnPremultiplied) && ( !premultSet || (premult != eImagePremultiplicationPremultiplied) ) ) {
                premult = eImagePremultiplicationUnPremultiplied;
                premultSet = true;
            }
        }

        if (input) {
            //Update deepest bitdepth and most components only if the infos are relevant, i.e: only if the clip is connected
            hasSetCompsAndDepth = true;
            if ( getSizeOfForBitDepth(deepestBitDepth) < getSizeOfForBitDepth(rawDepth) ) {
                deepestBitDepth = rawDepth;
            }

            if ( rawComp > mostComponents ) {
                mostComponents = rawComp;
            }
        }

    } // for each input


    if (!hasSetCompsAndDepth) {
        mostComponents = 4;
        deepestBitDepth = eImageBitDepthFloat;
    }

    // set some stuff up
    metadata.setOutputFrameRate(frameRate);
    metadata.setOutputFielding(eImageFieldingOrderNone);
    metadata.setIsFrameVarying( node->hasAnimatedKnob() );
    metadata.setIsContinuous(false);

    // now find the best depth that the plugin supports
    deepestBitDepth = node->getClosestSupportedBitDepth(deepestBitDepth);

    bool multipleClipsPAR = supportsMultipleClipPARs();


    Format projectFormat;
    getApp()->getProject()->getProjectDefaultFormat(&projectFormat);
    double projectPAR = projectFormat.getPixelAspectRatio();

    RectI firstOptionalInputFormat, firstNonOptionalInputFormat;

    // Format: Take format from the first non optional input if any. Otherwise from the first optional input.
    // Otherwise fallback on project format
    bool firstOptionalInputFormatSet = false, firstNonOptionalInputFormatSet = false;

    // now add the input gubbins to the per inputs metadata
    for (int i = -1; i < (int)inputs.size(); ++i) {
        EffectInstance* effect = 0;
        if (i >= 0) {
            effect = inputs[i].get();
        } else {
            effect = this;
        }

        double par;
        if (!multipleClipsPAR) {
            par = inputParSet ? inputPar : projectPAR;
        } else {
            if (inputParSet) {
                par = inputPar;
            } else {
                par = effect ? effect->getAspectRatio(-1) : projectPAR;
            }
        }
        metadata.setPixelAspectRatio(i, par);

        bool isOptional = i >= 0 && isInputOptional(i);
        if (i >= 0) {
            if (isOptional) {
                if (!firstOptionalInputFormatSet && effect) {
                    firstOptionalInputFormat = effect->getOutputFormat();
                    firstOptionalInputFormatSet = true;
                }
            } else {
                if (!firstNonOptionalInputFormatSet && effect) {
                    firstNonOptionalInputFormat = effect->getOutputFormat();
                    firstNonOptionalInputFormatSet = true;
                }
            }
        }

        if ( (i == -1) || isOptional ) {
            // "Optional input clips can always have their component types remapped"
            // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#id482755
            ImageBitDepthEnum depth = deepestBitDepth;
            int remappedComps = mostComponents;
            remappedComps = findClosestSupportedComponents(i, ImagePlaneDesc::mapNCompsToColorPlane(remappedComps)).getNumComponents();
            metadata.setNComps(i, remappedComps);
            metadata.setComponentsType(i, kNatronColorPlaneID);
            if ( (i == -1) && !premultSet &&
                ( ( remappedComps == 4 ) || ( remappedComps == 1 ) ) ) {
                premult = eImagePremultiplicationPremultiplied;
                premultSet = true;
            }


            metadata.setBitDepth(i, depth);
        } else {

            int rawComps = getUnmappedComponentsForInput(this, i, inputs, firstNonOptionalConnectedInputComps);
            ImageBitDepthEnum rawDepth = effect ? effect->getBitDepth(-1) : eImageBitDepthFloat;

            ImageBitDepthEnum depth = multiBitDepth ? node->getClosestSupportedBitDepth(rawDepth) : deepestBitDepth;
            metadata.setBitDepth(i, depth);

            metadata.setNComps(i, rawComps);
            metadata.setComponentsType(i, kNatronColorPlaneID);
        }
    }
    
    // default to a reasonable value if there is no input
    if (!premultSet) {
        premult = eImagePremultiplicationOpaque;
    }
    // set output premultiplication
    metadata.setOutputPremult(premult);

    RectI outputFormat;

    if (firstNonOptionalInputFormatSet) {
        outputFormat = firstNonOptionalInputFormat;
    } else if (firstOptionalInputFormatSet) {
        outputFormat = firstOptionalInputFormat;
    } else {
        outputFormat = projectFormat;
    }


    metadata.setOutputFormat(outputFormat);

    return eStatusOK;
} // EffectInstance::getDefaultMetadata

RectI
EffectInstance::getOutputFormat() const
{
    QMutexLocker k(&_imp->metadataMutex);
    return _imp->metadata.getOutputFormat();
}

void
EffectInstance::getMetadataComponents(int inputNb, ImagePlaneDesc* plane, ImagePlaneDesc* pairedPlane) const
{
    int nComps;
    std::string componentsType;
    {
        QMutexLocker k(&_imp->metadataMutex);
        nComps = _imp->metadata.getNComps(inputNb);
        componentsType = _imp->metadata.getComponentsType(inputNb);
    }
    if (componentsType == kNatronColorPlaneID) {
        *plane = ImagePlaneDesc::mapNCompsToColorPlane(nComps);
    } else if (componentsType == kNatronDisparityComponentsLabel) {
        *plane = ImagePlaneDesc::getDisparityLeftComponents();
        *pairedPlane = ImagePlaneDesc::getDisparityRightComponents();
    } else if (componentsType == kNatronMotionComponentsLabel) {
        *plane = ImagePlaneDesc::getBackwardMotionComponents();
        *pairedPlane = ImagePlaneDesc::getForwardMotionComponents();
    } else {
        *plane = ImagePlaneDesc::getNoneComponents();
    }
}

int
EffectInstance::getMetadataNComps(int inputNb) const
{
    QMutexLocker k(&_imp->metadataMutex);
    return _imp->metadata.getNComps(inputNb);
}

ImageBitDepthEnum
EffectInstance::getBitDepth(int inputNb) const
{
    QMutexLocker k(&_imp->metadataMutex);

    return _imp->metadata.getBitDepth(inputNb);
}

double
EffectInstance::getFrameRate() const
{
    QMutexLocker k(&_imp->metadataMutex);

    return _imp->metadata.getOutputFrameRate();
}

double
EffectInstance::getAspectRatio(int inputNb) const
{
    QMutexLocker k(&_imp->metadataMutex);

    return _imp->metadata.getPixelAspectRatio(inputNb);
}

ImagePremultiplicationEnum
EffectInstance::getPremult() const
{
    QMutexLocker k(&_imp->metadataMutex);

    return _imp->metadata.getOutputPremult();
}

bool
EffectInstance::isFrameVarying() const
{
    QMutexLocker k(&_imp->metadataMutex);

    return _imp->metadata.getIsFrameVarying();
}

bool
EffectInstance::canRenderContinuously() const
{
    QMutexLocker k(&_imp->metadataMutex);

    return _imp->metadata.getIsContinuous();
}

/**
 * @brief Returns the field ordering of images produced by this plug-in
 **/
ImageFieldingOrderEnum
EffectInstance::getFieldingOrder() const
{
    QMutexLocker k(&_imp->metadataMutex);

    return _imp->metadata.getOutputFielding();
}

bool
EffectInstance::refreshMetadata_recursive(std::list<Node*> & markedNodes)
{
    NodePtr node = getNode();
    std::list<Node*>::iterator found = std::find( markedNodes.begin(), markedNodes.end(), node.get() );

    if ( found != markedNodes.end() ) {
        return false;
    }

    if (_imp->runningClipPreferences) {
        return false;
    }

    ClipPreferencesRunning_RAII runningflag_(this);
    bool ret = refreshMetadata_public(false);
    node->refreshIdentityState();

    if ( !node->duringInputChangedAction() ) {
        ///The channels selector refreshing is already taken care of in the inputChanged action
        node->refreshChannelSelectors();
    }

    markedNodes.push_back( node.get() );

    NodesList outputs;
    node->getOutputsWithGroupRedirection(outputs);
    for (NodesList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        (*it)->getEffectInstance()->refreshMetadata_recursive(markedNodes);
    }

    return ret;
}

void
EffectInstance::setDefaultMetadata()
{
    NodeMetadata metadata;
    StatusEnum stat = getDefaultMetadata(metadata);

    if (stat == eStatusFailed) {
        return;
    }
    {
        QMutexLocker k(&_imp->metadataMutex);
        _imp->metadata = metadata;
    }
    onMetadataRefreshed(metadata);
}

bool
EffectInstance::setMetadataInternal(const NodeMetadata& metadata)
{
    bool ret;
    {
        QMutexLocker k(&_imp->metadataMutex);
        ret = metadata != _imp->metadata;
        if (ret) {
            _imp->metadata = metadata;
        }
    }
    return ret;
}

bool
EffectInstance::refreshMetadata_internal()
{
    NodeMetadata metadata;

    getPreferredMetadata_public(metadata);
    _imp->checkMetadata(metadata);

    bool ret = setMetadataInternal(metadata);
    onMetadataRefreshed(metadata);
    if (ret) {
        NodePtr node = getNode();
        node->checkForPremultWarningAndCheckboxes();

        ImagePlaneDesc plane, pairedPlane;
        getMetadataComponents(-1, &plane, &pairedPlane);
        node->refreshEnabledKnobsLabel(plane);
    }

    return ret;
}

bool
EffectInstance::refreshMetadata_public(bool recurse)
{
    assert( QThread::currentThread() == qApp->thread() );

    if (recurse) {

        {
            std::list<Node*> markedNodes;

            return refreshMetadata_recursive(markedNodes);
        }
    } else {
        bool ret = refreshMetadata_internal();
        if (ret) {
            NodePtr node = getNode();
            NodesList children;
            node->getChildrenMultiInstance(&children);
            if ( !children.empty() ) {
                for (NodesList::iterator it = children.begin(); it != children.end(); ++it) {
                    (*it)->getEffectInstance()->refreshMetadata_internal();
                }
            }
        }

        return ret;
    }
}

/**
 * @brief The purpose of this function is to check that the meta data returned by the plug-ins are valid and to
 * check for warnings
 **/
void
EffectInstance::Implementation::checkMetadata(NodeMetadata &md)
{
    NodePtr node = _publicInterface->getNode();

    if (!node) {
        return;
    }
    //Make sure it is valid
    int nInputs = node->getNInputs();

    for (int i = -1; i < nInputs; ++i) {
        md.setBitDepth( i, node->getClosestSupportedBitDepth( md.getBitDepth(i) ) );
        int nComps = md.getNComps(i);
        bool isAlpha = false;
        bool isRGB = false;
        if (i == -1) {
            if ( nComps == 3) {
                isRGB = true;
            } else if (nComps == 1) {
                isAlpha = true;
            }
        }

        if ( md.getComponentsType(i) == kNatronColorPlaneID ) {
            md.setNComps(i, node->findClosestSupportedComponents(i, ImagePlaneDesc::mapNCompsToColorPlane(nComps)).getNumComponents());
        }

        if (i == -1) {
            //Force opaque for RGB and premult for alpha
            if (isRGB) {
                md.setOutputPremult(eImagePremultiplicationOpaque);
            } else if (isAlpha) {
                md.setOutputPremult(eImagePremultiplicationPremultiplied);
            }
        }
    }


    ///Set a warning on the node if the bitdepth conversion from one of the input clip to the output clip is lossy
    QString bitDepthWarning = tr("This nodes converts higher bit depths images from its inputs to work. As "
                                 "a result of this process, the quality of the images is degraded. The following conversions are done:");
    bitDepthWarning.append( QChar::fromLatin1('\n') );
    bool setBitDepthWarning = false;
    const bool supportsMultipleClipDepths = _publicInterface->supportsMultipleClipDepths();
    const bool supportsMultipleClipPARs = _publicInterface->supportsMultipleClipPARs();
    const bool supportsMultipleClipFPSs = _publicInterface->supportsMultipleClipFPSs();
    std::vector<EffectInstancePtr> inputs(nInputs);
    for (int i = 0; i < nInputs; ++i) {
        inputs[i] = _publicInterface->getInput(i);
    }


    ImageBitDepthEnum outputDepth = md.getBitDepth(-1);
    double outputPAR = md.getPixelAspectRatio(-1);
    bool outputFrameRateSet = false;
    double outputFrameRate = md.getOutputFrameRate();
    bool mustWarnFPS = false;
    bool mustWarnPAR = false;

    int nbConnectedInputs = 0;
    for (int i = 0; i < nInputs; ++i) {
        //Check that the bitdepths are all the same if the plug-in doesn't support multiple depths
        if ( !supportsMultipleClipDepths && (md.getBitDepth(i) != outputDepth) ) {
            md.setBitDepth(i, outputDepth);
        }

        const double pixelAspect = md.getPixelAspectRatio(i);

        if (!supportsMultipleClipPARs) {
            if (pixelAspect != outputPAR) {
                mustWarnPAR = true;
                md.setPixelAspectRatio(i, outputPAR);
            }
        }

        if (!inputs[i]) {
            continue;
        }

        ++nbConnectedInputs;

        const double fps = inputs[i]->getFrameRate();



        if (!supportsMultipleClipFPSs) {
            if (!outputFrameRateSet) {
                outputFrameRate = fps;
                outputFrameRateSet = true;
            } else if (std::abs(outputFrameRate - fps) > 0.01) {
                // We have several inputs with different frame rates
                mustWarnFPS = true;
            }
        }


        ImageBitDepthEnum inputOutputDepth = inputs[i]->getBitDepth(-1);

        //If the bit-depth conversion will be lossy, warn the user
        if ( Image::isBitDepthConversionLossy( inputOutputDepth, md.getBitDepth(i) ) ) {
            bitDepthWarning.append( QString::fromUtf8( inputs[i]->getNode()->getLabel_mt_safe().c_str() ) );
            bitDepthWarning.append( QString::fromUtf8(" (") + QString::fromUtf8( Image::getDepthString(inputOutputDepth).c_str() ) + QChar::fromLatin1(')') );
            bitDepthWarning.append( QString::fromUtf8(" ----> ") );
            bitDepthWarning.append( QString::fromUtf8( node->getLabel_mt_safe().c_str() ) );
            bitDepthWarning.append( QString::fromUtf8(" (") + QString::fromUtf8( Image::getDepthString( md.getBitDepth(i) ).c_str() ) + QChar::fromLatin1(')') );
            bitDepthWarning.append( QChar::fromLatin1('\n') );
            setBitDepthWarning = true;
        }


        if ( !supportsMultipleClipPARs && (pixelAspect != outputPAR) ) {
            qDebug() << node->getScriptName_mt_safe().c_str() << ": The input " << inputs[i]->getNode()->getScriptName_mt_safe().c_str()
                     << ") has a pixel aspect ratio (" << md.getPixelAspectRatio(i)
                     << ") different than the output clip (" << outputPAR << ") but it doesn't support multiple clips PAR. "
                     << "This should have been handled earlier before connecting the nodes, @see Node::canConnectInput.";
        }
    }

    std::map<Node::StreamWarningEnum, QString> warnings;
    if (setBitDepthWarning) {
        warnings[Node::eStreamWarningBitdepth] = bitDepthWarning;
    } else {
        warnings[Node::eStreamWarningBitdepth] = QString();
    }

    if (mustWarnFPS && nbConnectedInputs > 1) {
        QString fpsWarning = tr("One or multiple inputs have a frame rate different of the output. "
                                "It is not handled correctly by this node. To remove this warning make sure all inputs have "
                                "the same frame-rate, either by adjusting project settings or the upstream Read node.");
        warnings[Node::eStreamWarningFrameRate] = fpsWarning;
    } else {
        warnings[Node::eStreamWarningFrameRate] = QString();
    }

    if (mustWarnPAR && nbConnectedInputs > 1) {
        QString parWarnings = tr("One or multiple input have a pixel aspect ratio different of the output. It is not "
                                 "handled correctly by this node and may yield unwanted results. Please adjust the "
                                 "pixel aspect ratios of the inputs so that they match by using a Reformat node.");
        warnings[Node::eStreamWarningPixelAspectRatio] = parWarnings;
    } else {
        warnings[Node::eStreamWarningPixelAspectRatio] = QString();
    }


    node->setStreamWarnings(warnings);
} //refreshMetadataProxy

void
EffectInstance::refreshExtraStateAfterTimeChanged(bool isPlayback,
                                                  double time)
{
    KnobHolder::refreshExtraStateAfterTimeChanged(isPlayback, time);

    getNode()->refreshIdentityState();
}

void
EffectInstance::assertActionIsNotRecursive() const
{
# ifdef DEBUG
    ///Only check recursions which are on a render threads, because we do authorize recursions in getRegionOfDefinition and such
    if ( QThread::currentThread() != qApp->thread() ) {
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
    EffectTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();

    assert(tls);
    ++tls->actionRecursionLevel;
}

void
EffectInstance::decrementRecursionLevel()
{
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();

    assert(tls);
    --tls->actionRecursionLevel;
}

int
EffectInstance::getRecursionLevel() const
{
    EffectTLSDataPtr tls = _imp->tlsData->getTLSData();

    if (!tls) {
        return 0;
    }

    return tls->actionRecursionLevel;
}

void
EffectInstance::setClipPreferencesRunning(bool running)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->runningClipPreferences = running;
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_EffectInstance.cpp"

