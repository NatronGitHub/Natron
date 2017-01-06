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

#include "Engine/RotoPaint.h"
#include "Engine/RenderQueue.h"

NATRON_NAMESPACE_ENTER;


void
Node::createMaskSelectors(const std::vector<std::pair<bool, bool> >& hasMaskChannelSelector,
                          const std::vector<std::string>& inputLabels,
                          const KnobPagePtr& mainPage,
                          bool addNewLineOnLastMask,
                          KnobIPtr* lastKnobCreated)
{
    assert( hasMaskChannelSelector.size() == inputLabels.size() );

    for (std::size_t i = 0; i < hasMaskChannelSelector.size(); ++i) {
        if (!hasMaskChannelSelector[i].first) {
            continue;
        }


        MaskSelector sel;
        KnobBoolPtr enabled = AppManager::createKnob<KnobBool>(_imp->effect, inputLabels[i], 1, false);

        enabled->setDefaultValue(false);
        enabled->setAddNewLine(false);
        enabled->setIsMetadataSlave(true);
        if (hasMaskChannelSelector[i].second) {
            std::string enableMaskName(std::string(kEnableMaskKnobName) + "_" + inputLabels[i]);
            enabled->setName(enableMaskName);
            enabled->setHintToolTip( tr("Enable the mask to come from the channel named by the choice parameter on the right. "
                                        "Turning this off will act as though the mask was disconnected.") );
        } else {
            std::string enableMaskName(std::string(kEnableInputKnobName) + "_" + inputLabels[i]);
            enabled->setName(enableMaskName);
            enabled->setHintToolTip( tr("Enable the image to come from the channel named by the choice parameter on the right. "
                                        "Turning this off will act as though the input was disconnected.") );
        }
        enabled->setAnimationEnabled(false);
        if (mainPage) {
            mainPage->addKnob(enabled);
        }


        sel.enabled = enabled;

        KnobChoicePtr channel = AppManager::createKnob<KnobChoice>(_imp->effect, std::string(), 1, false);

        // By default if connected it should be filled with None, Color.R, Color.G, Color.B, Color.A (@see refreshChannelSelectors)
        channel->setDefaultValue(4);
        channel->setAnimationEnabled(false);
        channel->setIsMetadataSlave(true);
        channel->setHintToolTip( tr("Use this channel from the original input to mix the output with the original input. "
                                    "Setting this to None is the same as disconnecting the input.") );
        if (hasMaskChannelSelector[i].second) {
            std::string channelMaskName(std::string(kMaskChannelKnobName) + "_" + inputLabels[i]);
            channel->setName(channelMaskName);
        } else {
            std::string channelMaskName(std::string(kInputChannelKnobName) + "_" + inputLabels[i]);
            channel->setName(channelMaskName);
        }
        sel.channel = channel;
        if (mainPage) {
            mainPage->addKnob(channel);
        }

        //Make sure the first default param in the vector is MaskInvert

        if (!addNewLineOnLastMask) {
            //If there is a MaskInvert parameter, make it on the same line as the Mask channel parameter
            channel->setAddNewLine(false);
        }
        if (!*lastKnobCreated) {
            *lastKnobCreated = enabled;
        }

        _imp->maskSelectors[i] = sel;
    } // for (int i = 0; i < inputsCount; ++i) {
} // Node::createMaskSelectors


void
NodePrivate::createChannelSelector(int inputNb,
                                   const std::string & inputName,
                                   bool isOutput,
                                   const KnobPagePtr& page,
                                   KnobIPtr* lastKnobBeforeAdvancedOption)
{
    ChannelSelector sel;
    KnobChoicePtr layer = AppManager::createKnob<KnobChoice>(effect, isOutput ? tr("Output Layer") : tr("%1 Layer").arg( QString::fromUtf8( inputName.c_str() ) ), 1, false);
    layer->setNewOptionCallback(&Node::choiceParamAddLayerCallback);
    if (!isOutput) {
        layer->setName( inputName + std::string("_") + std::string(kOutputChannelsKnobName) );
    } else {
        layer->setName(kOutputChannelsKnobName);
    }
    if (isOutput) {
        layer->setHintToolTip( tr("Select here the layer onto which the processing should occur.") );
    } else {
        layer->setHintToolTip( tr("Select here the layer that will be used in input %1.").arg( QString::fromUtf8( inputName.c_str() ) ) );
    }
    layer->setAnimationEnabled(false);
    layer->setSecret(!isOutput);
    layer->setIsMetadataSlave(true);
    page->addKnob(layer);


    if (isOutput) {
        layer->setAddNewLine(false);
        KnobBoolPtr processAllKnob = AppManager::createKnob<KnobBool>(effect, tr(kNodeParamProcessAllLayersLabel), 1, false);
        processAllKnob->setName(kNodeParamProcessAllLayers);
        processAllKnob->setHintToolTip(tr(kNodeParamProcessAllLayersHint));
        processAllKnob->setAnimationEnabled(false);
        processAllKnob->setIsMetadataSlave(true);
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
    layer->setDefaultValue(isOutput ? 0 : 1);

    if (!*lastKnobBeforeAdvancedOption) {
        *lastKnobBeforeAdvancedOption = layer;
    }

    channelsSelectors[inputNb] = sel;
} // createChannelSelector


KnobChoicePtr
Node::getChannelSelectorKnob(int inputNb) const
{
    std::map<int, ChannelSelector>::const_iterator found = _imp->channelsSelectors.find(inputNb);

    if ( found == _imp->channelsSelectors.end() ) {
        if (inputNb == -1) {
            ///The effect might be multi-planar and supply its own
            KnobIPtr knob = getKnobByName(kNatronOfxParamOutputChannels);
            if (!knob) {
                return KnobChoicePtr();
            }

            return toKnobChoice(knob);
        }
        
        return KnobChoicePtr();
    }
    
    return found->second.layer.lock();
}


void
Node::findPluginFormatKnobs()
{
    findPluginFormatKnobs(getKnobs(), true);
}

void
Node::findRightClickMenuKnob(const KnobsVec& knobs)
{
    for (std::size_t i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->getName() == kNatronOfxParamRightClickMenu) {
            KnobIPtr rightClickKnob = knobs[i];
            KnobChoicePtr isChoice = toKnobChoice(rightClickKnob);
            if (isChoice) {
                QObject::connect( isChoice.get(), SIGNAL(populated()), this, SIGNAL(rightClickMenuKnobPopulated()) );
            }
            break;
        }
    }
}

void
Node::findPluginFormatKnobs(const KnobsVec & knobs,
                            bool loadingSerialization)
{
    ///Try to find a format param and hijack it to handle it ourselves with the project's formats
    KnobIPtr formatKnob;

    for (std::size_t i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->getName() == kNatronParamFormatChoice) {
            formatKnob = knobs[i];
            break;
        }
    }
    if (formatKnob) {
        KnobIPtr formatSize;
        for (std::size_t i = 0; i < knobs.size(); ++i) {
            if (knobs[i]->getName() == kNatronParamFormatSize) {
                formatSize = knobs[i];
                break;
            }
        }
        KnobIPtr formatPar;
        for (std::size_t i = 0; i < knobs.size(); ++i) {
            if (knobs[i]->getName() == kNatronParamFormatPar) {
                formatPar = knobs[i];
                break;
            }
        }
        if (formatSize && formatPar) {
            _imp->pluginFormatKnobs.formatChoice = toKnobChoice(formatKnob);
            formatSize->setEvaluateOnChange(false);
            formatPar->setEvaluateOnChange(false);
            formatSize->setSecret(true);
            formatPar->setSecret(true);
            _imp->pluginFormatKnobs.size = toKnobInt(formatSize);
            _imp->pluginFormatKnobs.par = toKnobDouble(formatPar);

            std::vector<ChoiceOption> formats;
            int defValue;
            getApp()->getProject()->getProjectFormatEntries(&formats, &defValue);

            refreshFormatParamChoice(formats, defValue, loadingSerialization);
        }
    }
}

