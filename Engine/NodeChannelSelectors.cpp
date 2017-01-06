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

NATRON_NAMESPACE_ENTER;

static std::string generateChannelID(const std::string& channelName)
{
    if (boost::iequals(channelName, "a") || boost::iequals(channelName, "alpha")) {
        return "a";
    } else if (boost::iequals(channelName, "r") || boost::iequals(channelName, "red")) {
        return "r";
    } else if (boost::iequals(channelName, "g") || boost::iequals(channelName, "green")) {
        return "g";
    } else if (boost::iequals(channelName, "b") || boost::iequals(channelName, "blue")) {
        return "b";
    } else {
        return channelName;
    }
}

static ChoiceOption makeChannelOption(const std::string& layerName, const std::string& channelName)
{
    std::string optionID, optionLabel;
    optionID += layerName;
    if ( !layerName.empty() ) {
        optionID += '.';
    }
    optionLabel = optionID;

    // For the option label, append the name of the channel
    optionLabel += channelName;

    // For the option ID, if this is the alpha channel we need to do something different:
    // The Color.RGBA and Color.Alpha planes both have alpha. To overcome this
    optionID += generateChannelID(channelName);
    return ChoiceOption(optionID, optionLabel, "");
}

bool
Node::refreshChannelSelectors()
{
    if ( !isNodeCreated() ) {
        return false;
    }

    TimeValue time = TimeValue(getApp()->getTimeLine()->currentFrame());

    // Refresh each layer selector (input and output)
    bool hasChanged = false;
    for (std::map<int, ChannelSelector>::iterator it = _imp->channelsSelectors.begin(); it != _imp->channelsSelectors.end(); ++it) {

        int inputNb = it->first;


        // The Output Layer menu has a All choice, input layers menus have a None choice.
        std::vector<ChoiceOption> choices;
        if (inputNb >= 0) {
            choices.push_back(ChoiceOption("None", "", ""));
        }


        std::list<ImageComponents> availableComponents;
        {
            ActionRetCodeEnum stat = _imp->effect->getAvailableLayers(time, ViewIdx(0), inputNb, TreeRenderNodeArgsPtr(), &availableComponents);
            (void)stat;
        }

        for (std::list<ImageComponents>::const_iterator it2 = availableComponents.begin(); it2 != availableComponents.end(); ++it2) {
            std::string optionLabel = it2->getLayerName() + "." + it2->getComponentsGlobalName();

            // The option ID is always the name of the layer, this ensures for the Color plane that even if the components type changes, the choice stays
            // the same in the parameter.
            const std::string& optionID = it2->getLayerName();
            choices.push_back(ChoiceOption(optionID, optionLabel, ""));
        }

        {
            KnobChoicePtr layerKnob = it->second.layer.lock();

            bool menuChanged = layerKnob->populateChoices(choices);
            if (menuChanged) {
                hasChanged = true;
                if (inputNb == -1) {
                    s_outputLayerChanged();
                }
            }
        }
    }  // for each layer selector

    // Refresh each mask channel selector

    for (std::map<int, MaskSelector>::iterator it = _imp->maskSelectors.begin(); it != _imp->maskSelectors.end(); ++it) {

        int inputNb = it->first;
        std::vector<ChoiceOption> choices;
        choices.push_back(ChoiceOption("None", "",""));

        // Get the mask input components
        std::list<ImageComponents> availableComponents;
        {
            ActionRetCodeEnum stat = _imp->effect->getAvailableLayers(time, ViewIdx(0), inputNb, TreeRenderNodeArgsPtr(), &availableComponents);
            (void)stat;
        }

        for (std::list<ImageComponents>::const_iterator it2 = availableComponents.begin(); it2 != availableComponents.end(); ++it2) {

            const std::string& layerName = it2->getLayerName();
            const std::vector<std::string>& channels = it2->getComponentsNames();
            for (std::size_t c = 0; c < channels.size(); ++c) {
                choices.push_back(makeChannelOption(layerName, channels[c]));
            }
        }


        KnobChoicePtr channelKnob = it->second.channel.lock();

        hasChanged |= channelKnob->populateChoices(choices);

    }
    return hasChanged;
} // refreshChannelSelectors


