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
#include "Engine/ReadNode.h"
#include "Engine/Settings.h"
#include "Engine/Timer.h"
#include "Engine/TreeRender.h"
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

EffectInstance::EffectInstance(const EffectInstancePtr& other, const TreeRenderPtr& render)
    : NamedKnobHolder(other, render)
    , _node( other->getNode() )
    , _imp( new Implementation(this, *other->_imp) )
{
    int nInputs = other->getMaxInputCount();
    _imp->renderData->inputs.resize(nInputs);
 
}

EffectInstance::~EffectInstance()
{
}

void
EffectInstance::setRenderCloneInput(const EffectInstancePtr& input, int inputNb)
{
    assert(inputNb < (int)_imp->renderData->inputs.size());
    if (inputNb < (int)_imp->renderData->inputs.size()) {
        _imp->renderData->inputs[inputNb] = input;
    }
}

KnobHolderPtr
EffectInstance::createRenderCopy(const TreeRenderPtr& render) const
{
    EffectRenderCloneBuilder createFunc = (EffectRenderCloneBuilder)getNode()->getPlugin()->getPropertyUnsafe<void*>(kNatronPluginPropCreateRenderCloneFunc);
    assert(createFunc);
    if (!createFunc) {
        throw std::invalid_argument("EffectInstance::createRenderCopy: No kNatronPluginPropCreateRenderCloneFunc property set on plug-in!");
    }
    EffectInstancePtr clone = createFunc(boost::const_pointer_cast<EffectInstance>(shared_from_this()), render);
    return clone;
}

RenderEngine*
EffectInstance::createRenderEngine()
{
    return new RenderEngine(getNode());
}

