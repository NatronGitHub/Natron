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

#include "NodePrivate.h"

#include "Engine/AppInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/ImageComponents.h"
#include "Engine/Project.h"
#include "Engine/Timer.h"

NATRON_NAMESPACE_ENTER

NodePrivate::NodePrivate(Node* publicInterface,
                         const AppInstancePtr& app_,
                         const NodeCollectionPtr& collection,
                         const PluginPtr& plugin_)
: _publicInterface(publicInterface)
, group(collection)
, precomp()
, app(app_)
, isPersistent(true)
, knobsInitialized(false)
, inputsInitialized(false)
, outputsMutex()
, outputs()
, guiOutputs()
, inputsMutex()
, inputs()
, guiInputs()
, effect()
, inputsComponents()
, outputComponents()
, inputsLabelsMutex()
, inputLabels()
, scriptName()
, label()
, deactivatedState()
, activatedMutex()
, activated(true)
, plugin(plugin_)
, pyPlugHandle()
, isPyPlug(false)
, computingPreview(false)
, previewThreadQuit(false)
, computingPreviewMutex()
, pluginInstanceMemoryUsed(0)
, memoryUsedMutex()
, mustQuitPreview(0)
, mustQuitPreviewMutex()
, mustQuitPreviewCond()
, renderInstancesSharedMutex(QMutex::Recursive)
, masterNodeMutex()
, masterNode()
, nodeLinks()
, ioContainer()
, frameIncrKnob()
, nodeSettingsPage()
, nodeLabelKnob()
, previewEnabledKnob()
, disableNodeKnob()
, infoPage()
, nodeInfos()
, refreshInfoButton()
, useFullScaleImagesWhenRenderScaleUnsupported()
, forceCaching()
, hideInputs()
, beforeFrameRender()
, beforeRender()
, afterFrameRender()
, afterRender()
, enabledChan()
, channelsSelectors()
, maskSelectors()
, rotoContext()
, trackContext()
, imagesBeingRenderedMutex()
, imageBeingRenderedCond()
, imagesBeingRendered()
, supportedDepths()
, keyframesDisplayedOnTimeline(false)
, lastRenderStartedMutex()
, lastRenderStartedSlotCallTime()
, renderStartedCounter(0)
, inputIsRenderingCounter(0)
, lastInputNRenderStartedSlotCallTime()
, nodeIsDequeuing(false)
, nodeIsDequeuingMutex()
, nodeIsDequeuingCond()
, nodeIsRendering(0)
, nodeIsRenderingMutex()
, persistentMessage()
, persistentMessageType(0)
, persistentMessageMutex()
, guiPointer()
, nativeOverlays()
, nodeCreated(false)
, wasCreatedSilently(false)
, createdComponentsMutex()
, createdComponents()
, paintStroke()
, pluginsPropMutex()
, pluginSafety(eRenderSafetyInstanceSafe)
, currentThreadSafety(eRenderSafetyInstanceSafe)
, currentSupportTiles(false)
, currentSupportOpenGLRender(ePluginOpenGLRenderSupportNone)
, currentSupportSequentialRender(eSequentialPreferenceNotSequential)
, currentCanTransform(false)
, draftModeUsed(false)
, mustComputeInputRelatedData(true)
, duringPaintStrokeCreation(false)
, lastStrokeMovementMutex()
, strokeBitmapCleared(false)
, paintBuffer()
, isBeingDestroyedMutex()
, isBeingDestroyed(false)
, inputModifiedRecursion(0)
, inputsModified()
, refreshIdentityStateRequestsCount(0)
, isRefreshingInputRelatedData(false)
, streamWarnings()
, requiresGLFinishBeforeRender(false)
, nodePositionCoords()
, nodeSize()
, nodeColor()
, overlayColor()
, nodeIsSelected(false)
, restoringDefaults(false)
, pythonCallbacksPrefixedWithPluginID(false)
{
    nodePositionCoords[0] = nodePositionCoords[1] = INT_MIN;
    nodeSize[0] = nodeSize[1] = -1;
    nodeColor[0] = nodeColor[1] = nodeColor[2] = -1;
    overlayColor[0] = overlayColor[1] = overlayColor[2] = -1;


    ///Initialize timers
    gettimeofday(&lastRenderStartedSlotCallTime, 0);
    gettimeofday(&lastInputNRenderStartedSlotCallTime, 0);
}

