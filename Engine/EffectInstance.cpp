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
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include <QtCore/QReadWriteLock>
#include <QtCore/QCoreApplication>
#include <QtConcurrentMap> // QtCore on Qt4, QtConcurrent on Qt5
#include <QtConcurrentRun> // QtCore on Qt4, QtConcurrent on Qt5

#include "Global/QtCompat.h"

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/EffectOpenGLContextData.h"
#include "Engine/Cache.h"
#include "Engine/EffectInstanceActionResults.h"
#include "Engine/EffectInstanceTLSData.h"
#include "Engine/Image.h"
#include "Engine/Hash64.h"
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

RenderEngine*
EffectInstance::createRenderEngine()
{
    return new RenderEngine(getNode());
}

void
EffectInstance::getTimeViewParametersDependingOnFrameViewVariance(TimeValue time, ViewIdx view, const TreeRenderNodeArgsPtr& render, TimeValue* timeOut, ViewIdx* viewOut)
{
    bool frameVarying = isFrameVarying(render);

    // If the node is frame varying, append the time to its hash.
    // Do so as well if it is view varying
    if (frameVarying) {
        // Make sure the time is rounded to the image equality epsilon to account for double precision if we want to reproduce the
        // same hash
        *timeOut = roundImageTimeToEpsilon(time);
    } else {
        *timeOut = TimeValue(0);
    }

    if (isViewInvariant() == eViewInvarianceAllViewsVariant) {
        *viewOut = view;
    } else {
        *viewOut = ViewIdx(0);
    }
}