void
Node::createNodePage(const KnobPagePtr& settingsPage)
{
    KnobBoolPtr hideInputs = AppManager::createKnob<KnobBool>(_imp->effect, tr("Hide inputs"), 1, false);

    hideInputs->setName("hideInputs");
    hideInputs->setDefaultValue(false);
    hideInputs->setAnimationEnabled(false);
    hideInputs->setAddNewLine(false);
    hideInputs->setIsPersistent(true);
    hideInputs->setEvaluateOnChange(false);
    hideInputs->setHintToolTip( tr("When checked, the input arrows of the node in the nodegraph will be hidden") );
    _imp->hideInputs = hideInputs;
    settingsPage->addKnob(hideInputs);


    KnobBoolPtr fCaching = AppManager::createKnob<KnobBool>(_imp->effect, tr("Force caching"), 1, false);
    fCaching->setName("forceCaching");
    fCaching->setDefaultValue(false);
    fCaching->setAnimationEnabled(false);
    fCaching->setAddNewLine(false);
    fCaching->setIsPersistent(true);
    fCaching->setEvaluateOnChange(false);
    fCaching->setHintToolTip( tr("When checked, the output of this node will always be kept in the RAM cache for fast access of already computed "
                                 "images.") );
    _imp->forceCaching = fCaching;
    settingsPage->addKnob(fCaching);

    KnobBoolPtr previewEnabled = AppManager::createKnob<KnobBool>(_imp->effect, tr("Preview"), 1, false);
    assert(previewEnabled);
    previewEnabled->setDefaultValue( makePreviewByDefault() );
    previewEnabled->setName(kEnablePreviewKnobName);
    previewEnabled->setAnimationEnabled(false);
    previewEnabled->setAddNewLine(false);
    previewEnabled->setIsPersistent(false);
    previewEnabled->setEvaluateOnChange(false);
    previewEnabled->setHintToolTip( tr("Whether to show a preview on the node box in the node-graph.") );
    settingsPage->addKnob(previewEnabled);
    _imp->previewEnabledKnob = previewEnabled;

    KnobBoolPtr disableNodeKnob = AppManager::createKnob<KnobBool>(_imp->effect, tr("Disable"), 1, false);
    assert(disableNodeKnob);
    disableNodeKnob->setAnimationEnabled(false);
    disableNodeKnob->setIsMetadataSlave(true);
    disableNodeKnob->setName(kDisableNodeKnobName);
    disableNodeKnob->setAddNewLine(false);
    _imp->effect->addOverlaySlaveParam(disableNodeKnob);
    disableNodeKnob->setHintToolTip( tr("When disabled, this node acts as a pass through.") );
    settingsPage->addKnob(disableNodeKnob);
    _imp->disableNodeKnob = disableNodeKnob;


    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>(_imp->effect, tr(kNatronNodeKnobKeepInAnimationModuleButtonLabel), 1, false);
        param->setName(kNatronNodeKnobKeepInAnimationModuleButton);
        param->setHintToolTip(tr(kNatronNodeKnobKeepInAnimationModuleButtonHint));
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "DS_CE_Icon.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "DS_CE_Icon.png", false);
        settingsPage->addKnob(param);
        _imp->keepInAnimationModuleKnob = param;
    }



    KnobIntPtr lifeTimeKnob = AppManager::createKnob<KnobInt>(_imp->effect, tr("Lifetime Range"), 2, false);
    assert(lifeTimeKnob);
    lifeTimeKnob->setAnimationEnabled(false);
    lifeTimeKnob->setIsMetadataSlave(true);
    lifeTimeKnob->setName(kLifeTimeNodeKnobName);
    lifeTimeKnob->setAddNewLine(false);
    lifeTimeKnob->setHintToolTip( tr("This is the frame range during which the node will be active if Enable Lifetime is checked") );
    settingsPage->addKnob(lifeTimeKnob);
    _imp->lifeTimeKnob = lifeTimeKnob;


    KnobBoolPtr enableLifetimeNodeKnob = AppManager::createKnob<KnobBool>(_imp->effect, tr("Enable Lifetime"), 1, false);
    assert(enableLifetimeNodeKnob);
    enableLifetimeNodeKnob->setAnimationEnabled(false);
    enableLifetimeNodeKnob->setDefaultValue(false);
    enableLifetimeNodeKnob->setIsMetadataSlave(true);
    enableLifetimeNodeKnob->setName(kEnableLifeTimeNodeKnobName);
    enableLifetimeNodeKnob->setHintToolTip( tr("When checked, the node is only active during the specified frame range by the Lifetime Range parameter. "
                                               "Outside of this frame range, it behaves as if the Disable parameter is checked") );
    settingsPage->addKnob(enableLifetimeNodeKnob);
    _imp->enableLifeTimeKnob = enableLifetimeNodeKnob;


    PluginOpenGLRenderSupport glSupport = ePluginOpenGLRenderSupportNone;

    PluginPtr plugin = getPlugin();
    if (plugin && plugin->isOpenGLEnabled()) {
        glSupport = (PluginOpenGLRenderSupport)plugin->getProperty<int>(kNatronPluginPropOpenGLSupport);
    }
    // The Roto node needs to have a "GPU enabled" knob to control the nodes internally
    if (glSupport != ePluginOpenGLRenderSupportNone || dynamic_cast<RotoPaint*>(_imp->effect.get())) {
        getOrCreateOpenGLEnabledKnob();
    }


    KnobStringPtr knobChangedCallback = AppManager::createKnob<KnobString>(_imp->effect, tr("After param changed callback"), 1, false);
    knobChangedCallback->setHintToolTip( tr("Set here the name of a function defined in Python which will be called for each  "
                                            "parameter change. Either define this function in the Script Editor "
                                            "or in the init.py script or even in the script of a Python group plug-in.\n"
                                            "The signature of the callback is: callback(thisParam, thisNode, thisGroup, app, userEdited) where:\n"
                                            "- thisParam: The parameter which just had its value changed\n"
                                            "- userEdited: A boolean informing whether the change was due to user interaction or "
                                            "because something internally triggered the change.\n"
                                            "- thisNode: The node holding the parameter\n"
                                            "- app: points to the current application instance\n"
                                            "- thisGroup: The group holding thisNode (only if thisNode belongs to a group)") );
    knobChangedCallback->setAnimationEnabled(false);
    knobChangedCallback->setName("onParamChanged");
    settingsPage->addKnob(knobChangedCallback);
    _imp->knobChangedCallback = knobChangedCallback;

    KnobStringPtr inputChangedCallback = AppManager::createKnob<KnobString>(_imp->effect, tr("After input changed callback"), 1, false);
    inputChangedCallback->setHintToolTip( tr("Set here the name of a function defined in Python which will be called after "
                                             "each connection is changed for the inputs of the node. "
                                             "Either define this function in the Script Editor "
                                             "or in the init.py script or even in the script of a Python group plug-in.\n"
                                             "The signature of the callback is: callback(inputIndex, thisNode, thisGroup, app):\n"
                                             "- inputIndex: the index of the input which changed, you can query the node "
                                             "connected to the input by calling the getInput(...) function.\n"
                                             "- thisNode: The node holding the parameter\n"
                                             "- app: points to the current application instance\n"
                                             "- thisGroup: The group holding thisNode (only if thisNode belongs to a group)") );

    inputChangedCallback->setAnimationEnabled(false);
    inputChangedCallback->setName("onInputChanged");
    settingsPage->addKnob(inputChangedCallback);
    _imp->inputChangedCallback = inputChangedCallback;

    NodeGroupPtr isGroup = isEffectNodeGroup();
    if (isGroup) {
        KnobStringPtr onNodeCreated = AppManager::createKnob<KnobString>(_imp->effect, tr("After Node Created"), 1, false);
        onNodeCreated->setName("afterNodeCreated");
        onNodeCreated->setHintToolTip( tr("Add here the name of a Python-defined function that will be called each time a node "
                                          "is created in the group. This will be called in addition to the After Node Created "
                                          " callback of the project for the group node and all nodes within it (not recursively).\n"
                                          "The boolean variable userEdited will be set to True if the node was created "
                                          "by the user or False otherwise (such as when loading a project, or pasting a node).\n"
                                          "The signature of the callback is: callback(thisNode, app, userEdited) where:\n"
                                          "- thisNode: the node which has just been created\n"
                                          "- userEdited: a boolean indicating whether the node was created by user interaction or from "
                                          "a script/project load/copy-paste\n"
                                          "- app: points to the current application instance.") );
        onNodeCreated->setAnimationEnabled(false);
        _imp->nodeCreatedCallback = onNodeCreated;
        settingsPage->addKnob(onNodeCreated);

        KnobStringPtr onNodeDeleted = AppManager::createKnob<KnobString>(_imp->effect, tr("Before Node Removal"), 1, false);
        onNodeDeleted->setName("beforeNodeRemoval");
        onNodeDeleted->setHintToolTip( tr("Add here the name of a Python-defined function that will be called each time a node "
                                          "is about to be deleted. This will be called in addition to the Before Node Removal "
                                          " callback of the project for the group node and all nodes within it (not recursively).\n"
                                          "This function will not be called when the project is closing.\n"
                                          "The signature of the callback is: callback(thisNode, app) where:\n"
                                          "- thisNode: the node about to be deleted\n"
                                          "- app: points to the current application instance.") );
        onNodeDeleted->setAnimationEnabled(false);
        _imp->nodeRemovalCallback = onNodeDeleted;
        settingsPage->addKnob(onNodeDeleted);
    }
    if (_imp->effect->getItemsTable()) {
        KnobStringPtr param = AppManager::createKnob<KnobString>(_imp->effect, tr("After Items Selection Changed"), 1, false);
        param->setName("afterItemsSelectionChanged");
        param->setHintToolTip( tr("Add here the name of a Python-defined function that will be called each time the "
                                  "selection in the table changes. "
                                  "The variable \"reason\" will be set to a value of type NatronEngine.Natron.TableChangeReasonEnum "
                                  "depending on where the selection was made from. If reason is "
                                  "NatronEngine.Natron.TableChangeReasonEnum.eTableChangeReasonViewer then the selection was made "
                                  "from the viewer. If reason is NatronEngine.Natron.TableChangeReasonEnum.eTableChangeReasonPanel "
                                  "then the selection was made from the settings panel. Otherwise the selection was not changed "
                                  "by the user directly and results from an internal A.P.I call.\n"
                                  "The signature of the callback is: callback(thisNode, app, deselected, selected, reason) where:\n"
                                  "- thisNode: the node holding the items table\n"
                                  "- app: points to the current application instance\n"
                                  "- deselected: a sequence of items that were removed from the selection\n"
                                  "- selected: a sequence of items that were added to the selection\n"
                                  "- reason: a value of type NatronEngine.Natron.TableChangeReasonEnum") );
        param->setAnimationEnabled(false);
        _imp->tableSelectionChangedCallback = param;
        settingsPage->addKnob(param);

    }
} // Node::createNodePage