void
EffectInstance::getTimeViewParametersDependingOnFrameViewVariance(TimeValue time, ViewIdx view, TimeValue* timeOut, ViewIdx* viewOut)
{
    bool frameVarying = isFrameVarying();

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

U64
EffectInstance::computeHash(const ComputeHashArgs& args)
{
    if (_imp->renderData) {
        U64 hash;
        switch (args.hashType) {
            case HashableObject::eComputeHashTypeTimeViewVariant: {
                if (_imp->renderData->getFrameViewHash(args.time, args.view, &hash)) {
                    return hash;
                }
            }   break;
            case HashableObject::eComputeHashTypeTimeViewInvariant: {
                if (_imp->renderData->getTimeViewInvariantHash(&hash)) {
                    return hash;
                }
            }   break;
            case HashableObject::eComputeHashTypeOnlyMetadataSlaves: {
                if (_imp->renderData->getTimeInvariantMetadataHash(&hash)) {
                    return hash;
                }
            }   break;
        }
    }
    return HashableObject::computeHash(args);
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
                U64 inputHash = input->computeHash(args);
                hash->append(inputHash);
            }
        }
    } else {
        // We must add the input hash at the frames needed because a node may depend on a value at a different frame


        ActionRetCodeEnum stat = getFramesNeeded_public(args.time, args.view, &framesNeededResults);

        FramesNeededMap framesNeeded;
        if (!isFailureRetCode(stat)) {
            framesNeededResults->getFramesNeeded(&framesNeeded);
        }
        for (FramesNeededMap::const_iterator it = framesNeeded.begin(); it != framesNeeded.end(); ++it) {

            EffectInstancePtr inputEffect = getInput(it->first);
            if (!inputEffect) {
                continue;
            }

            // For all views requested in input
            for (FrameRangesMap::const_iterator viewIt = it->second.begin(); viewIt != it->second.end(); ++viewIt) {

                // For all ranges in this view
                for (U32 range = 0; range < viewIt->second.size(); ++range) {

                    // For all frames in the range
                    for (double f = viewIt->second[range].min; f <= viewIt->second[range].max; f += 1.) {

                        ComputeHashArgs inputArgs = args;
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
    bool disabled = isNodeDisabledForFrame(args.time, args.view);
    hash->append(disabled);

    hash->computeHash();

    U64 hashValue = hash->value();

    // If we used getFramesNeeded, cache it now if possible
    if (framesNeededResults) {
        GetFramesNeededKeyPtr cacheKey;

        {
            TimeValue timeKey;
            ViewIdx viewKey;
            getTimeViewParametersDependingOnFrameViewVariance(args.time, args.view, &timeKey, &viewKey);
            cacheKey.reset(new GetFramesNeededKey(hashValue, timeKey, viewKey, getNode()->getPluginID()));
        }

        CacheEntryLockerPtr cacheAccess = appPTR->getCache()->get(framesNeededResults);
        
        CacheEntryLocker::CacheEntryStatusEnum cacheStatus = cacheAccess->getStatus();
        if (cacheStatus == CacheEntryLocker::eCacheEntryStatusMustCompute) {
            cacheAccess->insertInCache();
        }
    }

    if (_imp->renderData) {
        switch (args.hashType) {
            case HashableObject::eComputeHashTypeTimeViewVariant: {
                _imp->renderData->setFrameViewHash(args.time, args.view, hashValue);
            }   break;
            case HashableObject::eComputeHashTypeTimeViewInvariant:
                _imp->renderData->setTimeViewInvariantHash(hashValue);
                break;
            case HashableObject::eComputeHashTypeOnlyMetadataSlaves:
                _imp->renderData->setTimeInvariantMetadataHash(hashValue);
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

void
EffectInstance::refreshMetadaWarnings(const NodeMetadata &metadata)
{
    assert(QThread::currentThread() == qApp->thread());

    int nInputs = getMaxInputCount();

    QString bitDepthWarning = tr("This nodes converts higher bit depths images from its inputs to a lower bitdepth image. As "
                                 "a result of this process, the quality of the images is degraded. The following conversions are done:\n");
    bool setBitDepthWarning = false;
    const bool multipleClipDepths = supportsMultipleClipDepths();
    const bool multipleClipPARs = supportsMultipleClipPARs();
    const bool multipleClipFPSs = supportsMultipleClipFPSs();
    std::vector<EffectInstancePtr> inputs(nInputs);
    for (int i = 0; i < nInputs; ++i) {
        inputs[i] = getInput(i);
    }


    ImageBitDepthEnum outputDepth = metadata.getBitDepth(-1);
    double outputPAR = metadata.getPixelAspectRatio(-1);
    bool outputFrameRateSet = false;
    double outputFrameRate = metadata.getOutputFrameRate();
    bool mustWarnFPS = false;
    bool mustWarnPAR = false;

    int nbConnectedInputs = 0;
    for (int i = 0; i < nInputs; ++i) {
        //Check that the bitdepths are all the same if the plug-in doesn't support multiple depths
        if ( !multipleClipDepths && (metadata.getBitDepth(i) != outputDepth) ) {
        }

        const double pixelAspect = metadata.getPixelAspectRatio(i);

        if (!multipleClipPARs) {
            if (pixelAspect != outputPAR) {
                mustWarnPAR = true;
            }
        }

        if (!inputs[i]) {
            continue;
        }

        ++nbConnectedInputs;

        const double fps = inputs[i]->getFrameRate();



        if (!multipleClipFPSs) {
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
        if ( Image::isBitDepthConversionLossy( inputOutputDepth, metadata.getBitDepth(i) ) ) {
            bitDepthWarning.append( QString::fromUtf8( inputs[i]->getNode()->getLabel_mt_safe().c_str() ) );
            bitDepthWarning.append( QString::fromUtf8(" (") + QString::fromUtf8( Image::getDepthString(inputOutputDepth).c_str() ) + QChar::fromLatin1(')') );
            bitDepthWarning.append( QString::fromUtf8(" ----> ") );
            bitDepthWarning.append( QString::fromUtf8( getNode()->getLabel_mt_safe().c_str() ) );
            bitDepthWarning.append( QString::fromUtf8(" (") + QString::fromUtf8( Image::getDepthString( metadata.getBitDepth(i) ).c_str() ) + QChar::fromLatin1(')') );
            bitDepthWarning.append( QChar::fromLatin1('\n') );
            setBitDepthWarning = true;
        }


        if ( !multipleClipPARs && (pixelAspect != outputPAR) ) {
            qDebug() << getScriptName_mt_safe().c_str() << ": The input " << inputs[i]->getScriptName_mt_safe().c_str()
            << ") has a pixel aspect ratio (" << metadata.getPixelAspectRatio(i)
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
    
    
    getNode()->setStreamWarnings(warnings);
} // refreshMetadaWarnings


bool
EffectInstance::shouldCacheOutput(bool isFrameVaryingOrAnimated,
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
        RotoDrawableItemPtr attachedStroke = getAttachedRotoItem();

        return isForceCachingEnabled() || appPTR->isAggressiveCachingEnabled() ||
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
    if ( !getCurrentSupportTiles() ) {
        // No tiles, image is going to be produced fully, cache it to prevent multiple access
        // with different RoIs
        return true;
    }
    if ( isForceCachingEnabled() ) {
        // Users wants it cached
        return true;
    }

    NodeGroupPtr parentIsGroup = toNodeGroup( node->getGroup() );
    if ( parentIsGroup && parentIsGroup->isForceCachingEnabled() && (parentIsGroup->getOutputNodeInput() == node) ) {
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

    if ( isDuringPaintStrokeCreation() ) {
        // When painting we must always cache
        return true;
    }

    RotoDrawableItemPtr attachedStroke = getAttachedRotoItem();
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
    if (_imp->renderData) {
        if (n < 0 || n >= (int)_imp->renderData->inputs.size()) {
            return EffectInstancePtr();
        }
        return _imp->renderData->inputs[n].lock();
    } else {
        NodePtr inputNode = getNode()->getInput(n);
        if (inputNode) {
            return inputNode->getEffectInstance();
        }

        return EffectInstancePtr();
    }
}

std::string
EffectInstance::getInputLabel(int inputNb) const
{
    std::string out;

    out.append( 1, (char)(inputNb + 65) );

    return out;
}

void
EffectInstance::onInputLabelChanged(int inputNb, const std::string& label)
{
    std::map<int, MaskSelector>::iterator foundMask = _imp->defKnobs->maskSelectors.find(inputNb);
    if (foundMask != _imp->defKnobs->maskSelectors.end()) {
        foundMask->second.channel.lock()->setLabel(label);
    }

    std::map<int, ChannelSelector>::iterator foundChannel = _imp->defKnobs->channelsSelectors.find(inputNb);
    if (foundChannel != _imp->defKnobs->channelsSelectors.end()) {
        foundChannel->second.layer.lock()->setLabel(label + std::string(" Layer"));
    }
}

std::string
EffectInstance::getInputHint(int /*inputNb*/) const
{
    return std::string();
}



bool
EffectInstance::resolveRoIForGetImage(const GetImageInArgs& inArgs,
                                      TimeValue inputTime,
                                      RectD* roiCanonical)
{


    // Use the provided image bounds if any
    if (inArgs.optionalBounds) {
        *roiCanonical = * inArgs.optionalBounds;
        return true;
    }

    if (!inArgs.requestData) {
        // We are not in a render, let the ctor of TreeRender ask for the RoD
        return false;
    }


    // Is there a request that was filed with getFramesNeeded on this input at the given time/view ?
    // There may be not as we do not create a FrameViewRequest for all frames needed by a plug-in
    // e.g: Slitscan can ask for over 1000 frames in input.
    {
        EffectInstancePtr inputEffect = getInput(inArgs.inputNb);
        FrameViewRequestPtr inputRequestPass = inputEffect->_imp->getFrameViewRequest(inputTime, inArgs.inputView);
        if (inputRequestPass) {
            *roiCanonical = inputRequestPass->getCurrentRoI();
            return true;
        }
    }
    

    // We must call getRegionOfInterest on the time and view and the current render window of the current action of this effect.
    RenderScale currentScale = EffectInstance::Implementation::getCombinedScale(inArgs.requestData->getRenderMappedMipMapLevel(), inArgs.requestData->getProxyScale());


    // If we are during a render action, retrieve the current renderWindow
    RectD thisEffectRenderWindowCanonical;
    if (inArgs.currentRenderWindow) {
        assert(inArgs.requestData);
        double par = getAspectRatio(-1);
        RectD rod;
        {
            GetRegionOfDefinitionResultsPtr results;
            ActionRetCodeEnum stat = getRegionOfDefinition_public(inArgs.requestData->getTime(), currentScale, inArgs.requestData->getView(), &results);
            if (isFailureRetCode(stat)) {
                return false;
            }
            rod = results->getRoD();
        }
        inArgs.currentRenderWindow->toCanonical(currentScale, par, rod, &thisEffectRenderWindowCanonical);
    }

    // If we are not during a render action, we may be in an action called on a render thread and have a FrameViewRequest:
    // we at least now the RoI asked on this image
    if (thisEffectRenderWindowCanonical.isNull() && inArgs.requestData) {
        thisEffectRenderWindowCanonical = inArgs.requestData->getCurrentRoI();
    }


    // If we still did not figure out the current render window so far, that means we are probably not on a render thread anyway so just
    // return false and let getImagePlane ask for the RoD in input.
    if (thisEffectRenderWindowCanonical.isNull()) {
        return false;
    }

    // Get the roi for the current render window

    RoIMap inputRoisMap;
    ActionRetCodeEnum stat = getRegionsOfInterest_public(inArgs.requestData->getTime(), currentScale, thisEffectRenderWindowCanonical, inArgs.requestData->getView(), &inputRoisMap);
    if (isFailureRetCode(stat)) {
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
, optionalBounds(0)
, plane(0)
, renderBackend(0)
, currentRenderWindow(0)
, requestData()
, draftMode(false)
, playback(false)
, byPassCache(false)
{

}

EffectInstance::GetImageInArgs::GetImageInArgs(const FrameViewRequestPtr& requestPass, const RectI* renderWindow, const RenderBackendTypeEnum* backend)
: inputNb(0)
, inputTime(0)
, inputView(0)
, inputProxyScale(1.)
, inputMipMapLevel(0)
, optionalBounds(0)
, plane(0)
, renderBackend(backend)
, currentRenderWindow(renderWindow)
, requestData(requestPass)
, draftMode(false)
, playback(false)
, byPassCache(false)
{
    if (requestPass) {
        inputTime = requestPass->getTime();
        inputView = requestPass->getView();
        inputMipMapLevel = requestPass->getRenderMappedMipMapLevel();
        inputProxyScale = requestPass->getProxyScale();
        TreeRenderPtr render = requestPass->getRenderClone()->getCurrentRender();
        draftMode = render->isDraftRender();
        playback = render->isPlayback();
        byPassCache = render->isByPassCacheEnabled();
    }


}

bool
EffectInstance::getImagePlane(const GetImageInArgs& inArgs, GetImageOutArgs* outArgs)
{
    if (inArgs.inputTime != inArgs.inputTime) {
        // time is NaN
#ifdef DEBUG
        qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inArgs.inputNb << "failing because time is NaN";
#endif
        return false;
    }

    EffectInstancePtr inputEffect = getInput(inArgs.inputNb);

    if (!inputEffect) {
        // Disconnected input
        return false;
    }

    // Get the requested RoI for the input, if we can recover it, otherwise TreeRender will render the RoD.
    RectD roiCanonical;
    if (!resolveRoIForGetImage(inArgs, inArgs.inputTime, &roiCanonical)) {
        // If we did not resolve the RoI, ask for the RoD
        GetRegionOfDefinitionResultsPtr results;
        RenderScale combinedScale = EffectInstance::Implementation::getCombinedScale(inArgs.inputMipMapLevel, inArgs.inputProxyScale);
        ActionRetCodeEnum stat = inputEffect->getRegionOfDefinition_public(inArgs.inputTime, combinedScale, inArgs.inputView, &results);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        assert(results);
        roiCanonical = results->getRoD();
    }

    if (roiCanonical.isNull()) {
        return false;
    }

    // Launch a render to recover the image.
    // It should be very fast if the image was already rendered.
    TreeRenderPtr renderObject = getCurrentRender();
    FrameViewRequestPtr outputRequest;
    if (renderObject) {
        ActionRetCodeEnum status = renderObject->launchRenderWithArgs(inputEffect, inArgs.inputTime, inArgs.inputView, inArgs.inputProxyScale, inArgs.inputMipMapLevel, inArgs.plane, &roiCanonical, &outputRequest);
        if (isFailureRetCode(status)) {
            return false;
        }
    } else {
        // We are not during a render, create one.
        TreeRender::CtorArgsPtr rargs(new TreeRender::CtorArgs());
        rargs->time = inArgs.inputTime;
        rargs->view = inArgs.inputView;
        rargs->treeRootEffect = inputEffect;
        rargs->canonicalRoI = &roiCanonical;
        rargs->proxyScale = inArgs.inputProxyScale;
        rargs->mipMapLevel = inArgs.inputMipMapLevel;
        rargs->plane = inArgs.plane;
        rargs->draftMode = inArgs.draftMode;
        rargs->playback = inArgs.playback;
        rargs->byPassCache = inArgs.byPassCache;
        renderObject = TreeRender::create(rargs);
        ActionRetCodeEnum status = renderObject->launchRender(&outputRequest);
        if (isFailureRetCode(status)) {
            return false;
        }
    }


    // Copy in output the distortion stack
    outArgs->distortionStack = outputRequest->getDistorsionStack();


    // Get the RoI in pixel coordinates of the effect we rendered
    RenderScale inputCombinedScale = EffectInstance::Implementation::getCombinedScale(inArgs.inputMipMapLevel, inArgs.inputProxyScale);
    double inputPar = getAspectRatio(inArgs.inputNb);
    roiCanonical.toPixelEnclosing(inputCombinedScale, inputPar, &outArgs->roiPixel);


    // Map the output image to the plug-in preferred format
    ImageBufferLayoutEnum thisEffectSupportedImageLayout = getPreferredBufferLayout();

    const bool supportsMultiPlane = isMultiPlanar();

    // The output image unmapped
    outArgs->image = outputRequest->getImagePlane();

    bool mustConvertImage = false;
    StorageModeEnum storage = outArgs->image->getStorageMode();
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


    ImageBufferLayoutEnum imageLayout = outArgs->image->getBufferFormat();
    if (imageLayout != thisEffectSupportedImageLayout) {
        mustConvertImage = true;
    }


    ImagePlaneDesc preferredLayer = outArgs->image->getLayer();

    // If this node does not support multi-plane or the image is the color plane,
    // map it to this node preferred color plane
    if (!supportsMultiPlane || outArgs->image->getLayer().isColorPlane()) {
        ImagePlaneDesc plane, pairedPlane;
        getMetadataComponents(inArgs.inputNb, &plane, &pairedPlane);
        if (outArgs->image->getLayer() != plane) {
            mustConvertImage = true;
            preferredLayer = plane;
        }
    }
    ImageBitDepthEnum thisBitDepth = getBitDepth(inArgs.inputNb);
    // Map bit-depth
    if (thisBitDepth != outArgs->image->getBitDepth()) {
        mustConvertImage = true;
    }

    ImagePtr convertedImage = outArgs->image;
    if (mustConvertImage) {

        Image::InitStorageArgs initArgs;
        {
            initArgs.bounds = outArgs->image->getBounds();
            initArgs.proxyScale = outArgs->image->getProxyScale();
            initArgs.mipMapLevel = outArgs->image->getMipMapLevel();
            initArgs.layer = preferredLayer;
            initArgs.bitdepth = thisBitDepth;
            initArgs.bufferFormat = thisEffectSupportedImageLayout;
            initArgs.storage = preferredStorage;
            initArgs.renderClone = shared_from_this();
            initArgs.glContext = getCurrentRender()->getGPUOpenGLContext();

        }

        convertedImage = Image::create(initArgs);


        int channelForMask = - 1;
        ImagePlaneDesc maskComps;
        {
            std::list<ImagePlaneDesc> upstreamAvailableLayers;
            ActionRetCodeEnum stat = getAvailableLayers(getCurrentTime_TLS(), getCurrentView_TLS(), inArgs.inputNb, &upstreamAvailableLayers);
            if (isFailureRetCode(stat)) {
                return EffectInstancePtr();
            }
            channelForMask = getMaskChannel(inArgs.inputNb, upstreamAvailableLayers, &maskComps);
        }
        
        
        Image::CopyPixelsArgs copyArgs;
        {
            copyArgs.roi = initArgs.bounds;
            copyArgs.conversionChannel = channelForMask;
            copyArgs.srcColorspace = getApp()->getDefaultColorSpaceForBitDepth(outArgs->image->getBitDepth());
            copyArgs.dstColorspace = getApp()->getDefaultColorSpaceForBitDepth(thisBitDepth);
            copyArgs.monoConversion = Image::eMonoToPackedConversionCopyToChannelAndFillOthers;
        }
        convertedImage->copyPixels(*outArgs->image, copyArgs);
        outArgs->image = convertedImage;
    } // mustConvertImage

    return true;
} // getImagePlane

TimeValue
EffectInstance::getCurrentTime_TLS() const
{
    if (!_imp->renderData) {
        return KnobHolder::getCurrentTime_TLS();
    }

    FrameViewRequestPtr requestData = _imp->renderData->currentFrameView.lock();
    if (requestData) {
        return requestData->getTime();
    }

    return getCurrentRender()->getTime();
}

ViewIdx
EffectInstance::getCurrentView_TLS() const
{
    if (!_imp->renderData) {
        return KnobHolder::getCurrentView_TLS();
    }
    FrameViewRequestPtr requestData = _imp->renderData->currentFrameView.lock();
    if (requestData) {
        return requestData->getView();
    }

    return getCurrentRender()->getView();
}

bool
EffectInstance::isRenderAborted() const
{
    TreeRenderPtr render = getCurrentRender();
    if (!render) {
        return false;
    }
    return render->isRenderAborted();

}

EffectInstanceTLSDataPtr
EffectInstance::getTLSObject() const
{
    return EffectInstanceTLSDataPtr();
}

EffectInstanceTLSDataPtr
EffectInstance::getTLSObjectForThread(QThread* /*thread*/) const
{
    return EffectInstanceTLSDataPtr();
}

EffectInstanceTLSDataPtr
EffectInstance::getOrCreateTLSObject() const
{
    return EffectInstanceTLSDataPtr();
}

FrameViewRequestPtr
EffectInstance::getCurrentFrameViewRequest() const
{
    assert(_imp->renderData);
    return _imp->renderData->currentFrameView.lock();
}

void
EffectInstance::setCurrentFrameViewRequest(const FrameViewRequestPtr& request)
{
    assert(_imp->renderData);
    _imp->renderData->currentFrameView = request;
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
    TreeRenderPtr render = getCurrentRender();
    if (render && render->isRenderAborted()) {
        return false;
    }

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

bool
EffectInstance::getCreateChannelSelectorKnob() const
{
    return ( !isMultiPlanar() && !isReader() && !isWriter() &&
             !boost::starts_with(getNode()->getPluginID(), "uk.co.thefoundry.furnace") );
}


PluginOpenGLRenderSupport
EffectInstance::getCurrentOpenGLSupport() const
{
    return (PluginOpenGLRenderSupport)getNode()->getPlugin()->getPropertyUnsafe<int>(kNatronPluginPropOpenGLSupport);
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
    {
        QMutexLocker k(&_imp->common->accumBufferMutex);
        _imp->common->accumBuffer.reset();
    }
    invalidateHashCache();
}

void
EffectInstance::setAccumBuffer(const ImagePtr& accumBuffer)
{
    ImagePtr curAccumBuffer;
    {
        QMutexLocker k(&_imp->common->accumBufferMutex);
        curAccumBuffer = _imp->common->accumBuffer;
        _imp->common->accumBuffer = accumBuffer;
    }
    // Ensure it is not destroyed while under the mutex, this could lead to a deadlock if the OpenGL context
    // switches during the texture destruction.
    curAccumBuffer.reset();
}

ImagePtr
EffectInstance::getAccumBuffer() const
{
    {
        QMutexLocker k(&_imp->common->accumBufferMutex);
        return _imp->common->accumBuffer;
    }
}

bool
EffectInstance::isPaintingOverItselfEnabled() const
{
    return isDuringPaintStrokeCreation();
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
EffectInstance::getOutputFormat()
{
    GetTimeInvariantMetaDatasResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetaDatas_public(&results);
    if (isFailureRetCode(stat)) {
        return RectI();
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getOutputFormat();
    }
}


bool
EffectInstance::isFrameVarying()
{
    GetTimeInvariantMetaDatasResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetaDatas_public(&results);
    if (isFailureRetCode(stat)) {
        return true;
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getIsFrameVarying();
    }
}


double
EffectInstance::getFrameRate()
{
    GetTimeInvariantMetaDatasResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetaDatas_public(&results);
    if (isFailureRetCode(stat)) {
        return 24.;
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getOutputFrameRate();
    }

}


ImagePremultiplicationEnum
EffectInstance::getPremult()
{
    GetTimeInvariantMetaDatasResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetaDatas_public(&results);
    if (isFailureRetCode(stat)) {
        return eImagePremultiplicationPremultiplied;
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getOutputPremult();
    }
}

bool
EffectInstance::canRenderContinuously()
{
    GetTimeInvariantMetaDatasResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetaDatas_public(&results);
    if (isFailureRetCode(stat)) {
        return true;
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getIsContinuous();
    }
}

ImageFieldingOrderEnum
EffectInstance::getFieldingOrder()
{
    GetTimeInvariantMetaDatasResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetaDatas_public(&results);
    if (isFailureRetCode(stat)) {
        return eImageFieldingOrderNone;
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getOutputFielding();
    }
}


double
EffectInstance::getAspectRatio(int inputNb)
{
    GetTimeInvariantMetaDatasResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetaDatas_public(&results);
    if (isFailureRetCode(stat)) {
        return 1.;
    } else {
        const NodeMetadataPtr& metadatas = results->getMetadatasResults();
        return metadatas->getPixelAspectRatio(inputNb);
    }
}

void
EffectInstance::getMetadataComponents(int inputNb, ImagePlaneDesc* plane, ImagePlaneDesc* pairedPlane)
{
    GetTimeInvariantMetaDatasResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetaDatas_public(&results);
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
EffectInstance::getBitDepth(int inputNb)
{
    GetTimeInvariantMetaDatasResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetaDatas_public(&results);
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

void
EffectInstance::checkForPremultWarningAndCheckboxes()
{
    if ( isOutput() || isGenerator() || isReader() ) {
        return;
    }
    KnobBoolPtr chans[4];
    KnobStringPtr premultWarn = _imp->defKnobs->premultWarning.lock();
    if (!premultWarn) {
        return;
    }
    NodePtr prefInput = getNode()->getPreferredInputNode();

    if ( !prefInput ) {
        //No input, do not warn
        premultWarn->setSecret(true);

        return;
    }
    for (int i = 0; i < 4; ++i) {
        chans[i] = _imp->defKnobs->enabledChan[i].lock();

        //No checkboxes
        if (!chans[i]) {
            premultWarn->setSecret(true);

            return;
        }

        //not RGBA
        if ( chans[i]->getIsSecret() ) {
            return;
        }
    }

    ImagePremultiplicationEnum premult = getPremult();

    //not premult
    if (premult != eImagePremultiplicationPremultiplied) {
        premultWarn->setSecret(true);

        return;
    }

    bool checked[4];
    checked[3] = chans[3]->getValue();

    //alpha unchecked
    if (!checked[3]) {
        premultWarn->setSecret(true);

        return;
    }
    for (int i = 0; i < 3; ++i) {
        checked[i] = chans[i]->getValue();
        if (!checked[i]) {
            premultWarn->setSecret(false);

            return;
        }
    }

    //RGB checked
    premultWarn->setSecret(true);
} // checkForPremultWarningAndCheckboxes


std::string
EffectInstance::makeInfoForInput(int inputNumber)
{
    if ( (inputNumber < -1) || ( inputNumber >= getMaxInputCount() ) ) {
        return "";
    }
    EffectInstancePtr input;
    if (inputNumber != -1) {
        input = getInput(inputNumber);
    } else {
        input = boost::const_pointer_cast<EffectInstance>(shared_from_this());
    }

    if (!input) {
        return "";
    }


    TimeValue time(getApp()->getTimeLine()->currentFrame());
    std::stringstream ss;
    { // input name
        QString inputName;
        if (inputNumber != -1) {
            inputName = QString::fromUtf8( getInputLabel(inputNumber).c_str() );
        } else {
            inputName = tr("Output");
        }
        ss << "<b><font color=\"orange\">" << tr("%1:").arg(inputName).toStdString() << "</font></b><br />";
    }
    { // image format
        ss << "<b>" << tr("Image planes:").toStdString() << "</b> <font color=#c8c8c8>";

        std::list<ImagePlaneDesc> availableLayers;
        getAvailableLayers(time, ViewIdx(0), inputNumber,  &availableLayers);

        std::list<ImagePlaneDesc>::iterator next = availableLayers.begin();
        if ( next != availableLayers.end() ) {
            ++next;
        }
        for (std::list<ImagePlaneDesc>::iterator it = availableLayers.begin(); it != availableLayers.end(); ++it) {

            ss << " "  << it->getPlaneID();
            if ( next != availableLayers.end() ) {
                ss << ", ";
                ++next;
            }
        }
        ss << "</font><br />";
    }
    {
        ImageBitDepthEnum depth = getBitDepth(inputNumber);
        QString depthStr = tr("unknown");
        switch (depth) {
            case eImageBitDepthByte:
                depthStr = tr("8u");
                break;
            case eImageBitDepthShort:
                depthStr = tr("16u");
                break;
            case eImageBitDepthFloat:
                depthStr = tr("32fp");
                break;
            case eImageBitDepthHalf:
                depthStr = tr("16fp");
            case eImageBitDepthNone:
                break;
        }
        ss << "<b>" << tr("BitDepth:").toStdString() << "</b> <font color=#c8c8c8>" << depthStr.toStdString() << "</font><br />";
    }
    { // premult
        ImagePremultiplicationEnum premult = input->getPremult();
        QString premultStr = tr("unknown");
        switch (premult) {
            case eImagePremultiplicationOpaque:
                premultStr = tr("opaque");
                break;
            case eImagePremultiplicationPremultiplied:
                premultStr = tr("premultiplied");
                break;
            case eImagePremultiplicationUnPremultiplied:
                premultStr = tr("unpremultiplied");
                break;
        }
        ss << "<b>" << tr("Alpha premultiplication:").toStdString() << "</b> <font color=#c8c8c8>" << premultStr.toStdString() << "</font><br />";
    }
    { // par
        double par = input->getAspectRatio(-1);
        ss << "<b>" << tr("Pixel aspect ratio:").toStdString() << "</b> <font color=#c8c8c8>" << par << "</font><br />";
    }
    { // fps
        double fps = input->getFrameRate();
        ss << "<b>" << tr("Frame rate:").toStdString() << "</b> <font color=#c8c8c8>" << tr("%1fps").arg(fps).toStdString() << "</font><br />";
    }
    {
        RangeD range = {1., 1.};
        {
            GetFrameRangeResultsPtr results;
            ActionRetCodeEnum stat = input->getFrameRange_public(&results);
            if (!isFailureRetCode(stat)) {
                results->getFrameRangeResults(&range);
            }
        }
        ss << "<b>" << tr("Frame range:").toStdString() << "</b> <font color=#c8c8c8>" << range.min << " - " << range.max << "</font><br />";
    }

    {
        GetRegionOfDefinitionResultsPtr results;
        ActionRetCodeEnum stat = input->getRegionOfDefinition_public(time, RenderScale(1.), ViewIdx(0), &results);
        if (!isFailureRetCode(stat)) {
            RectD rod = results->getRoD();
            ss << "<b>" << tr("Region of Definition (at t=%1):").arg(time).toStdString() << "</b> <font color=#c8c8c8>";
            ss << tr("left = %1 bottom = %2 right = %3 top = %4").arg(rod.x1).arg(rod.y1).arg(rod.x2).arg(rod.y2).toStdString() << "</font><br />";
        }
    }


    return ss.str();
} // makeInfoForInput

void
EffectInstance::refreshInfos()
{
    std::stringstream ssinfo;
    int maxinputs = getMaxInputCount();
    for (int i = 0; i < maxinputs; ++i) {
        std::string inputInfo = makeInfoForInput(i);
        if ( !inputInfo.empty() ) {
            ssinfo << inputInfo << "<br />";
        }
    }
    std::string outputInfo = makeInfoForInput(-1);
    ssinfo << outputInfo << "<br />";
    ssinfo << "<b>" << tr("Supports tiles:").toStdString() << "</b> <font color=#c8c8c8>";
    ssinfo << ( getCurrentSupportTiles() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
    {
        ssinfo << "<b>" << tr("Supports multiresolution:").toStdString() << "</b> <font color=#c8c8c8>";
        ssinfo << ( supportsMultiResolution() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
        ssinfo << "<b>" << tr("Supports renderscale:").toStdString() << "</b> <font color=#c8c8c8>";
        if (!getCurrentSupportRenderScale()) {
            ssinfo << tr("No").toStdString();
        } else {
            ssinfo << tr("Yes").toStdString();
        }
        ssinfo << "</font><br />";
        ssinfo << "<b>" << tr("Supports multiple clip PARs:").toStdString() << "</b> <font color=#c8c8c8>";
        ssinfo << ( supportsMultipleClipPARs() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
        ssinfo << "<b>" << tr("Supports multiple clip depths:").toStdString() << "</b> <font color=#c8c8c8>";
        ssinfo << ( supportsMultipleClipDepths() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
    }
    ssinfo << "<b>" << tr("Render thread safety:").toStdString() << "</b> <font color=#c8c8c8>";
    switch ( getCurrentRenderThreadSafety() ) {
        case eRenderSafetyUnsafe:
            ssinfo << tr("Unsafe").toStdString();
            break;

        case eRenderSafetyInstanceSafe:
            ssinfo << tr("Safe").toStdString();
            break;

        case eRenderSafetyFullySafe:
            ssinfo << tr("Fully safe").toStdString();
            break;

        case eRenderSafetyFullySafeFrame:
            ssinfo << tr("Fully safe frame").toStdString();
            break;
    }
    ssinfo << "</font><br />";
    ssinfo << "<b>" << tr("OpenGL Rendering Support:").toStdString() << "</b>: <font color=#c8c8c8>";
    PluginOpenGLRenderSupport glSupport = getCurrentOpenGLRenderSupport();
    switch (glSupport) {
        case ePluginOpenGLRenderSupportNone:
            ssinfo << tr("No").toStdString();
            break;
        case ePluginOpenGLRenderSupportNeeded:
            ssinfo << tr("Yes but CPU rendering is not supported").toStdString();
            break;
        case ePluginOpenGLRenderSupportYes:
            ssinfo << tr("Yes").toStdString();
            break;
        default:
            break;
    }
    ssinfo << "</font>";
    _imp->defKnobs->nodeInfos.lock()->setValue( ssinfo.str() );
} // refreshInfos


void
EffectInstance::setRenderThreadSafety(RenderSafetyEnum safety)
{
    QMutexLocker k(&_imp->common->pluginsPropMutex);

    _imp->common->props.currentThreadSafety = safety;
}

RenderSafetyEnum
EffectInstance::getCurrentRenderThreadSafety() const
{
    if (_imp->renderData) {
        return _imp->renderData->props.currentThreadSafety;
    }
    if ( !getNode()->isMultiThreadingSupportEnabledForPlugin() ) {
        return eRenderSafetyUnsafe;
    }
    QMutexLocker k(&_imp->common->pluginsPropMutex);

    return _imp->common->props.currentThreadSafety;
}

void
EffectInstance::revertToPluginThreadSafety()
{
    QMutexLocker k(&_imp->common->pluginsPropMutex);

    _imp->common->props.currentThreadSafety = _imp->common->pluginSafety;
}


RenderSafetyEnum
EffectInstance::getPluginRenderThreadSafety() const
{
    QMutexLocker k(&_imp->common->pluginsPropMutex);
    return _imp->common->pluginSafety;
}

void
EffectInstance::setCurrentOpenGLRenderSupport(PluginOpenGLRenderSupport support)
{
    QMutexLocker k(&_imp->common->pluginsPropMutex);

    _imp->common->props.currentSupportOpenGLRender = support;
}

PluginOpenGLRenderSupport
EffectInstance::getCurrentOpenGLRenderSupport()
{
    if (_imp->renderData) {
        return _imp->renderData->props.currentSupportOpenGLRender;
    }

    PluginPtr plugin = getNode()->getPlugin();
    if (plugin) {
        PluginOpenGLRenderSupport pluginProp = (PluginOpenGLRenderSupport)plugin->getPropertyUnsafe<int>(kNatronPluginPropOpenGLSupport);
        if (pluginProp != ePluginOpenGLRenderSupportYes) {
            return pluginProp;
        }
    }

    if (!getApp()->getProject()->isGPURenderingEnabledInProject()) {
        return ePluginOpenGLRenderSupportNone;
    }

    // Ok still turned on, check the value of the opengl support knob in the Node page
    KnobChoicePtr openglSupportKnob = getOrCreateOpenGLEnabledKnob();
    if (openglSupportKnob) {
        int index = openglSupportKnob->getValue();
        if (index == 1) {
            return ePluginOpenGLRenderSupportNone;
        } else if (index == 2 && getApp()->isBackground()) {
            return ePluginOpenGLRenderSupportNone;
        }
    }

    // Descriptor returned that it supported OpenGL, let's see if it turned off/on in the instance the OpenGL rendering
    QMutexLocker k(&_imp->common->pluginsPropMutex);

    return _imp->common->props.currentSupportOpenGLRender;
}

void
EffectInstance::setCurrentSequentialRenderSupport(SequentialPreferenceEnum support)
{
    QMutexLocker k(&_imp->common->pluginsPropMutex);

    _imp->common->props.currentSupportSequentialRender = support;
}

SequentialPreferenceEnum
EffectInstance::getCurrentSequentialRenderSupport() const
{
    if (_imp->renderData) {
        return _imp->renderData->props.currentSupportSequentialRender;
    }

    QMutexLocker k(&_imp->common->pluginsPropMutex);

    return _imp->common->props.currentSupportSequentialRender;
}

void
EffectInstance::setCurrentSupportTiles(bool support)
{
    QMutexLocker k(&_imp->common->pluginsPropMutex);

    _imp->common->props.currentSupportTiles = support;
}

bool
EffectInstance::getCurrentSupportTiles() const
{
    if (_imp->renderData) {
        return _imp->renderData->props.currentSupportTiles;
    }
    QMutexLocker k(&_imp->common->pluginsPropMutex);

    return _imp->common->props.currentSupportTiles;
}

void
EffectInstance::setCurrentSupportRenderScale(bool support)
{
    QMutexLocker k(&_imp->common->pluginsPropMutex);

    _imp->common->props.currentSupportsRenderScale = support;
}

bool
EffectInstance::getCurrentSupportRenderScale() const
{
    if (_imp->renderData) {
        return _imp->renderData->props.currentSupportsRenderScale;
    }
    QMutexLocker k(&_imp->common->pluginsPropMutex);

    return _imp->common->props.currentSupportsRenderScale;
}

void
EffectInstance::setCurrentCanDistort(bool support)
{
    QMutexLocker k(&_imp->common->pluginsPropMutex);

    _imp->common->props.currentCanDistort = support;
}

bool
EffectInstance::getCurrentCanDistort() const
{
    if (_imp->renderData) {
        return _imp->renderData->props.currentCanDistort;
    }
    QMutexLocker k(&_imp->common->pluginsPropMutex);

    return _imp->common->props.currentCanDistort;
}

void
EffectInstance::setCurrentCanTransform(bool support)
{

    QMutexLocker k(&_imp->common->pluginsPropMutex);

    _imp->common->props.currentDeprecatedTransformSupport = support;
}

bool
EffectInstance::getCurrentCanTransform() const
{
    if (_imp->renderData) {
        return _imp->renderData->props.currentDeprecatedTransformSupport;
    }
    QMutexLocker k(&_imp->common->pluginsPropMutex);
    
    return _imp->common->props.currentDeprecatedTransformSupport;
}


void
EffectInstance::refreshDynamicProperties()
{
    PluginOpenGLRenderSupport pluginGLSupport = ePluginOpenGLRenderSupportNone;
    PluginPtr plugin = getNode()->getPlugin();
    if (plugin) {
        pluginGLSupport = (PluginOpenGLRenderSupport)plugin->getPropertyUnsafe<int>(kNatronPluginPropOpenGLSupport);
        if (plugin->isOpenGLEnabled() && pluginGLSupport == ePluginOpenGLRenderSupportYes) {
            // Ok the plug-in supports OpenGL, figure out now if can be turned on/off by the instance
            pluginGLSupport = getCurrentOpenGLSupport();
        }
    }


    setCurrentOpenGLRenderSupport(pluginGLSupport);
    bool tilesSupported = supportsTiles();
    bool renderScaleSupported = supportsRenderScale();
    bool multiResSupported = supportsMultiResolution();
    bool canDistort = getCanDistort();
    bool currentDeprecatedTransformSupport = getCanTransform();

    _imp->common->pluginSafety = getCurrentRenderThreadSafety();

    if (!tilesSupported && _imp->common->pluginSafety == eRenderSafetyFullySafeFrame) {
        // an effect which does not support tiles cannot support host frame threading
        setRenderThreadSafety(eRenderSafetyFullySafe);
    } else {
        setRenderThreadSafety(_imp->common->pluginSafety);
    }

    setCurrentSupportTiles(multiResSupported && tilesSupported);
    setCurrentSupportRenderScale(renderScaleSupported);
    setCurrentSequentialRenderSupport( getSequentialPreference() );
    setCurrentCanDistort(canDistort);
    setCurrentCanTransform(currentDeprecatedTransformSupport);
}

void
EffectInstance::attachRotoItem(const RotoDrawableItemPtr& stroke)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->common->paintStroke = stroke;
    setProcessChannelsValues(true, true, true, true);
}

RotoDrawableItemPtr
EffectInstance::getOriginalAttachedItem() const
{
    return _imp->common->paintStroke.lock();
}

RotoDrawableItemPtr
EffectInstance::getAttachedRotoItem() const
{

    RotoDrawableItemPtr thisItem = _imp->common->paintStroke.lock();
    if (!thisItem) {
        return thisItem;
    }
    assert(!thisItem->isRenderClone());
    // On a render thread, use the local thread copy
    TreeRenderPtr currentRender = getCurrentRender();
    if (currentRender && thisItem->isRenderCloneNeeded()) {
        return boost::dynamic_pointer_cast<RotoDrawableItem>(toRotoItem(thisItem->getRenderClone(currentRender)));
    }
    return thisItem;
}


bool
EffectInstance::isDuringPaintStrokeCreation() const
{
    // We should render only
    RotoStrokeItemPtr attachedStroke = toRotoStrokeItem(getAttachedRotoItem());
    if (!attachedStroke) {
        return false;
    }
    return attachedStroke->isCurrentlyDrawing();
}

KnobBoolPtr
EffectInstance::getPreviewEnabledKnob() const
{
    return _imp->defKnobs->previewEnabledKnob.lock();
}

void
EffectInstance::setProcessChannelsValues(bool doR,
                               bool doG,
                               bool doB,
                               bool doA)
{
    KnobBoolPtr eR = getProcessChannelKnob(0);

    if (eR) {
        eR->setValue(doR);
    }
    KnobBoolPtr eG = getProcessChannelKnob(1);
    if (eG) {
        eG->setValue(doG);
    }
    KnobBoolPtr eB = getProcessChannelKnob(2);
    if (eB) {
        eB->setValue(doB);
    }
    KnobBoolPtr eA = getProcessChannelKnob(3);
    if (eA) {
        eA->setValue(doA);
    }
}

std::string
EffectInstance::getKnobChangedCallback() const
{
    KnobStringPtr s = _imp->defKnobs->knobChangedCallback.lock();

    return s ? s->getValue() : std::string();
}

std::string
EffectInstance::getInputChangedCallback() const
{
    KnobStringPtr s = _imp->defKnobs->inputChangedCallback.lock();

    return s ? s->getValue() : std::string();
}


std::string
EffectInstance::getBeforeRenderCallback() const
{
    KnobStringPtr s = _imp->defKnobs->beforeRender.lock();

    return s ? s->getValue() : std::string();
}

std::string
EffectInstance::getBeforeFrameRenderCallback() const
{
    KnobStringPtr s = _imp->defKnobs->beforeFrameRender.lock();

    return s ? s->getValue() : std::string();
}

std::string
EffectInstance::getAfterRenderCallback() const
{
    KnobStringPtr s = _imp->defKnobs->afterRender.lock();

    return s ? s->getValue() : std::string();
}

std::string
EffectInstance::getAfterFrameRenderCallback() const
{
    KnobStringPtr s = _imp->defKnobs->afterFrameRender.lock();

    return s ? s->getValue() : std::string();
}

std::string
EffectInstance::getAfterNodeCreatedCallback() const
{
    KnobStringPtr s = _imp->defKnobs->nodeCreatedCallback.lock();

    return s ? s->getValue() : std::string();
}

std::string
EffectInstance::getBeforeNodeRemovalCallback() const
{
    KnobStringPtr s = _imp->defKnobs->nodeRemovalCallback.lock();

    return s ? s->getValue() : std::string();
}

std::string
EffectInstance::getAfterSelectionChangedCallback() const
{
    KnobStringPtr s = _imp->defKnobs->tableSelectionChangedCallback.lock();

    return s ? s->getValue() : std::string();
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_EffectInstance.cpp"

