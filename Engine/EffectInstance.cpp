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
#include "Engine/GroupInput.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/PluginMemory.h"
#include "Engine/Project.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/RenderEngine.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/ReadNode.h"
#include "Engine/TreeRenderQueueManager.h"
#include "Engine/Settings.h"
#include "Engine/Plugin.h"
#include "Engine/Timer.h"
#include "Engine/TreeRender.h"
#include "Engine/Transform.h"
#include "Engine/UndoCommand.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"


NATRON_NAMESPACE_ENTER


EffectInstance::EffectInstance(const NodePtr& node)
    : NamedKnobHolder( node ? node->getApp() : AppInstancePtr() )
    , _imp( new Implementation(this) )
{
    _imp->common->node = node;

    // For the main instance, the descriptionPtr points to the common descriptor
    _imp->descriptionPtr = _imp->common->descriptor;

    if (node) {
        // Copy the default plug-in properties
        EffectDescriptionPtr pluginDesc = node->getPlugin()->getEffectDescriptor();
        _imp->descriptionPtr->cloneProperties(*pluginDesc);
    }
}

EffectInstance::EffectInstance(const EffectInstancePtr& other, const FrameViewRenderKey& key)
    : NamedKnobHolder(other, key)
    , _imp( new Implementation(this, *other->_imp) )
{
    _imp->renderData->node = other->getNode();
    _imp->common->node = _imp->renderData->node;
    assert(_imp->renderData->node);

    {
        QMutexLocker k(&_imp->common->pluginsPropMutex);
        _imp->descriptionPtr = boost::make_shared<EffectDescription>();
        _imp->descriptionPtr->cloneProperties(*_imp->common->descriptor);
    }
}

EffectInstance::~EffectInstance()
{
}

NodePtr
EffectInstance::getNode() const
{

    NodePtr ret = _imp->common->node.lock();
    if (ret) {
        return ret;
    }

    // At this point, the main node pointer is not alive, if we are a RenderClone, we should still have a shared pointer
    // to the node for safety, this pointer will die when the render is finished.
    if (_imp->renderData) {
        return _imp->renderData->node;
    }
    return NodePtr();
}


KnobHolderPtr
EffectInstance::createRenderCopy(const FrameViewRenderKey& key) const
{
    GCC_DIAG_PEDANTIC_OFF
    EffectRenderCloneBuilder createFunc = (EffectRenderCloneBuilder)getNode()->getPlugin()->getPropertyUnsafe<void*>(kNatronPluginPropCreateRenderCloneFunc);
    GCC_DIAG_PEDANTIC_ON
    assert(createFunc);
    if (!createFunc) {
        throw std::invalid_argument("EffectInstance::createRenderCopy: No kNatronPluginPropCreateRenderCloneFunc property set on plug-in!");
    }

    // A node group is never cloned since its not part of the render tree
    if (dynamic_cast<const NodeGroup*>(this)) {
        return boost::const_pointer_cast<EffectInstance>(shared_from_this());
    }

    NodePtr node = getNode();
    if (!node) {
        // No node pointer ? It is probably because destroyNode() has been called, return a NULL pointer to
        // fail the render.
        return KnobHolderPtr();
    }

    EffectInstancePtr clone = createFunc(boost::const_pointer_cast<EffectInstance>(shared_from_this()), key);
    

    // Make a copy of the main instance input locally so the state of the graph does not change throughout the render
    int nInputs = getNInputs();


    clone->_imp->renderData->mainInstanceInputs.resize(nInputs);
    clone->_imp->renderData->renderInputs.resize(nInputs);

    FrameViewPair p = {key.time, key.view};
    for (int i = 0; i < nInputs; ++i) {
       
        EffectInstancePtr mainInstanceInput = getInputMainInstance(i);
        clone->_imp->renderData->mainInstanceInputs[i] = mainInstanceInput;
        if (mainInstanceInput) {
            EffectInstancePtr inputClone = toEffectInstance(mainInstanceInput->createRenderClone(key));
            clone->_imp->renderData->renderInputs[i][p] = inputClone;

        }
    }

    return clone;
}