void
Node::createInfoPage()
{
    KnobPagePtr infoPage = AppManager::createKnob<KnobPage>(_imp->effect, tr(kInfoPageParamLabel), 1, false);
    infoPage->setName(kInfoPageParamName);
    _imp->infoPage = infoPage;

    KnobStringPtr nodeInfos = AppManager::createKnob<KnobString>(_imp->effect, std::string(), 1, false);
    nodeInfos->setName("nodeInfos");
    nodeInfos->setAnimationEnabled(false);
    nodeInfos->setIsPersistent(false);
    nodeInfos->setAsMultiLine();
    nodeInfos->setAsCustomHTMLText(true);
    nodeInfos->setEvaluateOnChange(false);
    nodeInfos->setHintToolTip( tr("Input and output informations, press Refresh to update them with current values") );
    infoPage->addKnob(nodeInfos);
    _imp->nodeInfos = nodeInfos;


    KnobButtonPtr refreshInfoButton = AppManager::createKnob<KnobButton>(_imp->effect, tr("Refresh Info"), 1, false);
    refreshInfoButton->setName("refreshButton");
    refreshInfoButton->setEvaluateOnChange(false);
    infoPage->addKnob(refreshInfoButton);
    _imp->refreshInfoButton = refreshInfoButton;
}

void
Node::createPyPlugExportGroup()
{
    // Create the knobs in either page since they are hidden anyway
    KnobPagePtr mainPage;
    const KnobsVec& knobs = getKnobs();
    for (std::size_t i = 0; i < knobs.size(); ++i) {
        KnobPagePtr p = toKnobPage(knobs[i]);
        if (p) {
            mainPage = p;
            break;
        }
    }
    assert(mainPage);
    KnobGroupPtr group;
    {
        KnobGroupPtr param = AppManager::createKnob<KnobGroup>( _imp->effect, tr(kNatronNodeKnobExportPyPlugGroupLabel), 1, false );
        group = param;
        param->setName(kNatronNodeKnobExportPyPlugGroup);
        param->setEvaluateOnChange(false);
        param->setDefaultValue(false);
        param->setIsPersistent(false);
        param->setAsDialog(true);
        if (mainPage) {
            mainPage->addKnob(param);
        }
        _imp->pyPlugExportDialog = param;
    }

    {
        KnobFilePtr param = AppManager::createKnob<KnobFile>(_imp->effect, tr(kNatronNodeKnobExportDialogFilePathLabel), 1, false);
        param->setName(kNatronNodeKnobExportDialogFilePath);
        param->setDialogType(KnobFile::eKnobFileDialogTypeSaveFile);
        {
            std::vector<std::string> filters;
            filters.push_back(NATRON_PRESETS_FILE_EXT);
            param->setDialogFilters(filters);
        }
        param->setEvaluateOnChange(false);
        param->setIsPersistent(false);
        param->setHintToolTip(tr(kNatronNodeKnobExportDialogFilePathHint));
        group->addKnob(param);
        _imp->pyPlugExportDialogFile = param;
    }
    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( _imp->effect, tr(kNatronNodeKnobExportDialogOkButtonLabel), 1, false );
        param->setName(kNatronNodeKnobExportDialogOkButton);
        param->setAddNewLine(false);
        param->setEvaluateOnChange(false);
        param->setSpacingBetweenItems(3);
        param->setIsPersistent(false);
        group->addKnob(param);
        _imp->pyPlugExportDialogOkButton = param;
    }
    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( _imp->effect, tr(kNatronNodeKnobExportDialogCancelButtonLabel), 1, false );
        param->setName(kNatronNodeKnobExportDialogCancelButton);
        param->setEvaluateOnChange(false);
        param->setSpacingBetweenItems(3);
        param->setIsPersistent(false);
        group->addKnob(param);
        _imp->pyPlugExportDialogCancelButton = param;
    }
}

