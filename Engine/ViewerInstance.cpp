/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#include "ViewerInstance.h"

#include <algorithm> // min, max
#include <stdexcept>
#include <cassert>
#include <cstring> // for std::memcpy
#include <cfloat> // DBL_MAX

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Image.h"
#include "Engine/Lut.h"
#include "Engine/NodeMetadata.h"
#include "Engine/Node.h"
#include "Engine/InputDescription.h"
#include "Engine/Hash64.h"
#include "Engine/OpenGLViewerI.h"
#include "Engine/KnobTypes.h"
#include "Engine/MultiThread.h"
#include "Engine/Project.h"
#include "Engine/RamBuffer.h"
#include "Engine/Settings.h"
#include "Engine/ViewerNode.h"

#define GAMMA_LUT_NB_VALUES 1023


#ifndef M_LN2
#define M_LN2       0.693147180559945309417232121458176568  /* loge(2)        */
#endif

NATRON_NAMESPACE_ENTER


struct ViewerInstancePrivate
{

    ViewerInstance* _publicInterface;

    // The layer to render for RGB
    KnobChoiceWPtr layerChoiceKnob;

    // The layer to use in input for the alpha channel
    KnobChoiceWPtr alphaChannelChoiceKnob;

    // What to output
    KnobChoiceWPtr displayChannels;

    KnobChoiceWPtr outputColorspace;

    KnobDoubleWPtr gammaKnob;
    KnobDoubleWPtr gainKnob;
    KnobButtonWPtr autoContrastKnob;

    KnobDoubleWPtr userRoIBottomLeftKnob, userRoISizeKnob;
    KnobButtonWPtr userRoIEnabled;

    KnobButtonWPtr clipToFormatButtonKnob;

    bool viewerChannelsAutoswitchedToAlpha;
    bool layerAndAlphaChoiceRefreshEnabled;

    ViewerInstancePrivate(ViewerInstance* publicInterface)
    : _publicInterface(publicInterface)
    , layerChoiceKnob()
    , alphaChannelChoiceKnob()
    , displayChannels()
    , outputColorspace()
    , gammaKnob()
    , gainKnob()
    , autoContrastKnob()
    , userRoIBottomLeftKnob()
    , userRoISizeKnob()
    , userRoIEnabled()
    , clipToFormatButtonKnob()
    , viewerChannelsAutoswitchedToAlpha(false)
    , layerAndAlphaChoiceRefreshEnabled(true)
    {

    }


    static void buildGammaLut(double gamma, RamBuffer<float>* gammaLookup);

    static float lookupGammaLut(float value, const float* gammaLookupBuffer);

    void refreshLayerAndAlphaChannelComboBox();

    ImagePlaneDesc getSelectedLayer(const std::list<ImagePlaneDesc>& availableLayers) const;

    ImagePlaneDesc getSelectedAlphaChannel(const std::list<ImagePlaneDesc>& availableLayers, int *channelIndex) const;

    ImagePlaneDesc getComponentsFromDisplayChannels(const ImagePlaneDesc& alphaLayer) const;

    ActionRetCodeEnum getChannelOptions(TimeValue time, ImagePlaneDesc* rgbLayer, ImagePlaneDesc* alphaLayer, int* alphaChannelIndex, ImagePlaneDesc* displayChannels) const;

    void setDisplayChannelsFromLayer(const std::list<ImagePlaneDesc>& availableLayers);
    
};


const Color::Lut*
ViewerInstance::lutFromColorspace(ViewerColorSpaceEnum cs)
{
    const Color::Lut* lut;

    switch (cs) {
    case eViewerColorSpaceSRGB:
        lut = Color::LutManager::sRGBLut();
        break;
    case eViewerColorSpaceRec709:
        lut = Color::LutManager::Rec709Lut();
        break;
    case eViewerColorSpaceLinear:
    default:
        lut = 0;
        break;
    }
    if (lut) {
        lut->validate();
    }

    return lut;
}

EffectInstancePtr
ViewerInstance::create(const NodePtr& node)
{
    return EffectInstancePtr( new ViewerInstance(node) );
}

PluginPtr
ViewerInstance::createPlugin()
{
    std::vector<std::string> grouping;
    grouping.push_back(PLUGIN_GROUP_IMAGE);
    PluginPtr ret = Plugin::create(ViewerInstance::create, ViewerInstance::createRenderClone, PLUGINID_NATRON_VIEWER_INTERNAL, "ViewerProcess", 1, 0, grouping);
    ret->setProperty<std::string>(kNatronPluginPropIconFilePath,  "Images/viewer_icon.png");
    QString desc =  EffectInstance::tr("The Viewer node can display the output of a node graph.");
    ret->setProperty<bool>(kNatronPluginPropIsInternalOnly, true);
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    EffectDescriptionPtr effectDesc = ret->getEffectDescriptor();
    effectDesc->setProperty<RenderSafetyEnum>(kEffectPropRenderThreadSafety, eRenderSafetyFullySafe);
    effectDesc->setProperty<bool>(kEffectPropSupportsTiles, true);
    ret->setProperty<bool>(kNatronPluginPropMultiPlanar, true);
    ret->setProperty<bool>(kNatronPluginPropSupportsMultiInputsBitDepths, true);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthFloat, 0);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthByte, 1);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthShort, 2);
    ret->setProperty<std::bitset<4> >(kNatronPluginPropOutputSupportedComponents, std::bitset<4>(std::string("1111")));
    {
        InputDescriptionPtr input = InputDescription::create("Source", "Source", "", false, false, std::bitset<4>(std::string("1111")));
        ret->addInputDescription(input);
    }
    return ret;
}

ViewerInstance::ViewerInstance(const NodePtr& node)
    : EffectInstance(node)
    , _imp( new ViewerInstancePrivate(this) )
{

}

ViewerInstance::ViewerInstance(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key)
: EffectInstance(mainInstance, key)
, _imp( new ViewerInstancePrivate(this) )
{

}

ViewerInstance::~ViewerInstance()
{
}

ViewerNodePtr
ViewerInstance::getViewerNodeGroup() const
{
    NodeCollectionPtr collection = getNode()->getGroup();
    assert(collection);
    NodeGroupPtr isGroup = toNodeGroup(collection);
    assert(isGroup);
    ViewerNodePtr ret = toViewerNode(isGroup);
    assert(ret);
    return ret;
}

void
ViewerInstance::fetchRenderCloneKnobs()
{
    _imp->layerChoiceKnob = getKnobByNameAndType<KnobChoice>(kViewerInstanceParamOutputLayer);
    _imp->alphaChannelChoiceKnob = getKnobByNameAndType<KnobChoice>(kViewerInstanceParamAlphaChannel);
    _imp->displayChannels = getKnobByNameAndType<KnobChoice>(kViewerInstanceParamDisplayChannels);
    _imp->gainKnob = getKnobByNameAndType<KnobDouble>(kViewerInstanceNodeParamGain);
    _imp->gammaKnob = getKnobByNameAndType<KnobDouble>(kViewerInstanceParamGamma);
    _imp->outputColorspace = getKnobByNameAndType<KnobChoice>(kViewerInstanceParamColorspace);
    _imp->autoContrastKnob = getKnobByNameAndType<KnobButton>(kViewerInstanceParamEnableAutoContrast);
    _imp->userRoIEnabled = getKnobByNameAndType<KnobButton>(kViewerInstanceParamEnableUserRoI);
    _imp->userRoIBottomLeftKnob = getKnobByNameAndType<KnobDouble>(kViewerInstanceParamUserRoIBottomLeft);
    _imp->userRoISizeKnob = getKnobByNameAndType<KnobDouble>(kViewerInstanceParamUserRoISize);
    _imp->clipToFormatButtonKnob = getKnobByNameAndType<KnobButton>(kViewerInstanceParamClipToFormat);
}