RenderEnginePtr
EffectInstance::createRenderEngine()
{
    return RenderEngine::create(getNode());
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
        int nInputs = getNInputs();
        for (int i = 0; i < nInputs; ++i) {
            EffectInstancePtr input = getInputRenderEffectAtAnyTimeView(i);
            if (!input) {
                hash->append(0);
            } else {
                U64 inputHash = input->computeHash(args);
                hash->append(inputHash);
            }
        }
    } else {
        // We must add the input hash at the frames needed because a node may depend on a value at a different frame

        if (isFrameVarying()) {
            hash->append((double)roundImageTimeToEpsilon(args.time));
        }
        if (getViewVariance() == eViewInvarianceAllViewsVariant) {
            hash->append((int)args.view);
        }

        ActionRetCodeEnum stat = getFramesNeeded_public(args.time, args.view, &framesNeededResults);

        FramesNeededMap framesNeeded;
        if (!isFailureRetCode(stat)) {
            framesNeededResults->getFramesNeeded(&framesNeeded);
        }
        for (FramesNeededMap::const_iterator it = framesNeeded.begin(); it != framesNeeded.end(); ++it) {

            // For all views requested in input
            for (FrameRangesMap::const_iterator viewIt = it->second.begin(); viewIt != it->second.end(); ++viewIt) {

                // For all ranges in this view
                for (U32 range_index = 0; range_index < viewIt->second.size(); ++range_index) {

                    const RangeD& range = viewIt->second[range_index];

                    if (range.min == INT_MIN || range.max == INT_MAX) {
                        // Infinite range ? This is probably because the upstream node returned the default value for
                        // EffectInstance::getFrameRange() and no input node was connected.
                        // Just do not do anything
                        continue;
                    }

                    // For all frames in the range
                    for (double f = range.min; f <= range.max; f += 1.) {

                        ComputeHashArgs inputArgs = args;
                        inputArgs.time = TimeValue(f);
                        inputArgs.view = viewIt->first;

                        EffectInstancePtr inputEffect = getInputRenderEffect(it->first, inputArgs.time, inputArgs.view);
                        if (!inputEffect) {
                            continue;
                        }

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
        cacheKey = boost::make_shared<GetFramesNeededKey>(hashValue, getNode()->getPluginID() );


        CacheEntryLockerBasePtr cacheAccess = appPTR->getGeneralPurposeCache()->get(framesNeededResults);
        
        CacheEntryLockerBase::CacheEntryStatusEnum cacheStatus = cacheAccess->getStatus();
        if (cacheStatus == CacheEntryLockerBase::eCacheEntryStatusMustCompute) {
            cacheAccess->insertInCache();
        }
    }


} // appendToHash

bool
EffectInstance::invalidateHashCacheRecursive(const bool recurse, std::set<HashableObject*>* invalidatedObjects)
{
    // Clear hash on this node

    if (!HashableObject::invalidateHashCacheInternal(invalidatedObjects)) {
        return false;
    }

    //qDebug() << "Invalidate hash of" << getScriptName_mt_safe().c_str();


    if (recurse) {
        NodesList outputs;
        getNode()->getOutputsWithGroupRedirection(outputs);
        for (NodesList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
            const NodePtr& outputNode = *it;
            if (!outputNode) {
                continue;
            }
            EffectInstancePtr effect = outputNode->getEffectInstance();
            if (effect) {
                effect->invalidateHashCacheRecursive(recurse, invalidatedObjects);
            }
        }
    }
    return true;
} // invalidateHashCacheImplementation

bool
EffectInstance::invalidateHashCacheInternal(std::set<HashableObject*>* invalidatedObjects)
{
    return invalidateHashCacheRecursive(true /*recurse*/, invalidatedObjects);
}

void
EffectInstance::refreshMetadaWarnings(const NodeMetadata &metadata)
{
    assert(QThread::currentThread() == qApp->thread());

    int nInputs = getNInputs();

    QString bitDepthWarning = tr("This nodes converts higher bit depths images from its inputs to a lower bitdepth image. As "
                                 "a result of this process, the quality of the images is degraded. The following conversions are done:\n");
    bool setBitDepthWarning = false;
    const bool multipleClipDepths = isMultipleInputsWithDifferentBitDepthsSupported();
    const bool multipleClipPARs = isMultipleInputsWithDifferentPARSupported();
    const bool multipleClipFPSs = isMultipleInputsWithDifferentFPSSupported();
    std::vector<EffectInstancePtr> inputs(nInputs);
    for (int i = 0; i < nInputs; ++i) {
        inputs[i] = getInputMainInstance(i);
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

    OutputNodesMap outputs;
    node->getOutputs(outputs);
    for (OutputNodesMap::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        if (it->first->isSettingsPanelVisible()) {
            // An output node has its settings panel opened, meaning the user is likely to be heavily editing
            // that node, hence requesting this node's image a lot. Cache it.
            return true;
        }
    }

    if (!isFrameVaryingOrAnimated) {
        // This image never changes, cache it once.
        return true;
    }

    if ( isTemporalImageAccessEnabled() ) {
        // Very heavy to compute since many frames are fetched upstream. Cache it.
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
EffectInstance::getInputMainInstance(int n) const
{
    if (_imp->renderData) {
        if (n < 0 || n >= (int)_imp->renderData->mainInstanceInputs.size()) {
            return EffectInstancePtr();
        }
        return _imp->renderData->mainInstanceInputs[n].lock();
    } else {
        NodePtr inputNode = getNode()->getInput(n);
        if (inputNode) {
            return inputNode->getEffectInstance();
        }

        return EffectInstancePtr();
    }
}

EffectInstancePtr
EffectInstance::getInputRenderEffect(int n, TimeValue time, ViewIdx view) const
{
    if (!isRenderClone()) {
        return getInputMainInstance(n);
    }
    if (n < 0 || n >= (int)_imp->renderData->renderInputs.size()) {
        return EffectInstancePtr();
    }
    FrameViewPair p = {time, view};
    const FrameViewEffectMap& effectsMap = _imp->renderData->renderInputs[n];
    FrameViewEffectMap::const_iterator found = effectsMap.find(p);
    if (found == effectsMap.end()) {
        return _imp->renderData->mainInstanceInputs[n].lock();
    }
    return found->second.lock();
}

EffectInstancePtr
EffectInstance::getInputRenderEffectAtAnyTimeView(int n) const
{
    if (!isRenderClone()) {
        return getInputMainInstance(n);
    }
    if (n < 0 || n >= (int)_imp->renderData->renderInputs.size()) {
        return EffectInstancePtr();
    }
    if (_imp->renderData->renderInputs[n].empty()) {
        EffectInstancePtr mainInstanceInput = _imp->renderData->mainInstanceInputs[n].lock();
        return mainInstanceInput;
    } else {
        return _imp->renderData->renderInputs[n].begin()->second.lock();
    }
}

int
EffectInstance::getNInputs() const
{
    if (!isRenderClone()) {
        return getNode()->getNInputs();
    }
    return (int)_imp->renderData->mainInstanceInputs.size();
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
        foundChannel->second.layer.lock()->setLabel(label/* + std::string(" Layer")*/); // note: why add " Layer"?
    }
}


bool
EffectInstance::resolveRoIForGetImage(const GetImageInArgs& inArgs,
                                      unsigned int mipMapLevel,
                                      const RenderScale& proxyScale,
                                      RectD* roiCanonical,
                                      RectD* roiExpand)
{


    // Use the provided image bounds if any
    if (inArgs.optionalBounds) {
        *roiCanonical = *inArgs.optionalBounds;
        *roiExpand = *inArgs.optionalBounds;
        return true;
    }

    
    // Get the RoI on the input:
    // We must call getRegionOfInterest on the time and view and the current render window of the current action of this effect.
    RenderScale currentScale = EffectInstance::getCombinedScale(mipMapLevel, proxyScale);


    // If we are during a render action, retrieve the current renderWindow
    RectD thisEffectRenderWindowCanonical;
    if (inArgs.currentRenderWindow) {
        double par = getAspectRatio(-1);
        RectD rod;
        {
            GetRegionOfDefinitionResultsPtr results;
            ActionRetCodeEnum stat = getRegionOfDefinition_public(getCurrentRenderTime(), currentScale, getCurrentRenderView(), &results);
            if (isFailureRetCode(stat)) {
                return false;
            }
            rod = results->getRoD();
        }
        inArgs.currentRenderWindow->toCanonical(currentScale, par, rod, &thisEffectRenderWindowCanonical);
    }

    // If we still did not figure out the current render window so far, that means we are probably not on a render thread anyway so just
    // return false and let getImagePlane ask for the RoD in input.
    if (thisEffectRenderWindowCanonical.isNull()) {
        return false;
    }

    // Get the roi for the current render window

    RoIMap inputRoisMap;
    ActionRetCodeEnum stat = getRegionsOfInterest_public(getCurrentRenderTime(), currentScale, thisEffectRenderWindowCanonical, getCurrentRenderView(), &inputRoisMap);
    if (isFailureRetCode(stat)) {
        return false;
    }

    bool supportsMultiRes = supportsMultiResolution();
    if (!supportsMultiRes) {
        bool roiSet = false;
        for (RoIMap::const_iterator it = inputRoisMap.begin(); it != inputRoisMap.end(); ++it) {
            if (!roiSet) {
                *roiExpand = it->second;
                roiSet = true;
            } else {
                roiExpand->merge(it->second);
            }
        }
    }

    RoIMap::iterator foundInputEffectRoI = inputRoisMap.find(inArgs.inputNb);
    if (foundInputEffectRoI != inputRoisMap.end()) {
        *roiCanonical = foundInputEffectRoI->second;
        if (supportsMultiRes) {
            *roiExpand = *roiCanonical;
        }
        return true;
    }

    return false;
} // resolveRoIForGetImage

EffectInstance::GetImageInArgs::GetImageInArgs()
: inputNb(0)
, inputTime(0)
, inputView(0)
, currentActionProxyScale(0)
, currentActionMipMapLevel(0)
, inputProxyScale(0)
, inputMipMapLevel(0)
, optionalBounds(0)
, plane(0)
, renderBackend(0)
, currentRenderWindow(0)
, draftMode(0)
, playback(0)
, byPassCache(false)
{

}

EffectInstance::GetImageInArgs::GetImageInArgs(const unsigned int* currentMipMapLevel, const RenderScale* currentProxyScale, const RectI* currentRenderWindow, const RenderBackendTypeEnum* backend)
: inputNb(0)
, inputTime(0)
, inputView(0)
, currentActionProxyScale(currentProxyScale)
, currentActionMipMapLevel(currentMipMapLevel)
, inputProxyScale(0)
, inputMipMapLevel(0)
, optionalBounds(0)
, plane(0)
, renderBackend(backend)
, currentRenderWindow(currentRenderWindow)
, draftMode(0)
, playback(0)
, byPassCache(false)
{

}


bool
EffectInstance::getImagePlane(const GetImageInArgs& inArgs, GetImageOutArgs* outArgs)
{

    // Extract arguments and make default values if needed
    TreeRenderPtr currentRender = getCurrentRender();

    TimeValue inputTime = inArgs.inputTime ? *inArgs.inputTime : getCurrentRenderTime();

    if (inputTime != inputTime) {
        // time is NaN
#ifdef DEBUG
        qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inArgs.inputNb << "failing because time is NaN";
#endif
        return false;
    }

    const bool inputIsMask = getNode()->isInputMask(inArgs.inputNb);
    std::bitset<4> inputSupportedComps = getNode()->getSupportedComponents(inArgs.inputNb);
    bool supportsOnlyAlpha = inputSupportedComps[0] && !inputSupportedComps[1] && !inputSupportedComps[2] && !inputSupportedComps[3];
    if ( (inputIsMask || supportsOnlyAlpha) && !isMaskEnabled(inArgs.inputNb) ) {
        return false;
    }

    unsigned int currentMipMapLevel = inArgs.currentActionMipMapLevel ? *inArgs.currentActionMipMapLevel : 0;

    RenderScale currentProxyScale = inArgs.currentActionProxyScale ? *inArgs.currentActionProxyScale : RenderScale(1.);

    ViewIdx inputView = inArgs.inputView ? *inArgs.inputView : getCurrentRenderView();

    bool isDraftMode;
    if (!currentRender) {
        isDraftMode = false;
    } else {
        isDraftMode = inArgs.draftMode ? *inArgs.draftMode : currentRender->isDraftRender();
    }

    bool isPlayback;
    if (!currentRender) {
        isPlayback = false;
    } else {
        isPlayback = inArgs.playback ? *inArgs.playback : currentRender->isPlayback();
    }

    unsigned int inputMipMapLevel = inArgs.inputMipMapLevel ? *inArgs.inputMipMapLevel : currentMipMapLevel;
    RenderScale inputProxyScale = inArgs.inputProxyScale ? *inArgs.inputProxyScale : currentProxyScale;

    // Get the input effect pointer
    EffectInstancePtr inputEffect = getInputRenderEffect(inArgs.inputNb, inputTime, inputView);
    if (!inputEffect) {
        // Disconnected input
        return false;
    }


    // Get the requested RoI for the input, if we can recover it, otherwise TreeRender will render the RoD.
    // roiCanonical is the RoI returned by the getRegionsOfInterest action
    // roiExpand is the union of the RoI on all inputs in case this effect does not support multi-resolution
    RectD roiCanonical, roiExpand;
    RectD inputRoD;
    {
        // If we did not resolve the RoI, ask for the RoD
        GetRegionOfDefinitionResultsPtr results;
        RenderScale combinedScale = EffectInstance::getCombinedScale(inputMipMapLevel, inputProxyScale);
        ActionRetCodeEnum stat = inputEffect->getRegionOfDefinition_public(inputTime, combinedScale, inputView, &results);
        if (isFailureRetCode(stat)) {
            return stat;
        }
        assert(results);
        inputRoD = results->getRoD();
    }

    RenderScale inputCombinedScale = EffectInstance::getCombinedScale(inputMipMapLevel, inputProxyScale);

    double inputPar = getAspectRatio(inArgs.inputNb);
   /* RectI pixelRod;
    inputRoD.toPixelEnclosing(inputCombinedScale, inputPar, &pixelRod);
    qDebug() << QThread::currentThread() << "inputRod: " << pixelRod.x1 << pixelRod.y1 << pixelRod.x2 << pixelRod.y2 << "scale: " << inputCombinedScale.x << inputCombinedScale.y;*/

    if (!resolveRoIForGetImage(inArgs, currentMipMapLevel, currentProxyScale, &roiCanonical, &roiExpand)) {
        roiCanonical = roiExpand = inputRoD;
    } else {
        roiExpand.intersect(inputRoD, &roiExpand);
        if (!roiCanonical.intersect(inputRoD, &roiCanonical)) {
            return false;
        }
    }

    // Empty RoI
    if (roiCanonical.isNull()) {
        return false;
    }

    // Launch a render to recover the image.
    // It should be very fast if the image was already rendered.
    FrameViewRequestPtr outputRequest;
    {
        ActionRetCodeEnum status;
        if (currentRender) {

            AcceptedRequestConcatenationFlags concatFlags = _imp->getConcatenationFlagsForInput(inArgs.inputNb);

            const bool isPotentialHostFrameThreadingRender = getRenderThreadSafety() == eRenderSafetyFullySafeFrame && (!inArgs.renderBackend || *inArgs.renderBackend == eRenderBackendTypeCPU);

            TreeRenderExecutionDataPtr subLaunchData = launchSubRender(inputEffect, inputTime, inputView, inputProxyScale, inputMipMapLevel, inArgs.plane, &roiCanonical, currentRender, concatFlags, isPotentialHostFrameThreadingRender);
            outputRequest = subLaunchData->getOutputRequest();
            status = subLaunchData->getStatus();
        } else {
            // We are not during a render, create one.
            TreeRender::CtorArgsPtr rargs = boost::make_shared<TreeRender::CtorArgs>();
            rargs->provider = getThisTreeRenderQueueProviderShared();
            rargs->time = inputTime;
            rargs->view = inputView;
            rargs->treeRootEffect = inputEffect;
            rargs->canonicalRoI = roiCanonical;
            rargs->proxyScale = inputProxyScale;
            rargs->mipMapLevel = inputMipMapLevel;
            if (inArgs.plane) {
                rargs->plane = *inArgs.plane;
            }
            rargs->draftMode = isDraftMode;
            rargs->playback = isPlayback;
            rargs->byPassCache = false;
            TreeRenderPtr renderObject = TreeRender::create(rargs);
            if (!currentRender) {
                currentRender = renderObject;
            }
            launchRender(renderObject);
            status = waitForRenderFinished(renderObject);
            outputRequest = renderObject->getOutputRequest();
        }

        if (isFailureRetCode(status)) {
            return false;
        }
        assert(outputRequest);
    }


    // Copy in output the distortion stack
    outArgs->distortionStack = outputRequest->getDistorsionStack();


    // Get the RoI in pixel coordinates of the effect we rendered

    RectI roiPixels;
    RectI roiExpandPixels;
    roiExpand.toPixelEnclosing(inputCombinedScale, inputPar, &roiExpandPixels);
    roiCanonical.toPixelEnclosing(inputCombinedScale, inputPar, &roiPixels);
    assert(roiExpandPixels.contains(roiPixels) || outArgs->distortionStack);

    // Map the output image to the plug-in preferred format
    ImageBufferLayoutEnum thisEffectSupportedImageLayout = getPreferredBufferLayout();

    const bool supportsMultiPlane = isMultiPlanar();

    // The output image unmapped
    outArgs->image = outputRequest->getRequestedScaleImagePlane();

    if (!outArgs->image || !outArgs->image->isBufferAllocated()) {
        return false;
    }


    // In output of getImagePlane we also return the region that was rendered on the input image, so that
    // further code limits its processing to this region, and not actually the full image size.
    // This is useful for example if converting the image to another format (OpenFX, etc...) and to ensure
    // the plug-in is not accessing the input image at location it did not inform previously
    // with getRegionsOfInterest action.

    // Note that If we have a distortion upstream, we cannot return the roi that was requested here,
    // we would have to transform it by the *forward* distortion first.
    // However since we only have the backward distortion, our only way to recover it is to get the roi
    // that was rendered from the request object itself.
    if (outArgs->distortionStack && outArgs->image) {
        roiPixels = outArgs->image->getBounds();
        // Intersect it with the rod of the input

        EffectInstancePtr inputImageEffect = outArgs->distortionStack->getInputImageEffect();
        if (inputImageEffect) {
            GetRegionOfDefinitionResultsPtr rodResults;
            ActionRetCodeEnum stat = inputImageEffect->getRegionOfDefinition_public(inputTime, inputCombinedScale, inputView, &rodResults);
            if (isFailureRetCode(stat)) {
                return false;
            }
            RectD inputRod = rodResults->getRoD();
            RectI inputRodPixels;
            inputRod.toPixelEnclosing(inputCombinedScale, inputPar, &inputRodPixels);
            roiPixels.intersect(inputRodPixels, &roiPixels);
        }

        roiExpandPixels = roiPixels;
    }
    


    bool mustConvertImage = false;

    StorageModeEnum storage = outArgs->image->getStorageMode();
    StorageModeEnum preferredStorage = storage;
    if (inArgs.renderBackend) {
        switch (*inArgs.renderBackend) {
            case eRenderBackendTypeOpenGL: {
                preferredStorage = eStorageModeGLTex;
            }   break;
            case eRenderBackendTypeCPU:
            case eRenderBackendTypeOSMesa:
                preferredStorage = eStorageModeRAM;
                break;
        }
    }
    if (storage != preferredStorage) {
        mustConvertImage = true;
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


    if (roiExpandPixels != roiPixels) {
        mustConvertImage = true;
        outArgs->roiPixel = roiExpandPixels;
    } else {
        outArgs->roiPixel = roiPixels;
    }

    outArgs->roiPixel.intersect(outArgs->image->getBounds(), &outArgs->roiPixel);

    ImagePtr convertedImage = outArgs->image;
    if (mustConvertImage) {

        Image::InitStorageArgs initArgs;
        {

            initArgs.bounds = outArgs->roiPixel;
            initArgs.proxyScale = outArgs->image->getProxyScale();
            initArgs.mipMapLevel = outArgs->image->getMipMapLevel();
            initArgs.plane = preferredLayer;
            initArgs.bitdepth = thisBitDepth;
            initArgs.bufferFormat = thisEffectSupportedImageLayout;
            initArgs.storage = preferredStorage;
            initArgs.renderClone = inputEffect;
            initArgs.glContext = currentRender->getGPUOpenGLContext();

        }

        convertedImage = Image::create(initArgs);
        if (!convertedImage) {
            return false;
        }

        // inputIsMask || supportsOnlyAlpha
        int channelForMask = - 1;
        ImagePlaneDesc maskComps;
        if (inputIsMask || supportsOnlyAlpha) {
            std::list<ImagePlaneDesc> upstreamAvailableLayers;
            ActionRetCodeEnum stat = getAvailableLayers(inputTime, inputView, inArgs.inputNb, &upstreamAvailableLayers);
            if (isFailureRetCode(stat)) {
                return false;
            }
            channelForMask = getMaskChannel(inArgs.inputNb, upstreamAvailableLayers, &maskComps);
            if (channelForMask == -1) {
                return false;
            }
        }
        
        
        Image::CopyPixelsArgs copyArgs;
        {
            copyArgs.roi = initArgs.bounds;
            copyArgs.conversionChannel = channelForMask;
            copyArgs.srcColorspace = getApp()->getDefaultColorSpaceForBitDepth(outArgs->image->getBitDepth());
            copyArgs.dstColorspace = getApp()->getDefaultColorSpaceForBitDepth(thisBitDepth);

            // The alphaHandling flag is used when converting anything to Alpha or when converting to RGBA
            // By deafult always us
            {
                int imageNComps = outArgs->image->getComponentsCount();
                if (imageNComps == 1 || imageNComps == 4) {
                    copyArgs.alphaHandling = Image::eAlphaChannelHandlingFillFromChannel;
                    if (copyArgs.conversionChannel == -1 || copyArgs.conversionChannel >= imageNComps) {
                        if (imageNComps == 4) {
                            // If converting from RGBA, use the last channel
                            copyArgs.conversionChannel = 3;
                        } else {
                            copyArgs.conversionChannel = 0;
                        }
                    }
                } else {
                    // When converty XY or RGB to something else, use alpha 0 or 1 to fill depending on the plug-in property.
                    if (getAlphaFillWith1()) {
                        copyArgs.alphaHandling = Image::eAlphaChannelHandlingCreateFill1;
                    } else {
                        copyArgs.alphaHandling = Image::eAlphaChannelHandlingCreateFill0;
                    }
                }
            }

            // If converting from single component image to multiple component image, use the single channel
            if (channelForMask != -1) {
                copyArgs.monoConversion = Image::eMonoToPackedConversionCopyToChannelAndFillOthers;
            } else {
                copyArgs.monoConversion = Image::eMonoToPackedConversionCopyToAll;
            }
        }
        ActionRetCodeEnum stat = convertedImage->copyPixels(*outArgs->image, copyArgs);
        if (isFailureRetCode(stat)) {
            return false;   
        }
        outArgs->image = convertedImage;
    } // mustConvertImage

    // If the effect does not support multi-resolution image, add black borders so that all images have the same size in input.
    if (roiExpandPixels != roiPixels) {
        ActionRetCodeEnum stat = outArgs->image->fillOutSideWithBlack(roiPixels);
        if (isFailureRetCode(stat)) {
            return false;
        }
    }

  //  qDebug() << QThread::currentThread() << "input roi: " << outArgs->roiPixel.x1 << outArgs->roiPixel.y1 << outArgs->roiPixel.x2 << outArgs->roiPixel.y2;

    return true;
} // getImagePlane


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
                         bool refreshMetadata)
{

    NodePtr node = getNode();

    if ( refreshMetadata && node && node->isNodeCreated() ) {
        
        // Force a re-compute of the meta-data if needed
        onMetadataChanged_recursive_public();
    }

    if (isSignificant) {
        getApp()->triggerAutoSave();
    }

    node->refreshIdentityState();


    // Get the connected viewers downstream and re-render or redraw them.
    if (isSignificant) {
        getApp()->renderAllViewers();
    } else {
        getApp()->redrawAllViewers();
    }

    // If significant, also refresh previews downstream
    if (isSignificant) {
        node->refreshPreviewsRecursivelyDownstream();
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

int
EffectInstance::getInputNumber(const EffectInstancePtr& inputEffect) const
{
    for (int i = 0; i < getNInputs(); ++i) {
        if (getInputMainInstance(i) == inputEffect) {
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
EffectInstance::createMemoryChunk(size_t nBytes)
{
    PluginMemoryPtr mem = createPluginMemory();
    PluginMemAllocateMemoryArgs args(nBytes);
    mem->allocateMemory(args);
    QMutexLocker k(&_imp->pluginMemoryChunksMutex);
    _imp->pluginMemoryChunks.push_back(mem);
    return mem;
}

PluginMemoryPtr
EffectInstance::createPluginMemory()
{
    PluginMemoryPtr ret( new PluginMemory() );
    return ret;
}

void
EffectInstance::releasePluginMemory(const PluginMemory* mem)
{
    QMutexLocker k(&_imp->pluginMemoryChunksMutex);
    for (std::list<PluginMemoryPtr>::iterator it = _imp->pluginMemoryChunks.begin(); it != _imp->pluginMemoryChunks.end(); ++it) {
        if (it->get() == mem) {
            _imp->pluginMemoryChunks.erase(it);
            return;
        }
    }
}

void
EffectInstance::registerOverlay(OverlayViewportTypeEnum type, const OverlayInteractBasePtr& overlay, const std::map<std::string,std::string>& knobs)
{

    overlay->setEffect(shared_from_this());
    overlay->fetchKnobs_public(knobs);

    assert(QThread::currentThread() == qApp->thread());

    std::list<OverlayInteractBasePtr>* list = 0;
    switch (type) {
        case eOverlayViewportTypeViewer:
            list = &_imp->common->interacts;
            break;

        case eOverlayViewportTypeTimeline:
            list = &_imp->common->timelineInteracts;
            break;
    }

    std::list<OverlayInteractBasePtr>::iterator found = std::find(list->begin(), list->end(), overlay);
    if (found == list->end()) {

        list->push_back(overlay);
        overlay->redraw();
    }
}

void
EffectInstance::removeOverlay(OverlayViewportTypeEnum type, const OverlayInteractBasePtr& overlay)
{
    assert(QThread::currentThread() == qApp->thread());

    std::list<OverlayInteractBasePtr>* list = 0;
    switch (type) {
        case eOverlayViewportTypeViewer:
            list = &_imp->common->interacts;
            break;

        case eOverlayViewportTypeTimeline:
            list = &_imp->common->timelineInteracts;
            break;
    }
    std::list<OverlayInteractBasePtr>::iterator found = std::find(list->begin(), list->end(), overlay);
    if (found != list->end()) {
        list->erase(found);
    }
}

void
EffectInstance::clearOverlays(OverlayViewportTypeEnum type)
{
    switch (type) {
        case eOverlayViewportTypeViewer:
            _imp->common->interacts.clear();
            break;

        case eOverlayViewportTypeTimeline:
            _imp->common->timelineInteracts.clear();
            break;
    }
}

void
EffectInstance::getOverlays(OverlayViewportTypeEnum type, std::list<OverlayInteractBasePtr> *overlays) const
{
    assert(QThread::currentThread() == qApp->thread());
    switch (type) {
        case eOverlayViewportTypeViewer:
            *overlays = _imp->common->interacts;
            break;

        case eOverlayViewportTypeTimeline:
            *overlays = _imp->common->timelineInteracts;
            break;
    }

}

bool
EffectInstance::hasOverlayInteract(OverlayViewportTypeEnum type) const
{
    assert(QThread::currentThread() == qApp->thread());
    switch (type) {
        case eOverlayViewportTypeViewer:
            return _imp->common->interacts.size() > 0;
        case eOverlayViewportTypeTimeline:
            return _imp->common->timelineInteracts.size() > 0;
    }
    return false;
}

void
EffectInstance::setInteractColourPicker_public(const ColorRgba<double>& color, bool setColor, bool hasColor)
{

    const KnobsVec& knobs = getKnobs();
    for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
        const KnobIPtr& k = *it2;
        if (!k) {
            continue;
        }
        OverlayInteractBasePtr interact = k->getCustomInteract();
        if (!interact) {
            continue;

        }
        interact->setInteractColourPicker(color, setColor, hasColor);
    }
    for (std::list<OverlayInteractBasePtr>::iterator it = _imp->common->interacts.begin(); it != _imp->common->interacts.end(); ++it) {
        (*it)->setInteractColourPicker(color, setColor, hasColor);

    }


}

bool
EffectInstance::isDoingInteractAction() const
{
    return getNode()->isDoingInteractAction();
}

bool
EffectInstance::onKnobValueChanged(const KnobIPtr& /*k*/,
                                   ValueChangedReasonEnum /*reason*/,
                                   TimeValue /*time*/,
                                   ViewSetSpec /*view*/)
{
    return false;
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
    std::map<ImagePlaneDesc,ImagePtr> accumBuffer;
    {
        QMutexLocker k(&_imp->common->accumBufferMutex);
        accumBuffer = _imp->common->accumBuffer;
        _imp->common->accumBuffer.clear();
    }
    accumBuffer.clear();
}

void
EffectInstance::setAccumBuffer(const ImagePlaneDesc& plane, const ImagePtr& accumBuffer)
{
    ImagePtr curAccumBuffer;
    {
        QMutexLocker k(&_imp->common->accumBufferMutex);
        std::map<ImagePlaneDesc,ImagePtr>::iterator found = _imp->common->accumBuffer.find(plane);
        if (found != _imp->common->accumBuffer.end()) {
            curAccumBuffer = found->second;
            _imp->common->accumBuffer.erase(found);
        }
        _imp->common->accumBuffer.insert(std::make_pair(plane, accumBuffer));
    }
    // Ensure it is not destroyed while under the mutex, this could lead to a deadlock if the OpenGL context
    // switches during the texture destruction.
    curAccumBuffer.reset();
}

ImagePtr
EffectInstance::getAccumBuffer(const ImagePlaneDesc& plane) const
{
    {
        QMutexLocker k(&_imp->common->accumBufferMutex);
        std::map<ImagePlaneDesc,ImagePtr>::const_iterator found = _imp->common->accumBuffer.find(plane);
        if (found != _imp->common->accumBuffer.end()) {
            return found->second;
        }

    }
    return ImagePtr();
}

bool
EffectInstance::isAccumulationEnabled() const
{
    return isDuringPaintStrokeCreation();
}

bool
EffectInstance::getAccumulationUpdateRoI(RectD* updateArea) const
{
    TreeRenderPtr render = getCurrentRender();
    if (!render) {
        return false;
    }

    RotoDrawableItemPtr activeItem = render->getCurrentlyDrawingItem();
    if (!activeItem) {
        return false;
    }

    FrameViewRenderKey renderKey;
    renderKey.render = render;
    renderKey.time = getCurrentRenderTime();
    renderKey.view = getCurrentRenderView();
    RotoStrokeItemPtr attachedStroke = toRotoStrokeItem(activeItem->createRenderClone(renderKey));
    if (!attachedStroke) {
        return false;
    }
    *updateArea = attachedStroke->getLastStrokeMovementBbox();
    return true;
}

static bool hasActiveStrokeItemNodeDrawingUpstream(const EffectInstancePtr& effect,
                                            const RotoDrawableItemPtr& activeItem,
                                            std::set<EffectInstancePtr>* markedEffects)
{
    if (markedEffects->find(effect) != markedEffects->end()) {
        return false;
    }

    markedEffects->insert(effect);

    RotoStrokeItemPtr attachedStroke = toRotoStrokeItem(effect->getAttachedRotoItem());
    if (attachedStroke) {
        bool isDrawing = attachedStroke->isCurrentlyDrawing();

        // Only one single item can be drawn at once
        assert(!isDrawing || attachedStroke->getMainInstance() == activeItem);

        if (isDrawing) {
            return true;
        }
    }

    int nInputs = effect->getNInputs();
    for (int i = 0; i < nInputs; ++i) {
        EffectInstancePtr inputEffect = effect->getInputRenderEffectAtAnyTimeView(i);
        if (!inputEffect) {
            continue;
        }
        bool inputIsDrawing = hasActiveStrokeItemNodeDrawingUpstream(inputEffect, activeItem, markedEffects);
        if (inputIsDrawing) {
            return true;
        }
    }


    return false;
} // hasActiveStrokeItemNodeDrawingUpstream

bool
EffectInstance::isDuringPaintStrokeCreation() const
{
    TreeRenderPtr render = getCurrentRender();
    if (!render) {
        // Not during a render
        return false;
    }

    RotoDrawableItemPtr activeItem = render->getCurrentlyDrawingItem();
    if (!activeItem) {
        return false;
    }

    // Now if this node has the item attached to itself (i.e: it is a node of the internal roto paint graph for this item)
    // or it is below the node attached to this item then we can check if it is currently drawing
    std::set<EffectInstancePtr> markedEffects;
    return hasActiveStrokeItemNodeDrawingUpstream(boost::const_pointer_cast<EffectInstance>(shared_from_this()), activeItem, &markedEffects);
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
    GetTimeInvariantMetadataResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetadata_public(&results);
    if (isFailureRetCode(stat)) {
        return RectI();
    } else {
        const NodeMetadataPtr& metadata = results->getMetadataResults();
        return metadata->getOutputFormat();
    }
}


bool
EffectInstance::isFrameVarying()
{
    GetTimeInvariantMetadataResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetadata_public(&results);
    if (isFailureRetCode(stat)) {
        return true;
    } else {
        const NodeMetadataPtr& metadata = results->getMetadataResults();
        return metadata->getIsFrameVarying();
    }
}


double
EffectInstance::getFrameRate()
{
    GetTimeInvariantMetadataResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetadata_public(&results);
    if (isFailureRetCode(stat)) {
        return 24.;
    } else {
        const NodeMetadataPtr& metadata = results->getMetadataResults();
        return metadata->getOutputFrameRate();
    }

}


ImagePremultiplicationEnum
EffectInstance::getPremult()
{
    GetTimeInvariantMetadataResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetadata_public(&results);
    if (isFailureRetCode(stat)) {
        return eImagePremultiplicationPremultiplied;
    } else {
        const NodeMetadataPtr& metadata = results->getMetadataResults();
        return metadata->getOutputPremult();
    }
}

bool
EffectInstance::canRenderContinuously()
{
    if (getHasAnimation()) {
        return true;
    }
    GetTimeInvariantMetadataResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetadata_public(&results);
    if (isFailureRetCode(stat)) {
        return true;
    } else {
        const NodeMetadataPtr& metadata = results->getMetadataResults();
        return metadata->getIsContinuous();
    }
}

ImageFieldingOrderEnum
EffectInstance::getFieldingOrder()
{
    GetTimeInvariantMetadataResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetadata_public(&results);
    if (isFailureRetCode(stat)) {
        return eImageFieldingOrderNone;
    } else {
        const NodeMetadataPtr& metadata = results->getMetadataResults();
        return metadata->getOutputFielding();
    }
}


double
EffectInstance::getAspectRatio(int inputNb)
{
    GetTimeInvariantMetadataResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetadata_public(&results);
    if (isFailureRetCode(stat)) {
        return 1.;
    } else {
        const NodeMetadataPtr& metadata = results->getMetadataResults();
        return metadata->getPixelAspectRatio(inputNb);
    }
}

void
EffectInstance::getMetadataComponents(int inputNb, ImagePlaneDesc* plane, ImagePlaneDesc* pairedPlane)
{
    GetTimeInvariantMetadataResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetadata_public(&results);
    if (isFailureRetCode(stat)) {
        *plane = ImagePlaneDesc::getNoneComponents();
    } else {
        const NodeMetadataPtr& metadata = results->getMetadataResults();
        std::string componentsType = metadata->getComponentsType(inputNb);
        if (componentsType == kNatronColorPlaneID) {
            int nComps = metadata->getColorPlaneNComps(inputNb);
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
}

ImageBitDepthEnum
EffectInstance::getBitDepth(int inputNb)
{
    GetTimeInvariantMetadataResultsPtr results;
    ActionRetCodeEnum stat = getTimeInvariantMetadata_public(&results);
    if (isFailureRetCode(stat)) {
        return eImageBitDepthFloat;
    } else {
        const NodeMetadataPtr& metadata = results->getMetadataResults();
        return metadata->getBitDepth(inputNb);
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
    if ( (inputNumber < -1) || ( inputNumber >= getNInputs() ) ) {
        return "";
    }
    EffectInstancePtr input;
    if (inputNumber != -1) {
        input = getInputMainInstance(inputNumber);
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
            inputName = QString::fromUtf8( getNode()->getInputLabel(inputNumber).c_str() );
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

            ss << " "  << it->getPlaneLabel() << '.' << it->getChannelsLabel();
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
    {
        RectI format = input->getOutputFormat();
        if ( !format.isNull() ) {
            ss << "<b>" << tr("Format (pixels):").toStdString() << "</b> <font color=#c8c8c8>";
            ss << tr("left = %1 bottom = %2 right = %3 top = %4").arg(format.x1).arg(format.y1).arg(format.x2).arg(format.y2).toStdString() << "</font><br />";
        }
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
    int maxinputs = getNInputs();
    for (int i = 0; i < maxinputs; ++i) {
        std::string inputInfo = makeInfoForInput(i);
        if ( !inputInfo.empty() ) {
            ssinfo << inputInfo << "<br />";
        }
    }
    std::string outputInfo = makeInfoForInput(-1);
    ssinfo << outputInfo << "<br />";
    ssinfo << "<b>" << tr("Supports tiles:").toStdString() << "</b> <font color=#c8c8c8>";
    ssinfo << ( supportsTiles() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
    {
        ssinfo << "<b>" << tr("Supports multiresolution:").toStdString() << "</b> <font color=#c8c8c8>";
        ssinfo << ( supportsMultiResolution() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
        ssinfo << "<b>" << tr("Supports renderscale:").toStdString() << "</b> <font color=#c8c8c8>";
        if (!supportsRenderScale()) {
            ssinfo << tr("No").toStdString();
        } else {
            ssinfo << tr("Yes").toStdString();
        }
        ssinfo << "</font><br />";
        ssinfo << "<b>" << tr("Supports multiple clip PARs:").toStdString() << "</b> <font color=#c8c8c8>";
        ssinfo << ( isMultipleInputsWithDifferentPARSupported() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
        ssinfo << "<b>" << tr("Supports multiple clip depths:").toStdString() << "</b> <font color=#c8c8c8>";
        ssinfo << ( isMultipleInputsWithDifferentBitDepthsSupported() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
    }
    ssinfo << "<b>" << tr("Render thread safety:").toStdString() << "</b> <font color=#c8c8c8>";
    switch ( getRenderThreadSafety() ) {
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
    PluginOpenGLRenderSupport glSupport = getOpenGLRenderSupport();
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


bool
EffectInstance::isViewAware() const
{
    return getNode()->getPlugin()->getPropertyUnsafe<bool>(kNatronPluginPropViewAware);
}


ViewInvarianceLevel
EffectInstance::getViewVariance() const
{
    return getNode()->getPlugin()->getPropertyUnsafe<ViewInvarianceLevel>(kNatronPluginPropViewInvariant);
}

bool
EffectInstance::isMultiPlanar() const
{
    return getNode()->getPlugin()->getPropertyUnsafe<bool>(kNatronPluginPropMultiPlanar);
}

PlanePassThroughEnum
EffectInstance::getPlanePassThrough() const
{
    return getNode()->getPlugin()->getPropertyUnsafe<PlanePassThroughEnum>(kNatronPluginPropPlanesPassThrough);
}

bool
EffectInstance::isRenderAllPlanesAtOncePreferred() const
{
    return getNode()->getPlugin()->getPropertyUnsafe<bool>(kNatronPluginPropRenderAllPlanesAtOnce);
}

bool
EffectInstance::isDraftRenderSupported() const
{
    return getNode()->getPlugin()->getPropertyUnsafe<bool>(kNatronPluginPropSupportsDraftRender);
}

bool
EffectInstance::isHostChannelSelectorEnabled() const
{
    return getNode()->getPlugin()->getPropertyUnsafe<bool>(kNatronPluginPropHostChannelSelector);
}

std::bitset<4>
EffectInstance::getHostChannelSelectorDefaultValue() const
{
    return getNode()->getPlugin()->getPropertyUnsafe<std::bitset<4> >(kNatronPluginPropHostChannelSelectorValue);
}

bool
EffectInstance::isHostMixEnabled() const
{
    return getNode()->getPlugin()->getPropertyUnsafe<bool>(kNatronPluginPropHostMix);
}

bool
EffectInstance::isHostMaskEnabled() const
{
    return getNode()->getPlugin()->getPropertyUnsafe<bool>(kNatronPluginPropHostMask);
}

bool
EffectInstance::isHostPlaneSelectorEnabled() const
{
    PluginPtr plugin = getNode()->getPlugin();
    if (plugin->getPropertyUnsafe<bool>(kNatronPluginPropMultiPlanar)) {
        // A multi-planar plug-in handles planes on its own
        return false;
    }
    
    return plugin->getPropertyUnsafe<bool>(kNatronPluginPropHostPlaneSelector);
}

bool
EffectInstance::isMultipleInputsWithDifferentPARSupported() const
{
    return getNode()->getPlugin()->getPropertyUnsafe<bool>(kNatronPluginPropSupportsMultiInputsPAR);
}

bool
EffectInstance::isMultipleInputsWithDifferentFPSSupported() const
{
    return getNode()->getPlugin()->getPropertyUnsafe<bool>(kNatronPluginPropSupportsMultiInputsFPS);
}

bool
EffectInstance::isMultipleInputsWithDifferentBitDepthsSupported() const
{
    return getNode()->getPlugin()->getPropertyUnsafe<bool>(kNatronPluginPropSupportsMultiInputsBitDepths);
}

void
EffectInstance::setPropertiesLocked(bool locked)
{
    {
        QMutexLocker k(&_imp->common->pluginsPropMutex);
        if (_imp->common->descriptorLocked == locked) {
            return;
        }
        _imp->common->descriptorLocked = locked;
    }
    if (!locked) {
        refreshDynamicProperties();
    }
}

void
EffectInstance::setRenderThreadSafety(RenderSafetyEnum safety)
{

    QMutexLocker k(&_imp->common->pluginsPropMutex);
    _imp->descriptionPtr->setProperty(kEffectPropRenderThreadSafety, safety);
    if (!getMainInstance()) {
        onPropertiesChanged(*_imp->descriptionPtr);
    }
}

RenderSafetyEnum
EffectInstance::getRenderThreadSafety() const
{
    // Don't need to lock, since the render instance has a local copy of the properties
    return _imp->descriptionPtr->getPropertyUnsafe<RenderSafetyEnum>(kEffectPropRenderThreadSafety);
}

void
EffectInstance::setOpenGLRenderSupport(PluginOpenGLRenderSupport support)
{
    QMutexLocker k(&_imp->common->pluginsPropMutex);
    _imp->descriptionPtr->setProperty(kEffectPropSupportsOpenGLRendering, support);
    if (!getMainInstance()) {
        onPropertiesChanged(*_imp->descriptionPtr);
    }
}

PluginOpenGLRenderSupport
EffectInstance::getOpenGLRenderSupport() const
{
    // Don't need to lock, since the render instance has a local copy of the properties
    return _imp->descriptionPtr->getPropertyUnsafe<PluginOpenGLRenderSupport>(kEffectPropSupportsOpenGLRendering);
}

void
EffectInstance::setSequentialRenderSupport(SequentialPreferenceEnum support)
{
    QMutexLocker k(&_imp->common->pluginsPropMutex);
    _imp->descriptionPtr->setProperty(kEffectPropSupportsSequentialRender, support);
    if (!getMainInstance()) {
        onPropertiesChanged(*_imp->descriptionPtr);
    }
}

SequentialPreferenceEnum
EffectInstance::getSequentialRenderSupport() const
{
    // Don't need to lock, since the render instance has a local copy of the properties
    return _imp->descriptionPtr->getPropertyUnsafe<SequentialPreferenceEnum>(kEffectPropSupportsSequentialRender);
}

bool
EffectInstance::isTemporalImageAccessEnabled() const
{
    // Don't need to lock, since the render instance has a local copy of the properties
    return _imp->descriptionPtr->getPropertyUnsafe<bool>(kEffectPropTemporalImageAccess);
}

void
EffectInstance::setTemporalImageAccessEnabled(bool enabled)
{
    QMutexLocker k(&_imp->common->pluginsPropMutex);
    _imp->descriptionPtr->setProperty(kEffectPropTemporalImageAccess, enabled);
    if (!getMainInstance()) {
        onPropertiesChanged(*_imp->descriptionPtr);
    }

}

void
EffectInstance::setSupportsTiles(bool support)
{
    QMutexLocker k(&_imp->common->pluginsPropMutex);
    _imp->descriptionPtr->setProperty(kEffectPropSupportsTiles, support);
    if (!getMainInstance()) {
        onPropertiesChanged(*_imp->descriptionPtr);
    }
}

bool
EffectInstance::supportsTiles() const
{
    // Don't need to lock, since the render instance has a local copy of the properties
    return _imp->descriptionPtr->getPropertyUnsafe<bool>(kEffectPropSupportsTiles);
}

ImageBufferLayoutEnum
EffectInstance::getPreferredBufferLayout() const
{
    // Don't need to lock, since the render instance has a local copy of the properties
    return _imp->descriptionPtr->getPropertyUnsafe<ImageBufferLayoutEnum>(kEffectPropImageBufferLayout);
}

void
EffectInstance::setPreferredBufferLayout(ImageBufferLayoutEnum layout)
{
    QMutexLocker k(&_imp->common->pluginsPropMutex);
    _imp->descriptionPtr->setProperty(kEffectPropImageBufferLayout, layout);
    if (!getMainInstance()) {
        onPropertiesChanged(*_imp->descriptionPtr);
    }
}

void
EffectInstance::setAlphaFillWith1(bool enabled)
{
    QMutexLocker k(&_imp->common->pluginsPropMutex);
    _imp->descriptionPtr->setProperty(kEffectPropSupportsAlphaFillWith1, enabled);
    if (!getMainInstance()) {
        onPropertiesChanged(*_imp->descriptionPtr);
    }
}

bool
EffectInstance::getAlphaFillWith1() const
{
    // Don't need to lock, since the render instance has a local copy of the properties
    return _imp->descriptionPtr->getPropertyUnsafe<bool>(kEffectPropSupportsAlphaFillWith1);
}

void
EffectInstance::setSupportsMultiResolution(bool support)
{
    QMutexLocker k(&_imp->common->pluginsPropMutex);
    _imp->descriptionPtr->setProperty(kEffectPropSupportsMultiResolution, support);
    if (!getMainInstance()) {
        onPropertiesChanged(*_imp->descriptionPtr);
    }
}

bool
EffectInstance::supportsMultiResolution() const
{
    // Don't need to lock, since the render instance has a local copy of the properties
    return _imp->descriptionPtr->getPropertyUnsafe<bool>(kEffectPropSupportsMultiResolution);
}

void
EffectInstance::setSupportsRenderScale(bool support)
{
    QMutexLocker k(&_imp->common->pluginsPropMutex);
    _imp->descriptionPtr->setProperty(kEffectPropSupportsRenderScale, support);
    if (!getMainInstance()) {
        onPropertiesChanged(*_imp->descriptionPtr);
    }
}

bool
EffectInstance::supportsRenderScale() const
{
    // Don't need to lock, since the render instance has a local copy of the properties
    return _imp->descriptionPtr->getPropertyUnsafe<bool>(kEffectPropSupportsRenderScale);
}

void
EffectInstance::setCanDistort(bool support)
{
    QMutexLocker k(&_imp->common->pluginsPropMutex);
    _imp->descriptionPtr->setProperty(kEffectPropSupportsCanReturnDistortion, support);
    if (!getMainInstance()) {
        onPropertiesChanged(*_imp->descriptionPtr);
    }
}

bool
EffectInstance::getCanDistort() const
{
    // Don't need to lock, since the render instance has a local copy of the properties
    return _imp->descriptionPtr->getPropertyUnsafe<bool>(kEffectPropSupportsCanReturnDistortion);
}

void
EffectInstance::setCanTransform3x3(bool support)
{
     QMutexLocker k(&_imp->common->pluginsPropMutex);
    _imp->descriptionPtr->setProperty(kEffectPropSupportsCanReturn3x3Transform, support);
    if (!getMainInstance()) {
        onPropertiesChanged(*_imp->descriptionPtr);
    }
}

bool
EffectInstance::getCanTransform3x3() const
{
    // Don't need to lock, since the render instance has a local copy of the properties
    return _imp->descriptionPtr->getPropertyUnsafe<bool>(kEffectPropSupportsCanReturn3x3Transform);
}

void
EffectInstance::onPropertiesChanged(const EffectDescription& /*description*/)
{

}

void
EffectInstance::updatePropertiesInternal(const EffectDescription& description)
{
    // A group doesn't have any meaningful properties
    if (dynamic_cast<NodeGroup*>(this)) {
        return;
    }
    // Private - must be locked
    assert(!_imp->common->pluginsPropMutex.tryLock());

    if (getMainInstance()) {
        // Only update for the main instance!
        return;
    }

    // The descriptor is locked, do not update properties
    if (_imp->common->descriptorLocked) {
        return;
    }

    // Clone the properties
    _imp->descriptionPtr->cloneProperties(description);

    PluginPtr plugin = getNode()->getPlugin();


    // Fix OpenGL support property to take into account plug--in pref and project etc...
    PluginOpenGLRenderSupport effectGLSupport = _imp->descriptionPtr->getPropertyUnsafe<PluginOpenGLRenderSupport>(kEffectPropSupportsOpenGLRendering);
    PluginOpenGLRenderSupport pluginGLSupport = plugin->getEffectDescriptor()->getPropertyUnsafe<PluginOpenGLRenderSupport>(kEffectPropSupportsOpenGLRendering);
    if (pluginGLSupport == ePluginOpenGLRenderSupportNeeded) {
        // OpenGL is needed or not supported, force the value
        effectGLSupport = pluginGLSupport;
    } else {
        if ((!getApp()->getProject()->isOpenGLRenderActivated() || !plugin->isOpenGLEnabled()) && pluginGLSupport == ePluginOpenGLRenderSupportYes) {
            pluginGLSupport = ePluginOpenGLRenderSupportNone;
        }
    }
    _imp->descriptionPtr->setProperty(kEffectPropSupportsOpenGLRendering, effectGLSupport);

    // Make sure render scale is disabled for readers and writers
    bool renderScaleSupported = _imp->descriptionPtr->getPropertyUnsafe<bool>(kEffectPropSupportsRenderScale);
    // Ensure render scale can be supported
    if ((((isReader() || isWriter()) && getNode()->getIOContainer()) || !plugin->isRenderScaleEnabled()) && renderScaleSupported) {
        _imp->descriptionPtr->setProperty(kEffectPropSupportsRenderScale, false);
    }

    // Make sure thread safety is OK with user preference on the plug-in
    RenderSafetyEnum renderSafety = _imp->descriptionPtr->getPropertyUnsafe<RenderSafetyEnum>(kEffectPropRenderThreadSafety);
    if (!plugin->isMultiThreadingEnabled() && renderSafety != eRenderSafetyUnsafe) {
        _imp->descriptionPtr->setProperty(kEffectPropRenderThreadSafety, eRenderSafetyUnsafe);
    }


    // Refresh the OpenGL knob in sync with the property
    KnobChoicePtr openGLEnabledKnob = _imp->defKnobs->openglRenderingEnabledKnob.lock();
    if (openGLEnabledKnob) {
        // Do not call knobChanged callback otherwise it will call refreshDynamicProperties() which will recursively call this function
        openGLEnabledKnob->blockValueChanges();
        if (effectGLSupport == ePluginOpenGLRenderSupportNone) {
            openGLEnabledKnob->setValue(1);
        } else {
            openGLEnabledKnob->setValue(0);
        }
        openGLEnabledKnob->unblockValueChanges();
    }

} // updatePropertiesInternal

void
EffectInstance::refreshDynamicProperties()
{

    QMutexLocker k(&_imp->common->pluginsPropMutex);
    updatePropertiesInternal(*_imp->descriptionPtr);

} // refreshDynamicProperties

void
EffectInstance::updateProperties(const EffectDescription& description)
{
    QMutexLocker k(&_imp->common->pluginsPropMutex);
    updatePropertiesInternal(description);
    if (!getMainInstance()) {
        onPropertiesChanged(*_imp->descriptionPtr);
    }
}

bool
EffectInstance::isFullAnimationToHashEnabled() const
{
    return isTemporalImageAccessEnabled();
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
    TimeValue currentTime = getCurrentRenderTime();
    ViewIdx currentView = getCurrentRenderView();
    if (currentRender && thisItem->isRenderCloneNeeded()) {
        FrameViewRenderKey k = {currentTime, currentView, currentRender};
        return boost::dynamic_pointer_cast<RotoDrawableItem>(toRotoItem(thisItem->createRenderClone(k)));
    }
    return thisItem;
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

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_EffectInstance.cpp"