bool
Node::getSelectedLayer(int inputNb,
                       std::bitset<4> *processChannels,
                       bool* isAll,
                       ImageComponents* layer) const
{


    // If there's a mask channel selector, fetch the mask layer
    int chanIndex = getMaskChannel(inputNb, layer);
    if (chanIndex != -1) {
        *isAll = false;
        Q_UNUSED(chanIndex);
        if (processChannels) {
            (*processChannels)[0] = true;
            (*processChannels)[1] = true;
            (*processChannels)[2] = true;
            (*processChannels)[3] = true;
        }

        return true;
    }


    std::map<int, ChannelSelector>::const_iterator foundSelector = _imp->channelsSelectors.find(inputNb);

    if ( foundSelector == _imp->channelsSelectors.end() ) {
        // Fetch in input what the user has set for the output
        foundSelector = _imp->channelsSelectors.find(-1);
    }


    // Check if the checkbox "All layers" is checked or not
    KnobBoolPtr processAllKnob = _imp->processAllLayersKnob.lock();
    *isAll = false;
    if (processAllKnob) {
        *isAll = processAllKnob->getValue();
    }

    if (!*isAll && foundSelector != _imp->channelsSelectors.end()) {
        *layer = _imp->getSelectedLayerInternal(inputNb, foundSelector->second);
    }

    if (processChannels) {
        if (_imp->hostChannelSelectorEnabled &&  _imp->enabledChan[0].lock() ) {
            (*processChannels)[0] = _imp->enabledChan[0].lock()->getValue();
            (*processChannels)[1] = _imp->enabledChan[1].lock()->getValue();
            (*processChannels)[2] = _imp->enabledChan[2].lock()->getValue();
            (*processChannels)[3] = _imp->enabledChan[3].lock()->getValue();
        } else {
            (*processChannels)[0] = true;
            (*processChannels)[1] = true;
            (*processChannels)[2] = true;
            (*processChannels)[3] = true;
        }
    }
    return foundSelector != _imp->channelsSelectors.end();
} // getSelectedLayer

ImageComponents
NodePrivate::getSelectedLayerInternal(int inputNb, const ChannelSelector& selector) const
{
    NodePtr node;

    assert(_publicInterface);
    if (!_publicInterface) {
        return ImageComponents();
    }
    if (inputNb == -1) {
        node = _publicInterface->shared_from_this();
    } else {
        node = _publicInterface->getInput(inputNb);
    }

    KnobChoicePtr layerKnob = selector.layer.lock();
    if (!layerKnob) {
        return ImageComponents();
    }
    std::string layerID = layerKnob->getActiveEntryID();

    // Get the mask input components
    std::list<ImageComponents> availableComponents;
    {
        ActionRetCodeEnum stat = effect->getAvailableLayers(effect->getCurrentTime_TLS(), ViewIdx(0), inputNb, effect->getCurrentRender_TLS(), &availableComponents);
        (void)stat;
    }

    for (std::list<ImageComponents>::const_iterator it2 = availableComponents.begin(); it2 != availableComponents.end(); ++it2) {

        const std::string& layerName = it2->getLayerName();
        if (layerID == layerName) {
            return *it2;
        }
    }
    return ImageComponents();
} // getSelectedLayerInternal

int
Node::getMaskChannel(int inputNb, ImageComponents* comps) const
{
    *comps = ImageComponents::getNoneComponents();
    
    std::map<int, MaskSelector >::const_iterator it = _imp->maskSelectors.find(inputNb);

    if ( it == _imp->maskSelectors.end() ) {
        return -1;
    }
    std::string maskChannelID =  it->second.channel.lock()->getActiveEntryID();

    // Get the mask input components
    std::list<ImageComponents> availableComponents;
    {
        ActionRetCodeEnum stat = _imp->effect->getAvailableLayers(getEffectInstance()->getCurrentTime_TLS(), ViewIdx(0), inputNb, getEffectInstance()->getCurrentRender_TLS(), &availableComponents);
        (void)stat;
    }

    for (std::list<ImageComponents>::const_iterator it2 = availableComponents.begin(); it2 != availableComponents.end(); ++it2) {

        const std::string& layerName = it2->getLayerName();
        const std::vector<std::string>& channels = it2->getComponentsNames();
        for (std::size_t c = 0; c < channels.size(); ++c) {
            ChoiceOption channelOption = makeChannelOption(layerName, channels[c]);
            if (channelOption.id == maskChannelID) {
                *comps = *it2;
                return c;
            }
        }
    }
    return -1;

} // getMaskChannel

