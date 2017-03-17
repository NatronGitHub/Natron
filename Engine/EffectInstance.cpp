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

EffectInstance::EffectInstance(const EffectInstancePtr& other, const FrameViewRenderKey& key)
    : NamedKnobHolder(other, key)
    , _node( other->getNode() )
    , _imp( new Implementation(this, *other->_imp) )
{
}

EffectInstance::~EffectInstance()
{
}


KnobHolderPtr
EffectInstance::createRenderCopy(const FrameViewRenderKey& key) const
{
    EffectRenderCloneBuilder createFunc = (EffectRenderCloneBuilder)getNode()->getPlugin()->getPropertyUnsafe<void*>(kNatronPluginPropCreateRenderCloneFunc);
    assert(createFunc);
    if (!createFunc) {
        throw std::invalid_argument("EffectInstance::createRenderCopy: No kNatronPluginPropCreateRenderCloneFunc property set on plug-in!");
    }
    EffectInstancePtr clone = createFunc(boost::const_pointer_cast<EffectInstance>(shared_from_this()), key);


    // Make a copy of the main instance input locally so the state of the graph does not change throughout the render
    int nInputs = getMaxInputCount();
    clone->_imp->renderData->mainInstanceInputs.resize(nInputs);
    clone->_imp->renderData->renderInputs.resize(nInputs);

    FrameViewPair p = {key.time, key.view};
    for (int i = 0; i < nInputs; ++i) {
        if (clone->isInputMask(i) && !clone->isMaskEnabled(i)) {
            continue;
        }
        EffectInstancePtr mainInstanceInput = getInputMainInstance(i);
        clone->_imp->renderData->mainInstanceInputs[i] = mainInstanceInput;
        if (mainInstanceInput) {
            EffectInstancePtr inputClone = toEffectInstance(mainInstanceInput->createRenderClone(key));
            clone->_imp->renderData->renderInputs[i][p] = inputClone;
        }
    }

#pragma message WARN("check this")
#if 0
    // Visit all nodes that expressions of this node knobs may rely upon so we ensure they get a proper render object
    // and a render time and view when we run the expression.
    std::set<NodePtr> expressionsDeps;
    getAllExpressionDependenciesRecursive(expressionsDeps);

    for (std::set<NodePtr>::const_iterator it = expressionsDeps.begin(); it != expressionsDeps.end(); ++it) {
        (*it)->getEffectInstance()->createRenderClone(render);
    }
#endif
    return clone;
}