void
Node::createPyPlugPage()
{
    PluginPtr pyPlug = _imp->pyPlugHandle.lock();
    KnobPagePtr page = AppManager::createKnob<KnobPage>(_imp->effect, tr(kPyPlugPageParamLabel), 1, false);
    page->setName(kPyPlugPageParamName);
    page->setSecret(true);
    _imp->pyPlugPage = page;

    {
        KnobStringPtr param = AppManager::createKnob<KnobString>(_imp->effect, tr(kNatronNodeKnobPyPlugPluginIDLabel), 1, false);
        param->setName(kNatronNodeKnobPyPlugPluginID);
        if (pyPlug) {
            param->setValue(pyPlug->getPluginID());
        }
        param->setEvaluateOnChange(false);
        param->setHintToolTip(tr(kNatronNodeKnobPyPlugPluginIDHint));
        page->addKnob(param);
        _imp->pyPlugIDKnob = param;
    }
    {
        KnobStringPtr param = AppManager::createKnob<KnobString>(_imp->effect, tr(kNatronNodeKnobPyPlugPluginLabelLabel), 1, false);
        param->setName(kNatronNodeKnobPyPlugPluginLabel);
        if (pyPlug) {
            param->setValue(pyPlug->getPluginLabel());
        }
        param->setEvaluateOnChange(false);
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginLabelHint));
        page->addKnob(param);
        _imp->pyPlugLabelKnob = param;
    }
    {
        KnobStringPtr param = AppManager::createKnob<KnobString>(_imp->effect, tr(kNatronNodeKnobPyPlugPluginGroupingLabel), 1, false);
        param->setName(kNatronNodeKnobPyPlugPluginGrouping);
        if (pyPlug) {
            param->setValue(pyPlug->getGroupingString());
        } else {
            param->setValue("PyPlugs");
        }
        param->setEvaluateOnChange(false);
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginGroupingHint));
        page->addKnob(param);
        _imp->pyPlugGroupingKnob = param;
    }
    {
        KnobStringPtr param = AppManager::createKnob<KnobString>(_imp->effect, tr(kNatronNodeKnobPyPlugPluginDescriptionLabel), 1, false);
        param->setName(kNatronNodeKnobPyPlugPluginDescription);
        param->setEvaluateOnChange(false);
        param->setAsMultiLine();
        if (pyPlug) {
            param->setValue(pyPlug->getProperty<std::string>(kNatronPluginPropDescription));
        }
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginDescriptionHint));
        param->setAddNewLine(false);
        page->addKnob(param);
        _imp->pyPlugDescKnob = param;
    }
    {
        KnobBoolPtr param = AppManager::createKnob<KnobBool>(_imp->effect, tr(kNatronNodeKnobPyPlugPluginDescriptionIsMarkdownLabel), 1, false);
        param->setName(kNatronNodeKnobPyPlugPluginDescriptionIsMarkdown);
        param->setEvaluateOnChange(false);
        if (pyPlug) {
            param->setValue(pyPlug->getProperty<bool>(kNatronPluginPropDescriptionIsMarkdown));
        }
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginDescriptionIsMarkdownHint));
        page->addKnob(param);
        _imp->pyPlugDescIsMarkdownKnob = param;
    }
    {
        KnobIntPtr param = AppManager::createKnob<KnobInt>(_imp->effect, tr(kNatronNodeKnobPyPlugPluginVersionLabel), 2, false);
        param->setName(kNatronNodeKnobPyPlugPluginVersion);
        param->setEvaluateOnChange(false);
        param->setDimensionName(DimIdx(0), "Major");
        param->setDimensionName(DimIdx(1), "Minor");
        if (pyPlug) {
            param->setValue((int)pyPlug->getProperty<unsigned int>(kNatronPluginPropVersion, 0));
            param->setValue((int)pyPlug->getProperty<unsigned int>(kNatronPluginPropVersion, 1), ViewSetSpec::all(), DimSpec(1));
        }
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginVersionHint));
        page->addKnob(param);
        _imp->pyPlugVersionKnob = param;
    }
    {
        KnobIntPtr param = AppManager::createKnob<KnobInt>(_imp->effect, tr(kNatronNodeKnobPyPlugPluginShortcutLabel), 2, false);
        param->setName(kNatronNodeKnobPyPlugPluginShortcut);
        param->setEvaluateOnChange(false);
        param->setAsShortcutKnob(true);
        if (pyPlug) {
            param->setValue(pyPlug->getProperty<int>(kNatronPluginPropShortcut, 0));
            param->setValue(pyPlug->getProperty<int>(kNatronPluginPropShortcut, 1), ViewSetSpec::all(), DimSpec(1));
        }
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginShortcutHint));
        page->addKnob(param);
        _imp->pyPlugShortcutKnob = param;
    }
    {
        KnobFilePtr param = AppManager::createKnob<KnobFile>(_imp->effect, tr(kNatronNodeKnobPyPlugPluginCallbacksPythonScriptLabel), 1, false);
        param->setName(kNatronNodeKnobPyPlugPluginCallbacksPythonScript);
        param->setEvaluateOnChange(false);
        if (pyPlug) {
            param->setValue(pyPlug->getProperty<std::string>(kNatronPluginPropPyPlugExtScriptFile));
        }
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginCallbacksPythonScriptHint));
        page->addKnob(param);
        _imp->pyPlugExtPythonScript = param;
    }
    {
        KnobFilePtr param = AppManager::createKnob<KnobFile>(_imp->effect, tr(kNatronNodeKnobPyPlugPluginIconFileLabel), 1, false);
        param->setName(kNatronNodeKnobPyPlugPluginIconFile);
        param->setEvaluateOnChange(false);
        if (pyPlug) {
            param->setValue(pyPlug->getProperty<std::string>(kNatronPluginPropIconFilePath));
        }
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginIconFileHint));
        page->addKnob(param);
        _imp->pyPlugIconKnob = param;
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>(_imp->effect, tr(kNatronNodeKnobExportPyPlugButtonLabel), 1, false);
        param->setName(kNatronNodeKnobExportPyPlugButton);
        param->setEvaluateOnChange(false);
        param->setHintToolTip( tr("Click to export this group to a PyPlug file (.%1)").arg(QLatin1String(NATRON_PRESETS_FILE_EXT)));
        page->addKnob(param);
        _imp->pyPlugExportButtonKnob = param;
    }


}