void
EffectInstance::appendToHash(const ComputeHashArgs& args, Hash64* hash)
{
    NodePtr node = getNode();

    // Append the plug-in ID in case for there is a coincidence of all parameter values (and ordering!) between 2 plug-ins
    Hash64::appendQString(QString::fromUtf8(node->getPluginID().c_str()), hash);

    // Also append the project knobs to the hash. Their hash will only change when the project properties have been invalidated
    U64 projectHash = getApp()->getProject()->computeHash(args);
    hash->append(projectHash);

    // Append all knobs hash
    KnobHolder::appendToHash(args, hash);

    GetFramesNeededResultsPtr framesNeededResults;


    // Append input hash for each frames needed.
    if (args.hashType != HashableObject::eComputeHashTypeTimeViewVariant) {
        // We don't need to be frame varying for the hash, just append the hash of the inputs at the current time
        int nInputs = getMaxInputCount();
        for (int i = 0; i < nInputs; ++i) {
            EffectInstancePtr input = getInput(i);
            if (!input) {
                hash->append(0);
            } else {
                ComputeHashArgs inputArgs = args;
                if (args.render) {
                    inputArgs.render = args.render->getInputRenderArgs(i);
                }
                U64 inputHash = input->computeHash(inputArgs);
                hash->append(inputHash);
            }
        }
    } else {
        // We must add the input hash at the frames needed because a node may depend on a value at a different frame


        ActionRetCodeEnum stat = getFramesNeeded_public(args.time, args.view, args.render, &framesNeededResults);

        FramesNeededMap framesNeeded;
        if (!isFailureRetCode(stat)) {
            framesNeededResults->getFramesNeeded(&framesNeeded);
        }
        for (FramesNeededMap::const_iterator it = framesNeeded.begin(); it != framesNeeded.end(); ++it) {

            EffectInstancePtr inputEffect = resolveInputEffectForFrameNeeded(it->first, 0);
            if (!inputEffect) {
                continue;
            }

            // If during a render we must also find the render args for the input node
            TreeRenderNodeArgsPtr inputRenderArgs;
            if (args.render) {
                inputRenderArgs = args.render->getInputRenderArgs(it->first);
            }

            // For all views requested in input
            for (FrameRangesMap::const_iterator viewIt = it->second.begin(); viewIt != it->second.end(); ++viewIt) {

                // For all ranges in this view
                for (U32 range = 0; range < viewIt->second.size(); ++range) {

                    // For all frames in the range
                    for (double f = viewIt->second[range].min; f <= viewIt->second[range].max; f += 1.) {

                        ComputeHashArgs inputArgs = args;
                        if (args.render) {
                            inputArgs.render = args.render->getInputRenderArgs(it->first);
                        }
                        inputArgs.time = TimeValue(f);
                        inputArgs.view = viewIt->first;
                        U64 inputHash = inputEffect->computeHash(inputArgs);
                        
                        // Append the input hash
                        hash->append(inputHash);
                    }
                }
                
            }
        }
    } // args.hashType != HashableObject::eComputeHashTypeTimeViewVariant

    // Also append the disabled state of the node. This is useful because the knob disabled itself is not enough: if the node is not disabled
    // but inside a disabled group, it is considered disabled but yet has the same hash than when not disabled.
    bool disabled = node->isNodeDisabledForFrame(args.time, args.view);
    hash->append(disabled);

    hash->computeHash();

    U64 hashValue = hash->value();

    // If we used getFramesNeeded, cache it now if possible
    if (framesNeededResults) {
        GetFramesNeededKeyPtr cacheKey;

        {
            TimeValue timeKey;
            ViewIdx viewKey;
            getTimeViewParametersDependingOnFrameViewVariance(args.time, args.view, args.render, &timeKey, &viewKey);
            cacheKey.reset(new GetFramesNeededKey(hashValue, timeKey, viewKey, getNode()->getPluginID()));
        }

        CacheEntryLockerPtr cacheAccess = appPTR->getCache()->get(framesNeededResults);
        
        CacheEntryLocker::CacheEntryStatusEnum cacheStatus = cacheAccess->getStatus();
        if (cacheStatus == CacheEntryLocker::eCacheEntryStatusMustCompute) {
            cacheAccess->insertInCache();
        }
    }

    if (args.render) {
        switch (args.hashType) {
            case HashableObject::eComputeHashTypeTimeViewVariant: {
                FrameViewRequestPtr fvRequest;
                bool created = args.render->getOrCreateFrameViewRequest(args.time, args.view, &fvRequest);
                (void)created;
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



#ifdef DEBUG
void
EffectInstance::checkCanSetValueAndWarn() const
{
    EffectInstanceTLSDataPtr tls = _imp->tlsData->getTLSData();

    if (!tls) {
        return;
    }

    if (tls->isDuringActionThatCannotSetValue()) {
        qDebug() << getScriptName_mt_safe().c_str() << ": setValue()/setValueAtTime() was called during an action that is not allowed to call this function.";
    }
}

#endif //DEBUG


bool
EffectInstance::shouldCacheOutput(bool isFrameVaryingOrAnimated,
                                  const TreeRenderNodeArgsPtr& render,
                                  int visitsCount) const
{
    if (visitsCount > 1) {
        // The node is referenced multiple times by getFramesNeeded of downstream nodes, cache it
        return true;
    }

    NodePtr node = getNode();

    std::list<NodeWPtr> outputs;
    node->getOutputs_mt_safe(outputs);
    std::size_t nOutputNodes = outputs.size();

    if (nOutputNodes == 0) {
        // outputs == 0, never cache, unless explicitly set or rotopaint internal node
        RotoDrawableItemPtr attachedStroke = node->getAttachedRotoItem();

        return node->isForceCachingEnabled() || appPTR->isAggressiveCachingEnabled() ||
        ( attachedStroke && attachedStroke->getModel()->getNode()->isSettingsPanelVisible() );

    } else if (nOutputNodes > 1) {
        return true;
    }

    NodePtr output = outputs.front().lock();

    if (!isFrameVaryingOrAnimated) {
        // This image never changes, cache it once.
        return true;
    }
    if ( output->isSettingsPanelVisible() ) {
        // Output node has panel opened, meaning the user is likely to be heavily editing
        // that output node, hence requesting this node a lot. Cache it.
        return true;
    }
    if ( doesTemporalClipAccess() ) {
        // Very heavy to compute since many frames are fetched upstream. Cache it.
        return true;
    }
    if ( !render->getCurrentTilesSupport() ) {
        // No tiles, image is going to be produced fully, cache it to prevent multiple access
        // with different RoIs
        return true;
    }
    if ( node->isForceCachingEnabled() ) {
        // Users wants it cached
        return true;
    }

    NodeGroupPtr parentIsGroup = toNodeGroup( node->getGroup() );
    if ( parentIsGroup && parentIsGroup->getNode()->isForceCachingEnabled() && (parentIsGroup->getOutputNodeInput() == node) ) {
        // If the parent node is a group and it has its force caching enabled, cache the output of the Group Output's node input.
        return true;
    }

    if ( appPTR->isAggressiveCachingEnabled() ) {
        ///Users wants all nodes cached
        return true;
    }

    if ( node->isPreviewEnabled() && !appPTR->isBackground() ) {
        // The node has a preview, meaning the image will be computed several times between previews & actual renders. Cache it.
        return true;
    }

    if ( node->isDuringPaintStrokeCreation() ) {
        // When painting we must always cache
        return true;
    }

    RotoDrawableItemPtr attachedStroke = node->getAttachedRotoItem();
    if ( attachedStroke && attachedStroke->getModel()->getNode()->isSettingsPanelVisible() ) {
        // Internal RotoPaint tree and the Roto node has its settings panel opened, cache it.
        return true;
    }

    return false;
} // shouldCacheOutput

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
EffectInstance::resolveRoIForGetImage(const GetImageInArgs& inArgs,
                                      TimeValue inputTime,
                                      RectD* roiCanonical,
                                      RenderRoITypeEnum* type)
{

    *type = eRenderRoITypeKnownFrame;

    TreeRenderNodeArgsPtr inputRenderArgs = inArgs.renderArgs->getInputRenderArgs(inArgs.inputNb);
    assert(inputRenderArgs);
    if (!inputRenderArgs) {
        // Serious bug, all inputs should always have a render object.
        return false;
    }

    // Is there a request that was filed with getFramesNeeded on this input at the given time/view ?
    FrameViewRequestPtr requestPass;
    bool createdRequestPass = inputRenderArgs->getOrCreateFrameViewRequest(inputTime, inArgs.inputView, &requestPass);
    bool frameWasRequestedWithGetFramesNeeded = false;
    if (!createdRequestPass) {
        EffectInstancePtr thisShared = shared_from_this();
        frameWasRequestedWithGetFramesNeeded = requestPass->wasFrameViewRequestedByEffect(thisShared);
    }


    if (frameWasRequestedWithGetFramesNeeded) {
        // The frame was requested by getFramesNeeded: the RoI contains at least the RoI of this effect and maybe
        // also the RoI from other branches requesting this frame/view, thus we may have a chance to render the effect
        // a single time.
        *roiCanonical = requestPass->getCurrentRoI();
        return true;
    }



    // This node did not request anything on the input in getFramesNeeded action: it did a raw call to fetchImage without advertising us.
    //
    // Either the user passed an optional bounds parameter or we have to compute the RoI using getRegionsOfInterest.
    // We must call getRegionOfInterest on the time and view of the current action of this effect.

#ifdef DEBUG
    qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inArgs.inputNb << "at time" << inArgs.inputTime << "and view" << inArgs.inputView <<
    ": The frame was not requested properly in the getFramesNeeded action. This is a bug either in this plug-in or a plug-in downstream (which should also have this warning displayed)";
#endif

    *type = eRenderRoITypeUnknownFrame;



    // Use the provided image bounds if any
    if (inArgs.optionalBounds) {
        *roiCanonical = * inArgs.optionalBounds;
        return true;
    }

    // We have to retrieve the time and view of the current action in the TLS since it was not passed in parameter.
    RectD thisEffectRenderWindowCanonical;

    {

        EffectInstanceTLSDataPtr tls = _imp->tlsData->getTLSData();
        assert(tls);


        // To call getRegionsOfInterest we need to pass a render window, but
        // we don't know this effect current render window.
        // Either we are in the render action and we can retrieve the render action current render window or we have
        // to assume the current render window is the full RoD.
        RectD thisEffectRoD;
        {

            GetRegionOfDefinitionResultsPtr results;
            ActionRetCodeEnum stat = getRegionOfDefinition_public(inArgs.currentTime, inArgs.currentScale, inArgs.currentView, inArgs.renderArgs, &results);
            if (isFailureRetCode(stat)) {
#ifdef DEBUG
                qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inArgs.inputNb << "failing because getRegionOfDefinition failed";
#endif
                return false;
            }
            thisEffectRoD = results->getRoD();
            if (thisEffectRoD.isNull()) {
#ifdef DEBUG
                qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inArgs.inputNb << "failing because the region of definition of this effect is NULL";
#endif
                return false;
            }
        }

        RectI thisEffectRenderWindowPixels;
        bool gotRenderActionTLSData = tls->getCurrentRenderActionArgs(0, 0, 0, &thisEffectRenderWindowPixels, 0);
        if (gotRenderActionTLSData) {
            double thisEffectOutputPar = getAspectRatio(inArgs.renderArgs, -1);
            thisEffectRenderWindowPixels.toCanonical(inArgs.currentScale, thisEffectOutputPar, thisEffectRoD, &thisEffectRenderWindowCanonical);
        } else {
            thisEffectRenderWindowCanonical = thisEffectRoD;

        }
    }


    // Get the roi for the current render window

    RoIMap inputRoisMap;
    ActionRetCodeEnum stat = getRegionsOfInterest_public(inArgs.currentTime, inArgs.currentScale, thisEffectRenderWindowCanonical, inArgs.currentView, inArgs.renderArgs, &inputRoisMap);
    if (isFailureRetCode(stat)) {
#ifdef DEBUG
        qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inArgs.inputNb << "failing because getRegionsOfInterest failed";
#endif
        return false;
    }

    RoIMap::iterator foundInputEffectRoI = inputRoisMap.find(inArgs.inputNb);
    if (foundInputEffectRoI != inputRoisMap.end()) {
        *roiCanonical = foundInputEffectRoI->second;
        return true;
    }

    return false;
} // resolveRoIForGetImage

EffectInstance::GetImageInArgs::GetImageInArgs()
: inputNb(0)
, inputTime(0)
, inputView(0)
, inputProxyScale(1.)
, inputMipMapLevel(0)
, currentScale(1.)
, currentTime(0)
, currentView(0)
, optionalBounds(0)
, layers(0)
, renderBackend(0)
, renderArgs()
{

}

EffectInstance::GetImageInArgs::GetImageInArgs(const RenderActionArgs& args)
: inputNb(0)
, inputTime(args.time)
, inputView(args.view)
, inputProxyScale(args.renderArgs->getParentRender()->getProxyScale())
, inputMipMapLevel(args.renderArgs->getParentRender()->getMipMapLevel())
, currentScale(args.renderScale)
, currentTime(args.time)
, currentView(args.view)
, optionalBounds(0)
, layers(0)
, renderBackend(&args.backendType)
, renderArgs(args.renderArgs)
{

}

bool
EffectInstance::getImagePlanes(const GetImageInArgs& inArgs, GetImageOutArgs* outArgs)
{
    if (inArgs.inputTime != inArgs.inputTime) {
        // time is NaN
#ifdef DEBUG
        qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inArgs.inputNb << "failing because time is NaN";
#endif
        return false;
    }

    int channelForMask = -1;
    EffectInstancePtr inputEffect= resolveInputEffectForFrameNeeded(inArgs.inputNb, &channelForMask);

    if (!inputEffect) {
        // Disconnected input
        // No need to display a warning, the effect can perfectly call this function even if it did not check isConnected() before
        //qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inputNb << "failing because the input is not connected";
        return false;
    }

    // If this effect is not continuous, no need to ask for a floating point time upstream
    TimeValue inputTime = inArgs.inputTime;
    {
        int roundedTime = std::floor(inputTime + 0.5);
        if (roundedTime != inputTime && !inputEffect->canRenderContinuously(inArgs.renderArgs)) {
            inputTime = TimeValue(roundedTime);
        }
    }

    std::list<ImagePlaneDesc> componentsToRender;
    if (inArgs.layers) {
        componentsToRender = *inArgs.layers;
    } else {
        GetComponentsResultsPtr results;
        ActionRetCodeEnum stat = getLayersProducedAndNeeded_public(inArgs.currentTime, inArgs.currentView, inArgs.renderArgs, &results);
        if (isFailureRetCode(stat)) {
#ifdef DEBUG
            qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inArgs.inputNb << "failing because the get components action failed";
#endif
            return false;
        }
        assert(results);

        std::map<int, std::list<ImagePlaneDesc> > neededInputLayers;
        std::list<ImagePlaneDesc> producedLayers, availableLayers;
        int passThroughInputNb;
        TimeValue passThroughTime;
        ViewIdx passThroughView;
        std::bitset<4> processChannels;
        bool processAll;
        results->getResults(&neededInputLayers, &producedLayers, &availableLayers, &passThroughInputNb, &passThroughTime, &passThroughView, &processChannels, &processAll);

        std::map<int, std::list<ImagePlaneDesc> >::const_iterator foundInput = neededInputLayers.find(inArgs.inputNb);
        if (foundInput == neededInputLayers.end()) {
#ifdef DEBUG
            qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inArgs.inputNb << "failing because the get components action did not specify any componentns to fetch";
#endif
            return false;
        }
        componentsToRender = foundInput->second;
    }


    if (!inArgs.renderArgs) {

        // We were not during a render, create one and render the tree upstream.
        TreeRender::CtorArgsPtr rargs(new TreeRender::CtorArgs());
        rargs->time = inputTime;
        rargs->view = inArgs.inputView;
        rargs->treeRoot = inputEffect->getNode();
        rargs->canonicalRoI = inArgs.optionalBounds;
        rargs->proxyScale = inArgs.inputProxyScale;
        rargs->mipMapLevel = inArgs.inputMipMapLevel;
        rargs->layers = &componentsToRender;
        rargs->draftMode = false;
        rargs->playback = false;
        rargs->byPassCache = false;

        TreeRenderPtr renderObject = TreeRender::create(rargs);
        ActionRetCodeEnum status = renderObject->launchRender(&outArgs->imagePlanes);
        if (isFailureRetCode(status) || outArgs->imagePlanes.empty()) {
            return false;
        }
        return true;
    }

    assert(inArgs.renderArgs);
    TreeRenderNodeArgsPtr inputRenderArgs = inArgs.renderArgs->getInputRenderArgs(inArgs.inputNb);
    assert(inputRenderArgs);

    // Ok now we are during a render. The effect may be in any action called on a render thread:
    // getDistortion, render, getRegionOfDefinition...
    //
    // Determine if the frame is "known" (i.e: it was requested from getFramesNeeded) or not.
    //
    // For a known frame we know the exact final RoI to ask for on the effect and we can guarantee
    // that the effect will be rendered once. We cache the resulting image only during the render of this node
    // frame, then we can throw it away.
    //
    // For an unknown frame, we don't know if the frame will be asked for another time hence we lock
    // it in the cache throughout the lifetime of the render.
    RectD roiCanonical;
    RenderRoITypeEnum renderType;
    if (!resolveRoIForGetImage(inArgs, inputTime, &roiCanonical, &renderType)) {
        return false;
    }

    RenderScale inputCombinedScale = EffectInstance::Implementation::getCombinedScale(inArgs.inputMipMapLevel, inArgs.inputProxyScale);

    // Clip the RoI to the input effect RoD
    {
        GetRegionOfDefinitionResultsPtr rodResults;
        ActionRetCodeEnum stat = inputEffect->getRegionOfDefinition_public(inputTime, inputCombinedScale, inArgs.inputView, inputRenderArgs, &rodResults);
        if (isFailureRetCode(stat)) {
            return false;
        }
        const RectD& inputRod = rodResults->getRoD();

        // If the plug-in RoD is null, there's nothing to render.
        if (inputRod.isNull()) {
            return false;
        }

        // Intersect the RoI to the input RoD
        if (!roiCanonical.intersect(inputRod, &roiCanonical)) {
            return false;
        }

    }


    if (roiCanonical.isNull()) {
        // No RoI
#ifdef DEBUG
        qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inArgs.inputNb << "failing because the region to render on the input is empty";
#endif
        return false;
    }



    // Retrieve an image with the format given by this node preferences
    const double par = getAspectRatio(inArgs.renderArgs, inArgs.inputNb);

    // Convert the roi to pixel coordinates
    RectI pixelRoI;
    roiCanonical.toPixelEnclosing(inputCombinedScale, par, &pixelRoI);


    // Get the request of this effect current action
    FrameViewRequestPtr thisFrameViewRequest = inArgs.renderArgs->getFrameViewRequest(inArgs.currentTime, inArgs.currentView);
    // We have to have a request because we were called in an action that was part of a render. If it crashes here this is a bug
    assert(thisFrameViewRequest);
    if (!thisFrameViewRequest) {
        return false;
    }



    std::map<ImagePlaneDesc, ImagePtr> inputRenderedPlanes;


    // Look for any pre-rendered result for this frame-view. If not rendered already then call renderRoI
    // renderRoI may find stuff in the cache and not render anyway
    {
        std::list<ImagePlaneDesc> planesLeftToRender;
        thisFrameViewRequest->getPreRenderedInputs(inArgs.inputNb, inputTime, inArgs.inputView, pixelRoI, componentsToRender, &inputRenderedPlanes, &planesLeftToRender, &outArgs->distortionStack);
        componentsToRender = planesLeftToRender;
    }
    
    if (!componentsToRender.empty()) {

        EffectInstancePtr thisShared = shared_from_this();
        RenderRoIResults inputRenderResults;
        boost::scoped_ptr<RenderRoIArgs> rargs(new RenderRoIArgs(inputTime,
                                                                 inArgs.inputView,
                                                                 pixelRoI,
                                                                 inArgs.inputProxyScale,
                                                                 inArgs.inputMipMapLevel,
                                                                 componentsToRender,
                                                                 inputRenderArgs));
        if (renderType == eRenderRoITypeUnknownFrame) {
            // Disable GPU rendering for an unknown frame since we don't know how long it should stay around.
            rargs->allowGPURendering = false;
        }
        rargs->canReturnDeprecatedTransform3x3 = inArgs.renderArgs->getCurrentTransformationSupport_deprecated();
        rargs->canReturnDistortionFunc = inArgs.renderArgs->getCurrentDistortSupport();

        ActionRetCodeEnum retCode = inputEffect->renderRoI(*rargs, &inputRenderResults);

        if (isFailureRetCode(retCode) || inputRenderResults.outputPlanes.empty()) {
            return false;
        }

        inputRenderedPlanes.insert(inputRenderResults.outputPlanes.begin(), inputRenderResults.outputPlanes.end());
        outArgs->distortionStack = inputRenderResults.distortionStack;

    }


    // Return in output the region that was asked to the input
    outArgs->roiPixel = pixelRoI;

    ImageBufferLayoutEnum thisEffectSupportedImageLayout = getPreferredBufferLayout();

    const bool supportsMultiPlane = isMultiPlanar();

    for (std::map<ImagePlaneDesc, ImagePtr>::iterator it = inputRenderedPlanes.begin(); it != inputRenderedPlanes.end(); ++it) {

        if ( !pixelRoI.intersects( it->second->getBounds() ) ) {
            // The RoI requested does not intersect with the bounds of the input image computed, return a NULL image.
#ifdef DEBUG
            qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inArgs.inputNb << "failing because the RoI requested does not intersect the bounds of the input image fetched. This is a bug, please investigate!";
#endif
            return ImagePtr();
        }

        assert(it->second->getProxyScale().x == inArgs.inputProxyScale.x && it->second->getProxyScale().y == inArgs.inputProxyScale.y);
        assert(it->second->getMipMapLevel() == inArgs.inputMipMapLevel);


        bool mustConvertImage = false;
        StorageModeEnum storage = it->second->getStorageMode();
        StorageModeEnum preferredStorage = storage;
        if (inArgs.renderBackend) {
            switch (*inArgs.renderBackend) {
                case eRenderBackendTypeOpenGL: {
                    preferredStorage = eStorageModeGLTex;
                    if (storage != eStorageModeGLTex) {
                        mustConvertImage = true;
                    }
                }   break;
                case eRenderBackendTypeCPU:
                case eRenderBackendTypeOSMesa:
                    preferredStorage = eStorageModeRAM;
                    if (storage == eStorageModeGLTex) {
                        mustConvertImage = true;
                    }
                    break;
            }
        }

        ImageBufferLayoutEnum imageLayout = it->second->getBufferFormat();
        if (imageLayout != thisEffectSupportedImageLayout) {
            mustConvertImage = true;
        }

        ImagePlaneDesc preferredLayer = it->second->getLayer();

        // If this node does not support multi-plane or the image is the color plane,
        // map it to this node preferred color plane
        if (it->second->getLayer().isColorPlane() || !supportsMultiPlane) {
            ImagePlaneDesc plane, pairedPlane;
            getMetadataComponents(inArgs.renderArgs, inArgs.inputNb, &plane, &pairedPlane);
            if (it->second->getLayer() != plane) {
                mustConvertImage = true;
                preferredLayer = plane;
            }
        }
        ImageBitDepthEnum thisBitDepth = getBitDepth(inArgs.renderArgs, inArgs.inputNb);
        // Map bit-depth
        if (thisBitDepth != it->second->getBitDepth()) {
            mustConvertImage = true;
        }

        ImagePtr convertedImage = it->second;
        if (mustConvertImage) {

            Image::InitStorageArgs initArgs;
            {
                initArgs.bounds = pixelRoI;
                initArgs.proxyScale = it->second->getProxyScale();
                initArgs.mipMapLevel = it->second->getMipMapLevel();
                initArgs.layer = preferredLayer;
                initArgs.bitdepth = thisBitDepth;
                initArgs.bufferFormat = thisEffectSupportedImageLayout;
                initArgs.storage = preferredStorage;
                initArgs.renderArgs = inArgs.renderArgs;
                initArgs.glContext = inArgs.renderArgs->getParentRender()->getGPUOpenGLContext();
            }
            
            convertedImage = Image::create(initArgs);

            Image::CopyPixelsArgs copyArgs;
            {
                copyArgs.roi = initArgs.bounds;
                copyArgs.conversionChannel = channelForMask;
                copyArgs.srcColorspace = getApp()->getDefaultColorSpaceForBitDepth(it->second->getBitDepth());
                copyArgs.dstColorspace = getApp()->getDefaultColorSpaceForBitDepth(thisBitDepth);
                copyArgs.monoConversion = Image::eMonoToPackedConversionCopyToChannelAndFillOthers;
            }
            convertedImage->copyPixels(*it->second, copyArgs);

        } // mustConvertImage

        outArgs->imagePlanes[preferredLayer] = convertedImage;

    } // for each plane requested

    // Hold a pointer to the rendered results until this effect render is finished: subsequent calls to getImagePlanes for this frame/view
    // should return these pointers immediately.
    // This will overwrite previous pointers if there was any since we converted the images.
    thisFrameViewRequest->appendPreRenderedInputs(inArgs.inputNb, inputTime, inArgs.inputView, outArgs->imagePlanes, outArgs->distortionStack);
    
    assert(!outArgs->imagePlanes.empty());
    return true;
} // getImagePlanes

TimeValue
EffectInstance::getCurrentTime_TLS() const
{
    EffectInstanceTLSDataPtr tls = _imp->tlsData->getTLSData();
    if (!tls) {
        return KnobHolder::getCurrentTime_TLS();
    }

    TimeValue ret;
    if (tls->getCurrentActionArgs(&ret, 0, 0)) {
        return ret;
    } else {
        TreeRenderNodeArgsPtr render = tls->getRenderArgs();
        if (render) {
            return render->getTime();
        } else {
            return KnobHolder::getCurrentTime_TLS();
        }
    }
}

ViewIdx
EffectInstance::getCurrentView_TLS() const
{
    EffectInstanceTLSDataPtr tls = _imp->tlsData->getTLSData();
    if (!tls) {
        return KnobHolder::getCurrentView_TLS();
    }

    ViewIdx ret;
    if (tls->getCurrentActionArgs(0, &ret, 0)) {
        return ret;
    } else {
        TreeRenderNodeArgsPtr render = tls->getRenderArgs();
        if (render) {
            return render->getView();
        } else {
            return KnobHolder::getCurrentView_TLS();
        }
    }
}

RenderValuesCachePtr
EffectInstance::getRenderValuesCache_TLS(TimeValue* currentTime, ViewIdx* currentView) const
{
    EffectInstanceTLSDataPtr tls = _imp->tlsData->getTLSData();
    if (!tls) {
        return RenderValuesCachePtr();
    }

    const TreeRenderNodeArgsPtr& render = tls->getRenderArgs();
    if (!render) {
        return RenderValuesCachePtr();
    }
    if (currentTime || currentView) {
        if (!tls->getCurrentActionArgs(currentTime, currentView, 0)) {
            if (currentTime) {
                *currentTime = render->getTime();
            }
            if (currentView) {
                *currentView = render->getView();
            }

        }
    }
    return render->getRenderValuesCache();


}

void
EffectInstance::setCurrentRender_TLS(const TreeRenderNodeArgsPtr& render)
{
    EffectInstanceTLSDataPtr tls = _imp->tlsData->getOrCreateTLSData();
    assert(!tls->getRenderArgs());
    tls->setRenderArgs(render);
}

TreeRenderNodeArgsPtr
EffectInstance::getCurrentRender_TLS() const
{
    EffectInstanceTLSDataPtr tls = _imp->tlsData->getTLSData();
    if (!tls) {
        return TreeRenderNodeArgsPtr();
    }
    return tls->getRenderArgs();
}

EffectInstanceTLSDataPtr
EffectInstance::getTLSObject() const
{
    return _imp->tlsData->getTLSData();
}

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


void
EffectInstance::evaluate(bool isSignificant,
                         bool refreshMetadatas)
{

    NodePtr node = getNode();

    if ( refreshMetadatas && node && node->isNodeCreated() ) {
        
        // Force a re-compute of the meta-data if needed
        onMetadataChanged_recursive_public();
    }

    if (isSignificant) {
        getApp()->triggerAutoSave();
    }

    node->refreshIdentityState();


    TimeValue time = getTimelineCurrentTime();

    // Get the connected viewers downstream and re-render or redraw them.
    if (isSignificant) {
        getApp()->renderAllViewers();
    } else {
        getApp()->redrawAllViewers();
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
    PluginMemoryPtr ret( new PluginMemory( shared_from_this() ) ); 

    PluginMemAllocateMemoryArgs args(nBytes);
    ret->allocateMemory(args);
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
EffectInstance::getCreateChannelSelectorKnob() const
{
    return ( !isMultiPlanar() && !isReader() && !isWriter() &&
             !boost::starts_with(getNode()->getPluginID(), "uk.co.thefoundry.furnace") );
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


bool
EffectInstance::isPaintingOverItselfEnabled() const
{
    return getNode()->isDuringPaintStrokeCreation();
}

void
EffectInstance::refreshExtraStateAfterTimeChanged(bool isPlayback,
                                                  TimeValue /*time*/)
{
    if (!isPlayback) {
        getNode()->refreshIdentityState();
    }
}


RectI
EffectInstance::getOutputFormat(const TreeRenderNodeArgsPtr& render)
{
    GetTimeInvariantMetaDatasResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetaDatas_public(render, &results);
    if (isFailureRetCode(stat)) {
        return RectI();
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getOutputFormat();
    }
}


bool
EffectInstance::isFrameVarying(const TreeRenderNodeArgsPtr& render)
{
    GetTimeInvariantMetaDatasResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetaDatas_public(render, &results);
    if (isFailureRetCode(stat)) {
        return true;
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getIsFrameVarying();
    }
}


double
EffectInstance::getFrameRate(const TreeRenderNodeArgsPtr& render)
{
    GetTimeInvariantMetaDatasResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetaDatas_public(render, &results);
    if (isFailureRetCode(stat)) {
        return 24.;
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getOutputFrameRate();
    }

}


ImagePremultiplicationEnum
EffectInstance::getPremult(const TreeRenderNodeArgsPtr& render)
{
    GetTimeInvariantMetaDatasResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetaDatas_public(render, &results);
    if (isFailureRetCode(stat)) {
        return eImagePremultiplicationPremultiplied;
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getOutputPremult();
    }
}

bool
EffectInstance::canRenderContinuously(const TreeRenderNodeArgsPtr& render)
{
    GetTimeInvariantMetaDatasResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetaDatas_public(render, &results);
    if (isFailureRetCode(stat)) {
        return true;
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getIsContinuous();
    }
}

ImageFieldingOrderEnum
EffectInstance::getFieldingOrder(const TreeRenderNodeArgsPtr& render)
{
    GetTimeInvariantMetaDatasResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetaDatas_public(render, &results);
    if (isFailureRetCode(stat)) {
        return eImageFieldingOrderNone;
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getOutputFielding();
    }
}


double
EffectInstance::getAspectRatio(const TreeRenderNodeArgsPtr& render, int inputNb)
{
    GetTimeInvariantMetaDatasResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetaDatas_public(render, &results);
    if (isFailureRetCode(stat)) {
        return 1.;
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getPixelAspectRatio(inputNb);
    }
}

void
EffectInstance::getMetadataComponents(const TreeRenderNodeArgsPtr& render, int inputNb, ImagePlaneDesc* plane, ImagePlaneDesc* pairedPlane)
{
    GetTimeInvariantMetaDatasResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetaDatas_public(render, &results);
    if (isFailureRetCode(stat)) {
        *plane = ImagePlaneDesc::getNoneComponents();
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        std::string componentsType = metadatas->getComponentsType(inputNb);
        if (componentsType == kNatronColorPlaneID) {
            int nComps = metadatas->getColorPlaneNComps(inputNb);
            *plane = ImagePlaneDesc::mapNCompsToColorPlane(nComps);
        } else if (componentsType == kNatronDisparityComponentsLabel) {
            *plane = ImagePlaneDesc::getDisparityLeftComponents();
            *pairedPlane = ImagePlaneDesc::getDisparityRightComponents();
        } else if (componentsType == kNatronMotionComponentsLabel) {
            *plane = ImagePlaneDesc::getBackwardMotionComponents();
            *pairedPlane = ImagePlaneDesc::getForwardMotionComponents();
        } else {
            *plane = ImagePlaneDesc::getNoneComponents();
            assert(false);
            throw std::logic_error("");
        }

    }
}

ImageBitDepthEnum
EffectInstance::getBitDepth(const TreeRenderNodeArgsPtr& render, int inputNb)
{
    GetTimeInvariantMetaDatasResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetaDatas_public(render, &results);
    if (isFailureRetCode(stat)) {
        return eImageBitDepthFloat;
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getBitDepth(inputNb);
    }
}


bool
EffectInstance::ifInfiniteclipRectToProjectDefault(RectD* rod) const
{

    /*If the rod is infinite clip it to the project's default*/
    Format projectDefault;
    getApp()->getProject()->getProjectDefaultFormat(&projectDefault);
    // BE CAREFUL:
    // std::numeric_limits<int>::infinity() does not exist (check std::numeric_limits<int>::has_infinity)
    // an int can not be equal to (or compared to) std::numeric_limits<double>::infinity()
    bool isRodProjectFormat = false;
    if (rod->left() <= kOfxFlagInfiniteMin) {
        rod->set_left( projectDefault.left() );
        isRodProjectFormat = true;
    }
    if (rod->bottom() <= kOfxFlagInfiniteMin) {
        rod->set_bottom( projectDefault.bottom() );
        isRodProjectFormat = true;
    }
    if (rod->right() >= kOfxFlagInfiniteMax) {
        rod->set_right( projectDefault.right() );
        isRodProjectFormat = true;
    }
    if (rod->top() >= kOfxFlagInfiniteMax) {
        rod->set_top( projectDefault.top() );
        isRodProjectFormat = true;
    }

    return isRodProjectFormat;
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_EffectInstance.cpp"

