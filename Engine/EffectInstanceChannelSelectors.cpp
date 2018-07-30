/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#include "EffectInstancePrivate.h"
#include "Engine/AppInstance.h"
#include "Engine/Node.h"
#include "Engine/TimeLine.h"
#include "Engine/KnobTypes.h"

NATRON_NAMESPACE_ENTER

struct ChoiceOptionCompareLess
{
    bool operator() (const ChoiceOption& lhs, const ChoiceOption& rhs) const
    {
        bool lIsColor = lhs.id == kNatronColorPlaneID;
        bool rIsColor = rhs.id == kNatronColorPlaneID;
        if (lIsColor && rIsColor) {
            return false;
        } else if (lIsColor) {
            return true;
        } else if (rIsColor) {
            return false;
        }
        return lhs.id < rhs.id;
    }
};

bool
EffectInstance::refreshChannelSelectors()
{

    TimeValue time = TimeValue(getApp()->getTimeLine()->currentFrame());

    // Refresh each layer selector (input and output)
    bool hasChanged = false;
    for (std::map<int, ChannelSelector>::iterator it = _imp->defKnobs->channelsSelectors.begin(); it != _imp->defKnobs->channelsSelectors.end(); ++it) {

        int inputNb = it->first;


        // The Output Layer menu has a All choice, input layers menus have a None choice.
        std::vector<ChoiceOption> choices;

        std::list<ImagePlaneDesc> availableComponents;
        {
            ActionRetCodeEnum stat = getAvailableLayers(time, ViewIdx(0), inputNb, &availableComponents);
            (void)stat;
        }

        for (std::list<ImagePlaneDesc>::const_iterator it2 = availableComponents.begin(); it2 != availableComponents.end(); ++it2) {
            ChoiceOption layerOption = it2->getPlaneOption();
            choices.push_back(layerOption);
        }


        std::sort(choices.begin(), choices.end(), ChoiceOptionCompareLess());

        if (inputNb >= 0) {
            choices.insert(choices.begin(), ChoiceOption("None", "", ""));
        }


        {
            KnobChoicePtr layerKnob = it->second.layer.lock();

            bool menuChanged = layerKnob->populateChoices(choices);
            if (menuChanged) {
                hasChanged = true;
                if (inputNb == -1) {
                    getNode()->s_outputLayerChanged();
                }
            }
        }
    }  // for each layer selector

    // Refresh each mask channel selector

    for (std::map<int, MaskSelector>::iterator it = _imp->defKnobs->maskSelectors.begin(); it != _imp->defKnobs->maskSelectors.end(); ++it) {

        int inputNb = it->first;
        std::vector<ChoiceOption> choices;
        choices.push_back(ChoiceOption("None", "",""));

        // Get the mask input components
        std::list<ImagePlaneDesc> availableComponents;
        {
            ActionRetCodeEnum stat = getAvailableLayers(time, ViewIdx(0), inputNb, &availableComponents);
            (void)stat;
        }

        for (std::list<ImagePlaneDesc>::const_iterator it2 = availableComponents.begin(); it2 != availableComponents.end(); ++it2) {

            std::size_t nChans = (std::size_t)it2->getNumComponents();
            for (std::size_t c = 0; c < nChans; ++c) {
                choices.push_back(it2->getChannelOption(c));
            }
        }


        KnobChoicePtr channelKnob = it->second.channel.lock();

        hasChanged |= channelKnob->populateChoices(choices);

    }
    return hasChanged;
} // refreshChannelSelectors