void
Node::createPythonPage()
{
    KnobPagePtr pythonPage = AppManager::createKnob<KnobPage>(_imp->effect, tr("Python"), 1, false);
    KnobStringPtr beforeFrameRender =  AppManager::createKnob<KnobString>(_imp->effect, tr("Before frame render"), 1, false);

    beforeFrameRender->setName("beforeFrameRender");
    beforeFrameRender->setAnimationEnabled(false);
    beforeFrameRender->setHintToolTip( tr("Add here the name of a Python defined function that will be called before rendering "
                                          "any frame.\n "
                                          "The signature of the callback is: callback(frame, thisNode, app) where:\n"
                                          "- frame: the frame to be rendered\n"
                                          "- thisNode: points to the writer node\n"
                                          "- app: points to the current application instance") );
    pythonPage->addKnob(beforeFrameRender);
    _imp->beforeFrameRender = beforeFrameRender;

    KnobStringPtr beforeRender =  AppManager::createKnob<KnobString>(_imp->effect, tr("Before render"), 1, false);
    beforeRender->setName("beforeRender");
    beforeRender->setAnimationEnabled(false);
    beforeRender->setHintToolTip( tr("Add here the name of a Python defined function that will be called once when "
                                     "starting rendering.\n "
                                     "The signature of the callback is: callback(thisNode, app) where:\n"
                                     "- thisNode: points to the writer node\n"
                                     "- app: points to the current application instance") );
    pythonPage->addKnob(beforeRender);
    _imp->beforeRender = beforeRender;

    KnobStringPtr afterFrameRender =  AppManager::createKnob<KnobString>(_imp->effect, tr("After frame render"), 1, false);
    afterFrameRender->setName("afterFrameRender");
    afterFrameRender->setAnimationEnabled(false);
    afterFrameRender->setHintToolTip( tr("Add here the name of a Python defined function that will be called after rendering "
                                         "any frame.\n "
                                         "The signature of the callback is: callback(frame, thisNode, app) where:\n"
                                         "- frame: the frame that has been rendered\n"
                                         "- thisNode: points to the writer node\n"
                                         "- app: points to the current application instance") );
    pythonPage->addKnob(afterFrameRender);
    _imp->afterFrameRender = afterFrameRender;

    KnobStringPtr afterRender =  AppManager::createKnob<KnobString>(_imp->effect, tr("After render"), 1, false);
    afterRender->setName("afterRender");
    afterRender->setAnimationEnabled(false);
    afterRender->setHintToolTip( tr("Add here the name of a Python defined function that will be called once when the rendering "
                                    "is finished.\n "
                                    "The signature of the callback is: callback(aborted, thisNode, app) where:\n"
                                    "- aborted: True if the render ended because it was aborted, False upon completion\n"
                                    "- thisNode: points to the writer node\n"
                                    "- app: points to the current application instance") );
    pythonPage->addKnob(afterRender);
    _imp->afterRender = afterRender;
} // Node::createPythonPage

KnobDoublePtr
Node::getOrCreateHostMixKnob(const KnobPagePtr& mainPage)
{
    KnobDoublePtr mixKnob = _imp->mixWithSource.lock();
    if (!mixKnob) {
        mixKnob = AppManager::createKnob<KnobDouble>(_imp->effect, tr(kHostMixingKnobLabel));
        mixKnob->setName(kHostMixingKnobName);
        mixKnob->setDeclaredByPlugin(false);
        mixKnob->setHintToolTip( tr(kHostMixingKnobHint) );
        mixKnob->setRange(0., 1.);
        mixKnob->setDefaultValue(1.);
        if (mainPage) {
            mainPage->addKnob(mixKnob);
        }
        _imp->mixWithSource = mixKnob;
    }
    
    return mixKnob;
}


KnobPagePtr
Node::getOrCreateMainPage()
{
    const KnobsVec & knobs = _imp->effect->getKnobs();
    KnobPagePtr mainPage;

    for (std::size_t i = 0; i < knobs.size(); ++i) {
        KnobPagePtr p = toKnobPage(knobs[i]);
        if ( p && (p->getName() != kInfoPageParamName) &&
            (p->getName() != kNodePageParamName) ) {
            mainPage = p;
            break;
        }
    }
    if (!mainPage) {
        mainPage = AppManager::createKnob<KnobPage>( _imp->effect, tr("Settings") );
    }

    return mainPage;
}

void
Node::createLabelKnob(const KnobPagePtr& settingsPage,
                      const std::string& label)
{
    KnobStringPtr nodeLabel = AppManager::createKnob<KnobString>(_imp->effect, label, 1, false);

    assert(nodeLabel);
    nodeLabel->setName(kUserLabelKnobName);
    nodeLabel->setAnimationEnabled(false);
    nodeLabel->setEvaluateOnChange(false);
    nodeLabel->setAsMultiLine();
    nodeLabel->setUsesRichText(true);
    nodeLabel->setHintToolTip( tr("This label gets appended to the node name on the node graph.") );
    settingsPage->addKnob(nodeLabel);
    _imp->nodeLabelKnob = nodeLabel;
}



int
Node::getFrameStepKnobValue() const
{
    KnobIPtr knob = getKnobByName(kNatronWriteParamFrameStep);
    if (!knob) {
        return 1;
    }
    KnobIntPtr k = toKnobInt(knob);

    if (!k) {
        return 1;
    } else {
        int v = k->getValue();

        return std::max(1, v);
    }
}



bool
Node::handleFormatKnob(const KnobIPtr& knob)
{
    KnobChoicePtr choice = _imp->pluginFormatKnobs.formatChoice.lock();

    if (!choice) {
        return false;
    }

    if (knob != choice) {
        return false;
    }
    if (choice->getIsSecret()) {
        return true;
    }
    int curIndex = choice->getValue();
    Format f;
    if ( !getApp()->getProject()->getProjectFormatAtIndex(curIndex, &f) ) {
        assert(false);

        return true;
    }

    KnobIntPtr size = _imp->pluginFormatKnobs.size.lock();
    KnobDoublePtr par = _imp->pluginFormatKnobs.par.lock();
    assert(size && par);

    _imp->effect->beginChanges();
    size->blockValueChanges();
    std::vector<int> values(2);
    values[0] = f.width();
    values[1] = f.height();
    size->setValueAcrossDimensions(values);
    size->unblockValueChanges();
    par->blockValueChanges();
    par->setValue( f.getPixelAspectRatio() );
    par->unblockValueChanges();

    _imp->effect->endChanges();

    return true;
}

void
Node::refreshFormatParamChoice(const std::vector<ChoiceOption>& entries,
                               int defValue,
                               bool loadingProject)
{
    KnobChoicePtr choice = _imp->pluginFormatKnobs.formatChoice.lock();

    if (!choice) {
        return;
    }
    int curIndex = choice->getValue();
    choice->populateChoices(entries);
    choice->beginChanges();

    choice->setDefaultValueWithoutApplying(defValue);
    // We don't want to serialize the default value even if it changed, because it will be restored by the project
    choice->setCurrentDefaultValueAsInitialValue();

    if (!loadingProject) {
        //changedKnob was not called because we are initializing knobs
        handleFormatKnob(choice);
    } else {
        if ( curIndex < (int)entries.size() ) {
            choice->setValue(curIndex);
        }
    }
    
    choice->endChanges();
}