bool
Node::addUserComponents(const ImageComponents& comps)
{
    ///The node has node channel selector, don't allow adding a custom plane.
    KnobIPtr outputLayerKnob = getKnobByName(kNatronOfxParamOutputChannels);

    if (_imp->channelsSelectors.empty() && !outputLayerKnob) {
        return false;
    }

    if (!outputLayerKnob) {
        //The effect does not have kNatronOfxParamOutputChannels but maybe the selector provided by Natron
        std::map<int, ChannelSelector>::iterator found = _imp->channelsSelectors.find(-1);
        if ( found == _imp->channelsSelectors.end() ) {
            return false;
        }
        outputLayerKnob = found->second.layer.lock();
    }

    {
        QMutexLocker k(&_imp->createdComponentsMutex);
        for (std::list<ImageComponents>::iterator it = _imp->createdComponents.begin(); it != _imp->createdComponents.end(); ++it) {
            if ( it->getLayerName() == comps.getLayerName() ) {
                return false;
            }
        }

        _imp->createdComponents.push_back(comps);
    }

    {
        ///Set the selector to the new channel
        KnobChoicePtr layerChoice = toKnobChoice(outputLayerKnob);
        if (layerChoice) {
            layerChoice->setValueFromLabel(comps.getLayerName());
        }
    }

    return true;
} // addUserComponents

void
Node::getUserCreatedComponents(std::list<ImageComponents>* comps)
{
    QMutexLocker k(&_imp->createdComponentsMutex);
    
    *comps = _imp->createdComponents;
}


KnobChoicePtr
Node::getLayerChoiceKnob(int inputNb) const
{
    std::map<int, ChannelSelector>::iterator found = _imp->channelsSelectors.find(inputNb);

    if ( found == _imp->channelsSelectors.end() ) {
        return KnobChoicePtr();
    }
    return found->second.layer.lock();
}

void
Node::refreshLayersSelectorsVisibility()
{

    KnobBoolPtr processAllKnob = _imp->processAllLayersKnob.lock();
    if (!processAllKnob) {
        return;
    }
    bool outputIsAll = processAllKnob->getValue();

    // Disable all input selectors as it doesn't make sense to edit them whilst output is All

    ImageComponents mainInputComps, outputComps;

    int mainInputIndex = getPreferredInput();

    for (std::map<int, ChannelSelector>::iterator it = _imp->channelsSelectors.begin(); it != _imp->channelsSelectors.end(); ++it) {
        if (it->first >= 0) {
            NodePtr inp = getInput(it->first);
            bool mustBeSecret = !inp.get() || outputIsAll;
            KnobChoicePtr layerKnob = it->second.layer.lock();
            layerKnob->setSecret(mustBeSecret);

            if (mainInputIndex != -1 && mainInputIndex == it->first) {
                // This is the main-input
                mainInputComps = _imp->getSelectedLayerInternal(it->first, it->second);
            }

        } else {
            it->second.layer.lock()->setSecret(outputIsAll);
            outputComps = _imp->getSelectedLayerInternal(it->first, it->second);
        }
    }


    // Refresh RGBA checkbox visibility
    KnobBoolPtr enabledChan[4];
    bool hasRGBACheckboxes = false;
    for (int i = 0; i < 4; ++i) {
        enabledChan[i] = _imp->enabledChan[i].lock();
        hasRGBACheckboxes |= (bool)enabledChan[i];
    }
    if (hasRGBACheckboxes) {
        if (outputIsAll) {
            for (int i = 0; i < 4; ++i) {
                enabledChan[i]->setSecret(true);
            }
        } else {
            refreshEnabledKnobsLabel(mainInputComps, outputComps);
        }
    }
}