RectD
ViewerInstance::getViewerRoI()
{
    ViewerNodePtr viewerNode = getViewerNodeGroup();
    assert(viewerNode);
    RectD rod;
    RectD viewerRoI = viewerNode->getUiContext()->getImageRectangleDisplayed();
    bool clipToFormat = _imp->clipToFormatButtonKnob.lock()->getValue();
    if (!clipToFormat) {
        rod = viewerRoI;
    } else {
        RectI format = getOutputFormat();
        double par = getAspectRatio(-1);
        RectD formatCanonical;
        format.toCanonical_noClipping(0, par, &formatCanonical);
        viewerRoI.intersect(formatCanonical, &rod);
    }

    bool userRoiEnabled = _imp->userRoIEnabled.lock()->getValue();
    if (userRoiEnabled) {
        RectD userRoI;
        KnobDoublePtr btmLeft = _imp->userRoIBottomLeftKnob.lock();
        KnobDoublePtr size = _imp->userRoISizeKnob.lock();
        userRoI.x1 = btmLeft->getValue();
        userRoI.y1 = btmLeft->getValue(DimIdx(1));
        userRoI.x2 = userRoI.x1 + size->getValue();
        userRoI.y2 = userRoI.y1 + size->getValue(DimIdx(1));

        rod.intersect(userRoI, &rod);
    }
    return rod;
}

void
ViewerInstance::initializeKnobs()
{
    EffectInstancePtr thisShared = shared_from_this();
    KnobPagePtr page = createKnob<KnobPage>("controlsPage");
    page->setLabel(EffectInstance::tr("Controls"));


    {
        KnobChoicePtr param = createKnob<KnobChoice>(kViewerInstanceParamOutputLayer);
        param->setLabel(tr(kViewerInstanceParamOutputLayerLabel) );
        param->setHintToolTip(tr(kViewerInstanceParamOutputLayerHint));
        page->addKnob(param);
        _imp->layerChoiceKnob = param;
    }

    {
        KnobChoicePtr param = createKnob<KnobChoice>(kViewerInstanceParamAlphaChannel );
        param->setLabel(tr(kViewerInstanceParamAlphaChannelLabel));
        param->setHintToolTip(tr(kViewerInstanceParamAlphaChannelHint));
        page->addKnob(param);
        _imp->alphaChannelChoiceKnob = param;
    }
    {
        std::vector<ChoiceOption> displayChannelEntries;
        KnobChoicePtr param = createKnob<KnobChoice>(kViewerInstanceParamDisplayChannels);
        param->setLabel(tr(kViewerInstanceParamDisplayChannelsLabel) );
        param->setHintToolTip(tr(kViewerInstanceParamDisplayChannelsHint));
        {

            displayChannelEntries.push_back(ChoiceOption("Luminance", EffectInstance::tr("Luminance").toStdString(), ""));
            displayChannelEntries.push_back(ChoiceOption("RGB", EffectInstance::tr("RGB").toStdString(), ""));
            displayChannelEntries.push_back(ChoiceOption("Red", EffectInstance::tr("Red").toStdString(), ""));
            displayChannelEntries.push_back(ChoiceOption("Green", EffectInstance::tr("Green").toStdString(), ""));
            displayChannelEntries.push_back(ChoiceOption("Blue", EffectInstance::tr("Blue").toStdString(), ""));
            displayChannelEntries.push_back(ChoiceOption("Alpha", EffectInstance::tr("Alpha").toStdString(), ""));
            displayChannelEntries.push_back(ChoiceOption("Matte", EffectInstance::tr("Matte").toStdString(), ""));
            param->populateChoices(displayChannelEntries);
        }
        _imp->displayChannels = param;
    }
    {
        KnobDoublePtr param = createKnob<KnobDouble>(kViewerInstanceNodeParamGain);
        param->setLabel(tr(kViewerInstanceNodeParamGainLabel));
        param->setHintToolTip(tr(kViewerInstanceNodeParamGainHint));
        page->addKnob(param);
        param->setDisplayRange(-6., 6.);
        _imp->gainKnob = param;
    }
    {
        KnobDoublePtr param = createKnob<KnobDouble>(kViewerInstanceParamGamma );
        param->setLabel(tr(kViewerInstanceParamGammaLabel));
        param->setHintToolTip(tr(kViewerInstanceParamGammaHint));
        param->setDefaultValue(1.);
        page->addKnob(param);
        param->setDisplayRange(0., 5.);
        _imp->gammaKnob = param;
    }
    {
        KnobChoicePtr param = createKnob<KnobChoice>(kViewerInstanceParamColorspace);
        param->setLabel(tr(kViewerInstanceParamColorspaceLabel) );
        param->setHintToolTip(tr(kViewerInstanceParamColorspaceHint));
        {
            std::vector<ChoiceOption> entries;
            entries.push_back(ChoiceOption("Linear(None)", "", ""));
            entries.push_back(ChoiceOption("sRGB", "", ""));
            entries.push_back(ChoiceOption("Rec.709", "", ""));
            param->populateChoices(entries);
        }
        param->setDefaultValue(1);
        page->addKnob(param);
        _imp->outputColorspace = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerInstanceParamEnableAutoContrast);
        param->setLabel(tr(kViewerInstanceParamEnableAutoContrastLabel) );
        param->setHintToolTip(tr(kViewerInstanceParamEnableAutoContrastHint));
        page->addKnob(param);
        param->setCheckable(true);
        _imp->autoContrastKnob = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerInstanceParamEnableUserRoI);
        param->setLabel(tr(kViewerInstanceParamEnableUserRoILabel));
        param->setHintToolTip(tr(kViewerInstanceParamEnableUserRoIHint));
        param->setCheckable(true);
        page->addKnob(param);
        param->setSecret(true);
        _imp->userRoIEnabled = param;
    }

    {
        KnobDoublePtr param = createKnob<KnobDouble>(std::string(kViewerInstanceParamUserRoIBottomLeft), 2 );
        param->setDefaultValuesAreNormalized(true);
        param->setSecret(true);
        param->setDefaultValue(0.2, DimIdx(0));
        param->setDefaultValue(0.2, DimIdx(1));
        page->addKnob(param);
        _imp->userRoIBottomLeftKnob = param;
    }
    {
        KnobDoublePtr param = createKnob<KnobDouble>(std::string(kViewerInstanceParamUserRoISize), 2 );
        param->setDefaultValuesAreNormalized(true);
        param->setDefaultValue(.6, DimIdx(0));
        param->setDefaultValue(.6, DimIdx(1));
        param->setSecret(true);
        page->addKnob(param);
        _imp->userRoISizeKnob = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kViewerInstanceParamClipToFormat);
        param->setLabel(tr(kViewerInstanceParamClipToFormatLabel));
        param->setHintToolTip(tr(kViewerInstanceParamClipToFormatHint));
        page->addKnob(param);
        param->setSecret(true);
        param->setCheckable(true);
        param->setDefaultValue(true);
        _imp->clipToFormatButtonKnob = param;
    }

} // initializeKnobs

void
ViewerInstancePrivate::buildGammaLut(double gamma, RamBuffer<float>* gammaLookup)
{
    gammaLookup->resize(GAMMA_LUT_NB_VALUES + 1);
    float* buf = gammaLookup->getData();
    if (gamma <= 0) {
        // gamma = 0: everything is zero, except gamma(1)=1
        memset(buf, 0, sizeof(float) * GAMMA_LUT_NB_VALUES);
        buf[GAMMA_LUT_NB_VALUES] = 1.f;
        return;
    }
    for (int position = 0; position <= GAMMA_LUT_NB_VALUES; ++position) {
        double parametricPos = double(position) / GAMMA_LUT_NB_VALUES;
        double value = std::pow(parametricPos, 1. / gamma);
        // set that in the lut
        buf[position] = (float)std::max( 0., std::min(1., value) );
    }
}

float
ViewerInstancePrivate::lookupGammaLut(float value, const float* gammaLookupBuffer)
{
    assert( !(boost::math::isnan)(value) ); // check for NaN
    if (value < 0.) {
        return 0.;
    } else if (value > 1.) {
        return 1.;
    } else {
        int i = (int)(value * GAMMA_LUT_NB_VALUES);
        assert(0 <= i && i <= GAMMA_LUT_NB_VALUES);
        float alpha = std::max( 0.f, std::min(value * GAMMA_LUT_NB_VALUES - i, 1.f) );
        float a = gammaLookupBuffer[i];
        float b = (i  < GAMMA_LUT_NB_VALUES) ? gammaLookupBuffer[i + 1] : 0.f;

        return a * (1.f - alpha) + b * alpha;
    }
}