void
NodePrivate::createChannelSelector(int inputNb,
                                            const std::string & inputName,
                                            bool isOutput,
                                            const KnobPagePtr& page,
                                            KnobIPtr* lastKnobBeforeAdvancedOption)
{
    ChannelSelector sel;
    KnobChoicePtr layer = AppManager::createKnob<KnobChoice>(effect, isOutput ? tr("Output Layer") : tr("%1 Layer").arg( QString::fromUtf8( inputName.c_str() ) ), 1, false);
    layer->setHostCanAddOptions(isOutput);
    if (!isOutput) {
        layer->setName( inputName + std::string("_") + std::string(kOutputChannelsKnobName) );
    } else {
        layer->setName(kOutputChannelsKnobName);
    }
    if (isOutput) {
        layer->setHintToolTip( tr("Select here the layer onto which the processing should occur.") );
    } else {
        layer->setHintToolTip( tr("Select here the layer that will be used in input by %1.").arg( QString::fromUtf8( inputName.c_str() ) ) );
    }
    layer->setAnimationEnabled(false);
    layer->setSecret(!isOutput);
    page->addKnob(layer);

    if (isOutput) {
        KnobBoolPtr processAllKnob = AppManager::createKnob<KnobBool>(effect, tr(kNodeParamProcessAllLayersLabel), 1, false);
        processAllKnob->setName(kNodeParamProcessAllLayers);
        processAllKnob->setHintToolTip(tr(kNodeParamProcessAllLayersHint));
        processAllKnob->setAnimationEnabled(false);
        page->addKnob(processAllKnob);

        // If the effect wants by default to render all planes set default value
        if ( isOutput && (effect->isPassThroughForNonRenderedPlanes() == EffectInstance::ePassThroughRenderAllRequestedPlanes) ) {
            processAllKnob->setDefaultValue(true);
            //Hide all other input selectors if choice is All in output
            for (std::map<int, ChannelSelector>::iterator it = channelsSelectors.begin(); it != channelsSelectors.end(); ++it) {
                it->second.layer.lock()->setSecret(true);
            }
        }
        processAllLayersKnob = processAllKnob;
    }

    sel.layer = layer;
    std::vector<std::string> baseLayers;
    if (!isOutput) {
        baseLayers.push_back("None");
    }

    std::map<std::string, int > defaultLayers;
    {
        std::vector<std::string> projectLayers = _publicInterface->getApp()->getProject()->getProjectDefaultLayerNames();
        for (std::size_t i = 0; i < projectLayers.size(); ++i) {
            defaultLayers[projectLayers[i]] = -1;
        }
    }
    baseLayers.push_back(kNatronRGBAPlaneUserName);
    for (std::map<std::string, int>::iterator itl = defaultLayers.begin(); itl != defaultLayers.end(); ++itl) {
        std::string choiceName = ImageComponents::mapNatronInternalPlaneNameToUserFriendlyPlaneName(itl->first);
        baseLayers.push_back(choiceName);
    }

    layer->populateChoices(baseLayers);
    layer->setDefaultValue(isOutput ? 0 : 1);

    if (!*lastKnobBeforeAdvancedOption) {
        *lastKnobBeforeAdvancedOption = layer;
    }

    channelsSelectors[inputNb] = sel;
} // createChannelSelector