void
Node::setPagesOrder(const std::list<std::string>& pages)
{
    //re-order the pages
    std::vector<KnobIPtr> pagesOrdered(pages.size());

    KnobsVec knobs = getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        KnobPagePtr isPage = toKnobPage(*it);
        if (!isPage) {
            continue;
        }

        // Look for it in the pages
        int i = 0;
        bool found = false;
        for (std::list<std::string>::const_iterator it2 = pages.begin(); it2 != pages.end(); ++it2, ++i) {
            if (*it2 == isPage->getName()) {
                pagesOrdered[i] = *it;
                _imp->effect->removeKnobFromList(isPage);
                isPage->setSecret(false);
                found = true;
                break;
            }
        }
        if (!found && isPage->isUserKnob()) {
            isPage->setSecret(true);
        }
    }
    int index = 0;
    for (std::vector<KnobIPtr >::iterator it =  pagesOrdered.begin(); it != pagesOrdered.end(); ++it, ++index) {
        if (*it) {
            _imp->effect->insertKnob(index, *it);
        }
    }
}

void
Node::refreshDefaultPagesOrder()
{
    _imp->refreshDefaultPagesOrder();
}


std::list<std::string>
Node::getPagesOrder() const
{
    KnobsVec knobs = _imp->effect->getKnobs_mt_safe();
    std::list<std::string> ret;

    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        KnobPagePtr ispage = toKnobPage(*it);
        if (ispage && !ispage->getChildren().empty() && !ispage->getIsSecret()) {
            ret.push_back( ispage->getName() );
        }
    }

    return ret;
}

bool
Node::hasPageOrderChangedSinceDefault() const
{
    std::list<std::string> pagesOrder = getPagesOrder();
    if (pagesOrder.size() != _imp->defaultPagesOrder.size()) {
        return true;
    }
    std::list<std::string>::const_iterator it2 = _imp->defaultPagesOrder.begin();
    for (std::list<std::string>::const_iterator it = pagesOrder.begin(); it!=pagesOrder.end(); ++it, ++it2) {
        if (*it != *it2) {
            return true;
        }
    }
    return false;
}


void
Node::restoreSublabel()
{

    KnobIPtr sublabelKnob = getKnobByName(kNatronOfxParamStringSublabelName);
    if (!sublabelKnob) {
        return;
    }

    // Make sure the knob is not persistent
    sublabelKnob->setIsPersistent(false);

    KnobStringPtr sublabelKnobIsString = toKnobString(sublabelKnob);
    _imp->ofxSubLabelKnob = sublabelKnobIsString;

    NodePtr ioContainer = getIOContainer();
    if (ioContainer) {
        ioContainer->_imp->ofxSubLabelKnob = sublabelKnobIsString;
        Q_EMIT ioContainer->nodeExtraLabelChanged();
    }
    
    
}

KnobChoicePtr
Node::getOrCreateOpenGLEnabledKnob() const
{
    KnobChoicePtr openglRenderingKnob = _imp->openglRenderingEnabledKnob.lock();
    if (openglRenderingKnob) {
        return openglRenderingKnob;
    }
    openglRenderingKnob = AppManager::checkIfKnobExistsWithNameOrCreate<KnobChoice>(_imp->effect, "enableGPURendering", tr("GPU Rendering"));
    openglRenderingKnob->setDeclaredByPlugin(false);
    assert(openglRenderingKnob);
    openglRenderingKnob->setAnimationEnabled(false);
    {
        std::vector<ChoiceOption> entries;
        entries.push_back(ChoiceOption("Enabled", "", tr("If a plug-in support GPU rendering, prefer rendering using the GPU if possible.").toStdString() ));
        entries.push_back(ChoiceOption("Disabled", "", tr("Disable GPU rendering for all plug-ins.").toStdString()));
        entries.push_back(ChoiceOption("Disabled if background", "", tr("Disable GPU rendering when rendering with NatronRenderer but not in GUI mode.").toStdString()));
        openglRenderingKnob->populateChoices(entries);
    }

    openglRenderingKnob->setHintToolTip( tr("Select when to activate GPU rendering for this node. Note that if the GPU Rendering parameter in the Project settings is set to disabled then GPU rendering will not be activated regardless of that value.") );

    KnobPagePtr settingsPageKnob = AppManager::checkIfKnobExistsWithNameOrCreate<KnobPage>(_imp->effect, kNodePageParamName, tr(kNodePageParamLabel));
    settingsPageKnob->addKnob(openglRenderingKnob);
    _imp->openglRenderingEnabledKnob = openglRenderingKnob;

    return openglRenderingKnob;
}

void
Node::onOpenGLEnabledKnobChangedOnProject(bool activated)
{
    bool enabled = activated;
    KnobChoicePtr k = _imp->openglRenderingEnabledKnob.lock();
    if (enabled) {
        if (k) {
            k->setEnabled(true);
            int thisKnobIndex = k->getValue();
            if (thisKnobIndex == 1 || (thisKnobIndex == 2 && getApp()->isBackground())) {
                enabled = false;
            }
        }
    } else {
        if (k) {
            k->setEnabled(true);
        }
    }
    _imp->effect->onEnableOpenGLKnobValueChanged(enabled);
    
}