ActionRetCodeEnum
ViewerInstance::isIdentity(TimeValue time,
                           const RenderScale & /*scale*/,
                           const RectI & /*roi*/,
                           ViewIdx view,
                           const ImagePlaneDesc& /*plane*/,
                           TimeValue* inputTime,
                           ViewIdx* inputView,
                           int* inputNb,
                           ImagePlaneDesc* /*inputPlane*/)
{
    ImagePlaneDesc selectedLayer, selectedAlphaLayer;
    int alphaChannelIndex;
    {
        ActionRetCodeEnum stat = _imp->getChannelOptions(time, &selectedLayer, &selectedAlphaLayer, &alphaChannelIndex, 0);
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }
    DisplayChannelsEnum displayChannels = (DisplayChannelsEnum)_imp->displayChannels.lock()->getValue();

    if (displayChannels != eDisplayChannelsRGB) {
        // If we need to apply channel operations, we are not identity
        *inputNb = -1;
        return eActionStatusOK;
    }

    if (_imp->gainKnob.lock()->getValue() != 1.) {
        *inputNb = -1;
        return eActionStatusOK;
    }

    if (_imp->gammaKnob.lock()->getValue() != 1.) {
        *inputNb = -1;
        return eActionStatusOK;
    }

    if (_imp->autoContrastKnob.lock()->getValue()) {
        *inputNb = -1;
        return eActionStatusOK;
    }

    *inputNb = 0;
    *inputTime = time;
    *inputView = view;
    return eActionStatusOK;

} // isIdentity

void
ViewerInstance::appendToHash(const ComputeHashArgs& args, Hash64* hash)
{
    EffectInstance::appendToHash(args, hash);

    // We rely on the viewers bit depth setting knob in the getTimeInvariantMetadata() function
    // so make sure it is part of the hash.
    appPTR->getCurrentSettings()->getViewerBitDepthKnob()->appendToHash(args, hash);

}

ActionRetCodeEnum
ViewerInstance::getTimeInvariantMetadata(NodeMetadata& metadata)
{

    // For now we always output 4 channel images
    metadata.setColorPlaneNComps(-1, 4);

    // Input should always be float, since we may do color-space conversion.
    metadata.setBitDepth(0, eImageBitDepthFloat);

    // Output however can be 8-bit
    ImageBitDepthEnum outputDepth = appPTR->getCurrentSettings()->getViewersBitDepth();
    metadata.setBitDepth(-1, outputDepth);


    return eActionStatusOK;
} // getTimeInvariantMetadata


void
ViewerInstance::onMetadataChanged(const NodeMetadata& metadata)
{
    if (_imp->layerAndAlphaChoiceRefreshEnabled) {
        _imp->refreshLayerAndAlphaChannelComboBox();
    }

    getViewerNodeGroup()->onViewerProcessNodeMetadataRefreshed(getNode(), metadata);
    EffectInstance::onMetadataChanged(metadata);

}

ActionRetCodeEnum
ViewerInstance::getLayersProducedAndNeeded(TimeValue time,
                                    ViewIdx view,
                                    std::map<int, std::list<ImagePlaneDesc> >* inputLayersNeeded,
                                    std::list<ImagePlaneDesc>* layersProduced,
                                    TimeValue* passThroughTime,
                                    ViewIdx* passThroughView,
                                    int* passThroughInputNb)
{
    *passThroughInputNb = 0;
    *passThroughTime = time;
    *passThroughView = view;


    ImagePlaneDesc selectedLayer, selectedAlphaLayer, selectedDisplayLayer;
    int alphaChannelIndex;
    {
        ActionRetCodeEnum stat = _imp->getChannelOptions(time, &selectedLayer, &selectedAlphaLayer, &alphaChannelIndex, &selectedDisplayLayer);
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }

    DisplayChannelsEnum outputChannels = (DisplayChannelsEnum)_imp->displayChannels.lock()->getValue();

    // In output we always produce a RGBA texture for the viewer
    layersProduced->push_back(ImagePlaneDesc::getRGBAComponents());

    std::list<ImagePlaneDesc>& neededLayers = (*inputLayersNeeded)[0];

    if (outputChannels != eDisplayChannelsA && selectedLayer.getNumComponents() > 0) {
        neededLayers.push_back(selectedLayer);
    }
    if (selectedAlphaLayer.getNumComponents() > 0) {
        if (outputChannels == eDisplayChannelsA || (outputChannels == eDisplayChannelsMatte && selectedAlphaLayer != selectedLayer)) {
            neededLayers.push_back(selectedAlphaLayer);
        }
    }
    return eActionStatusOK;
} // getComponentsNeededInternal


void
ViewerInstance::setRefreshLayerAndAlphaChoiceEnabled(bool enabled)
{
    _imp->layerAndAlphaChoiceRefreshEnabled = enabled;
}

void
ViewerInstance::getChannelOptions(TimeValue time, ImagePlaneDesc* rgbLayer, ImagePlaneDesc* alphaLayer, int* alphaChannelIndex, ImagePlaneDesc* displayChannels) const
{
    _imp->getChannelOptions(time, rgbLayer, alphaLayer, alphaChannelIndex, displayChannels);
}

ActionRetCodeEnum
ViewerInstancePrivate::getChannelOptions(TimeValue time, ImagePlaneDesc* rgbLayer, ImagePlaneDesc* alphaLayer, int* alphaChannelIndex, ImagePlaneDesc* displayChannels) const
{
    std::list<ImagePlaneDesc> upstreamAvailableLayers;
    {
        const int passThroughPlanesInputNb = 0;
        ActionRetCodeEnum stat = _publicInterface->getAvailableLayers(time, ViewIdx(0), passThroughPlanesInputNb, &upstreamAvailableLayers);
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }
    if (rgbLayer) {
        *rgbLayer = getSelectedLayer(upstreamAvailableLayers);
    }
    if (alphaLayer) {
        *alphaLayer = getSelectedAlphaChannel(upstreamAvailableLayers, alphaChannelIndex);
    }
    if (displayChannels && alphaLayer) {
        *displayChannels = getComponentsFromDisplayChannels(*alphaLayer);
    }
    return eActionStatusOK;

} // getChannelOptions

ImagePlaneDesc
ViewerInstancePrivate::getComponentsFromDisplayChannels(const ImagePlaneDesc& alphaLayer) const
{
    DisplayChannelsEnum outputChannels = (DisplayChannelsEnum)displayChannels.lock()->getValue();
    switch (outputChannels) {
        case eDisplayChannelsA:
        case eDisplayChannelsR:
        case eDisplayChannelsG:
        case eDisplayChannelsB:
        case eDisplayChannelsY:
            return ImagePlaneDesc::getAlphaComponents();
            break;
        case eDisplayChannelsRGB: {
            if (alphaLayer.getNumComponents() == 0) {
                return ImagePlaneDesc::getRGBComponents();
            } else {
                return ImagePlaneDesc::getRGBAComponents();
            }

        }   break;
        case eDisplayChannelsMatte:
            return ImagePlaneDesc::getRGBAComponents();
            break;
    }
    assert(false);

    return ImagePlaneDesc();
} // getComponentsFromDisplayChannels