ImageComponents
NodePrivate::getSelectedLayerInternal(int inputNb, const ChannelSelector& selector) const
{
    NodePtr node;

    if (inputNb == -1) {
        node = _publicInterface->shared_from_this();
    } else {
        node = _publicInterface->getInput(inputNb);
    }

    KnobChoicePtr layerKnob = selector.layer.lock();
    if (!layerKnob) {
        return ImageComponents();
    }
    std::string layer = layerKnob->getActiveEntryText_mt_safe();


    std::string mappedLayerName = ImageComponents::mapUserFriendlyPlaneNameToNatronInternalPlaneName(layer);
    bool isCurLayerColorComp = mappedLayerName == kNatronRGBAComponentsName || mappedLayerName == kNatronRGBComponentsName || mappedLayerName == kNatronAlphaComponentsName;
    EffectInstance::ComponentsAvailableMap compsAvailable;
    {
        QMutexLocker k(&selector.compsMutex);
        compsAvailable = selector.compsAvailable;
    }

    ImageComponents ret;
    if (node) {
        for (EffectInstance::ComponentsAvailableMap::iterator it2 = compsAvailable.begin(); it2 != compsAvailable.end(); ++it2) {
            if ( it2->first.isColorPlane() ) {
                if (isCurLayerColorComp) {
                    ret = it2->first;
                    break;
                }
            } else {
                if (it2->first.getLayerName() == mappedLayerName) {
                    ret = it2->first;
                    break;
                }
            }
        }
    }


    if ( (ret.getNumComponents() == 0) && _publicInterface ) {
        std::vector<ImageComponents> projectLayers = _publicInterface->getApp()->getProject()->getProjectDefaultLayers();
        for (std::size_t i = 0; i < projectLayers.size(); ++i) {
            if (projectLayers[i].getLayerName() == mappedLayerName) {
                ret = projectLayers[i];
                break;
            }
        }
    }
    return ret;
} // getSelectedLayerInternal


void
NodePrivate::onLayerChanged(bool isOutput)
{
    if (isOutput) {
        _publicInterface->refreshLayersSelectorsVisibility();
    }
    if (!isRefreshingInputRelatedData) {
        ///Clip preferences have changed
        RenderScale s(1.);
        effect->refreshMetaDatas_public(true);
    }

    if (isOutput) {
        _publicInterface->s_outputLayerChanged();
    }
}

void
NodePrivate::onMaskSelectorChanged(int inputNb,
                                            const MaskSelector& selector)
{
    KnobChoicePtr channel = selector.channel.lock();
    int index = channel->getValue();
    KnobBoolPtr enabled = selector.enabled.lock();

    if ( (index == 0) && enabled->isEnabled(0) ) {
        enabled->setValue(false);
        enabled->setEnabled(0, false);
    } else if ( !enabled->isEnabled(0) ) {
        enabled->setEnabled(0, true);
        if ( _publicInterface->getInput(inputNb) ) {
            enabled->setValue(true);
        }
    }

    if (!isRefreshingInputRelatedData) {
        ///Clip preferences have changed
        RenderScale s(1.);
        effect->refreshMetaDatas_public(true);
    }
}

void
NodePrivate::refreshDefaultPagesOrder()
{
    const KnobsVec& knobs = effect->getKnobs();
    defaultPagesOrder.clear();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        KnobPagePtr ispage = toKnobPage(*it);
        if (ispage && !ispage->getChildren().empty()) {
            defaultPagesOrder.push_back( ispage->getName() );
        }
    }

}



void
NodePrivate::refreshDefaultViewerKnobsOrder()
{
    KnobsVec knobs = effect->getViewerUIKnobs();
    defaultViewerKnobsOrder.clear();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        defaultViewerKnobsOrder.push_back( (*it)->getName() );
    }
}


void
NodePrivate::abortPreview_non_blocking()
{
    bool computing;
    {
        QMutexLocker locker(&computingPreviewMutex);
        computing = computingPreview;
    }

    if (computing) {
        QMutexLocker l(&mustQuitPreviewMutex);
        ++mustQuitPreview;
    }
}