void
Node::initializeDefaultKnobs(bool loadingSerialization, bool hasGUI)
{
    //Readers and Writers don't have default knobs since these knobs are on the ReadNode/WriteNode itself
    NodePtr ioContainer = getIOContainer();

    //Add the "Node" page
    KnobPagePtr settingsPage = AppManager::checkIfKnobExistsWithNameOrCreate<KnobPage>(_imp->effect, kNodePageParamName, tr(kNodePageParamLabel));
    settingsPage->setDeclaredByPlugin(false);

    //Create the "Label" knob
    BackdropPtr isBackdropNode = isEffectBackdrop();
    QString labelKnobLabel = isBackdropNode ? tr("Name label") : tr("Label");
    createLabelKnob( settingsPage, labelKnobLabel.toStdString() );

    if (isBackdropNode || isEffectStubNode()) {
        //backdrops just have a label
        return;
    }


    ///find in all knobs a page param to set this param into
    const KnobsVec & knobs = _imp->effect->getKnobs();

    findPluginFormatKnobs(knobs, loadingSerialization);
    findRightClickMenuKnob(knobs);

    KnobPagePtr mainPage;


    // Scan all inputs to find masks and get inputs labels
    // Pair hasMaskChannelSelector, isMask
    int inputsCount = getMaxInputCount();
    std::vector<std::pair<bool, bool> > hasMaskChannelSelector(inputsCount);
    std::vector<std::string> inputLabels(inputsCount);
    for (int i = 0; i < inputsCount; ++i) {
        inputLabels[i] = _imp->effect->getInputLabel(i);

        assert( i < (int)_imp->inputsComponents.size() );
        std::bitset<4> inputSupportedComps = _imp->inputsComponents[i];
        bool isMask = _imp->effect->isInputMask(i);
        bool supportsOnlyAlpha = inputSupportedComps[0] && !inputSupportedComps[1] && !inputSupportedComps[2] && !inputSupportedComps[3];

        hasMaskChannelSelector[i].first = false;
        hasMaskChannelSelector[i].second = isMask;

        if ( isMask || supportsOnlyAlpha ) {
            hasMaskChannelSelector[i].first = true;
            if (!mainPage) {
                mainPage = getOrCreateMainPage();
            }
        }
    }


    // Create the Output Layer choice if needed plus input layers selectors
    KnobIPtr lastKnobBeforeAdvancedOption;
    bool requiresLayerShuffle = _imp->effect->getCreateChannelSelectorKnob();
    if (requiresLayerShuffle) {
        if (!mainPage) {
            mainPage = getOrCreateMainPage();
        }
        createChannelSelectors(hasMaskChannelSelector, inputLabels, mainPage, &lastKnobBeforeAdvancedOption);
    }


    findOrCreateChannelEnabled();

    ///Find in the plug-in the Mask/Mix related parameter to re-order them so it is consistent across nodes
    std::vector<std::pair<std::string, KnobIPtr > > foundPluginDefaultKnobsToReorder;
    foundPluginDefaultKnobsToReorder.push_back( std::make_pair( kOfxMaskInvertParamName, KnobIPtr() ) );
    foundPluginDefaultKnobsToReorder.push_back( std::make_pair( kOfxMixParamName, KnobIPtr() ) );
    ///Insert auto-added knobs before mask invert if found
    for (std::size_t i = 0; i < knobs.size(); ++i) {
        for (std::size_t j = 0; j < foundPluginDefaultKnobsToReorder.size(); ++j) {
            if (knobs[i]->getName() == foundPluginDefaultKnobsToReorder[j].first) {
                foundPluginDefaultKnobsToReorder[j].second = knobs[i];
            }

        }
    }



    assert(foundPluginDefaultKnobsToReorder.size() > 0 && foundPluginDefaultKnobsToReorder[0].first == kOfxMaskInvertParamName);

    createMaskSelectors(hasMaskChannelSelector, inputLabels, mainPage, !foundPluginDefaultKnobsToReorder[0].second.get(), &lastKnobBeforeAdvancedOption);


    //Create the host mix if needed
    if ( _imp->effect->isHostMixingEnabled() ) {
        if (!mainPage) {
            mainPage = getOrCreateMainPage();
        }
        getOrCreateHostMixKnob(mainPage);
    }


    /*
     * Reposition the MaskInvert and Mix parameters declared by the plug-in
     */

    for (std::size_t i = 0; i < foundPluginDefaultKnobsToReorder.size(); ++i) {
        if (foundPluginDefaultKnobsToReorder[i].second) {
            if (!mainPage) {
                mainPage = getOrCreateMainPage();
            }
            if (foundPluginDefaultKnobsToReorder[i].second->getParentKnob() == mainPage) {
                mainPage->removeKnob( foundPluginDefaultKnobsToReorder[i].second);
                mainPage->addKnob(foundPluginDefaultKnobsToReorder[i].second);
            }
        }
    }


    if (lastKnobBeforeAdvancedOption && mainPage) {

        KnobsVec mainPageChildren = mainPage->getChildren();
        int i = 0;
        for (KnobsVec::iterator it = mainPageChildren.begin(); it != mainPageChildren.end(); ++it, ++i) {
            if (*it == lastKnobBeforeAdvancedOption) {
                if (i > 0) {
                    int j = i - 1;
                    KnobsVec::iterator prev = it;
                    --prev;

                    while (j >= 0 && (*prev)->getIsSecret()) {
                        --j;
                        --prev;
                    }

                    if ( j >= 0 && !toKnobSeparator(*prev) ) {
                        KnobSeparatorPtr sep = AppManager::createKnob<KnobSeparator>(_imp->effect, std::string(), 1, false);
                        sep->setName("advancedSep");
                        mainPage->insertKnob(i, sep);
                    }
                }

                break;
            }
        }
    }


    createNodePage(settingsPage);

    NodeGroupPtr isGrpNode = isEffectNodeGroup();
    if (!isGrpNode && !isBackdropNode) {
        createInfoPage();
    } else if (isGrpNode) {
        if (isGrpNode->isSubGraphPersistent()) {
            createPyPlugPage();

            if (hasGUI) {
                createPyPlugExportGroup();
            }

            {
                KnobButtonPtr param = AppManager::createKnob<KnobButton>(_imp->effect, tr(kNatronNodeKnobConvertToGroupButtonLabel), 1, false);
                param->setName(kNatronNodeKnobConvertToGroupButton);
                param->setEvaluateOnChange(false);
                param->setHintToolTip( tr("Converts this node to a Group: the internal node-graph and the user parameters will become editable") );
                settingsPage->addKnob(param);
                _imp->pyPlugConvertToGroupButtonKnob = param;
            }
        }
    }



    if (_imp->effect->isWriter()
        && !ioContainer) {
        //Create a frame step parameter for writers, and control it in OutputSchedulerThread.cpp

        if (!mainPage) {
            mainPage = getOrCreateMainPage();
        }

        KnobButtonPtr renderButton = AppManager::createKnob<KnobButton>(_imp->effect, tr("Render"), 1, false);
        renderButton->setHintToolTip( tr("Starts rendering the specified frame range.") );
        renderButton->setAsRenderButton();
        renderButton->setName("startRender");
        renderButton->setEvaluateOnChange(false);
        _imp->renderButton = renderButton;
        mainPage->addKnob(renderButton);

        createPythonPage();
    }
} // Node::initializeDefaultKnobs

void
Node::initializeKnobs(bool loadingSerialization, bool hasGUI)
{
    ////Only called by the main-thread


    _imp->effect->beginChanges();

    assert( QThread::currentThread() == qApp->thread() );

    InitializeKnobsFlag_RAII __isInitializingKnobsFlag__(getEffectInstance());

    //Initialize plug-in knobs
    _imp->effect->initializeKnobsPublic();

    if ( _imp->effect->getMakeSettingsPanel() ) {
        //initialize default knobs added by Natron
        initializeDefaultKnobs(loadingSerialization, hasGUI);
    }
    
    declarePythonKnobs();
    
    KnobItemsTablePtr table = _imp->effect->getItemsTable();
    if (table) {
        table->declareItemsToPython();
    }
    
    _imp->effect->endChanges();
    
    Q_EMIT knobsInitialized();
} // initializeKnobs