RenderEngine*
EffectInstance::createRenderEngine()
{
    return new RenderEngine(getNode());
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
        if (isViewInvariant() == eViewInvarianceAllViewsVariant) {
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
                for (U32 range = 0; range < viewIt->second.size(); ++range) {

                    // For all frames in the range
                    for (double f = viewIt->second[range].min; f <= viewIt->second[range].max; f += 1.) {

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
        cacheKey.reset(new GetFramesNeededKey(hashValue, getNode()->getPluginID()));


        CacheEntryLockerPtr cacheAccess = appPTR->getGeneralPurposeCache()->get(framesNeededResults);
        
        CacheEntryLocker::CacheEntryStatusEnum cacheStatus = cacheAccess->getStatus();
        if (cacheStatus == CacheEntryLocker::eCacheEntryStatusMustCompute) {
            cacheAccess->insertInCache();
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

void
EffectInstance::removeRenderCloneRecursive(const TreeRenderPtr& render)
{
    bool foundClone = removeRenderClone(render);
    if (!foundClone) {
        return;
    }

    // Recurse on inputs
    int nInputs = getMaxInputCount();
    for (int i = 0; i < nInputs; ++i) {
        EffectInstancePtr input = getInputMainInstance(i);
        if (input) {
            input->removeRenderCloneRecursive(render);
        }
    }
} // removeRenderCloneRecursive


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
                                      unsigned int mipMapLevel,
                                      const RenderScale& proxyScale,
                                      RectD* roiCanonical,
                                      RectD* roiExpand)
{


    // Use the provided image bounds if any
    if (inArgs.optionalBounds) {
        *roiCanonical = * inArgs.optionalBounds;
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

    bool supportsMultiRes = getCurrentSupportMultiRes();
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
    TreeRenderPtr currentRender = getCurrentRender();

    TimeValue inputTime = inArgs.inputTime ? *inArgs.inputTime : getCurrentRenderTime();


    if (inputTime != inputTime) {
        // time is NaN
#ifdef DEBUG
        qDebug() << QThread::currentThread() << getScriptName_mt_safe().c_str() << "getImage on input" << inArgs.inputNb << "failing because time is NaN";
#endif
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

    EffectInstancePtr inputEffect = getInputRenderEffect(inArgs.inputNb, inputTime, inputView);

    if (!inputEffect) {
        // Disconnected input
        return false;
    }

    // Get the requested RoI for the input, if we can recover it, otherwise TreeRender will render the RoD.
    RectD roiCanonical, roiExpand;
    if (!resolveRoIForGetImage(inArgs, currentMipMapLevel, currentProxyScale, &roiCanonical, &roiExpand)) {
        // If we did not resolve the RoI, ask for the RoD
        GetRegionOfDefinitionResultsPtr results;
        RenderScale combinedScale = EffectInstance::getCombinedScale(inputMipMapLevel, inputProxyScale);
        ActionRetCodeEnum stat = inputEffect->getRegionOfDefinition_public(inputTime, combinedScale, inputView, &results);
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
    FrameViewRequestPtr outputRequest;
    {
        ActionRetCodeEnum status;
        if (currentRender) {
            status = currentRender->launchRenderWithArgs(inputEffect, inputTime, inputView, inputProxyScale, inputMipMapLevel, inArgs.plane, &roiCanonical, &outputRequest);
        } else {
            // We are not during a render, create one.
            TreeRender::CtorArgsPtr rargs(new TreeRender::CtorArgs());
            rargs->time = inputTime;
            rargs->view = inputView;
            rargs->treeRootEffect = inputEffect;
            rargs->canonicalRoI = &roiCanonical;
            rargs->proxyScale = inputProxyScale;
            rargs->mipMapLevel = inputMipMapLevel;
            rargs->plane = inArgs.plane;
            rargs->draftMode = isDraftMode;
            rargs->playback = isPlayback;
            rargs->byPassCache = false;
            TreeRenderPtr renderObject = TreeRender::create(rargs);
            if (!currentRender) {
                currentRender = renderObject;
            }
            status = renderObject->launchRender(&outputRequest);
        }
        if (isFailureRetCode(status)) {
            return false;
        }
    }

    // Copy in output the distortion stack
    outArgs->distortionStack = outputRequest->getDistorsionStack();


    // Get the RoI in pixel coordinates of the effect we rendered
    RenderScale inputCombinedScale = EffectInstance::getCombinedScale(inputMipMapLevel, inputProxyScale);
    double inputPar = getAspectRatio(inArgs.inputNb);

    RectI roiPixels;
    RectI roiExpandPixels;
    roiExpand.toPixelEnclosing(inputCombinedScale, inputPar, &roiExpandPixels);
    roiCanonical.toPixelEnclosing(inputCombinedScale, inputPar, &roiPixels);
    assert(roiExpandPixels.contains(roiPixels));
    if (roiExpandPixels != roiPixels) {
        outArgs->roiPixel = roiExpandPixels;
    } else {
        outArgs->roiPixel = roiPixels;
    }



    // Map the output image to the plug-in preferred format
    ImageBufferLayoutEnum thisEffectSupportedImageLayout = getPreferredBufferLayout();

    const bool supportsMultiPlane = isMultiPlanar();

    // The output image unmapped
    outArgs->image = outputRequest->getImagePlane();

    outArgs->roiPixel.intersect(outArgs->image->getBounds(), &outArgs->roiPixel);


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

    if (roiExpand != roiCanonical) {
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

        int channelForMask = - 1;
        ImagePlaneDesc maskComps;
        {
            std::list<ImagePlaneDesc> upstreamAvailableLayers;
            ActionRetCodeEnum stat = getAvailableLayers(getCurrentRenderTime(), getCurrentRenderView(), inArgs.inputNb, &upstreamAvailableLayers);
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
        ActionRetCodeEnum stat = convertedImage->copyPixels(*outArgs->image, copyArgs);
        if (isFailureRetCode(stat)) {
            return false;   
        }
        outArgs->image = convertedImage;
    } // mustConvertImage

    // If the effect does not support multi-resolution image, add black borders so that all images have the same size in input.
    if (roiExpandPixels != roiPixels) {
        outArgs->image->fillOutSideWithBlack(roiPixels);
    }

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

<<<<<<< HEAD
=======
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
                                                    const ImageComponents & components,
                                                    const EffectInstance::InputImagesMap & inputImages,
                                                    const boost::shared_ptr<RenderStats> & stats,
                                                    const boost::shared_ptr<OSGLContextAttacher>& glContextAttacher,
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

            ///Throw away images that are not even what the node want to render
            /*if ( ( imgComps.isColorPlane() && nodePrefComps.isColorPlane() && (imgComps != nodePrefComps) ) || (imgDepth != nodePrefDepth) ) {
                appPTR->removeFromNodeCache(*it);
                continue;
            }*/

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

                boost::shared_ptr<ImageParams> imageParams = Image::makeParams(rod,
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


                boost::shared_ptr<Image> img;
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

                // When using the GPU, we dont want to retrieve partially rendered image because rendering the portion
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
                                   const ImageComponents & components,
                                   ImageBitDepthEnum depth,
                                   ImagePremultiplicationEnum premult,
                                   ImageFieldingOrderEnum fielding,
                                   double par,
                                   unsigned int mipmapLevel,
                                   bool renderFullScaleThenDownscale,
                                   StorageModeEnum storage,
                                   bool createInCache,
                                   boost::shared_ptr<Image>* fullScaleImage,
                                   boost::shared_ptr<Image>* downscaleImage)
{
    //If we're rendering full scale and with input images at full scale, don't cache the downscale image since it is cheap to
    //recreate, instead cache the full-scale image
    if (renderFullScaleThenDownscale) {
        downscaleImage->reset( new Image(components, rod, downscaleImageBounds, mipmapLevel, par, depth, premult, fielding, true) );
        boost::shared_ptr<ImageParams> upscaledImageParams = Image::makeParams(rod,
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
        boost::shared_ptr<ImageParams> cachedImgParams = Image::makeParams(rod,
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
        ///so taking this lock now ensures the image will be allocated completetly

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
                                        StorageModeEnum renderStorageMode,
                                        double time,
                                        ViewIdx view,
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
                                                      const ImageComponents & outputClipPrefsComps,
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

    const EffectInstance::PlaneToRender & firstPlaneToRender = planes->planes.begin()->second;
    // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
# ifndef NDEBUG
    RectI renderBounds = firstPlaneToRender.renderMappedImage->getBounds();
    assert(renderBounds.x1 <= renderMappedRectToRender.x1 && renderMappedRectToRender.x2 <= renderBounds.x2 &&
           renderBounds.y1 <= renderMappedRectToRender.y1 && renderMappedRectToRender.y2 <= renderBounds.y2);
# endif



    const boost::shared_ptr<ParallelRenderArgs>& frameArgs = tls->frameArgs.back();

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
#     endif // DEBUG

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
EffectInstance::Implementation::renderHandler(const EffectDataTLSPtr& tls,
                                              const unsigned int mipMapLevel,
                                              const bool renderFullScaleThenDownscale,
                                              const bool isSequentialRender,
                                              const bool isRenderResponseToUserInteraction,
                                              const RectI & renderMappedRectToRender,
                                              const RectI & downscaledRectToRender,
                                              const bool byPassCache,
                                              const ImageBitDepthEnum outputClipPrefDepth,
                                              const ImageComponents & outputClipPrefsComps,
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

    std::list<std::pair<ImageComponents, ImagePtr> > tmpPlanes;
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
        std::list<ImageComponents> comps;

        for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it) {
            //If color plane, request the preferred comp of the identity input
            if ( tls->currentRenderArgs.identityInput && it->second.renderMappedImage->getComponents().isColorPlane() ) {
                ImageComponents prefInputComps = tls->currentRenderArgs.identityInput->getComponents(-1);
                comps.push_back(prefInputComps);
            } else {
                comps.push_back( it->second.renderMappedImage->getComponents() );
            }
        }
        assert( !comps.empty() );
        std::map<ImageComponents, ImagePtr> identityPlanes;
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
            for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it) {
                it->second.renderMappedImage->fillZero(renderMappedRectToRender, glContext);

                if ( frameArgs->stats && frameArgs->stats->isInDepthProfilingEnabled() ) {
                    frameArgs->stats->addRenderInfosForNode( _publicInterface->getNode(),  NodePtr(), it->first.getComponentsGlobalName(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation() );
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
                for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it) {
                    it->second.renderMappedImage->fillZero(renderMappedRectToRender, glContext);

                    if ( frameArgs->stats && frameArgs->stats->isInDepthProfilingEnabled() ) {
                        frameArgs->stats->addRenderInfosForNode( _publicInterface->getNode(),  tls->currentRenderArgs.identityInput->getNode(), it->first.getComponentsGlobalName(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation() );
                    }
                }

                return eRenderingFunctorRetOK;
            } else {
                assert( identityPlanes.size() == planes.planes.size() );

                std::map<ImageComponents, ImagePtr>::iterator idIt = identityPlanes.begin();
                for (std::map<ImageComponents, EffectInstance::PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it, ++idIt) {
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
                                                         false) );

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
                                                       false) );
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
            prefComp = outputClipPrefsComps;
        }

        // OpenGL render never use the cache and bitmaps, all images are local to a render.
        if ( ( it->second.renderMappedImage->usesBitMap() || ( prefComp != it->second.renderMappedImage->getComponents() ) ||
               ( outputClipPrefDepth != it->second.renderMappedImage->getBitDepth() ) ) && !_publicInterface->isPaintingOverItselfEnabled() && !planes.useOpenGL ) {
            it->second.tmpImage.reset( new Image(prefComp,
                                                 it->second.renderMappedImage->getRoD(),
                                                 actionArgs.roi,
                                                 it->second.renderMappedImage->getMipMapLevel(),
                                                 it->second.renderMappedImage->getPixelAspectRatio(),
                                                 outputClipPrefDepth,
                                                 it->second.renderMappedImage->getPremultiplication(),
                                                 it->second.renderMappedImage->getFieldingOrder(),
                                                 false) ); //< no bitmap
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
         * the planes map from the thread local storage once the render action is finished
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
                for (std::map<ImageComponents, EffectInstance::PlaneToRender>::const_iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {
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
    } // for (std::list<std::list<std::pair<ImageComponents,ImagePtr> > >::iterator it = planesLists.begin(); it != planesLists.end(); ++it)

    assert(!renderAborted);

    bool unPremultIfNeeded = planes.outputPremult == eImagePremultiplicationPremultiplied;
    bool useMaskMix = _publicInterface->isHostMaskingEnabled() || _publicInterface->isHostMixingEnabled();
    double mix = useMaskMix ? _publicInterface->getNode()->getHostMixingValue(time, view) : 1.;
    bool doMask = useMaskMix ? _publicInterface->getNode()->isMaskEnabled(_publicInterface->getMaxInputCount() - 1) : false;

    //Check for NaNs, copy to output image and mark for rendered
    for (std::map<ImageComponents, EffectInstance::PlaneToRender>::const_iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {
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
                        ImagePtr tmp( new Image(it->second.tmpImage->getComponents(),
                                                it->second.tmpImage->getRoD(),
                                                renderMappedRectToRender,
                                                0,
                                                it->second.tmpImage->getPixelAspectRatio(),
                                                it->second.tmpImage->getBitDepth(),
                                                it->second.tmpImage->getPremultiplication(),
                                                it->second.tmpImage->getFieldingOrder(),
                                                false) );
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
                    ImagePtr tmp( new Image(it->second.fullscaleImage->getComponents(),
                                            it->second.tmpImage->getRoD(),
                                            renderMappedRectToRender,
                                            mipMapLevel,
                                            it->second.tmpImage->getPixelAspectRatio(),
                                            it->second.fullscaleImage->getBitDepth(),
                                            it->second.fullscaleImage->getPremultiplication(),
                                            it->second.fullscaleImage->getFieldingOrder(),
                                            false) );

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
    if ( boost::starts_with(getNode()->getPluginID(), "uk.co.thefoundry.furnace") ) {
        //Furnace plug-ins are bugged and do not render properly both planes, just wipe the image.
        useCache = false;
    }
    const ImagePtr & img = firstPlane.fullscaleImage->usesBitMap() ? firstPlane.fullscaleImage : firstPlane.downscaleImage;
    boost::shared_ptr<ImageParams> params = img->getParams();
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
        refreshMetaDatas_public(true);
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
>>>>>>> RB-2.2
    NodePtr node = getNode();
    assert(node);
    return node ? node->message(type, content) : false;
}
<<<<<<< HEAD
=======

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
                fk->setValue(pattern);
                break;
            }
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
EffectInstance::addPluginMemoryPointer(const PluginMemoryPtr& mem)
{
    QMutexLocker l(&_imp->pluginMemoryChunksMutex);

    _imp->pluginMemoryChunks.push_back(mem);
}

void
EffectInstance::removePluginMemoryPointer(const PluginMemory* mem)
{
    std::list<boost::shared_ptr<PluginMemory> > safeCopy;

    {
        QMutexLocker l(&_imp->pluginMemoryChunksMutex);
        // make a copy of the list so that elements don't get deleted while the mutex is held

        for (std::list<boost::weak_ptr<PluginMemory> >::iterator it = _imp->pluginMemoryChunks.begin(); it != _imp->pluginMemoryChunks.end(); ++it) {
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
EffectInstance::onKnobSlaved(const KnobPtr& slave,
                             const KnobPtr& master,
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
    //Don't chek if render is needed on pen motion, wait for the pen up

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
        const KnobPtr& k = *it2;
        if (!k) {
            continue;
        }
        boost::shared_ptr<OfxParamOverlayInteract> interact = k->getCustomInteract();
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
                                    ViewIdx view,
                                    EffectInstPtr* inputToTransform,
                                    Transform::Matrix3x3* transform)
{
    RECURSIVE_ACTION();
    //assert( getNode()->getCurrentCanTransform() ); // called in every case for overlays

    return getTransform(time, renderScale, view, inputToTransform, transform);
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
    assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(scale.x == 1. && scale.y == 1.) ) );

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
    boost::shared_ptr<RotoDrawableItem> rotoItem = getNode()->getAttachedRotoItem();
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
        if ( rod->isNull() ) {
            return eStatusFailed;
        }

        return eStatusOK;
    } else {
        ///If this is running on a render thread, attempt to find the RoD in the thread local storage.

        if ( QThread::currentThread() != qApp->thread() ) {
            EffectDataTLSPtr tls = _imp->tlsData->getTLSData();
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
>>>>>>> RB-2.2

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
EffectInstance::registerOverlay(const OverlayInteractBasePtr& overlay, const std::map<std::string,std::string>& knobs)
{

    overlay->setEffect(shared_from_this());
    overlay->fetchKnobs(knobs);

    assert(QThread::currentThread() == qApp->thread());
    std::list<OverlayInteractBasePtr>::iterator found = std::find(_imp->common->interacts.begin(), _imp->common->interacts.end(), overlay);
    if (found == _imp->common->interacts.end()) {

        _imp->common->interacts.push_back(overlay);
        overlay->redraw();
    }
}

void
EffectInstance::removeOverlay(const OverlayInteractBasePtr& overlay)
{
    assert(QThread::currentThread() == qApp->thread());
    std::list<OverlayInteractBasePtr>::iterator found = std::find(_imp->common->interacts.begin(), _imp->common->interacts.end(), overlay);
    if (found != _imp->common->interacts.end()) {
        _imp->common->interacts.erase(found);
    }
}

void
EffectInstance::getOverlays(std::list<OverlayInteractBasePtr> *overlays) const
{
    assert(QThread::currentThread() == qApp->thread());
    *overlays = _imp->common->interacts;
}

bool
EffectInstance::hasOverlayInteract() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->common->interacts.size() > 0;
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
        ssinfo << ( getCurrentSupportMultiRes() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
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
    {
        QMutexLocker k(&_imp->common->pluginsPropMutex);

        _imp->common->pluginSafety = safety;
        _imp->common->pluginSafetyLocked = true;
    }
    refreshDynamicProperties();
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
    {
        QMutexLocker k(&_imp->common->pluginsPropMutex);
        if (!_imp->common->pluginSafetyLocked) {
            return;
        }
        _imp->common->pluginSafetyLocked = false;
    }
    refreshDynamicProperties();
}


RenderSafetyEnum
EffectInstance::getPluginRenderThreadSafety() const
{
    return (RenderSafetyEnum)getNode()->getPlugin()->getPropertyUnsafe<int>(kNatronPluginPropRenderSafety);
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
EffectInstance::setCurrentSupportMultiRes(bool support)
{
    QMutexLocker k(&_imp->common->pluginsPropMutex);

    _imp->common->props.currentSupportMultires = support;
}

bool
EffectInstance::getCurrentSupportMultiRes() const
{
    if (_imp->renderData) {
        return _imp->renderData->props.currentSupportMultires;
    }
    QMutexLocker k(&_imp->common->pluginsPropMutex);

    return _imp->common->props.currentSupportMultires;
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
    if ((isReader() || isWriter()) && getNode()->getIOContainer()) {
        // If this is an encoder or decoder, we do not support render-scale.
        renderScaleSupported = false;
    }


    bool multiResSupported = supportsMultiResolution();
    bool canDistort = getCanDistort();
    bool currentDeprecatedTransformSupport = getCanTransform();

    bool safetyLocked;
    {
        QMutexLocker k(&_imp->common->pluginsPropMutex);
        safetyLocked = _imp->common->pluginSafetyLocked;
    }

    {
        QMutexLocker k(&_imp->common->pluginsPropMutex);
        if (!safetyLocked) {
            _imp->common->pluginSafety = getPluginRenderThreadSafety();
        }

        if (!tilesSupported && _imp->common->pluginSafety == eRenderSafetyFullySafeFrame) {
            // an effect which does not support tiles cannot support host frame threading
            _imp->common->props.currentThreadSafety = eRenderSafetyFullySafe;
        } else {
            _imp->common->props.currentThreadSafety = _imp->common->pluginSafety;
        }
    }



    setCurrentSupportMultiRes(multiResSupported);
    setCurrentSupportTiles(tilesSupported);
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
    TimeValue currentTime = getCurrentRenderTime();
    ViewIdx currentView = getCurrentRenderView();
    if (currentRender && thisItem->isRenderCloneNeeded()) {
        FrameViewRenderKey k = {currentTime, currentView, currentRender};
        return boost::dynamic_pointer_cast<RotoDrawableItem>(toRotoItem(thisItem->getRenderClone(k)));
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