void
NodePrivate::abortPreview_blocking(bool allowPreviewRenders)
{
    bool computing;
    {
        QMutexLocker locker(&computingPreviewMutex);
        computing = computingPreview;
        previewThreadQuit = !allowPreviewRenders;
    }

    if (computing) {
        QMutexLocker l(&mustQuitPreviewMutex);
        assert(!mustQuitPreview);
        ++mustQuitPreview;
        while (mustQuitPreview) {
            mustQuitPreviewCond.wait(&mustQuitPreviewMutex);
        }
    }
}

bool
NodePrivate::checkForExitPreview()
{
    {
        QMutexLocker locker(&mustQuitPreviewMutex);
        if (mustQuitPreview || previewThreadQuit) {
            mustQuitPreview = 0;
            mustQuitPreviewCond.wakeOne();

            return true;
        }

        return false;
    }
}



void
NodePrivate::runOnNodeCreatedCBInternal(const std::string& cb,
                                                 bool userEdited)
{
    std::vector<std::string> args;
    std::string error;
    if (_publicInterface->getScriptName_mt_safe().empty()) {
        return;
    }
    try {
        NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
    } catch (const std::exception& e) {
        _publicInterface->getApp()->appendToScriptEditor( std::string("Failed to run onNodeCreated callback: ")
                                                         + e.what() );

        return;
    }

    if ( !error.empty() ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeCreated callback: " + error);

        return;
    }

    std::string signatureError;
    signatureError.append("The on node created callback supports the following signature(s):\n");
    signatureError.append("- callback(thisNode,app,userEdited)");
    if (args.size() != 3) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeCreated callback: " + signatureError);

        return;
    }

    if ( (args[0] != "thisNode") || (args[1] != "app") || (args[2] != "userEdited") ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeCreated callback: " + signatureError);

        return;
    }

    std::string appID = _publicInterface->getApp()->getAppIDString();
    std::string scriptName = _publicInterface->getScriptName_mt_safe();
    if ( scriptName.empty() ) {
        return;
    }
    std::stringstream ss;
    ss << cb << "(" << appID << "." << _publicInterface->getFullyQualifiedName() << "," << appID << ",";
    if (userEdited) {
        ss << "True";
    } else {
        ss << "False";
    }
    ss << ")\n";
    std::string output;
    std::string script = ss.str();
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, &output) ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeCreated callback: " + error);
    } else if ( !output.empty() ) {
        _publicInterface->getApp()->appendToScriptEditor(output);
    }
} // Node::Implementation::runOnNodeCreatedCBInternal

void
NodePrivate::runOnNodeDeleteCBInternal(const std::string& cb)
{
    std::vector<std::string> args;
    std::string error;

    try {
        NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
    } catch (const std::exception& e) {
        _publicInterface->getApp()->appendToScriptEditor( std::string("Failed to run onNodeDeletion callback: ")
                                                         + e.what() );

        return;
    }

    if ( !error.empty() ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeDeletion callback: " + error);

        return;
    }

    std::string signatureError;
    signatureError.append("The on node deletion callback supports the following signature(s):\n");
    signatureError.append("- callback(thisNode,app)");
    if (args.size() != 2) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeDeletion callback: " + signatureError);

        return;
    }

    if ( (args[0] != "thisNode") || (args[1] != "app") ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeDeletion callback: " + signatureError);

        return;
    }

    std::string appID = _publicInterface->getApp()->getAppIDString();
    std::stringstream ss;
    ss << cb << "(" << appID << "." << _publicInterface->getFullyQualifiedName() << "," << appID << ")\n";

    std::string err;
    std::string output;
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(ss.str(), &err, &output) ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeDeletion callback: " + err);
    } else if ( !output.empty() ) {
        _publicInterface->getApp()->appendToScriptEditor(output);
    }
}