void
ViewerInstancePrivate::refreshLayerAndAlphaChannelComboBox()
{
    ViewerNodePtr viewerGroup = _publicInterface->getViewerNodeGroup();

    KnobChoicePtr layerKnob = layerChoiceKnob.lock();
    KnobChoicePtr alphaChannelKnob = alphaChannelChoiceKnob.lock();


    std::list<ImagePlaneDesc> upstreamAvailableLayers;

    {
        const int passThroughPlanesInputNb = 0;
        ActionRetCodeEnum stat = _publicInterface->getAvailableLayers(_publicInterface->getCurrentRenderTime(), ViewIdx(0), passThroughPlanesInputNb, &upstreamAvailableLayers);
        (void)stat;
    }

    std::vector<ChoiceOption> layerOptions;
    std::vector<ChoiceOption> channelOptions;
    
    // Append None choice
    layerOptions.push_back(ChoiceOption("-", "",""));
    channelOptions.push_back(ChoiceOption("-", "", ""));

    int foundColorPlaneIndex = -1;
    int foundColorPlaneAlpha = -1;
    for (std::list<ImagePlaneDesc>::iterator it2 = upstreamAvailableLayers.begin(); it2 != upstreamAvailableLayers.end(); ++it2) {

        if (foundColorPlaneIndex == -1 && it2->isColorPlane()) {
            foundColorPlaneIndex = (int)layerOptions.size();
        }

        ChoiceOption layerOption = it2->getPlaneOption();
        layerOptions.push_back(layerOption);

        std::size_t nChans = (std::size_t)it2->getNumComponents();
        if (foundColorPlaneAlpha == -1 && it2->isColorPlane() && nChans == 4) {
            foundColorPlaneAlpha = channelOptions.size() + 3;
        }
        for (std::size_t c = 0; c < nChans; ++c) {
            ChoiceOption chanOption = it2->getChannelOption(c);
            channelOptions.push_back(chanOption);
        }
    }

    layerKnob->populateChoices(layerOptions);
    alphaChannelKnob->populateChoices(channelOptions);

    // If the old layer choice does no longer exist or it is "-", fallback on color-plane
    if (layerKnob->getValue() == 0 || !layerKnob->isActiveEntryPresentInEntries(ViewIdx(0))) {
        if (foundColorPlaneIndex != -1) {
            layerKnob->setValue(foundColorPlaneIndex);
        } else {
            // Did not find color-plane, fallback on the first plane available that is not none, otherwise none
            int fallback_i = layerOptions.size() > 1 ? 1 : 0;
            layerKnob->setValue(fallback_i);
        }
    }

    // If the old alpha choice does no longer exist or it is "-" fallback on color-plane alpha channel
    if (alphaChannelKnob->getValue() == 0 || !alphaChannelKnob->isActiveEntryPresentInEntries(ViewIdx(0))) {
        if (foundColorPlaneIndex != -1) {
            alphaChannelKnob->setValue(foundColorPlaneAlpha);
        } else {
            alphaChannelKnob->setValue(0);
        }
    }
    setDisplayChannelsFromLayer(upstreamAvailableLayers);

} // refreshLayerAndAlphaChannelComboBox


ImagePlaneDesc
ViewerInstancePrivate::getSelectedLayer(const std::list<ImagePlaneDesc>& availableLayers) const
{
    ChoiceOption activeIndexID = layerChoiceKnob.lock()->getCurrentEntry(ViewIdx(0));
    for (std::list<ImagePlaneDesc>::const_iterator it = availableLayers.begin(); it != availableLayers.end(); ++it) {
        ChoiceOption opt = it->getPlaneOption();
        if (opt.id == activeIndexID.id) {
            return *it;
        }
    }
    return ImagePlaneDesc::getNoneComponents();

} // getSelectedLayer

ImagePlaneDesc
ViewerInstancePrivate::getSelectedAlphaChannel(const std::list<ImagePlaneDesc>& availableLayers, int *channelIndex) const
{
    ChoiceOption activeIndexID = alphaChannelChoiceKnob.lock()->getCurrentEntry(ViewIdx(0));
    for (std::list<ImagePlaneDesc>::const_iterator it = availableLayers.begin(); it != availableLayers.end(); ++it) {
        std::size_t nChans = (std::size_t)it->getNumComponents();
        for (std::size_t c = 0; c < nChans; ++c) {
            ChoiceOption chanOption = it->getChannelOption(c);
            if (chanOption.id == activeIndexID.id) {
                *channelIndex = (int)c;
                return *it;
            }
        }
    }
    *channelIndex = -1;
    return ImagePlaneDesc::getNoneComponents();
} // getSelectedAlphaChannel

void
ViewerInstancePrivate::setDisplayChannelsFromLayer(const std::list<ImagePlaneDesc>& availableLayers)
{
    ImagePlaneDesc layer = getSelectedLayer(availableLayers);
    if (layer.getNumComponents() == 1) {

        // Switch auto to alpha if there's only this to view
        displayChannels.lock()->setValue((int)eDisplayChannelsA);
        viewerChannelsAutoswitchedToAlpha = true;

    } else if (layer.getNumComponents() > 1) {

        // Switch back to RGB if we auto-switched to alpha
        if (viewerChannelsAutoswitchedToAlpha) {
            KnobChoicePtr displayChannelsKnob = displayChannels.lock();
            DisplayChannelsEnum curDisplayChannels = (DisplayChannelsEnum)displayChannelsKnob->getValue();
            if ( curDisplayChannels == eDisplayChannelsA) {
                displayChannelsKnob->setValue((int)eDisplayChannelsRGB);
            }
        }
    }

} // setDisplayChannelsFromLayer

bool
ViewerInstance::knobChanged(const KnobIPtr& knob,
                 ValueChangedReasonEnum /*reason*/,
                 ViewSetSpec /*view*/,
                 TimeValue /*time*/)
{
    bool handled = true;
    if (knob == _imp->autoContrastKnob.lock()) {
       bool autoContrastEnabled = _imp->autoContrastKnob.lock()->getValue();
       setSupportsTiles(!autoContrastEnabled);
    } else {
        handled = false;
    }
    return handled;
}


NATRON_NAMESPACE_ANONYMOUS_ENTER


struct MinMaxVal {
    MinMaxVal(double min_, double max_)
    : min(min_)
    , max(max_)
    {
    }
    MinMaxVal()
    : min(DBL_MAX)
    , max(-DBL_MAX)
    {
    }

    double min;
    double max;
};

template <typename PIX, int maxValue, int srcNComps, DisplayChannelsEnum channels>
ActionRetCodeEnum
findAutoContrastVminVmax_generic(const Image::CPUData& colorImage,
                                 const EffectInstancePtr& renderArgs,
                                 const RectI & roi,
                                 MinMaxVal* retValue)
{
    double localVmin = std::numeric_limits<double>::infinity();
    double localVmax = -std::numeric_limits<double>::infinity();

    for (int y = roi.y1; y < roi.y2; ++y) {

        if (renderArgs && renderArgs->isRenderAborted()) {
            *retValue = MinMaxVal(localVmin, localVmax);
            return eActionStatusAborted;
        }

        int pixelStride;
        const PIX* src_pixels[4] = {NULL, NULL, NULL, NULL};
        Image::getChannelPointers<PIX, srcNComps>((const PIX**)colorImage.ptrs, roi.x1, y, colorImage.bounds, (PIX**)src_pixels, &pixelStride);

        for (int x = roi.x1; x < roi.x2; ++x) {

            double tmpPix[4] = {0., 0., 0., 1.};


            // This switch will be optimized out by the compiler since it is a template parameter
            switch (srcNComps) {
                case 4:
                case 3:
                case 2: {
                    for (int i = 0; i < srcNComps; ++i) {
                        if (src_pixels[i]) {
                            tmpPix[i] = Image::convertPixelDepth<PIX, float>(*src_pixels[i]);
                        }
                    }
                }   break;
                case 1: {
                    if (src_pixels[0]) {
                        tmpPix[3] = Image::convertPixelDepth<PIX, float>(*src_pixels[0]);
                    }
                }   break;
                default:
                    break;
            }

            double mini, maxi;
            switch (channels) {
                case eDisplayChannelsRGB:
                    mini = std::min(std::min(tmpPix[0], tmpPix[1]), tmpPix[2]);
                    maxi = std::max(std::max(tmpPix[0], tmpPix[1]), tmpPix[2]);
                    break;
                case eDisplayChannelsY:
                    mini = 0.299 * tmpPix[0] + 0.587 * tmpPix[1] + 0.114 * tmpPix[2];
                    maxi = mini;
                    break;
                case eDisplayChannelsR:
                    mini = tmpPix[0];
                    maxi = mini;
                    break;
                case eDisplayChannelsG:
                    mini = tmpPix[1];
                    maxi = mini;
                    break;
                case eDisplayChannelsB:
                    mini = tmpPix[2];
                    maxi = mini;
                    break;
                case eDisplayChannelsA:
                    mini = tmpPix[3];
                    maxi = mini;
                    break;
                default:
                    mini = 0.;
                    maxi = 0.;
                    break;
            }
            if (mini < localVmin) {
                localVmin = mini;
            }
            if (maxi > localVmax) {
                localVmax = maxi;
            }

            for (int i = 0; i < srcNComps; ++i) {
                if (src_pixels[i]) {
                    src_pixels[i] += pixelStride;
                }
            }

        }
    }

    *retValue = MinMaxVal(localVmin, localVmax);
    return eActionStatusOK;
} // findAutoContrastVminVmax_generic