void
Node::refreshEnabledKnobsLabel(const ImageComponents& mainInputComps, const ImageComponents& outputComps)
{
    KnobBoolPtr enabledChan[4];
    bool hasRGBACheckboxes = false;
    for (int i = 0; i < 4; ++i) {
        enabledChan[i] = _imp->enabledChan[i].lock();
        hasRGBACheckboxes |= (bool)enabledChan[i];
    }
    if (!hasRGBACheckboxes) {
        return;
    }

    // We display the number of channels that the output layer can have
    int nOutputComps = outputComps.getNumComponents();

    // But we name the channels by the name of the input layer
    const std::vector<std::string>& inputChannelNames = mainInputComps.getComponentsNames();
    const std::vector<std::string>& outputChannelNames = outputComps.getComponentsNames();

    switch (nOutputComps) {
        case 1: {
            for (int i = 0; i < 3; ++i) {
                enabledChan[i]->setSecret(true);
            }
            enabledChan[3]->setSecret(false);
            std::string channelName;
            if (inputChannelNames.size() == 1) {
                channelName = inputChannelNames[0];
            } else if (inputChannelNames.size() == 4) {
                channelName = inputChannelNames[3];
            } else {
                channelName = outputChannelNames[0];
            }
            enabledChan[3]->setLabel(channelName);

            break;
        }
        case 2: {
            for (int i = 2; i < 4; ++i) {
                enabledChan[i]->setSecret(true);
            }
            for (int i = 0; i < 2; ++i) {
                enabledChan[i]->setSecret(false);
                if ((int)inputChannelNames.size() > i) {
                    enabledChan[i]->setLabel(inputChannelNames[i]);
                } else {
                    enabledChan[i]->setLabel(outputChannelNames[i]);
                }

            }
            break;
        }
        case 3: {
            for (int i = 3; i < 4; ++i) {
                enabledChan[i]->setSecret(true);
            }
            for (int i = 0; i < 3; ++i) {
                enabledChan[i]->setSecret(false);
                if ((int)inputChannelNames.size() > i) {
                    enabledChan[i]->setLabel(inputChannelNames[i]);
                } else {
                    enabledChan[i]->setLabel(outputChannelNames[i]);
                }
            }
            break;
        }
        case 4: {
            for (int i = 0; i < 4; ++i) {
                enabledChan[i]->setSecret(false);
                if ((int)inputChannelNames.size() > i) {
                    enabledChan[i]->setLabel(inputChannelNames[i]);
                } else {
                    enabledChan[i]->setLabel(outputChannelNames[i]);
                }
            }
            break;
        }

        case 0:
        default: {
            for (int i = 0; i < 4; ++i) {
                enabledChan[i]->setSecret(true);
            }
            break;
        }
    } // switch
} // Node::refreshEnabledKnobsLabel



bool
Node::isPluginUsingHostChannelSelectors() const
{
    return _imp->hostChannelSelectorEnabled;
}

bool
Node::getProcessChannel(int channelIndex) const
{
    if (!isPluginUsingHostChannelSelectors()) {
        return true;
    }
    assert(channelIndex >= 0 && channelIndex < 4);
    KnobBoolPtr k = _imp->enabledChan[channelIndex].lock();
    if (k) {
        return k->getValue();
    }

    return true;
}

KnobBoolPtr
Node::getProcessChannelKnob(int channelIndex) const
{
    assert(channelIndex >= 0 && channelIndex < 4);
    return _imp->enabledChan[channelIndex].lock();
}



bool
Node::hasAtLeastOneChannelToProcess() const
{
    std::map<int, ChannelSelector>::const_iterator foundSelector = _imp->channelsSelectors.find(-1);

    if ( foundSelector == _imp->channelsSelectors.end() ) {
        return true;
    }
    if ( _imp->enabledChan[0].lock() ) {
        std::bitset<4> processChannels;
        processChannels[0] = _imp->enabledChan[0].lock()->getValue();
        processChannels[1] = _imp->enabledChan[1].lock()->getValue();
        processChannels[2] = _imp->enabledChan[2].lock()->getValue();
        processChannels[3] = _imp->enabledChan[3].lock()->getValue();
        if (!processChannels[0] && !processChannels[1] && !processChannels[2] && !processChannels[3]) {
            return false;
        }
    }
    
    return true;
}



bool
Node::isSupportedComponent(int inputNb,
                           const ImageComponents& comp) const
{

    std::bitset<4> supported;
    {
        QMutexLocker l(&_imp->inputsMutex);

        if (inputNb >= 0) {
            assert( inputNb < (int)_imp->inputsComponents.size() );
            supported = _imp->inputsComponents[inputNb];
        } else {
            assert(inputNb == -1);
            supported = _imp->outputComponents;
        }
    }
    assert(comp.getNumComponents() <= 4);
    return supported[comp.getNumComponents()];
}