void
NodePrivate::runOnNodeCreatedCB(bool userEdited)
{

    std::string cb = _publicInterface->getApp()->getProject()->getOnNodeCreatedCB();
    NodeCollectionPtr group = _publicInterface->getGroup();

    if (!group) {
        return;
    }
    if ( !cb.empty() ) {
        runOnNodeCreatedCBInternal(cb, userEdited);
    }

    NodeGroupPtr isGroup = toNodeGroup(group);
    KnobStringPtr nodeCreatedCbKnob = nodeCreatedCallback.lock();
    if (!nodeCreatedCbKnob && isGroup) {
        cb = isGroup->getNode()->getAfterNodeCreatedCallback();
    } else if (nodeCreatedCbKnob) {
        cb = nodeCreatedCbKnob->getValue();
    }
    if ( !cb.empty() ) {
        runOnNodeCreatedCBInternal(cb, userEdited);
    }
}

void
NodePrivate::runOnNodeDeleteCB()
{

    if (_publicInterface->getScriptName_mt_safe().empty()) {
        return;
    }
    std::string cb = _publicInterface->getApp()->getProject()->getOnNodeDeleteCB();
    NodeCollectionPtr group = _publicInterface->getGroup();

    if (!group) {
        return;
    }
    if ( !cb.empty() ) {
        runOnNodeDeleteCBInternal(cb);
    }


    NodeGroupPtr isGroup = toNodeGroup(group);
    KnobStringPtr nodeDeletedKnob = nodeRemovalCallback.lock();
    if (!nodeDeletedKnob && isGroup) {
        NodePtr grpNode = isGroup->getNode();
        if (grpNode) {
            cb = grpNode->getBeforeNodeRemovalCallback();
        }
    } else if (nodeDeletedKnob) {
        cb = nodeDeletedKnob->getValue();
    }
    if ( !cb.empty() ) {
        runOnNodeDeleteCBInternal(cb);
    }
}


void
NodePrivate::runInputChangedCallback(int index,
                                              const std::string& cb)
{
    std::vector<std::string> args;
    std::string error;

    try {
        NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
    } catch (const std::exception& e) {
        _publicInterface->getApp()->appendToScriptEditor( std::string("Failed to run onInputChanged callback: ")
                                                         + e.what() );

        return;
    }

    if ( !error.empty() ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onInputChanged callback: " + error);

        return;
    }

    std::string signatureError;
    signatureError.append("The on input changed callback supports the following signature(s):\n");
    signatureError.append("- callback(inputIndex,thisNode,thisGroup,app)");
    if (args.size() != 4) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onInputChanged callback: " + signatureError);

        return;
    }

    if ( (args[0] != "inputIndex") || (args[1] != "thisNode") || (args[2] != "thisGroup") || (args[3] != "app") ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onInputChanged callback: " + signatureError);

        return;
    }

    std::string appID = _publicInterface->getApp()->getAppIDString();
    NodeCollectionPtr collection = _publicInterface->getGroup();
    assert(collection);
    if (!collection) {
        return;
    }

    std::string thisGroupVar;
    NodeGroupPtr isParentGrp = toNodeGroup(collection);
    if (isParentGrp) {
        std::string nodeName = isParentGrp->getNode()->getFullyQualifiedName();
        std::string nodeFullName = appID + "." + nodeName;
        thisGroupVar = nodeFullName;
    } else {
        thisGroupVar = appID;
    }

    std::stringstream ss;
    ss << cb << "(" << index << "," << appID << "." << _publicInterface->getFullyQualifiedName() << "," << thisGroupVar << "," << appID << ")\n";

    std::string script = ss.str();
    std::string output;
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, &output) ) {
        _publicInterface->getApp()->appendToScriptEditor( tr("Failed to execute callback: %1").arg( QString::fromUtf8( error.c_str() ) ).toStdString() );
    } else {
        if ( !output.empty() ) {
            _publicInterface->getApp()->appendToScriptEditor(output);
        }
    }
} //runInputChangedCallback


NATRON_NAMESPACE_EXIT