template <typename PIX, int maxValue, int srcNComps>
ActionRetCodeEnum
findAutoContrastVminVmaxForComponents(const Image::CPUData& colorImage,
                                      const EffectInstancePtr& renderArgs,
                                      DisplayChannelsEnum channels,
                                      const RectI & roi,
                                      MinMaxVal* ret)
    

{
    switch (channels) {
        case eDisplayChannelsA:
            return findAutoContrastVminVmax_generic<PIX, maxValue, srcNComps, eDisplayChannelsA>(colorImage, renderArgs, roi, ret);
        case eDisplayChannelsR:
            return findAutoContrastVminVmax_generic<PIX, maxValue, srcNComps, eDisplayChannelsR>(colorImage, renderArgs, roi, ret);
        case eDisplayChannelsG:
            return findAutoContrastVminVmax_generic<PIX, maxValue, srcNComps, eDisplayChannelsG>(colorImage, renderArgs, roi, ret);
        case eDisplayChannelsB:
            return findAutoContrastVminVmax_generic<PIX, maxValue, srcNComps, eDisplayChannelsB>(colorImage, renderArgs, roi, ret);
        case eDisplayChannelsY:
            return findAutoContrastVminVmax_generic<PIX, maxValue, srcNComps, eDisplayChannelsY>(colorImage, renderArgs, roi, ret);
        case eDisplayChannelsMatte:
            return findAutoContrastVminVmax_generic<PIX, maxValue, srcNComps, eDisplayChannelsMatte>(colorImage, renderArgs, roi, ret);
        case eDisplayChannelsRGB:
            return findAutoContrastVminVmax_generic<PIX, maxValue, srcNComps, eDisplayChannelsRGB>(colorImage, renderArgs, roi, ret);
    }
    assert(false);
    return eActionStatusFailed;
}


template <typename PIX, int maxValue>
ActionRetCodeEnum
findAutoContrastVminVmaxForDepth(const Image::CPUData& colorImage,
                                 const EffectInstancePtr& renderArgs,
                                 DisplayChannelsEnum channels,
                                 const RectI & roi,
                                 MinMaxVal* ret)


{
    switch (colorImage.nComps) {
        case 1:
            return findAutoContrastVminVmaxForComponents<PIX, maxValue, 1>(colorImage, renderArgs, channels, roi, ret);
        case 2:
            return findAutoContrastVminVmaxForComponents<PIX, maxValue, 2>(colorImage, renderArgs, channels, roi, ret);
        case 3:
            return findAutoContrastVminVmaxForComponents<PIX, maxValue, 3>(colorImage, renderArgs, channels, roi, ret);
        case 4:
            return findAutoContrastVminVmaxForComponents<PIX, maxValue, 4>(colorImage, renderArgs, channels, roi, ret);
        default:
            return eActionStatusFailed;

    }
    assert(false);
    return eActionStatusFailed;
}

ActionRetCodeEnum
findAutoContrastVminVmax(const Image::CPUData& colorImage,
                         const EffectInstancePtr& renderArgs,
                         DisplayChannelsEnum channels,
                         const RectI & roi,
                         MinMaxVal* ret)
{
    switch (colorImage.bitDepth) {
        case eImageBitDepthByte:
            return findAutoContrastVminVmaxForDepth<unsigned char, 255>(colorImage, renderArgs, channels, roi, ret);
        case eImageBitDepthFloat:
            return findAutoContrastVminVmaxForDepth<float, 1>(colorImage, renderArgs, channels, roi, ret);
        case eImageBitDepthHalf:
        case eImageBitDepthNone:
            return eActionStatusFailed;
        case eImageBitDepthShort:
            return findAutoContrastVminVmaxForDepth<unsigned short, 65535>(colorImage, renderArgs, channels, roi, ret);
    }
    assert(false);
    return eActionStatusFailed;
}

class FindAutoContrastProcessor : public ImageMultiThreadProcessorBase
{
    Image::CPUData _colorImage;
    DisplayChannelsEnum _channels;

    mutable QMutex _resultMutex;
    MinMaxVal _result;

public:

    FindAutoContrastProcessor(const EffectInstancePtr& renderArgs)
    : ImageMultiThreadProcessorBase(renderArgs)
    {
        _result.min = std::numeric_limits<double>::infinity();
        _result.max = -std::numeric_limits<double>::infinity();
    }

    virtual ~FindAutoContrastProcessor()
    {
    }

    void setValues(const Image::CPUData& colorImage, DisplayChannelsEnum channels)
    {
        _colorImage = colorImage;
        _channels = channels;
    }

    MinMaxVal getResults() const
    {
        return _result;
    }

private:

    virtual ActionRetCodeEnum multiThreadProcessImages(const RectI& renderWindow) OVERRIDE FINAL
    {
        MinMaxVal localResult;
        ActionRetCodeEnum stat = findAutoContrastVminVmax(_colorImage, _effect, _channels, renderWindow, &localResult);
        if (isFailureRetCode(stat)) {
            return stat;
        }

        QMutexLocker k(&_resultMutex);
        _result.min = std::min(_result.min, localResult.min);
        _result.max = std::max(_result.max, localResult.max);
        return eActionStatusOK;
    }
};

struct RenderViewerArgs
{
    Image::CPUData colorImage, alphaImage, dstImage;
    int alphaChannelIndex;
    EffectInstancePtr renderArgs;
    double gamma, gain, offset;
    DisplayChannelsEnum channels;
    const Color::Lut* srcColorspace;
    const Color::Lut* dstColorspace;
    const float* gammaLut;
};



