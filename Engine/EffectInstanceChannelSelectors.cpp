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

#include "EffectInstancePrivate.h"
#include "Engine/AppInstance.h"
#include "Engine/Node.h"
#include "Engine/TimeLine.h"
#include "Engine/KnobTypes.h"

NATRON_NAMESPACE_ENTER;



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
        if (inputNb >= 0) {
            choices.push_back(ChoiceOption("None", "", ""));
        }


        std::list<ImagePlaneDesc> availableComponents;
        {
            ActionRetCodeEnum stat = getAvailableLayers(time, ViewIdx(0), inputNb, &availableComponents);
            (void)stat;
        }

        for (std::list<ImagePlaneDesc>::const_iterator it2 = availableComponents.begin(); it2 != availableComponents.end(); ++it2) {
            ChoiceOption layerOption = it2->getPlaneOption();
            choices.push_back(layerOption);
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
        if (_imp->defKnobs->hostChannelSelectorEnabled &&  _imp->defKnobs->enabledChan[0].lock() ) {
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
    ChoiceOption layerID = layerKnob->getActiveEntry();

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
    ChoiceOption maskChannelID =  it->second.channel.lock()->getActiveEntry();

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

void
EffectInstance::refreshLayersSelectorsVisibility()
{

    KnobBoolPtr processAllKnob = _imp->defKnobs->processAllLayersKnob.lock();
    if (!processAllKnob) {
        return;
    }
    bool outputIsAll = processAllKnob->getValue();

    // Disable all input selectors as it doesn't make sense to edit them whilst output is All

    ImagePlaneDesc mainInputComps, outputComps;

    int mainInputIndex = getNode()->getPreferredInput();

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

            if (mainInputIndex != -1 && mainInputIndex == it->first) {
                // This is the main-input
                mainInputComps = _imp->getSelectedLayerInternal(availableComponents, it->second);
            }

        } else {
            it->second.layer.lock()->setSecret(outputIsAll);
            outputComps = _imp->getSelectedLayerInternal(availableComponents, it->second);
        }
    }

    for (std::map<int, MaskSelector>::iterator it = _imp->defKnobs->maskSelectors.begin(); it != _imp->defKnobs->maskSelectors.end(); ++it) {
        EffectInstancePtr inp = getInputMainInstance(it->first);

        KnobBoolPtr enabledKnob = it->second.enabled.lock();
        assert(enabledKnob);
        bool curValue = enabledKnob->getValue();
        bool newValue = inp ? true : false;
        bool changed = curValue != newValue;
        if (changed) {
            enabledKnob->setValue(newValue);
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
EffectInstance::isPluginUsingHostChannelSelectors() const
{
    return _imp->defKnobs->hostChannelSelectorEnabled;
}

bool
EffectInstance::getProcessChannel(int channelIndex) const
{
    if (!isPluginUsingHostChannelSelectors()) {
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

void
EffectInstance::refreshAcceptedComponents(int nInputs)
{
    QMutexLocker k(&_imp->common->supportedComponentsMutex);
    _imp->common->supportedInputComponents.resize(nInputs);
    for (int i = 0; i < nInputs; ++i) {
        _imp->common->supportedInputComponents[i].reset();
        if ( isInputMask(i) ) {
            //Force alpha for masks
            _imp->common->supportedInputComponents[i][0] = 1;
        } else {
            addAcceptedComponents(i, &_imp->common->supportedInputComponents[i]);
        }
    }
    _imp->common->supportedOutputComponents.reset();
    addAcceptedComponents(-1, &_imp->common->supportedOutputComponents);

}

bool
EffectInstance::isSupportedComponent(int inputNb,
                           const ImagePlaneDesc& comp) const
{

    std::bitset<4> supported;
    {
        QMutexLocker l(&_imp->common->supportedComponentsMutex);

        if (inputNb >= 0) {
            assert( inputNb < (int)_imp->common->supportedInputComponents.size() );
            supported = _imp->common->supportedInputComponents[inputNb];
        } else {
            assert(inputNb == -1);
            supported = _imp->common->supportedOutputComponents;
        }
    }
    assert(comp.getNumComponents() <= 4);
    return supported[comp.getNumComponents()];
}

std::bitset<4>
EffectInstance::getSupportedComponents(int inputNb) const
{
    std::bitset<4> supported;
    {
        QMutexLocker l(&_imp->common->supportedComponentsMutex);

        if (inputNb >= 0) {
            assert( inputNb < (int)_imp->common->supportedInputComponents.size() );
            supported = _imp->common->supportedInputComponents[inputNb];
        } else {
            assert(inputNb == -1);
            supported = _imp->common->supportedOutputComponents;
        }
    }
    return supported;
}

int
EffectInstance::findClosestSupportedNumberOfComponents(int inputNb,
                                             int nComps) const
{
    if (nComps < 0 || nComps > 4) {
        // Natron assumes that a layer must have between 1 and 4 channels.
        return 0;
    }
    std::bitset<4> supported = getSupportedComponents(inputNb);

    // Find a greater or equal number of components
    int foundSupportedNComps = -1;
    for (int i = nComps - 1; i < 4; ++i) {
        if (supported[i]) {
            foundSupportedNComps = i + 1;
            break;
        }
    }


    if (foundSupportedNComps == -1) {
        // Find a small number of components
        for (int i = nComps - 2; i >= 0; --i) {
            if (supported[i]) {
                foundSupportedNComps = i + 1;
                break;
            }
        }
    }

    if (foundSupportedNComps == -1) {
        return 0;
    }
    return foundSupportedNComps;

} // findClosestSupportedNumberOfComponents


void
EffectInstance::refreshAcceptedBitDepths()
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->common->supportedDepths.clear();
    addSupportedBitDepth(&_imp->common->supportedDepths);
    if ( _imp->common->supportedDepths.empty() ) {
        //From the spec:
        //The default for a plugin is to have none set, the plugin must define at least one in its describe action.
        throw std::runtime_error(tr("This plug-in does not support any of 8-bit, 16-bit or 32-bit floating point image processing").toStdString());
    }

}


ImageBitDepthEnum
EffectInstance::getClosestSupportedBitDepth(ImageBitDepthEnum depth)
{
    bool foundShort = false;
    bool foundByte = false;
    bool foundFloat = false;
    for (std::list<ImageBitDepthEnum>::const_iterator it = _imp->common->supportedDepths.begin(); it != _imp->common->supportedDepths.end(); ++it) {
        if (*it == depth) {
            return depth;
        } else if (*it == eImageBitDepthFloat) {
            foundFloat = true;
        } else if (*it == eImageBitDepthShort) {
            foundShort = true;
        } else if (*it == eImageBitDepthByte) {
            foundByte = true;
        }
    }
    if (foundFloat) {
        return eImageBitDepthFloat;
    } else if (foundShort) {
        return eImageBitDepthShort;
    } else if (foundByte) {
        return eImageBitDepthByte;
    } else {
        ///The plug-in doesn't support any bitdepth, the program shouldn't even have reached here.
        assert(false);

        return eImageBitDepthNone;
    }
}

ImageBitDepthEnum
EffectInstance::getBestSupportedBitDepth() const
{
    bool foundShort = false;
    bool foundByte = false;

    for (std::list<ImageBitDepthEnum>::const_iterator it = _imp->common->supportedDepths.begin(); it != _imp->common->supportedDepths.end(); ++it) {
        switch (*it) {
            case eImageBitDepthByte:
                foundByte = true;
                break;

            case eImageBitDepthShort:
                foundShort = true;
                break;
            case eImageBitDepthHalf:
                break;

            case eImageBitDepthFloat:

                return eImageBitDepthFloat;

            case eImageBitDepthNone:
                break;
        }
    }

    if (foundShort) {
        return eImageBitDepthShort;
    } else if (foundByte) {
        return eImageBitDepthByte;
    } else {
        ///The plug-in doesn't support any bitdepth, the program shouldn't even have reached here.
        assert(false);

        return eImageBitDepthNone;
    }
}

bool
EffectInstance::isSupportedBitDepth(ImageBitDepthEnum depth) const
{
    return std::find(_imp->common->supportedDepths.begin(), _imp->common->supportedDepths.end(), depth) != _imp->common->supportedDepths.end();
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

    bool pluginDefaultPref[4];
    _imp->defKnobs->hostChannelSelectorEnabled = isHostChannelSelectorSupported(&pluginDefaultPref[0], &pluginDefaultPref[1], &pluginDefaultPref[2], &pluginDefaultPref[3]);


    if (_imp->defKnobs->hostChannelSelectorEnabled) {
        if (foundAll) {
            std::cerr << getScriptName_mt_safe() << ": WARNING: property " << kNatronOfxImageEffectPropChannelSelector << " is different of " << kOfxImageComponentNone << " but uses its own checkboxes" << std::endl;
        } else {
            if (!mainPage) {
                mainPage = getOrCreateMainPage();
            }

            //Create the selectors
            for (int i = 0; i < 4; ++i) {
                foundEnabled[i] =  createKnob<KnobBool>(channelNames[i]);
                foundEnabled[i]->setDeclaredByPlugin(false);
                foundEnabled[i]->setLabel(channelLabels[i]);
                foundEnabled[i]->setAnimationEnabled(false);
                foundEnabled[i]->setAddNewLine(i == 3);
                foundEnabled[i]->setDefaultValue(pluginDefaultPref[i]);
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


    {
        QMutexLocker k(&_imp->common->createdPlanesMutex);
        for (std::list<ImagePlaneDesc>::iterator it = _imp->common->createdPlanes.begin(); it != _imp->common->createdPlanes.end(); ++it) {
            if ( it->getPlaneID() == comps.getPlaneID() ) {
                return false;
            }
        }

        _imp->common->createdPlanes.push_back(comps);
    }

    {
        ///The node has node channel selector, don't allow adding a custom plane.
        KnobIPtr outputLayerKnob = getKnobByName(kNatronOfxParamOutputChannels);

        if (_imp->defKnobs->channelsSelectors.empty() && !outputLayerKnob) {
            return false;
        }

        if (!outputLayerKnob) {
            //The effect does not have kNatronOfxParamOutputChannels but maybe the selector provided by Natron
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

void
EffectInstance::getUserCreatedComponents(std::list<ImagePlaneDesc>* comps)
{
    QMutexLocker k(&_imp->common->createdPlanesMutex);

    *comps = _imp->common->createdPlanes;
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
    } else if ( !enabled->isEnabled() ) {
        enabled->setEnabled(true);
        if ( _publicInterface->getInputMainInstance(inputNb) ) {
            enabled->setValue(true);
        }
    }

}

NATRON_NAMESPACE_EXIT;