void
Node::onFileNameParameterChanged(const KnobIPtr& fileKnob)
{
    if ( _imp->effect->isReader() ) {
        computeFrameRangeForReader(fileKnob, true);

        ///Refresh the preview automatically if the filename changed

        ///union the project frame range if not locked with the reader frame range
        bool isLocked = getApp()->getProject()->isFrameRangeLocked();
        if (!isLocked) {

            TimeValue left(INT_MIN), right(INT_MAX);
            {
                GetFrameRangeResultsPtr actionResults;
                ActionRetCodeEnum stat = _imp->effect->getFrameRange_public(TreeRenderNodeArgsPtr(), &actionResults);
                if (!isFailureRetCode(stat)) {
                    RangeD range;
                    actionResults->getFrameRangeResults(&range);
                    left = TimeValue(range.min);
                    right = TimeValue(range.max);
                }
            }

            if ( (left != INT_MIN) && (right != INT_MAX) ) {
                if ( getGroup() || getIOContainer() ) {
                    getApp()->getProject()->unionFrameRangeWith(left, right);
                }
            }
        }
    } else if ( _imp->effect->isWriter() ) {
        KnobFilePtr isFile = toKnobFile(fileKnob);
        KnobStringPtr sublabel = _imp->ofxSubLabelKnob.lock();
        if (isFile && sublabel) {

            std::string pattern = isFile->getValue();
            if (sublabel) {
                std::size_t foundSlash = pattern.find_last_of("/");
                if (foundSlash != std::string::npos) {
                    pattern = pattern.substr(foundSlash + 1);
                }

                sublabel->setValue(pattern);
            }
        }

        /*
         Check if the filename param has a %V in it, in which case make sure to hide the Views parameter
         */
        if (isFile) {
            std::string pattern = isFile->getValue();
            std::size_t foundViewPattern = pattern.find_first_of("%v");
            if (foundViewPattern == std::string::npos) {
                foundViewPattern = pattern.find_first_of("%V");
            }
            if (foundViewPattern != std::string::npos) {
                //We found view pattern
                KnobIPtr viewsKnob = getKnobByName(kWriteOIIOParamViewsSelector);
                if (viewsKnob) {
                    KnobChoicePtr viewsSelector = toKnobChoice(viewsKnob);
                    if (viewsSelector) {
                        viewsSelector->setSecret(true);
                    }
                }
            }
        }
    }
} // Node::onFileNameParameterChanged

/*
 This is called AFTER the instanceChanged action has been called on the plug-in
 */
bool
Node::onEffectKnobValueChanged(const KnobIPtr& what,
                               ValueChangedReasonEnum reason)
{
    if (!what) {
        return false;
    }
    for (std::map<int, MaskSelector >::iterator it = _imp->maskSelectors.begin(); it != _imp->maskSelectors.end(); ++it) {
        if (it->second.channel.lock() == what) {
            _imp->onMaskSelectorChanged(it->first, it->second);
            break;
        }
    }

    bool ret = true;
    if ( what == _imp->previewEnabledKnob.lock() ) {
        if (reason == eValueChangedReasonUserEdited ) {
            Q_EMIT previewKnobToggled();
        }
    } else if ( what == _imp->renderButton.lock() ) {
        if ( getEffectInstance()->isWriter() ) {
            RenderQueue::RenderWork w;
            w.treeRoot = shared_from_this();
            w.useRenderStats = getApp()->isRenderStatsActionChecked();
            std::list<RenderQueue::RenderWork> works;
            works.push_back(w);
            getApp()->getRenderQueue()->renderNonBlocking(works);
        }
    } else if (what == _imp->disableNodeKnob.lock()) {
        Q_EMIT disabledKnobToggled( _imp->disableNodeKnob.lock()->getValue() );
    } else if ( what == _imp->nodeLabelKnob.lock() || what == _imp->ofxSubLabelKnob.lock() ) {
        Q_EMIT nodeExtraLabelChanged();
    } else if ( what == _imp->hideInputs.lock() ) {
        Q_EMIT hideInputsKnobChanged( _imp->hideInputs.lock()->getValue() );
    } else if ( _imp->effect->isReader() && (what->getName() == kReadOIIOAvailableViewsKnobName) ) {
        refreshCreatedViews(what);
    } else if ( what == _imp->refreshInfoButton.lock() ||
               (what == _imp->infoPage.lock() && reason == eValueChangedReasonUserEdited) ) {
        refreshInfos();
    } else if ( what == _imp->openglRenderingEnabledKnob.lock() ) {
        bool enabled = true;
        int thisKnobIndex = _imp->openglRenderingEnabledKnob.lock()->getValue();
        if (thisKnobIndex == 1 || (thisKnobIndex == 2 && getApp()->isBackground())) {
            enabled = false;
        }
        if (enabled) {
            // Check value on project now
            if (!getApp()->getProject()->isOpenGLRenderActivated()) {
                enabled = false;
            }
        }
        _imp->effect->onEnableOpenGLKnobValueChanged(enabled);
    } else if (what == _imp->processAllLayersKnob.lock()) {

        std::map<int, ChannelSelector>::iterator foundOutput = _imp->channelsSelectors.find(-1);
        if (foundOutput != _imp->channelsSelectors.end()) {
            _imp->onLayerChanged(true);
        }
    } else if (what == _imp->pyPlugExportButtonKnob.lock() && reason != eValueChangedReasonRestoreDefault) {
        // Trigger a knob changed action on the group
        KnobGroupPtr k = _imp->pyPlugExportDialog.lock();
        if (k) {
            k->setValue(!k->getValue());
        }
    } else if (what == _imp->pyPlugConvertToGroupButtonKnob.lock() && reason != eValueChangedReasonRestoreDefault) {
        NodeGroupPtr isGroup = isEffectNodeGroup();
        if (isGroup) {
            isGroup->setSubGraphEditedByUser(true);
        }
    } else if (what == _imp->pyPlugExportDialogOkButton.lock() && reason == eValueChangedReasonUserEdited) {
        try {
            exportNodeToPyPlug(_imp->pyPlugExportDialogFile.lock()->getValue());
        } catch (const std::exception& e) {
            Dialogs::errorDialog(tr("Export").toStdString(), e.what());
        }
        _imp->pyPlugExportDialog.lock()->setValue(false);
    } else if (what == _imp->pyPlugExportDialogCancelButton.lock()) {
        _imp->pyPlugExportDialog.lock()->setValue(false);
    } else if (what == _imp->keepInAnimationModuleKnob.lock()) {
        Q_EMIT keepInAnimationModuleKnobChanged();
    } else {
        ret = false;
    }

    if (!ret) {
        KnobGroupPtr isGroup = toKnobGroup(what);
        if ( isGroup && isGroup->getIsDialog() && reason == eValueChangedReasonUserEdited) {
            NodeGuiIPtr gui = getNodeGui();
            if (gui) {
                gui->showGroupKnobAsDialog(isGroup);
                ret = true;
            }
        }
    }

    if (!ret) {
        for (std::map<int, ChannelSelector>::iterator it = _imp->channelsSelectors.begin(); it != _imp->channelsSelectors.end(); ++it) {
            if (it->second.layer.lock() == what) {
                _imp->onLayerChanged(it->first == -1);
                ret = true;
                break;
            }
        }
    }

    if (!ret) {
        for (int i = 0; i < 4; ++i) {
            KnobBoolPtr enabled = _imp->enabledChan[i].lock();
            if (!enabled) {
                break;
            }
            if (enabled == what) {
                checkForPremultWarningAndCheckboxes();
                ret = true;
                Q_EMIT enabledChannelCheckboxChanged();
                break;
            }
        }
    }

    if (!ret) {
        GroupInputPtr isInput = isEffectGroupInput();
        if (isInput) {
            if ( (what->getName() == kNatronGroupInputIsOptionalParamName)
                || ( what->getName() == kNatronGroupInputIsMaskParamName) ) {
                NodeCollectionPtr col = isInput->getNode()->getGroup();
                assert(col);
                NodeGroupPtr isGrp = toNodeGroup(col);
                assert(isGrp);
                if (isGrp) {
                    ///Refresh input arrows of the node to reflect the state
                    isGrp->getNode()->initializeInputs();
                    ret = true;
                }
            }
        }
    }
    
    return ret;
} // Node::onEffectKnobValueChanged


NATRON_NAMESPACE_EXIT;