template <typename PIX, int maxValue, int srcNComps, DisplayChannelsEnum channels>
void
genericViewerProcessFunctor(const RenderViewerArgs& args,
                            const PIX* color_pixels[4],
                            const PIX* alpha_pixels[4],
                            float tmpPix[4],
                            double* alphaMatteValue)
{
    memset(tmpPix, 0, sizeof(float) * 4);

    if (channels == eDisplayChannelsMatte || channels == eDisplayChannelsA) {

        // Get the alpha from the alpha image
        if (args.alphaChannelIndex != -1 && alpha_pixels[args.alphaChannelIndex]) {
            tmpPix[3] = Image::convertPixelDepth<PIX, float>(*alpha_pixels[args.alphaChannelIndex]);
        } else {
            tmpPix[3] = 1.;
        }
    } else {
        tmpPix[3] = 1.;
    }
    switch (channels) {
        case eDisplayChannelsA:
            tmpPix[0] = tmpPix[1] = tmpPix[2] = tmpPix[3];
            break;
        case eDisplayChannelsB:
            if (color_pixels[2]) {
                tmpPix[0] = Image::convertPixelDepth<PIX, float>(*color_pixels[2]);
                if (args.srcColorspace) {
                    tmpPix[0] = args.srcColorspace->fromColorSpaceFloatToLinearFloat(tmpPix[0]);
                }
            } else {
                tmpPix[0] = 0.;
            }
            tmpPix[1] = tmpPix[0];
            tmpPix[2] = tmpPix[0];
            break;
        case eDisplayChannelsG:
            if (color_pixels[1]) {
                tmpPix[0] = Image::convertPixelDepth<PIX, float>(*color_pixels[1]);
                if (args.srcColorspace) {
                    tmpPix[0] = args.srcColorspace->fromColorSpaceFloatToLinearFloat(tmpPix[0]);
                }
            } else {
                tmpPix[0] = 0.;
            }
            tmpPix[1] = tmpPix[0];
            tmpPix[2] = tmpPix[0];
            break;
        case eDisplayChannelsR:
            if (color_pixels[0]) {
                tmpPix[0] = Image::convertPixelDepth<PIX, float>(*color_pixels[0]);
                if (args.srcColorspace) {
                    tmpPix[0] = args.srcColorspace->fromColorSpaceFloatToLinearFloat(tmpPix[0]);
                }
            } else {
                tmpPix[0] = 0.;
            }
            tmpPix[1] = tmpPix[0];
            tmpPix[2] = tmpPix[0];
            break;
        case eDisplayChannelsY:
        case eDisplayChannelsRGB:
        case eDisplayChannelsMatte:
            for (int i = 0; i < 3; ++i) {
                if (color_pixels[i]) {
                    assert( !(boost::math::isnan)(*color_pixels[i]) ); // check for NaNs
                    tmpPix[i] = Image::convertPixelDepth<PIX, float>(*color_pixels[i]);
                    if (args.srcColorspace) {
                        tmpPix[i] = args.srcColorspace->fromColorSpaceFloatToLinearFloat(tmpPix[i]);
                    }

                }
            }
            if (srcNComps == 1) {
                tmpPix[1] = tmpPix[2] = tmpPix[3] = tmpPix[0];
            }
            break;
        default:
            break;
    }


    // Apply gain gamma and offset to the RGB channels
    for (int i = 0; i < 3; ++i) {
        tmpPix[i] = tmpPix[i] * args.gain + args.offset;
    }
    if (args.gamma <= 0.) {
        for (int i = 0; i < 3; ++i) {
            tmpPix[i] = (tmpPix[i]  < 1.) ? 0. : (tmpPix[i]  == 1. ? 1. : std::numeric_limits<double>::infinity() );
        }
    } else if (args.gamma != 1.) {
        for (int i = 0; i < 3; ++i) {
            tmpPix[i] = ViewerInstancePrivate::lookupGammaLut(tmpPix[i], args.gammaLut);
        }
    }

    if (channels == eDisplayChannelsY) {
        tmpPix[0] = 0.299 * tmpPix[1] + 0.587 * tmpPix[2] + 0.114 * tmpPix[3];
        tmpPix[1] = tmpPix[0];
        tmpPix[2] = tmpPix[0];
    } else if (channels == eDisplayChannelsMatte) {
        *alphaMatteValue = 0;

        // If this is the same image, use the already processed tmpPix
        if (args.colorImage.ptrs == args.alphaImage.ptrs) {
            *alphaMatteValue = tmpPix[args.alphaChannelIndex];
        } else {
            *alphaMatteValue = Image::convertPixelDepth<PIX, float>(*alpha_pixels[args.alphaChannelIndex]);
            // If the image has a color space, convert to linear float first
            if (args.srcColorspace) {
                *alphaMatteValue = args.srcColorspace->fromColorSpaceFloatToLinearFloat(*alphaMatteValue);
            }
        }
    }
} // genericViewerProcessFunctor

inline
unsigned int
toBGRA(unsigned char r,
       unsigned char g,
       unsigned char b,
       unsigned char a)
{
    return (a << 24) | (r << 16) | (g << 8) | b;
}

template <typename PIX, int maxValue, int srcNComps, DisplayChannelsEnum channels>
ActionRetCodeEnum
applyViewerProcess8bit_generic(const RenderViewerArgs& args, const RectI & roi)
{


    for (int y = roi.y1; y < roi.y2; ++y) {

        // Check for abort on every scan-line
        if (args.renderArgs && args.renderArgs->isRenderAborted()) {
            return eActionStatusAborted;
        }

        // For error diffusion, we start at each line at a random pixel along the line so it does
        // not create a pattern in the output image.
        const int startX = rand() % roi.width() + roi.x1;

        for (int backward = 0; backward < 2; ++backward) {

            int x = backward ? std::max(roi.x1 - 1, startX - 1) : startX;

            const int endX = backward ? roi.x1 - 1 : roi.x2;

            unsigned error[3] = {0x80, 0x80, 0x80};


            int colorPixelStride;
            const PIX* color_pixels[4] = {NULL, NULL, NULL, NULL};
            Image::getChannelPointers<PIX, srcNComps>((const PIX**)args.colorImage.ptrs, x, y, args.colorImage.bounds, (PIX**)color_pixels, &colorPixelStride);

            int alphaPixelStride;
            const PIX* alpha_pixels[4] = {NULL, NULL, NULL, NULL};
            if (channels == eDisplayChannelsMatte || channels == eDisplayChannelsA) {
            Image::getChannelPointers<PIX>((const PIX**)args.alphaImage.ptrs, x, y, args.alphaImage.bounds, args.alphaImage.nComps, (PIX**)alpha_pixels, &alphaPixelStride);
            } else {
                alphaPixelStride = 0;
                memset(alpha_pixels, 0, sizeof(PIX*) * 4);
            }

            int dstPixelStride;
            unsigned char* dst_pixels[4] = {NULL, NULL, NULL, NULL};

            Image::getChannelPointers<unsigned char>((const unsigned char**)args.dstImage.ptrs, x, y, args.dstImage.bounds, args.dstImage.nComps, (unsigned char**)dst_pixels, &dstPixelStride);
            unsigned int *dst_pixels_uint = reinterpret_cast<unsigned int*>(dst_pixels[0]);
            while (x != endX) {

                assert(args.colorImage.bounds.isNull() || args.colorImage.bounds.contains(x, y));
                assert(args.alphaImage.bounds.isNull() || args.alphaImage.bounds.contains(x, y));

                float tmpPix[4];
                double alphaMatteValue;
                genericViewerProcessFunctor<PIX, maxValue, srcNComps, channels>(args, color_pixels, alpha_pixels, tmpPix, &alphaMatteValue);

                unsigned char uTmpPix[4];
                if (!args.dstColorspace) {
                    for (int i = 0 ; i < 4; ++i) {
                        uTmpPix[i] = Color::floatToInt<256>(tmpPix[i]);
                    }
                } else {
                    for (int i = 0; i < 3; ++i) {
                        error[i] = (error[i] & 0xff) + args.dstColorspace->toColorSpaceUint8xxFromLinearFloatFast(tmpPix[i]);
                        assert(error[i] < 0x10000);
                        uTmpPix[i] = (unsigned char)(error[i] >> 8);
                    }
                    uTmpPix[3] = Color::floatToInt<256>(tmpPix[3]);
                }


                if (channels == eDisplayChannelsMatte) {
                    unsigned char matteA;
                    if (args.dstColorspace) {
                        matteA = args.dstColorspace->toColorSpaceUint8FromLinearFloatFast(alphaMatteValue) / 2;
                    } else {
                        matteA = Color::floatToInt<256>(alphaMatteValue) / 2;
                    }

                    // Add to the red channel the matte value
                    uTmpPix[0] = Image::clampIfInt<unsigned char>( (double)uTmpPix[0] + matteA );

                }
       
                // The viewer has the particularity to write-out BGRA 8-bit images instead of RGBA since the resulting
                // image is directly fed to the GL_BGRA OpenGL texture format.
                *dst_pixels_uint = toBGRA(uTmpPix[0], uTmpPix[1], uTmpPix[2], uTmpPix[3]);

                if (backward) {
                    --x;
                    --dst_pixels_uint;
                    for (int i = 0; i < 4; ++i) {
                        if (color_pixels[i]) {
                            color_pixels[i] -= colorPixelStride;
                        }
                        if (alpha_pixels[i]) {
                            alpha_pixels[i] -= alphaPixelStride;
                        }
                    }
                } else {
                    ++x;
                    ++dst_pixels_uint;
                    for (int i = 0; i < 4; ++i) {
                        if (color_pixels[i]) {
                            color_pixels[i] += colorPixelStride;
                        }
                        if (alpha_pixels[i]) {
                            alpha_pixels[i] += alphaPixelStride;
                        }
                    }
                }

            } // for each pixels on the line

        } // backward?

    } // for each scan-line
    return eActionStatusOK;
} // applyViewerProcess8bit_generic

