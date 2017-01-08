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

#include "ViewerInstance.h"

#include <algorithm> // min, max
#include <stdexcept>
#include <cassert>
#include <cstring> // for std::memcpy
#include <cfloat> // DBL_MAX



#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Image.h"
#include "Engine/Lut.h"
#include "Engine/NodeMetadata.h"
#include "Engine/Node.h"
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


NATRON_NAMESPACE_ENTER;


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
    KnobBoolWPtr autoContrastKnob;


    bool viewerChannelsAutoswitchedToAlpha;


    ViewerInstancePrivate(ViewerInstance* publicInterface)
    : _publicInterface(publicInterface)
    , layerChoiceKnob()
    , alphaChannelChoiceKnob()
    , displayChannels()
    , outputColorspace()
    , gammaKnob()
    , gainKnob()
    , autoContrastKnob()
    , viewerChannelsAutoswitchedToAlpha(false)
    {

    }


    static void buildGammaLut(double gamma, RamBuffer<float>* gammaLookup);

    static float lookupGammaLut(float value, const float* gammaLookupBuffer);

    void refreshLayerAndAlphaChannelComboBox();

    ImageComponents getSelectedLayer(const std::list<ImageComponents>& availableLayers) const;

    ImageComponents getSelectedAlphaChannel(const std::list<ImageComponents>& availableLayers, int *channelIndex) const;

    ImageComponents getComponentsFromDisplayChannels(const ImageComponents& alphaLayer) const;

    void getChannelOptions(const TreeRenderNodeArgsPtr& render, TimeValue time, ImageComponents* rgbLayer, ImageComponents* alphaLayer, int* alphaChannelIndex, ImageComponents* displayChannels) const;

    void setDisplayChannelsFromLayer(const std::list<ImageComponents>& availableLayers);
    
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
    PluginPtr ret = Plugin::create((void*)ViewerInstance::create, PLUGINID_NATRON_VIEWER_INTERNAL, "ViewerProcess", 1, 0, grouping);
    ret->setProperty<std::string>(kNatronPluginPropIconFilePath,  "Images/viewer_icon.png");
    QString desc =  tr("The Viewer node can display the output of a node graph.");
    ret->setProperty<bool>(kNatronPluginPropIsInternalOnly, true);
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    ret->setProperty<int>(kNatronPluginPropRenderSafety, (int)eRenderSafetyFullySafe);
    return ret;
}

ViewerInstance::ViewerInstance(const NodePtr& node)
    : EffectInstance(node)
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



std::string
ViewerInstance::getInputLabel(int /*inputNb*/) const
{
    return "Source";
}

int
ViewerInstance::getMaxInputCount() const
{
    return 1;
}

bool
ViewerInstance::isInputOptional(int /*n*/) const
{
    return false;
}

void
ViewerInstance::addAcceptedComponents(int /*inputNb*/,
                                      std::bitset<4>* supported)
{
    (*supported)[0] = (*supported)[1] = (*supported)[2] = (*supported)[3] = 1;
}



void
ViewerInstance::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthFloat);
    depths->push_back(eImageBitDepthShort);
    depths->push_back(eImageBitDepthByte);
}