bool
EffectInstance::getSelectedLayer(int inputNb,
                       const std::list<ImagePlaneDesc>& availableLayers,
                       std::bitset<4> *processChannels,
                       bool* isAll,
                       ImagePlaneDesc* layer) const
{


    // If there's a mask channel selector, fetch the mask layer
    int chanIndex = getMaskChannel(inputNb, availableLayers, layer);
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


    std::map<int, ChannelSelector>::const_iterator foundSelector = _imp->defKnobs->channelsSelectors.find(inputNb);

    if ( foundSelector == _imp->defKnobs->channelsSelectors.end() ) {
        // Fetch in input what the user has set for the output
        foundSelector = _imp->defKnobs->channelsSelectors.find(-1);
    }


    // Check if the checkbox "All layers" is checked or not
    KnobBoolPtr processAllKnob = _imp->defKnobs->processAllLayersKnob.lock();
    *isAll = false;
    if (processAllKnob) {
        *isAll = processAllKnob->getValue();
    }

    if (!*isAll && foundSelector != _imp->defKnobs->channelsSelectors.end()) {
        *layer = _imp->getSelectedLayerInternal(availableLayers, foundSelector->second);
    }

    if (processChannels) {
        if (isHostChannelSelectorEnabled() &&  _imp->defKnobs->enabledChan[0].lock() ) {
            (*processChannels)[0] = _imp->defKnobs->enabledChan[0].lock()->getValue();
            (*processChannels)[1] = _imp->defKnobs->enabledChan[1].lock()->getValue();
            (*processChannels)[2] = _imp->defKnobs->enabledChan[2].lock()->getValue();
            (*processChannels)[3] = _imp->defKnobs->enabledChan[3].lock()->getValue();
        } else {
            (*processChannels)[0] = true;
            (*processChannels)[1] = true;
            (*processChannels)[2] = true;
            (*processChannels)[3] = true;
        }
    }
    return foundSelector != _imp->defKnobs->channelsSelectors.end();
} // getSelectedLayer

ImagePlaneDesc
EffectInstance::Implementation::getSelectedLayerInternal(const std::list<ImagePlaneDesc>& availableLayers,
                                                         const ChannelSelector& selector) const
{
    
    assert(_publicInterface);
    if (!_publicInterface) {
        return ImagePlaneDesc();
    }

    KnobChoicePtr layerKnob = selector.layer.lock();
    if (!layerKnob) {
        return ImagePlaneDesc();
    }
    ChoiceOption layerID = layerKnob->getCurrentEntry();

    for (std::list<ImagePlaneDesc>::const_iterator it2 = availableLayers.begin(); it2 != availableLayers.end(); ++it2) {

        const std::string& layerName = it2->getPlaneID();
        if (layerID.id == layerName) {
            return *it2;
        }
    }
    return ImagePlaneDesc();
} // getSelectedLayerInternal

int
EffectInstance::getMaskChannel(int inputNb, const std::list<ImagePlaneDesc>& availableLayers, ImagePlaneDesc* comps) const
{
    *comps = ImagePlaneDesc::getNoneComponents();
    
    std::map<int, MaskSelector >::const_iterator it = _imp->defKnobs->maskSelectors.find(inputNb);

    if ( it == _imp->defKnobs->maskSelectors.end() ) {
        return -1;
    }
    ChoiceOption maskChannelID =  it->second.channel.lock()->getCurrentEntry();

    for (std::list<ImagePlaneDesc>::const_iterator it2 = availableLayers.begin(); it2 != availableLayers.end(); ++it2) {

        std::size_t nChans = (std::size_t)it2->getNumComponents();
        for (std::size_t c = 0; c < nChans; ++c) {
            ChoiceOption channelOption = it2->getChannelOption(c);
            if (channelOption.id == maskChannelID.id) {
                *comps = *it2;
                return c;
            }
        }
    }
    return -1;

} // getMaskChannel


KnobChoicePtr
EffectInstance::getLayerChoiceKnob(int inputNb) const
{
    std::map<int, ChannelSelector>::iterator found = _imp->defKnobs->channelsSelectors.find(inputNb);

    if ( found == _imp->defKnobs->channelsSelectors.end() ) {
        return KnobChoicePtr();
    }
    return found->second.layer.lock();
}

KnobChoicePtr
EffectInstance::getMaskChannelKnob(int inputNb) const
{
    std::map<int, MaskSelector >::const_iterator it = _imp->defKnobs->maskSelectors.find(inputNb);
    if ( it == _imp->defKnobs->maskSelectors.end() ) {
        return KnobChoicePtr();
    }
    return it->second.channel.lock();
}