template <typename PIX, int maxValue, int srcNComps>
ActionRetCodeEnum
applyViewerProcess8bitForComponents(const RenderViewerArgs& args, const RectI & roi)
{
    switch (args.channels) {
        case eDisplayChannelsA:
            return applyViewerProcess8bit_generic<PIX, maxValue, srcNComps, eDisplayChannelsA>(args, roi);
        case eDisplayChannelsR:
            return applyViewerProcess8bit_generic<PIX, maxValue, srcNComps, eDisplayChannelsR>(args, roi);
        case eDisplayChannelsG:
            return applyViewerProcess8bit_generic<PIX, maxValue, srcNComps, eDisplayChannelsG>(args, roi);
        case eDisplayChannelsB:
            return applyViewerProcess8bit_generic<PIX, maxValue, srcNComps, eDisplayChannelsB>(args, roi);
        case eDisplayChannelsRGB:
            return applyViewerProcess8bit_generic<PIX, maxValue, srcNComps, eDisplayChannelsRGB>(args, roi);
        case eDisplayChannelsY:
            return applyViewerProcess8bit_generic<PIX, maxValue, srcNComps, eDisplayChannelsY>(args, roi);
        case eDisplayChannelsMatte:
            return applyViewerProcess8bit_generic<PIX, maxValue, srcNComps, eDisplayChannelsMatte>(args, roi);

    }
    assert(false);
    return eActionStatusFailed;
}

template <typename PIX, int maxValue>
ActionRetCodeEnum
applyViewerProcess8bitForDepth(const RenderViewerArgs& args, const RectI & roi)
{
    switch (args.colorImage.nComps) {
        case 1:
            return applyViewerProcess8bitForComponents<PIX, maxValue, 1>(args, roi);
        case 2:
            return applyViewerProcess8bitForComponents<PIX, maxValue, 2>(args, roi);
        case 3:
            return applyViewerProcess8bitForComponents<PIX, maxValue, 3>(args, roi);
        case 4:
            return applyViewerProcess8bitForComponents<PIX, maxValue, 4>(args, roi);
        default:
            return eActionStatusFailed;
    }
}


static ActionRetCodeEnum
applyViewerProcess8bit(const RenderViewerArgs& args, const RectI & roi)
{
    switch ( args.colorImage.bitDepth ) {
        case eImageBitDepthFloat:
            return applyViewerProcess8bitForDepth<float, 1>(args, roi);
        case eImageBitDepthByte:
            return applyViewerProcess8bitForDepth<unsigned char, 255>(args, roi);
        case eImageBitDepthShort:
            return applyViewerProcess8bitForDepth<unsigned short, 65535>(args, roi);
        case eImageBitDepthHalf:
        case eImageBitDepthNone:
            return eActionStatusFailed;
    }
    assert(false);
    return eActionStatusFailed;
}



template <typename PIX, int maxValue, int srcNComps, DisplayChannelsEnum channels>
ActionRetCodeEnum
applyViewerProcess32bit_Generic(const RenderViewerArgs& args, const RectI & roi)
{


    for (int y = roi.y1; y < roi.y2; ++y) {

        // Check for abort on every scan-line
        if (args.renderArgs && args.renderArgs->isRenderAborted()) {
            return eActionStatusAborted;
        }


        int colorPixelStride;
        const PIX* color_pixels[4] = {NULL, NULL, NULL, NULL};
        Image::getChannelPointers<PIX, srcNComps>((const PIX**)args.colorImage.ptrs, roi.x1, y, args.colorImage.bounds, (PIX**)color_pixels, &colorPixelStride);

        int alphaPixelStride;
        const PIX* alpha_pixels[4] = {NULL, NULL, NULL, NULL};
        if (channels == eDisplayChannelsMatte || channels == eDisplayChannelsA) {
            Image::getChannelPointers<PIX>((const PIX**)args.alphaImage.ptrs, roi.x1, y, args.alphaImage.bounds, args.alphaImage.nComps, (PIX**)alpha_pixels, &alphaPixelStride);
        } else {
            alphaPixelStride = 0;
            memset(alpha_pixels, 0, sizeof(PIX*) * 4);
        }

        int dstPixelStride;
        float* dst_pixels[4] = {NULL, NULL, NULL, NULL};
        Image::getChannelPointers<float>((const float**)args.dstImage.ptrs, roi.x1, y, args.dstImage.bounds, args.dstImage.nComps, (float**)dst_pixels, &dstPixelStride);
        assert(dst_pixels[0]);

        for (int x = roi.x1; x < roi.x2; ++x) {

            float tmpPix[4];
            double alphaMatteValue;
            genericViewerProcessFunctor<PIX, maxValue, srcNComps, channels>(args, color_pixels, alpha_pixels, tmpPix, &alphaMatteValue);

            if (args.dstColorspace) {
                for (int i = 0; i < 3; ++i) {
                    tmpPix[i] = args.dstColorspace->toColorSpaceFloatFromLinearFloat(tmpPix[i]);
                }
            }

            if (channels == eDisplayChannelsMatte) {
                tmpPix[0] += alphaMatteValue * 0.5;
            }

            for (int i = 0; i < 4; ++i) {
                if (color_pixels[i]) {
                    color_pixels[i] += colorPixelStride;
                }
                if (alpha_pixels[i]) {
                    alpha_pixels[i] += alphaPixelStride;
                }
                if (dst_pixels[i]) {
                    *dst_pixels[i] = tmpPix[i];
                    dst_pixels[i] += dstPixelStride;
                }
            }

        } // for each pixel along the line

    } // for each scan-line
    return eActionStatusOK;
} // applyViewerProcess32bit_Generic

template <typename PIX, int maxValue, int srcNComps>
ActionRetCodeEnum
applyViewerProcess32bitForComponents(const RenderViewerArgs& args, const RectI & roi)
{
    switch (args.channels) {
        case eDisplayChannelsA:
            return applyViewerProcess32bit_Generic<PIX, maxValue, srcNComps, eDisplayChannelsA>(args, roi);
        case eDisplayChannelsR:
            return applyViewerProcess32bit_Generic<PIX, maxValue, srcNComps, eDisplayChannelsR>(args, roi);
        case eDisplayChannelsG:
            return applyViewerProcess32bit_Generic<PIX, maxValue, srcNComps, eDisplayChannelsG>(args, roi);
        case eDisplayChannelsB:
            return applyViewerProcess32bit_Generic<PIX, maxValue, srcNComps, eDisplayChannelsB>(args, roi);
        case eDisplayChannelsRGB:
            return applyViewerProcess32bit_Generic<PIX, maxValue, srcNComps, eDisplayChannelsRGB>(args, roi);
        case eDisplayChannelsY:
            return applyViewerProcess32bit_Generic<PIX, maxValue, srcNComps, eDisplayChannelsY>(args, roi);
        case eDisplayChannelsMatte:
            return applyViewerProcess32bit_Generic<PIX, maxValue, srcNComps, eDisplayChannelsMatte>(args, roi);
    }
    assert(false);
    return eActionStatusFailed;
}

template <typename PIX, int maxValue>
ActionRetCodeEnum
applyViewerProcess32bitForDepth(const RenderViewerArgs& args, const RectI & roi)
{
    switch (args.colorImage.nComps) {
        case 1:
            return applyViewerProcess32bitForComponents<PIX, maxValue, 1>(args, roi);
        case 2:
            return applyViewerProcess32bitForComponents<PIX, maxValue, 2>(args, roi);
        case 3:
            return applyViewerProcess32bitForComponents<PIX, maxValue, 3>(args, roi);
        case 4:
            return applyViewerProcess32bitForComponents<PIX, maxValue, 4>(args, roi);
        default:
            return eActionStatusFailed;
    }
}