void
ViewerInstancePrivate::buildGammaLut(double gamma, RamBuffer<float>* gammaLookup)
{
    // gammaLookupMutex should already be locked
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


bool
ViewerInstance::isMultiPlanar() const
{
    // We are multi-planar: we need to process multiple planes (The one selected in alpha + the output layer)
    return true;
}

EffectInstance::PassThroughEnum
ViewerInstance::isPassThroughForNonRenderedPlanes() const
{
    return ePassThroughPassThroughNonRenderedPlanes;
}

ActionRetCodeEnum
ViewerInstance::isIdentity(TimeValue time,
                           const RenderScale & /*scale*/,
                           const RectI & /*roi*/,
                           ViewIdx view,
                           const TreeRenderNodeArgsPtr& render,
                           TimeValue* inputTime,
                           ViewIdx* inputView,
                           int* inputNb)
{
    ImageComponents selectedLayer, selectedAlphaLayer;
    int alphaChannelIndex;
    _imp->getChannelOptions(render, time, &selectedLayer, &selectedAlphaLayer, &alphaChannelIndex, 0);
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

    if (args.hashType == HashableObject::eComputeHashTypeOnlyMetadataSlaves) {
        // We rely on the viewers bit depth setting knob in the getTimeInvariantMetaDatas() function
        // so make sure it is part of the hash.
        appPTR->getCurrentSettings()->getViewerBitDepthKnob()->appendToHash(args, hash);
    }
}

ActionRetCodeEnum
ViewerInstance::getTimeInvariantMetaDatas(NodeMetadata& metadata)
{

    ImageComponents selectedLayer, selectedAlphaLayer, selectedDisplayLayer;
    int alphaChannelIndex;
    _imp->getChannelOptions(TreeRenderNodeArgsPtr(), getCurrentTime_TLS(), &selectedLayer, &selectedAlphaLayer, &alphaChannelIndex, &selectedDisplayLayer);

    // In ouptut the color plane should be mapped to the display channels.
    metadata.setImageComponents(-1, selectedDisplayLayer);

    // Input should always be float, since we may do color-space conversion.
    metadata.setBitDepth(0, eImageBitDepthFloat);

    // Output however can be 8-bit
    ImageBitDepthEnum outputDepth = appPTR->getCurrentSettings()->getViewersBitDepth();
    metadata.setBitDepth(-1, outputDepth);


    _imp->refreshLayerAndAlphaChannelComboBox();
    getViewerNodeGroup()->onViewerProcessNodeMetadataRefreshed(getNode());

    return eActionStatusOK;
} // getTimeInvariantMetadatas

ActionRetCodeEnum
ViewerInstance::getComponentsAction(TimeValue time,
                                    ViewIdx view,
                                    const TreeRenderNodeArgsPtr& render,
                                    std::map<int, std::list<ImageComponents> >* inputLayersNeeded,
                                    std::list<ImageComponents>* layersProduced,
                                    TimeValue* passThroughTime,
                                    ViewIdx* passThroughView,
                                    int* passThroughInputNb)
{
    *passThroughInputNb = 0;
    *passThroughTime = time;
    *passThroughView = view;


    ImageComponents selectedLayer, selectedAlphaLayer, selectedDisplayLayer;
    int alphaChannelIndex;
    _imp->getChannelOptions(render, time, &selectedLayer, &selectedAlphaLayer, &alphaChannelIndex, &selectedDisplayLayer);

    layersProduced->push_back(selectedDisplayLayer);

    std::list<ImageComponents>& neededLayers = (*inputLayersNeeded)[0];
    if (selectedLayer.getNumComponents() > 0) {
        neededLayers.push_back(selectedLayer);
    }
    if (selectedAlphaLayer.getNumComponents() > 0 && selectedAlphaLayer != selectedLayer) {
        neededLayers.push_back(selectedAlphaLayer);
    }
    return eActionStatusOK;
} // getComponentsNeededInternal

void
ViewerInstancePrivate::getChannelOptions(const TreeRenderNodeArgsPtr& render, TimeValue time, ImageComponents* rgbLayer, ImageComponents* alphaLayer, int* alphaChannelIndex, ImageComponents* displayChannels) const
{
    std::list<ImageComponents> upstreamAvailableLayers;
    {
        const int passThroughPlanesInputNb = 0;
        ActionRetCodeEnum stat = _publicInterface->getAvailableLayers(time, ViewIdx(0), passThroughPlanesInputNb, render, &upstreamAvailableLayers);
        (void)stat;
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

} // getChannelOptions

ImageComponents
ViewerInstancePrivate::getComponentsFromDisplayChannels(const ImageComponents& alphaLayer) const
{
    DisplayChannelsEnum outputChannels = (DisplayChannelsEnum)displayChannels.lock()->getValue();
    switch (outputChannels) {
        case eDisplayChannelsA:
        case eDisplayChannelsR:
        case eDisplayChannelsG:
        case eDisplayChannelsB:
        case eDisplayChannelsY:
            return ImageComponents::getAlphaComponents();
            break;
        case eDisplayChannelsRGB: {
            if (alphaLayer.getNumComponents() == 0) {
                return ImageComponents::getRGBComponents();
            } else {
                return ImageComponents::getRGBAComponents();
            }

        }   break;
        case eDisplayChannelsMatte:
            return ImageComponents::getRGBAComponents();
            break;
    }

} // getComponentsFromDisplayChannels

void
ViewerInstancePrivate::refreshLayerAndAlphaChannelComboBox()
{
    ViewerNodePtr viewerGroup = _publicInterface->getViewerNodeGroup();

    KnobChoicePtr layerKnob = layerChoiceKnob.lock();
    KnobChoicePtr alphaChannelKnob = alphaChannelChoiceKnob.lock();


    std::list<ImageComponents> upstreamAvailableLayers;

    {
        const int passThroughPlanesInputNb = 0;
        ActionRetCodeEnum stat = _publicInterface->getAvailableLayers(_publicInterface->getCurrentTime_TLS(), ViewIdx(0), passThroughPlanesInputNb, TreeRenderNodeArgsPtr(), &upstreamAvailableLayers);
        (void)stat;
    }

    std::vector<ChoiceOption> layerOptions;
    std::vector<ChoiceOption> channelOptions;
    
    // Append None choice
    layerOptions.push_back(ChoiceOption("-", "",""));
    channelOptions.push_back(ChoiceOption("-", "", ""));

    int foundColorPlaneIndex = -1;

    for (std::list<ImageComponents>::iterator it2 = upstreamAvailableLayers.begin(); it2 != upstreamAvailableLayers.end(); ++it2) {

        if (foundColorPlaneIndex == -1 && it2->isColorPlane()) {
            foundColorPlaneIndex = (int)layerOptions.size();
        }

        ChoiceOption layerOption = it2->getLayerOption();
        layerOptions.push_back(layerOption);

        std::size_t nChans = (std::size_t)it2->getNumComponents();
        for (std::size_t c = 0; c < nChans; ++c) {
            ChoiceOption chanOption = it2->getChannelOption(c);
            channelOptions.push_back(chanOption);
        }
    }

    layerKnob->populateChoices(layerOptions);
    alphaChannelKnob->populateChoices(channelOptions);

    // If the old layer choice does no longer exist, fallback on color-plane
    if (!layerKnob->isActiveEntryPresentInEntries(ViewIdx(0))) {
        if (foundColorPlaneIndex != -1) {
            layerKnob->setValue(foundColorPlaneIndex);
        } else {
            // Did not find color-plane, fallback on the first plane available that is not none, otherwise none
            int fallback_i = layerOptions.size() > 1 ? 1 : 0;
            layerKnob->setValue(fallback_i);
        }
    }

    // If the old alpha channel choice does no longer exist, fallback on None
    if (!alphaChannelKnob->isActiveEntryPresentInEntries(ViewIdx(0))) {
        alphaChannelKnob->setValue(0);
    }

    setDisplayChannelsFromLayer(upstreamAvailableLayers);

} // refreshLayerAndAlphaChannelComboBox


ImageComponents
ViewerInstancePrivate::getSelectedLayer(const std::list<ImageComponents>& availableLayers) const
{
    std::string activeIndexID = layerChoiceKnob.lock()->getActiveEntryID(ViewIdx(0));
    for (std::list<ImageComponents>::const_iterator it = availableLayers.begin(); it != availableLayers.end(); ++it) {
        ChoiceOption opt = it->getLayerOption();
        if (opt.id == activeIndexID) {
            return *it;
        }
    }
    return ImageComponents::getNoneComponents();

} // getSelectedLayer

ImageComponents
ViewerInstancePrivate::getSelectedAlphaChannel(const std::list<ImageComponents>& availableLayers, int *channelIndex) const
{
    std::string activeIndexID = alphaChannelChoiceKnob.lock()->getActiveEntryID(ViewIdx(0));
    for (std::list<ImageComponents>::const_iterator it = availableLayers.begin(); it != availableLayers.end(); ++it) {
        std::size_t nChans = (std::size_t)it->getNumComponents();
        for (std::size_t c = 0; c < nChans; ++c) {
            ChoiceOption chanOption = it->getChannelOption(c);
            if (chanOption.id == activeIndexID) {
                *channelIndex = (int)c;
                return *it;
            }
        }
    }
    *channelIndex = -1;
    return ImageComponents::getNoneComponents();
} // getSelectedAlphaChannel

void
ViewerInstancePrivate::setDisplayChannelsFromLayer(const std::list<ImageComponents>& availableLayers)
{
    ImageComponents layer = getSelectedLayer(availableLayers);
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
ViewerInstance::supportsTiles() const
{
    // When computing auto-contrast we need the full image.
    bool autoContrastEnabled = _imp->autoContrastKnob.lock()->getValue();
    return !autoContrastEnabled;
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
MinMaxVal
findAutoContrastVminVmax_generic(const Image::CPUTileData& colorImage,
                                 const TreeRenderNodeArgsPtr& renderArgs,
                                 const RectI & roi)
{
    double localVmin = std::numeric_limits<double>::infinity();
    double localVmax = -std::numeric_limits<double>::infinity();

    for (int y = roi.y1; y < roi.y2; ++y) {

        if (renderArgs && renderArgs->isRenderAborted()) {
            return MinMaxVal(localVmin, localVmax);
        }

        int pixelStride;
        const PIX* src_pixels[4];
        Image::getChannelPointers<PIX, srcNComps>((const PIX**)colorImage.ptrs, roi.x1, y, colorImage.tileBounds, (PIX**)src_pixels, &pixelStride);

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

    return MinMaxVal(localVmin, localVmax);
} // findAutoContrastVminVmax_generic

template <typename PIX, int maxValue, int srcNComps>
MinMaxVal
findAutoContrastVminVmaxForComponents(const Image::CPUTileData& colorImage,
                                      const TreeRenderNodeArgsPtr& renderArgs,
                                      DisplayChannelsEnum channels,
                                      const RectI & roi)
    

{
    switch (channels) {
        case eDisplayChannelsA:
            return findAutoContrastVminVmax_generic<PIX, maxValue, srcNComps, eDisplayChannelsA>(colorImage, renderArgs, roi);
        case eDisplayChannelsR:
            return findAutoContrastVminVmax_generic<PIX, maxValue, srcNComps, eDisplayChannelsR>(colorImage, renderArgs, roi);
        case eDisplayChannelsG:
            return findAutoContrastVminVmax_generic<PIX, maxValue, srcNComps, eDisplayChannelsG>(colorImage, renderArgs, roi);
        case eDisplayChannelsB:
            return findAutoContrastVminVmax_generic<PIX, maxValue, srcNComps, eDisplayChannelsB>(colorImage, renderArgs, roi);
        case eDisplayChannelsY:
            return findAutoContrastVminVmax_generic<PIX, maxValue, srcNComps, eDisplayChannelsY>(colorImage, renderArgs, roi);
        case eDisplayChannelsMatte:
            return findAutoContrastVminVmax_generic<PIX, maxValue, srcNComps, eDisplayChannelsMatte>(colorImage, renderArgs, roi);
        case eDisplayChannelsRGB:
            return findAutoContrastVminVmax_generic<PIX, maxValue, srcNComps, eDisplayChannelsRGB>(colorImage, renderArgs, roi);
    }
}


template <typename PIX, int maxValue>
MinMaxVal
findAutoContrastVminVmaxForDepth(const Image::CPUTileData& colorImage,
                                 const TreeRenderNodeArgsPtr& renderArgs,
                                 DisplayChannelsEnum channels,
                                 const RectI & roi)


{
    switch (colorImage.nComps) {
        case 1:
            return findAutoContrastVminVmaxForComponents<PIX, maxValue, 1>(colorImage, renderArgs, channels, roi);
        case 2:
            return findAutoContrastVminVmaxForComponents<PIX, maxValue, 2>(colorImage, renderArgs, channels, roi);
        case 3:
            return findAutoContrastVminVmaxForComponents<PIX, maxValue, 3>(colorImage, renderArgs, channels, roi);
        case 4:
            return findAutoContrastVminVmaxForComponents<PIX, maxValue, 4>(colorImage, renderArgs, channels, roi);
        default:
            assert(false);
            return MinMaxVal(0,0);
    }
}

MinMaxVal
findAutoContrastVminVmax(const Image::CPUTileData& colorImage,
                         const TreeRenderNodeArgsPtr& renderArgs,
                         DisplayChannelsEnum channels,
                         const RectI & roi)
{
    switch (colorImage.bitDepth) {
        case eImageBitDepthByte:
            return findAutoContrastVminVmaxForDepth<unsigned char, 255>(colorImage, renderArgs, channels, roi);
        case eImageBitDepthFloat:
            return findAutoContrastVminVmaxForDepth<float, 1>(colorImage, renderArgs, channels, roi);
        case eImageBitDepthHalf:
        case eImageBitDepthNone:
            return MinMaxVal(0,0);
        case eImageBitDepthShort:
            return findAutoContrastVminVmaxForDepth<unsigned short, 65535>(colorImage, renderArgs, channels, roi);
    }
}

class FindAutoContrastProcessor : public ImageMultiThreadProcessorBase
{
    Image::CPUTileData _colorImage;
    DisplayChannelsEnum _channels;

    mutable QMutex _resultMutex;
    MinMaxVal _result;

public:

    FindAutoContrastProcessor(const TreeRenderNodeArgsPtr& renderArgs)
    : ImageMultiThreadProcessorBase(renderArgs)
    {
        _result.min = std::numeric_limits<double>::infinity();
        _result.max = -std::numeric_limits<double>::infinity();
    }

    virtual ~FindAutoContrastProcessor()
    {
    }

    void setValues(const Image::CPUTileData& colorImage, DisplayChannelsEnum channels)
    {
        _colorImage = colorImage;
        _channels = channels;
    }

    MinMaxVal getResults() const
    {
        return _result;
    }

private:

    virtual ActionRetCodeEnum multiThreadProcessImages(const RectI& renderWindow, const TreeRenderNodeArgsPtr& renderArgs) OVERRIDE FINAL
    {
        MinMaxVal localResult = findAutoContrastVminVmax(_colorImage, renderArgs, _channels, renderWindow);

        QMutexLocker k(&_resultMutex);
        _result.min = std::min(_result.min, localResult.min);
        _result.max = std::max(_result.max, localResult.max);
        return eActionStatusOK;
    }
};

struct RenderViewerArgs
{
    Image::CPUTileData colorImage, alphaImage, dstImage;
    int alphaChannelIndex;
    TreeRenderNodeArgsPtr renderArgs;
    double gamma, gain, offset;
    DisplayChannelsEnum channels;
    const Color::Lut* srcColorspace;
    const Color::Lut* dstColorspace;
    const float* gammaLut;
};


/**
 *@brief Actually converting to ARGB... but it is called BGRA by
 the texture format GL_UNSIGNED_INT_8_8_8_8_REV
 **/
static
unsigned int
toBGRA(unsigned char r,
       unsigned char g,
       unsigned char b,
       unsigned char a)
{
    return (a << 24) | (r << 16) | (g << 8) | b;
}

template <typename PIX, int maxValue, int srcNComps, DisplayChannelsEnum channels>
void
genericViewerProcessFunctor(const RenderViewerArgs& args,
                            const PIX* color_pixels[4],
                            const PIX* alpha_pixels[4],
                            float tmpPix[4],
                            double* alphaMatteValue)
{
    memset(tmpPix, 0, sizeof(float) * 4);

    for (int i = 0; i < 3; ++i) {
        if (color_pixels[i]) {
            tmpPix[i] = Image::convertPixelDepth<PIX, float>(*color_pixels[i]);
        }
    }

    // Get the alpha from the alpha image
    if (alpha_pixels[args.alphaChannelIndex]) {
        tmpPix[3] = Image::convertPixelDepth<PIX, float>(*alpha_pixels[args.alphaChannelIndex]);
    }
    if (srcNComps == 1) {
        // Single-channel image: replicate the single channel to all RGB channels
        for (int i = 0; i < 3; ++i) {
            tmpPix[i] = tmpPix[3];
        }
    }

    // If the image has a color space, convert to linear float first
    if (args.srcColorspace) {
        tmpPix[0] = args.srcColorspace->fromColorSpaceFloatToLinearFloat(tmpPix[0]);
        tmpPix[1] = args.srcColorspace->fromColorSpaceFloatToLinearFloat(tmpPix[1]);
        tmpPix[2] = args.srcColorspace->fromColorSpaceFloatToLinearFloat(tmpPix[2]);
    }


    // Apply gain gamma and offset to the RGB channels
    for (int i = 0; i < 3; ++i) {
        tmpPix[i] = tmpPix[i] * args.gain + args.offset;
    }
    if (args.gamma <= 0) {
        for (int i = 0; i < 3; ++i) {
            tmpPix[i] = (tmpPix[i]  < 1.) ? 0. : (tmpPix[i]  == 1. ? 1. : std::numeric_limits<double>::infinity() );
        }
    } else {
        for (int i = 0; i < 3; ++i) {
            tmpPix[i] = ViewerInstancePrivate::lookupGammaLut(tmpPix[i], args.gammaLut);
        }
    }

    if (channels == eDisplayChannelsY) {
        tmpPix[0] = 0.299 * tmpPix[1] + 0.587 * tmpPix[2] + 0.114 * tmpPix[3];
        tmpPix[1] = tmpPix[0];
        tmpPix[2] = tmpPix[0];
    }
    if (channels == eDisplayChannelsMatte) {
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

template <typename PIX, int maxValue, int srcNComps, DisplayChannelsEnum channels>
void
applyViewerProcess8bit_generic(const RenderViewerArgs& args, const RectI & roi)
{


    for (int y = roi.y1; y < roi.y2; ++y) {

        // Check for abort on every scan-line
        if (args.renderArgs && args.renderArgs->isRenderAborted()) {
            return;
        }

        // For error diffusion, we start at each line at a random pixel along the line so it does
        // not create a pattern in the output image.
        const int startX = (int)( rand() % (roi.x2 - roi.x1) );

        for (int backward = 0; backward < 2; ++backward) {

            int x = backward ? startX - 1 : startX;

            const int endX = backward ? -1 : roi.x2;

            unsigned error[3] = {0x80, 0x80, 0x80};


            int colorPixelStride;
            const PIX* color_pixels[4];
            Image::getChannelPointers<PIX, srcNComps>((const PIX**)args.colorImage.ptrs, x, y, args.colorImage.tileBounds, (PIX**)color_pixels, &colorPixelStride);

            int alphaPixelStride;
            const PIX* alpha_pixels[4];
            Image::getChannelPointers<PIX>((const PIX**)args.alphaImage.ptrs, x, y, args.alphaImage.tileBounds, args.alphaImage.nComps, (PIX**)alpha_pixels, &alphaPixelStride);

            int dstPixelStride;
            unsigned char* dst_pixels[4];
            Image::getChannelPointers<PIX, srcNComps>((unsigned char**)args.dstImage.ptrs, x, y, args.dstImage.tileBounds, (unsigned char**)dst_pixels, &dstPixelStride);

            while (x != endX) {
                float tmpPix[4];
                double alphaMatteValue;
                genericViewerProcessFunctor<PIX, maxValue, srcNComps, channels>(args, color_pixels, alpha_pixels, tmpPix, &alphaMatteValue);

                U8 uTmpPix[4];
                if (!args.dstColorspace) {
                    for (int i = 0 ; i < 4; ++i) {
                        uTmpPix[i] = Color::floatToInt<256>(tmpPix[i]);
                    }
                } else {
                    for (int i = 0; i < 3; ++i) {
                        error[i] = (error[i] & 0xff) + args.dstColorspace->toColorSpaceUint8xxFromLinearFloatFast(tmpPix[i]);
                        assert(error[i] < 0x10000);
                        uTmpPix[i] = (U8)(error[i] >> 8);
                    }
                    uTmpPix[3] = Color::floatToInt<256>(tmpPix[3]);
                }


                if (channels == eDisplayChannelsMatte) {
                    U8 matteA;
                    if (args.dstColorspace) {
                        matteA = args.dstColorspace->toColorSpaceUint8FromLinearFloatFast(alphaMatteValue) / 2;
                    } else {
                        matteA = Color::floatToInt<256>(alphaMatteValue) / 2;
                    }

                    // Add to the red channel the matte value
                    uTmpPix[0] = Image::clampIfInt<U8>( (double)uTmpPix[0] + matteA );
                }

                *reinterpret_cast<unsigned int*>(dst_pixels[0]) = toBGRA(uTmpPix[0], uTmpPix[1], uTmpPix[2], uTmpPix[3]);

                for (int i = 0; i < 4; ++i) {
                    if (color_pixels[i]) {
                        color_pixels[i] += colorPixelStride;
                    }
                    if (alpha_pixels[i]) {
                        alpha_pixels[i] += alphaPixelStride;
                    }
                    if (dst_pixels[i]) {
                        dst_pixels[i] += dstPixelStride;
                    }
                }

                if (backward) {
                    --x;
                } else {
                    ++x;
                }

            } // for each pixels on the line

        } // backward?

    } // for each scan-line

} // applyViewerProcess8bit_generic

template <typename PIX, int maxValue, int srcNComps>
void
applyViewerProcess8bitForComponents(const RenderViewerArgs& args, const RectI & roi)
{
    switch (args.channels) {
        case eDisplayChannelsA:
            applyViewerProcess8bit_generic<PIX, maxValue, srcNComps, eDisplayChannelsA>(args, roi);
            break;
        case eDisplayChannelsR:
            applyViewerProcess8bit_generic<PIX, maxValue, srcNComps, eDisplayChannelsR>(args, roi);
            break;
        case eDisplayChannelsG:
            applyViewerProcess8bit_generic<PIX, maxValue, srcNComps, eDisplayChannelsG>(args, roi);
            break;
        case eDisplayChannelsB:
            applyViewerProcess8bit_generic<PIX, maxValue, srcNComps, eDisplayChannelsB>(args, roi);
            break;
        case eDisplayChannelsRGB:
            applyViewerProcess8bit_generic<PIX, maxValue, srcNComps, eDisplayChannelsRGB>(args, roi);
            break;
        case eDisplayChannelsY:
            applyViewerProcess8bit_generic<PIX, maxValue, srcNComps, eDisplayChannelsY>(args, roi);
            break;
        case eDisplayChannelsMatte:
            applyViewerProcess8bit_generic<PIX, maxValue, srcNComps, eDisplayChannelsMatte>(args, roi);
            break;
    }
}

template <typename PIX, int maxValue>
void
applyViewerProcess8bitForDepth(const RenderViewerArgs& args, const RectI & roi)
{
    switch (args.colorImage.nComps) {
        case 1:
            applyViewerProcess8bitForComponents<PIX, maxValue, 1>(args, roi);
            break;
        case 2:
            applyViewerProcess8bitForComponents<PIX, maxValue, 2>(args, roi);
            break;
        case 3:
            applyViewerProcess8bitForComponents<PIX, maxValue, 3>(args, roi);
            break;
        case 4:
            applyViewerProcess8bitForComponents<PIX, maxValue, 4>(args, roi);
            break;
        default:
            break;
    }
}


static void
applyViewerProcess8bit(const RenderViewerArgs& args, const RectI & roi)
{
    switch ( args.colorImage.bitDepth ) {
        case eImageBitDepthFloat:
            applyViewerProcess8bitForDepth<float, 1>(args, roi);
            break;
        case eImageBitDepthByte:
            applyViewerProcess8bitForDepth<unsigned char, 255>(args, roi);
            break;
        case eImageBitDepthShort:
            applyViewerProcess8bitForDepth<unsigned short, 65535>(args, roi);
            break;
        case eImageBitDepthHalf:
            assert(false);
            break;
        case eImageBitDepthNone:
            break;
    }
}



template <typename PIX, int maxValue, int srcNComps, DisplayChannelsEnum channels>
void
applyViewerProcess32bit_Generic(const RenderViewerArgs& args, const RectI & roi)
{


    for (int y = roi.y1; y < roi.y2; ++y) {

        // Check for abort on every scan-line
        if (args.renderArgs && args.renderArgs->isRenderAborted()) {
            return;
        }


        int colorPixelStride;
        const PIX* color_pixels[4];
        Image::getChannelPointers<PIX, srcNComps>((const PIX**)args.colorImage.ptrs, roi.x1, y, args.colorImage.tileBounds, (PIX**)color_pixels, &colorPixelStride);

        int alphaPixelStride;
        const PIX* alpha_pixels[4];
        Image::getChannelPointers<PIX>((const PIX**)args.alphaImage.ptrs, roi.x1, y, args.alphaImage.tileBounds, args.alphaImage.nComps, (PIX**)alpha_pixels, &alphaPixelStride);

        int dstPixelStride;
        float* dst_pixels[4];
        Image::getChannelPointers<PIX, srcNComps>((float**)args.dstImage.ptrs, roi.x1, y, args.dstImage.tileBounds, (float**)dst_pixels, &dstPixelStride);

        for (int x = roi.x1; x < roi.x2; ++x) {

            float tmpPix[4];
            double alphaMatteValue;
            genericViewerProcessFunctor<PIX, maxValue, srcNComps, channels>(args, color_pixels, alpha_pixels, tmpPix, &alphaMatteValue);
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

} // applyViewerProcess32bit_Generic

template <typename PIX, int maxValue, int srcNComps>
void
applyViewerProcess32bitForComponents(const RenderViewerArgs& args, const RectI & roi)
{
    switch (args.channels) {
        case eDisplayChannelsA:
            applyViewerProcess32bit_Generic<PIX, maxValue, srcNComps, eDisplayChannelsA>(args, roi);
            break;
        case eDisplayChannelsR:
            applyViewerProcess32bit_Generic<PIX, maxValue, srcNComps, eDisplayChannelsR>(args, roi);
            break;
        case eDisplayChannelsG:
            applyViewerProcess32bit_Generic<PIX, maxValue, srcNComps, eDisplayChannelsG>(args, roi);
            break;
        case eDisplayChannelsB:
            applyViewerProcess32bit_Generic<PIX, maxValue, srcNComps, eDisplayChannelsB>(args, roi);
            break;
        case eDisplayChannelsRGB:
            applyViewerProcess32bit_Generic<PIX, maxValue, srcNComps, eDisplayChannelsRGB>(args, roi);
            break;
        case eDisplayChannelsY:
            applyViewerProcess32bit_Generic<PIX, maxValue, srcNComps, eDisplayChannelsY>(args, roi);
            break;
        case eDisplayChannelsMatte:
            applyViewerProcess32bit_Generic<PIX, maxValue, srcNComps, eDisplayChannelsMatte>(args, roi);
            break;
    }
}

template <typename PIX, int maxValue>
void
applyViewerProcess32bitForDepth(const RenderViewerArgs& args, const RectI & roi)
{
    switch (args.colorImage.nComps) {
        case 1:
            applyViewerProcess32bitForComponents<PIX, maxValue, 1>(args, roi);
            break;
        case 2:
            applyViewerProcess32bitForComponents<PIX, maxValue, 2>(args, roi);
            break;
        case 3:
            applyViewerProcess32bitForComponents<PIX, maxValue, 3>(args, roi);
            break;
        case 4:
            applyViewerProcess32bitForComponents<PIX, maxValue, 4>(args, roi);
            break;
        default:
            break;
    }
}


static void
applyViewerProcess32bit(const RenderViewerArgs& args, const RectI & roi)
{
    switch ( args.colorImage.bitDepth ) {
        case eImageBitDepthFloat:
            applyViewerProcess32bitForDepth<float, 1>(args, roi);
            break;
        case eImageBitDepthByte:
            applyViewerProcess32bitForDepth<unsigned char, 255>(args, roi);
            break;
        case eImageBitDepthShort:
            applyViewerProcess32bitForDepth<unsigned short, 65535>(args, roi);
            break;
        case eImageBitDepthHalf:
            assert(false);
            break;
        case eImageBitDepthNone:
            break;
    }
}

class ViewerProcessor : public ImageMultiThreadProcessorBase
{
    RenderViewerArgs _args;

public:

    ViewerProcessor(const TreeRenderNodeArgsPtr& renderArgs)
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

    virtual ActionRetCodeEnum multiThreadProcessImages(const RectI& renderWindow, const TreeRenderNodeArgsPtr& /*renderArgs*/) OVERRIDE FINAL
    {
        if (_args.dstImage.bitDepth == eImageBitDepthFloat) {
            applyViewerProcess32bit(_args, renderWindow);
        } else if (_args.dstImage.bitDepth == eImageBitDepthByte) {
            applyViewerProcess8bit(_args, renderWindow);
        } else {
            throw std::runtime_error("Unsupported bit-depth");
            assert(false);
            return eActionStatusFailed;
        }
        return eActionStatusOK;
    }
};

NATRON_NAMESPACE_ANONYMOUS_EXIT

ActionRetCodeEnum
ViewerInstance::render(const RenderActionArgs& args)
{

    ImageComponents selectedLayer, selectedAlphaLayer, selectedDisplayLayer;
    int alphaChannelIndex;
    _imp->getChannelOptions(args.renderArgs, args.time, &selectedLayer, &selectedAlphaLayer, &alphaChannelIndex, &selectedDisplayLayer);


    if (args.outputPlanes.size() != 1 || selectedDisplayLayer != args.outputPlanes.begin()->first) {
        setPersistentMessage(eMessageTypeError, tr("Host did not take into account output components").toStdString());
        return eActionStatusFailed;
    }

    ImagePtr dstImage = args.outputPlanes.begin()->second;


    DisplayChannelsEnum displayChannels = (DisplayChannelsEnum)_imp->displayChannels.lock()->getValue();

    std::list<ImageComponents> layersToFetch;
    if (selectedLayer.getNumComponents() > 0) {
        layersToFetch.push_back(selectedLayer);
    }
    if (selectedAlphaLayer.getNumComponents() > 0 && selectedAlphaLayer != selectedLayer) {
        layersToFetch.push_back(selectedAlphaLayer);
    }

    // Fetch the color and alpha image
    ImagePtr colorImage, alphaImage;
    {
        GetImageOutArgs outArgs;
        GetImageInArgs inArgs(args);
        inArgs.inputNb = 0;
        inArgs.layers = &layersToFetch;
        bool ok = getImagePlanes(inArgs, &outArgs);
        if (!ok) {
            return eActionStatusFailed;
        }
        {
            std::map<ImageComponents, ImagePtr>::iterator foundLayer = outArgs.imagePlanes.find(selectedLayer);
            if (foundLayer != outArgs.imagePlanes.end()) {
                colorImage = foundLayer->second;
            }
        }
        {
            std::map<ImageComponents, ImagePtr>::iterator foundAlphaLayer = outArgs.imagePlanes.find(selectedAlphaLayer);
            if (foundAlphaLayer != outArgs.imagePlanes.end()) {
                alphaImage = foundAlphaLayer->second;
            }
        }
    }

    if (!colorImage) {
        setPersistentMessage(eMessageTypeError, tr("Could not fetch source image for selected layer").toStdString());
        return eActionStatusFailed;
    }

    if (displayChannels == eDisplayChannelsA ||
        displayChannels == eDisplayChannelsMatte) {
        if (!alphaImage) {
            setPersistentMessage(eMessageTypeError, tr("Could not fetch source image for selected alpha channel").toStdString());
            return eActionStatusFailed;
        }
    }

    if (colorImage->getBitDepth() != alphaImage->getBitDepth()) {
        setPersistentMessage(eMessageTypeError, tr("Host did not take into account requested bit-depth").toStdString());
        return eActionStatusFailed;
    }

    RenderViewerArgs renderViewerArgs;
    renderViewerArgs.alphaChannelIndex = alphaChannelIndex;
    renderViewerArgs.renderArgs = args.renderArgs;
    renderViewerArgs.channels = displayChannels;
    if (colorImage) {
        Image::Tile tile;
        colorImage->getTileAt(0, &tile);
        colorImage->getCPUTileData(tile, &renderViewerArgs.colorImage);
    }
    if (alphaImage) {
        Image::Tile tile;
        alphaImage->getTileAt(0, &tile);
        alphaImage->getCPUTileData(tile, &renderViewerArgs.alphaImage);
    }

    {
        Image::Tile dstTile;
        dstImage->getTileAt(0, &dstTile);
        dstImage->getCPUTileData(dstTile, &renderViewerArgs.dstImage);
    }


    renderViewerArgs.gamma = _imp->gammaKnob.lock()->getValue();

    RamBuffer<float> gammaLut;
    ViewerInstancePrivate::buildGammaLut(renderViewerArgs.gamma, &gammaLut);
    renderViewerArgs.gammaLut = gammaLut.getData();

    bool doAutoContrast = _imp->autoContrastKnob.lock()->getValue();
    if (!doAutoContrast) {
        renderViewerArgs.gain = _imp->gainKnob.lock()->getValue();
        renderViewerArgs.offset = 0;
    } else {

        FindAutoContrastProcessor processor(args.renderArgs);
        processor.setValues(renderViewerArgs.colorImage, displayChannels);
        processor.setRenderWindow(args.roi);
        processor.process();

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

    renderViewerArgs.srcColorspace = lutFromColorspace(getApp()->getDefaultColorSpaceForBitDepth(renderViewerArgs.colorImage.bitDepth));
    renderViewerArgs.dstColorspace = lutFromColorspace((ViewerColorSpaceEnum)_imp->outputColorspace.lock()->getValue());


    ViewerProcessor processor(args.renderArgs);
    processor.setValues(renderViewerArgs);
    processor.setRenderWindow(args.roi);
    processor.process();

    return eActionStatusOK;
} // render


NATRON_NAMESPACE_EXIT;