void
EffectInstance::refreshLayersSelectorsVisibility()
{

    KnobBoolPtr processAllKnob = _imp->defKnobs->processAllLayersKnob.lock();
    if (!processAllKnob) {
        return;
    }
    bool outputIsAll = processAllKnob->getValue();

    // Disable all input selectors as it doesn't make sense to edit them whilst output is All

    ImagePlaneDesc mainInputComps, outputComps, tmpPlane;


    int mainInputIndex = getNode()->getPreferredInput();

    getMetadataComponents(mainInputIndex, &mainInputComps, &tmpPlane);
    getMetadataComponents(-1, &outputComps, &tmpPlane);

    for (std::map<int, ChannelSelector>::iterator it = _imp->defKnobs->channelsSelectors.begin(); it != _imp->defKnobs->channelsSelectors.end(); ++it) {

        // Get the mask input components
        std::list<ImagePlaneDesc> availableComponents;
        {
            ActionRetCodeEnum stat = getAvailableLayers(getCurrentRenderTime(), ViewIdx(0), it->first, &availableComponents);
            (void)stat;
        }


        if (it->first >= 0) {
            EffectInstancePtr inp = getInputMainInstance(it->first);
            bool mustBeSecret = !inp.get() || outputIsAll;
            KnobChoicePtr layerKnob = it->second.layer.lock();
            layerKnob->setSecret(mustBeSecret);

        } else {
            it->second.layer.lock()->setSecret(outputIsAll);
        }
    }


    // Refresh RGBA checkbox visibility
    KnobBoolPtr enabledChan[4];
    bool hasRGBACheckboxes = false;
    for (int i = 0; i < 4; ++i) {
        enabledChan[i] = _imp->defKnobs->enabledChan[i].lock();
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
EffectInstance::refreshEnabledKnobsLabel(const ImagePlaneDesc& mainInputComps, const ImagePlaneDesc& outputComps)
{
    KnobBoolPtr enabledChan[4];
    bool hasRGBACheckboxes = false;
    for (int i = 0; i < 4; ++i) {
        enabledChan[i] = _imp->defKnobs->enabledChan[i].lock();
        hasRGBACheckboxes |= (bool)enabledChan[i];
    }
    if (!hasRGBACheckboxes) {
        return;
    }

    // We display the number of channels that the output layer can have
    int nOutputComps = outputComps.getNumComponents();

    // But we name the channels by the name of the input layer
    const std::vector<std::string>& inputChannelNames = mainInputComps.getChannels();
    const std::vector<std::string>& outputChannelNames = outputComps.getChannels();

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
EffectInstance::getProcessChannel(int channelIndex) const
{
    if (!isHostChannelSelectorEnabled()) {
        return true;
    }
    assert(channelIndex >= 0 && channelIndex < 4);
    KnobBoolPtr k = _imp->defKnobs->enabledChan[channelIndex].lock();
    if (k) {
        return k->getValue();
    }

    return true;
}

KnobBoolPtr
EffectInstance::getProcessChannelKnob(int channelIndex) const
{
    assert(channelIndex >= 0 && channelIndex < 4);
    return _imp->defKnobs->enabledChan[channelIndex].lock();
}



bool
EffectInstance::hasAtLeastOneChannelToProcess() const
{
    std::map<int, ChannelSelector>::const_iterator foundSelector = _imp->defKnobs->channelsSelectors.find(-1);

    if ( foundSelector == _imp->defKnobs->channelsSelectors.end() ) {
        return true;
    }
    if ( _imp->defKnobs->enabledChan[0].lock() ) {
        std::bitset<4> processChannels;
        processChannels[0] = _imp->defKnobs->enabledChan[0].lock()->getValue();
        processChannels[1] = _imp->defKnobs->enabledChan[1].lock()->getValue();
        processChannels[2] = _imp->defKnobs->enabledChan[2].lock()->getValue();
        processChannels[3] = _imp->defKnobs->enabledChan[3].lock()->getValue();
        if (!processChannels[0] && !processChannels[1] && !processChannels[2] && !processChannels[3]) {
            return false;
        }
    }
    
    return true;
}


int
EffectInstance::isMaskChannelKnob(const KnobIConstPtr& knob) const
{
    for (std::map<int, MaskSelector >::const_iterator it = _imp->defKnobs->maskSelectors.begin(); it != _imp->defKnobs->maskSelectors.end(); ++it) {
        if (it->second.channel.lock() == knob) {
            return it->first;
        }
    }

    return -1;
}

bool
EffectInstance::isMaskEnabled(int inputNb) const
{
    std::map<int, MaskSelector >::const_iterator it = _imp->defKnobs->maskSelectors.find(inputNb);

    if ( it != _imp->defKnobs->maskSelectors.end() ) {
        return it->second.enabled.lock()->getValue();
    } else {
        return true;
    }
}



void
EffectInstance::findOrCreateChannelEnabled()
{
    //Try to find R,G,B,A parameters on the plug-in, if found, use them, otherwise create them
    static const std::string channelLabels[4] = {kNatronOfxParamProcessRLabel, kNatronOfxParamProcessGLabel, kNatronOfxParamProcessBLabel, kNatronOfxParamProcessALabel};
    static const std::string channelNames[4] = {kNatronOfxParamProcessR, kNatronOfxParamProcessG, kNatronOfxParamProcessB, kNatronOfxParamProcessA};
    static const std::string channelHints[4] = {kNatronOfxParamProcessRHint, kNatronOfxParamProcessGHint, kNatronOfxParamProcessBHint, kNatronOfxParamProcessAHint};
    KnobBoolPtr foundEnabled[4];
    const KnobsVec & knobs = getKnobs();

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

    KnobPagePtr mainPage;
    if (foundAll) {
        for (int i = 0; i < 4; ++i) {
            // Writers already have their checkboxes places correctly
            if (!isWriter()) {
                if (!mainPage) {
                    mainPage = getOrCreateMainPage();
                }
                if (foundEnabled[i]->getParentKnob() == mainPage) {
                    //foundEnabled[i]->setAddNewLine(i == 3);
                    mainPage->removeKnob(foundEnabled[i]);
                    mainPage->insertKnob(i, foundEnabled[i]);
                }
            }
            _imp->defKnobs->enabledChan[i] = foundEnabled[i];
        }
    }

    std::bitset<4> pluginDefaultPref;
    bool hostChannelSelectorEnabled = getNode()->getPlugin()->getPropertyUnsafe<bool>(kNatronPluginPropHostChannelSelector);
    pluginDefaultPref = getNode()->getPlugin()->getPropertyUnsafe<std::bitset<4> >(kNatronPluginPropHostChannelSelectorValue);


    if (hostChannelSelectorEnabled) {
        if (foundAll) {
            std::cerr << getScriptName_mt_safe() << ": WARNING: property " << kNatronOfxImageEffectPropChannelSelector << " is different of " << kOfxImageComponentNone << " but uses its own checkboxes" << std::endl;
        } else {
            if (!mainPage) {
                mainPage = getOrCreateMainPage();
            }

            //Create the selectors
            for (int i = 0; i < 4; ++i) {
                foundEnabled[i] =  createKnob<KnobBool>(channelNames[i]);
                foundEnabled[i]->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
                foundEnabled[i]->setLabel(channelLabels[i]);
                foundEnabled[i]->setAnimationEnabled(false);
                foundEnabled[i]->setAddNewLine(i == 3);
                foundEnabled[i]->setDefaultValue(pluginDefaultPref[pluginDefaultPref.size() - 1 - i]);
                foundEnabled[i]->setHintToolTip(channelHints[i]);
                mainPage->insertKnob(i, foundEnabled[i]);
                _imp->defKnobs->enabledChan[i] = foundEnabled[i];
            }
            foundAll = true;
        }
    }
    if ( !isWriter() && foundAll && !getApp()->isBackground() ) {
        if (!mainPage) {
            mainPage = getOrCreateMainPage();
        }
        _imp->defKnobs->enabledChan[3].lock()->setAddNewLine(false);
        KnobStringPtr premultWarning = createKnob<KnobString>("premultWarningKnob");
        premultWarning->setLabel(QString());
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
        _imp->defKnobs->premultWarning = premultWarning;
    }
} // findOrCreateChannelEnabled


bool
EffectInstance::addUserComponents(const ImagePlaneDesc& comps)
{

    KnobLayersPtr knob = _imp->defKnobs->createPlanesKnob.lock();
    if (!knob) {
        return false;
    }
    std::list<ImagePlaneDesc> existingPlanes = knob->decodePlanesList();

    for (std::list<ImagePlaneDesc>::iterator it = existingPlanes.begin(); it != existingPlanes.end(); ++it) {
        if ( it->getPlaneID() == comps.getPlaneID() ) {
            return false;
        }
    }

    existingPlanes.push_back(comps);

    std::string encoded = knob->encodePlanesList(existingPlanes);
    knob->setValue(encoded);
    
    {
        ///The node has node channel selector, don't allow adding a custom plane.
        KnobIPtr outputLayerKnob = getKnobByName(kNatronOfxParamOutputChannels);

        if (_imp->defKnobs->channelsSelectors.empty() && !outputLayerKnob) {
            return false;
        }

        if (!outputLayerKnob) {
            std::map<int, ChannelSelector>::iterator found = _imp->defKnobs->channelsSelectors.find(-1);
            if ( found == _imp->defKnobs->channelsSelectors.end() ) {
                return false;
            }
            outputLayerKnob = found->second.layer.lock();
        }
        
        ///Set the selector to the new channel
        KnobChoicePtr layerChoice = toKnobChoice(outputLayerKnob);
        if (layerChoice) {
            layerChoice->setValueFromID(comps.getPlaneID());
        }
    }

    return true;
} // addUserComponents

KnobLayersPtr
EffectInstance::getOrCreateUserPlanesKnob(const KnobPagePtr& page)
{
    KnobLayersPtr param = _imp->defKnobs->createPlanesKnob.lock();
    if (param) {
        return param;
    }
    param = createKnob<KnobLayers>(kNodeParamUserLayers);
    param->setLabel(tr(kNodeParamUserLayersLabel));
    param->setHintToolTip(tr(kNodeParamUserLayersHint));
    param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
    param->setAnimationEnabled(false);
    param->setIsMetadataSlave(true);
    param->setSecret(true);
    page->addKnob(param);
    _imp->defKnobs->createPlanesKnob = param;
    return param;
}

KnobLayersPtr
EffectInstance::getUserPlanesKnob() const
{
    return _imp->defKnobs->createPlanesKnob.lock();
}

void
EffectInstance::createChannelSelectors(const std::vector<std::pair<bool, bool> >& hasMaskChannelSelector,
                             const std::vector<std::string>& inputLabels,
                             const KnobPagePtr& mainPage,
                             KnobIPtr* lastKnobBeforeAdvancedOption)
{
    ///Create input layer selectors
    for (std::size_t i = 0; i < inputLabels.size(); ++i) {
        if (!hasMaskChannelSelector[i].first) {
            createChannelSelector(i, inputLabels[i], false, mainPage, lastKnobBeforeAdvancedOption);
        }
    }
    ///Create output layer selectors
    createChannelSelector(-1, "Output", true, mainPage, lastKnobBeforeAdvancedOption);
}


void
EffectInstance::Implementation::onLayerChanged(bool isOutput)
{
    if (isOutput) {
        _publicInterface->getNode()->s_outputLayerChanged();
    }
}

void
EffectInstance::Implementation::onMaskSelectorChanged(int inputNb,
                                   const MaskSelector& selector)
{
    KnobChoicePtr channel = selector.channel.lock();
    int index = channel->getValue();
    KnobBoolPtr enabled = selector.enabled.lock();

    if ( (index == 0) && enabled->isEnabled() ) {
        enabled->setValue(false);
        enabled->setEnabled(false);
    } else {
        enabled->setEnabled(true);
        if ( _publicInterface->getNode()->getRealInput(inputNb) ) {
            enabled->setValue(true);
        }
    }

}

NATRON_NAMESPACE_EXIT