ImageComponents
Node::findClosestSupportedNumberOfComponents(int inputNb,
                                             int nComps) const
{
    if (nComps < 0 || nComps >= 4) {
        // Natron assumes that a layer must have between 1 and 4 channels.
        return ImageComponents::getNoneComponents();
    }
    std::bitset<4> supported;
    {
        QMutexLocker l(&_imp->inputsMutex);

        if (inputNb >= 0) {
            assert( inputNb < (int)_imp->inputsComponents.size() );
            supported = _imp->inputsComponents[inputNb];
        } else {
            assert(inputNb == -1);
            supported = _imp->outputComponents;
        }
    }

    // Find a greater or equal number of components
    int foundSupportedNComps = -1;
    for (int i = nComps; i < 4; ++i) {
        if (supported[i]) {
            foundSupportedNComps = i;
            break;
        }
    }


    if (foundSupportedNComps == -1) {
        // Find a small number of components
        for (int i = nComps - 1; i >= 0; --i) {
            if (supported[i]) {
                foundSupportedNComps = i;
                break;
            }
        }
    }

    if (foundSupportedNComps == -1) {
        return ImageComponents::getNoneComponents();
    }
    switch (foundSupportedNComps) {
        case 1:
            return ImageComponents::getAlphaComponents();
        case 2:
            return ImageComponents::getXYComponents();
        case 3:
            return ImageComponents::getRGBComponents();
        case 4:
            return ImageComponents::getRGBAComponents();
        default:
            assert(false);
            break;
    }
    return ImageComponents::getNoneComponents();
} // findClosestSupportedNumberOfComponents

int
Node::isMaskChannelKnob(const KnobIConstPtr& knob) const
{
    for (std::map<int, MaskSelector >::const_iterator it = _imp->maskSelectors.begin(); it != _imp->maskSelectors.end(); ++it) {
        if (it->second.channel.lock() == knob) {
            return it->first;
        }
    }

    return -1;
}

bool
Node::isMaskEnabled(int inputNb) const
{
    std::map<int, MaskSelector >::const_iterator it = _imp->maskSelectors.find(inputNb);

    if ( it != _imp->maskSelectors.end() ) {
        return it->second.enabled.lock()->getValue();
    } else {
        return true;
    }
}