static ActionRetCodeEnum
applyViewerProcess32bit(const RenderViewerArgs& args, const RectI & roi)
{
    switch ( args.colorImage.bitDepth ) {
        case eImageBitDepthFloat:
            return applyViewerProcess32bitForDepth<float, 1>(args, roi);
        case eImageBitDepthByte:
            return applyViewerProcess32bitForDepth<unsigned char, 255>(args, roi);
        case eImageBitDepthShort:
            return applyViewerProcess32bitForDepth<unsigned short, 65535>(args, roi);
        case eImageBitDepthHalf:
        case eImageBitDepthNone:
            return eActionStatusFailed;
    }
    assert(false);
    return eActionStatusFailed;
}

class ViewerProcessor : public ImageMultiThreadProcessorBase
{
    RenderViewerArgs _args;

public:

    ViewerProcessor(const EffectInstancePtr& renderArgs)
    : ImageMultiThreadProcessorBase(renderArgs)
    {

    }

    virtual ~ViewerProcessor()

    {
    }

    void setValues(const RenderViewerArgs& args)

    {
        _args = args;
    }

private:

    virtual ActionRetCodeEnum multiThreadProcessImages(const RectI& renderWindow) OVERRIDE FINAL
    {
        if (_args.dstImage.bitDepth == eImageBitDepthFloat) {
            return applyViewerProcess32bit(_args, renderWindow);
        } else if (_args.dstImage.bitDepth == eImageBitDepthByte) {
            return applyViewerProcess8bit(_args, renderWindow);
        } else {
            throw std::runtime_error("Unsupported bit-depth");
            assert(false);
            return eActionStatusFailed;
        }
    }
};

NATRON_NAMESPACE_ANONYMOUS_EXIT

ActionRetCodeEnum
ViewerInstance::render(const RenderActionArgs& args)
{

    ImagePlaneDesc selectedLayer, selectedAlphaLayer, selectedDisplayLayer;
    int alphaChannelIndex;
    {
        ActionRetCodeEnum stat = _imp->getChannelOptions(args.time, &selectedLayer, &selectedAlphaLayer, &alphaChannelIndex, &selectedDisplayLayer);
        if (isFailureRetCode(stat)) {
            return stat;
        }
    }


    ImagePtr dstImage = args.outputPlanes.begin()->second;

    ImageBitDepthEnum bitdepth = getBitDepth(-1);
#ifdef DEBUG
    if (dstImage->getBitDepth() != bitdepth) {
        getNode()->setPersistentMessage(eMessageTypeError, kNatronPersistentErrorGenericRenderMessage, EffectInstance::tr("Host did not take into account requested bit depth.").toStdString());
        return eActionStatusFailed;
    }
#endif

    DisplayChannelsEnum displayChannels = (DisplayChannelsEnum)_imp->displayChannels.lock()->getValue();


    // Fetch the color and alpha image
    ImagePtr colorImage, alphaImage;
    if (displayChannels != eDisplayChannelsA) {
        GetImageOutArgs outArgs;
        GetImageInArgs inArgs(&args.mipMapLevel, &args.proxyScale, &args.roi, &args.backendType);
        inArgs.inputNb = 0;
        inArgs.plane = &selectedLayer;
        bool ok = getImagePlane(inArgs, &outArgs);
        if (ok) {
            colorImage = outArgs.image;
            alphaImage = colorImage;
        }
    }
    if (displayChannels == eDisplayChannelsA || displayChannels == eDisplayChannelsMatte) {

        if (selectedAlphaLayer == selectedLayer && colorImage) {
            alphaImage = colorImage;
        } else if (selectedAlphaLayer.getNumComponents() > 0) {
            GetImageOutArgs outArgs;
            GetImageInArgs inArgs(&args.mipMapLevel, &args.proxyScale, &args.roi, &args.backendType);
            inArgs.inputNb = 0;
            inArgs.plane = &selectedAlphaLayer;
            bool ok = getImagePlane(inArgs, &outArgs);
            if (!ok) {
                return eActionStatusFailed;
            }
            alphaImage = outArgs.image;
            if (!colorImage) {
                colorImage = alphaImage;
            }
        }
    }


    if (!colorImage && displayChannels != eDisplayChannelsA) {
        getNode()->setPersistentMessage(eMessageTypeError, kNatronPersistentErrorGenericRenderMessage, EffectInstance::tr("Could not fetch source image for selected layer.").toStdString());
        return eActionStatusFailed;
    }

    if (displayChannels == eDisplayChannelsA ||
        displayChannels == eDisplayChannelsMatte) {
        if (!alphaImage) {
            getNode()->setPersistentMessage(eMessageTypeError, kNatronPersistentErrorGenericRenderMessage, EffectInstance::tr("Could not fetch source image for selected alpha channel.").toStdString());
            return eActionStatusFailed;
        }
    }

    /*if (displayChannels != eDisplayChannelsA && colorImage && colorImage->getComponentsCount() != dstImage->getComponentsCount()) {
        getNode()->setPersistentMessage(eMessageTypeError, kNatronPersistentErrorGenericRenderMessage, tr("Host did not take into account requested bit-depth").toStdString());
        return eActionStatusFailed;
    }*/

    if (colorImage && alphaImage && colorImage->getBitDepth() != alphaImage->getBitDepth()) {
        getNode()->setPersistentMessage(eMessageTypeError, kNatronPersistentErrorGenericRenderMessage, EffectInstance::tr("Host did not take into account requested bit depth.").toStdString());
        return eActionStatusFailed;
    }

    RenderViewerArgs renderViewerArgs;
    renderViewerArgs.alphaChannelIndex = alphaChannelIndex;
    renderViewerArgs.renderArgs = shared_from_this();
    renderViewerArgs.channels = displayChannels;
    if (colorImage) {
        colorImage->getCPUData(&renderViewerArgs.colorImage);
    }
    if (alphaImage) {
        if (alphaImage == colorImage) {
            renderViewerArgs.alphaImage = renderViewerArgs.colorImage;
        } else {
            alphaImage->getCPUData(&renderViewerArgs.alphaImage);
        }
    }


    dstImage->getCPUData(&renderViewerArgs.dstImage);

    assert(colorImage->getBounds().contains(args.roi));
    assert(dstImage->getBounds().contains(args.roi));

    renderViewerArgs.gamma = _imp->gammaKnob.lock()->getValue();

    RamBuffer<float> gammaLut;
    ViewerInstancePrivate::buildGammaLut(renderViewerArgs.gamma, &gammaLut);
    renderViewerArgs.gammaLut = gammaLut.getData();

    bool doAutoContrast = _imp->autoContrastKnob.lock()->getValue();
    if (!doAutoContrast) {
        renderViewerArgs.gain = _imp->gainKnob.lock()->getValue();
        renderViewerArgs.gain = std::pow(2, renderViewerArgs.gain);
        renderViewerArgs.offset = 0;
    } else {

        FindAutoContrastProcessor processor(shared_from_this());
        processor.setValues(renderViewerArgs.colorImage, displayChannels);
#pragma message WARN("The viewer will always compute vmin/vmax for autocontrast on a region rounded to tile size. We should add a rectangle parameter specific to this feature.")
        processor.setRenderWindow(args.roi);
        ActionRetCodeEnum stat = processor.process();
        if (isFailureRetCode(stat)) {
            return stat;
        }

        MinMaxVal minMax = processor.getResults();

        if (minMax.max == minMax.min) {
            minMax.min = minMax.max - 1.;
        }

        if (minMax.max <= 0) {
            renderViewerArgs.gain = 0;
            renderViewerArgs.offset = 0;
        } else {
            renderViewerArgs.gain = 1 / (minMax.max - minMax.min);
            renderViewerArgs.offset =  -minMax.min / (minMax.max - minMax.min);
        }
    }

    renderViewerArgs.srcColorspace = lutFromColorspace(getApp()->getDefaultColorSpaceForBitDepth(getBitDepth(0)));
    renderViewerArgs.dstColorspace = lutFromColorspace((ViewerColorSpaceEnum)_imp->outputColorspace.lock()->getValue());


    ViewerProcessor processor(shared_from_this());
    processor.setValues(renderViewerArgs);
    processor.setRenderWindow(args.roi);
    ActionRetCodeEnum stat = processor.process();
    return stat;
} // render


NATRON_NAMESPACE_EXIT