void
Node::findOrCreateChannelEnabled()
{
    //Try to find R,G,B,A parameters on the plug-in, if found, use them, otherwise create them
    static const std::string channelLabels[4] = {kNatronOfxParamProcessRLabel, kNatronOfxParamProcessGLabel, kNatronOfxParamProcessBLabel, kNatronOfxParamProcessALabel};
    static const std::string channelNames[4] = {kNatronOfxParamProcessR, kNatronOfxParamProcessG, kNatronOfxParamProcessB, kNatronOfxParamProcessA};
    static const std::string channelHints[4] = {kNatronOfxParamProcessRHint, kNatronOfxParamProcessGHint, kNatronOfxParamProcessBHint, kNatronOfxParamProcessAHint};
    KnobBoolPtr foundEnabled[4];
    const KnobsVec & knobs = _imp->effect->getKnobs();

    for (int i = 0; i < 4; ++i) {
        KnobBoolPtr enabled;
        for (std::size_t j = 0; j < knobs.size(); ++j) {
            if (knobs[j]->getOriginalName() == channelNames[i]) {
                foundEnabled[i] = toKnobBool(knobs[j]);
                break;
            }
        }
    }

    bool foundAll = foundEnabled[0] && foundEnabled[1] && foundEnabled[2] && foundEnabled[3];
    bool isWriter = _imp->effect->isWriter();

    KnobPagePtr mainPage;
    if (foundAll) {
        for (int i = 0; i < 4; ++i) {
            // Writers already have their checkboxes places correctly
            if (!isWriter) {
                if (!mainPage) {
                    mainPage = getOrCreateMainPage();
                }
                if (foundEnabled[i]->getParentKnob() == mainPage) {
                    //foundEnabled[i]->setAddNewLine(i == 3);
                    mainPage->removeKnob(foundEnabled[i]);
                    mainPage->insertKnob(i, foundEnabled[i]);
                }
            }
            _imp->enabledChan[i] = foundEnabled[i];
        }
    }

    bool pluginDefaultPref[4];
    _imp->hostChannelSelectorEnabled = _imp->effect->isHostChannelSelectorSupported(&pluginDefaultPref[0], &pluginDefaultPref[1], &pluginDefaultPref[2], &pluginDefaultPref[3]);


    if (_imp->hostChannelSelectorEnabled) {
        if (foundAll) {
            std::cerr << getScriptName_mt_safe() << ": WARNING: property " << kNatronOfxImageEffectPropChannelSelector << " is different of " << kOfxImageComponentNone << " but uses its own checkboxes" << std::endl;
        } else {
            if (!mainPage) {
                mainPage = getOrCreateMainPage();
            }

            //Create the selectors
            for (int i = 0; i < 4; ++i) {
                foundEnabled[i] =  AppManager::createKnob<KnobBool>(_imp->effect, channelLabels[i], 1, false);
                foundEnabled[i]->setName(channelNames[i]);
                foundEnabled[i]->setAnimationEnabled(false);
                foundEnabled[i]->setAddNewLine(i == 3);
                foundEnabled[i]->setDefaultValue(pluginDefaultPref[i]);
                foundEnabled[i]->setHintToolTip(channelHints[i]);
                mainPage->insertKnob(i, foundEnabled[i]);
                _imp->enabledChan[i] = foundEnabled[i];
            }
            foundAll = true;
        }
    }
    if ( !isWriter && foundAll && !getApp()->isBackground() ) {
        if (!mainPage) {
            mainPage = getOrCreateMainPage();
        }
        _imp->enabledChan[3].lock()->setAddNewLine(false);
        KnobStringPtr premultWarning = AppManager::createKnob<KnobString>(_imp->effect, std::string(), 1, false);
        premultWarning->setName("premultWarningKnob");
        premultWarning->setIconLabel("dialog-warning");
        premultWarning->setSecret(true);
        premultWarning->setAsLabel();
        premultWarning->setEvaluateOnChange(false);
        premultWarning->setIsPersistent(false);
        premultWarning->setHintToolTip( tr("The alpha checkbox is checked and the RGB "
                                           "channels in output are alpha-premultiplied. Any of the unchecked RGB channel "
                                           "may be incorrect because the alpha channel changed but their value did not. "
                                           "To fix this, either check all RGB channels (or uncheck alpha) or unpremultiply the "
                                           "input image first.").toStdString() );
        mainPage->insertKnob(4, premultWarning);
        _imp->premultWarning = premultWarning;
    }
} // Node::findOrCreateChannelEnabled

void
Node::createChannelSelectors(const std::vector<std::pair<bool, bool> >& hasMaskChannelSelector,
                             const std::vector<std::string>& inputLabels,
                             const KnobPagePtr& mainPage,
                             KnobIPtr* lastKnobBeforeAdvancedOption)
{
    ///Create input layer selectors
    for (std::size_t i = 0; i < inputLabels.size(); ++i) {
        if (!hasMaskChannelSelector[i].first) {
            _imp->createChannelSelector(i, inputLabels[i], false, mainPage, lastKnobBeforeAdvancedOption);
        }
    }
    ///Create output layer selectors
    _imp->createChannelSelector(-1, "Output", true, mainPage, lastKnobBeforeAdvancedOption);
}


void
NodePrivate::onLayerChanged(bool isOutput)
{
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

    if ( (index == 0) && enabled->isEnabled() ) {
        enabled->setValue(false);
        enabled->setEnabled(false);
    } else if ( !enabled->isEnabled() ) {
        enabled->setEnabled(true);
        if ( _publicInterface->getInput(inputNb) ) {
            enabled->setValue(true);
        }
    }

}


bool
Node::refreshMaskEnabledNess(int inputNb)
{
    std::map<int, MaskSelector>::iterator found = _imp->maskSelectors.find(inputNb);
    NodePtr inp = getInput(inputNb);
    bool changed = false;

    if ( found != _imp->maskSelectors.end() ) {
        KnobBoolPtr enabled = found->second.enabled.lock();
        assert(enabled);
        bool curValue = enabled->getValue();
        bool newValue = inp ? true : false;
        changed = curValue != newValue;
        if (changed) {
            enabled->setValue(newValue);
        }
    }

    return changed;
}

NATRON_NAMESPACE_EXIT;