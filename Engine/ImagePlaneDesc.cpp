/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "ImagePlaneDesc.h"

#include <ofxNatron.h>

#include <cassert>
#include <sstream>
#include <stdexcept>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Serialization/NodeSerialization.h"


NATRON_NAMESPACE_ENTER

static const char* rgbaComps[4] = {"R", "G", "B", "A"};
static const char* rgbComps[3] = {"R", "G", "B"};
static const char* alphaComps[1] = {"A"};
static const char* motionComps[2] = {"U", "V"};
static const char* disparityComps[2] = {"X", "Y"};
static const char* xyComps[2] = {"X", "Y"};


ImagePlaneDesc::ImagePlaneDesc()
    : _planeID("none")

    , _planeLabel("none")
    , _channels()
    , _channelsLabel("none")

{
}

ImagePlaneDesc::ImagePlaneDesc(const std::string& planeID,
                               const std::string& planeLabel,
                               const std::string& channelsLabel,
                               const std::vector<std::string>& channels)
: _planeID(planeID)
, _planeLabel(planeLabel)
, _channels(channels)
, _channelsLabel(channelsLabel)
{
    if (planeLabel.empty()) {
        // Plane label is the ID if empty
        _planeLabel = _planeID;
    }
    if ( channelsLabel.empty() ) {
        // Channels label is the concatenation of all channels
        for (std::size_t i = 0; i < channels.size(); ++i) {
            _channelsLabel.append(channels[i]);
        }
    }
}

ImagePlaneDesc::ImagePlaneDesc(const std::string& planeName,
                               const std::string& planeLabel,
                               const std::string& channelsLabel,
                               const char** channels,
                               int count)
: _planeID(planeName)
, _planeLabel(planeLabel)
, _channels()
, _channelsLabel(channelsLabel)
{
    _channels.resize(count);
    for (int i = 0; i < count; ++i) {
        _channels[i] = channels[i];
    }

    if (planeLabel.empty()) {
        // Plane label is the ID if empty
        _planeLabel = _planeID;
    }
    if ( channelsLabel.empty() ) {
        // Channels label is the concatenation of all channels
        for (std::size_t i = 0; i < _channels.size(); ++i) {
            _channelsLabel.append(channels[i]);
        }
    }
}

ImagePlaneDesc::ImagePlaneDesc(const ImagePlaneDesc& other)
{
    *this = other;
}

ImagePlaneDesc&
ImagePlaneDesc::operator=(const ImagePlaneDesc& other)
{
    _planeID = other._planeID;
    _planeLabel = other._planeLabel;
    _channels = other._channels;
    _channelsLabel = other._channelsLabel;
    return *this;
}

ImagePlaneDesc::~ImagePlaneDesc()
{
}

bool
ImagePlaneDesc::isColorPlane(const std::string& planeID)
{
    return planeID == kNatronColorPlaneID;
}

bool
ImagePlaneDesc::isColorPlane() const
{
    return ImagePlaneDesc::isColorPlane(_planeID);
}



bool
ImagePlaneDesc::operator==(const ImagePlaneDesc& other) const
{
    if ( _channels.size() != other._channels.size() ) {
        return false;
    }
    return _planeID == other._planeID;
}

bool
ImagePlaneDesc::operator<(const ImagePlaneDesc& other) const
{
    return _planeID < other._planeID;
}

int
ImagePlaneDesc::getNumComponents() const
{
    return (int)_channels.size();
}

const std::string&
ImagePlaneDesc::getPlaneID() const
{
    return _planeID;
}

const std::string&
ImagePlaneDesc::getPlaneLabel() const
{
    return _planeLabel;
}

const std::string&
ImagePlaneDesc::getChannelsLabel() const
{
    return _channelsLabel;
}

const std::vector<std::string>&
ImagePlaneDesc::getChannels() const
{
    return _channels;
}

const ImagePlaneDesc&
ImagePlaneDesc::getNoneComponents()
{
    static const ImagePlaneDesc comp;
    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getRGBAComponents()
{
    static const ImagePlaneDesc comp(kNatronColorPlaneID, kNatronColorPlaneLabel, "", rgbaComps, 4);

    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getRGBComponents()
{
    static const ImagePlaneDesc comp(kNatronColorPlaneID, kNatronColorPlaneLabel, "", rgbComps, 3);

    return comp;
}


const ImagePlaneDesc&
ImagePlaneDesc::getXYComponents()
{
    static const ImagePlaneDesc comp(kNatronColorPlaneID, kNatronColorPlaneLabel, "XY", xyComps, 2);

    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getAlphaComponents()
{
    static const ImagePlaneDesc comp(kNatronColorPlaneID, kNatronColorPlaneLabel, "Alpha", alphaComps, 1);

    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getBackwardMotionComponents()
{
    static const ImagePlaneDesc comp(kNatronBackwardMotionVectorsPlaneID, kNatronBackwardMotionVectorsPlaneLabel, kNatronMotionComponentsLabel, motionComps, 2);

    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getForwardMotionComponents()
{
    static const ImagePlaneDesc comp(kNatronForwardMotionVectorsPlaneID, kNatronForwardMotionVectorsPlaneLabel, kNatronMotionComponentsLabel, motionComps, 2);

    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getDisparityLeftComponents()
{
    static const ImagePlaneDesc comp(kNatronDisparityLeftPlaneID, kNatronDisparityLeftPlaneLabel, kNatronDisparityComponentsLabel, disparityComps, 2);

    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getDisparityRightComponents()
{
    static const ImagePlaneDesc comp(kNatronDisparityRightPlaneID, kNatronDisparityRightPlaneLabel, kNatronDisparityComponentsLabel, disparityComps, 2);

    return comp;
}



void
ImagePlaneDesc::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj)
{
    SERIALIZATION_NAMESPACE::ImagePlaneDescSerialization* s = dynamic_cast<SERIALIZATION_NAMESPACE::ImagePlaneDescSerialization*>(obj);
    if (!s) {
        return;
    }
    s->planeID = _planeID;
    s->planeLabel = _planeLabel;
    s->channelsLabel = _channelsLabel;
    s->channelNames = _channels;
}

void
ImagePlaneDesc::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase & obj)
{
    const SERIALIZATION_NAMESPACE::ImagePlaneDescSerialization* s = dynamic_cast<const SERIALIZATION_NAMESPACE::ImagePlaneDescSerialization*>(&obj);
    if (!s) {
        return;
    }
    _planeID = s->planeID;
    _planeLabel = s->planeLabel;
    _channelsLabel = s->channelsLabel;
    _channels = s->channelNames;

}

ChoiceOption
ImagePlaneDesc::getChannelOption(int channelIndex) const
{
    if (channelIndex < 0 || channelIndex >= (int)_channels.size()) {
        assert(false);
        return ChoiceOption("","","");
    }
    std::string optionID, optionLabel;
    optionLabel += _planeLabel;
    optionID += _planeID;
    if ( !optionLabel.empty() ) {
        optionLabel += '.';
    }
    if (!optionID.empty()) {
        optionID += '.';
    }

    // For the option label, append the name of the channel
    optionLabel += _channels[channelIndex];
    optionID += _channels[channelIndex];

    return ChoiceOption(optionID, optionLabel, "");
}

ChoiceOption
ImagePlaneDesc::getPlaneOption() const
{
    std::string optionLabel = _planeLabel + "." + _channelsLabel;

    // The option ID is always the name of the layer, this ensures for the Color plane that even if the components type changes, the choice stays
    // the same in the parameter.
    return ChoiceOption(_planeID, optionLabel, "");

}

const ImagePlaneDesc&
ImagePlaneDesc::mapNCompsToColorPlane(int nComps)
{
    switch (nComps) {
        case 1:
            return ImagePlaneDesc::getAlphaComponents();
        case 2:
            return ImagePlaneDesc::getXYComponents();
        case 3:
            return ImagePlaneDesc::getRGBComponents();
        case 4:
            return ImagePlaneDesc::getRGBAComponents();
        default:
            return ImagePlaneDesc::getNoneComponents();
    }
}

static bool
extractOFXEncodedCustomPlane(const std::string& comp, std::string* layerName, std::string* layerLabel, std::string* channelsLabel, std::vector<std::string>* channels)
{

    // Find the plane unique identifier
    const std::size_t foundPlaneLen = std::strlen(kNatronOfxImageComponentsPlaneName);
    std::size_t foundPlane = comp.find(kNatronOfxImageComponentsPlaneName);
    if (foundPlane == std::string::npos) {
        return false;
    }

    const std::size_t planeNameStartIdx = foundPlane + foundPlaneLen;

    // Find the optional plane label
    // If planeLabelStartIdx = 0, there's no plane label.
    std::size_t planeLabelStartIdx = 0;


    const std::size_t foundPlaneLabelLen = std::strlen(kNatronOfxImageComponentsPlaneLabel);
    std::size_t foundPlaneLabel = comp.find(kNatronOfxImageComponentsPlaneLabel, planeNameStartIdx);
    if (foundPlaneLabel != std::string::npos) {
        planeLabelStartIdx = foundPlaneLabel + foundPlaneLabelLen;
    }



    // Find the optional channels label
    // If channelsLabelStartIdx = 0, there's no channels label.
    std::size_t channelsLabelStartIdx = 0;


    const std::size_t foundChannelsLabelLen = std::strlen(kNatronOfxImageComponentsPlaneChannelsLabel);

    // If there was a plane label before, pick from there otherwise pick from the name
    std::size_t findChannelsLabelStart = planeLabelStartIdx > 0 ? planeLabelStartIdx : planeNameStartIdx;
    std::size_t foundChannelsLabel = comp.find(kNatronOfxImageComponentsPlaneChannelsLabel, findChannelsLabelStart);
    if (foundChannelsLabel != std::string::npos) {
        channelsLabelStartIdx = foundChannelsLabel + foundChannelsLabelLen;
    }



    // Find the first channel
    // If there was a channels label before, find from there, otherwise if there was a plane label before
    // find from there, otherwise find from the name.
    std::size_t findChannelStart = 0;
    if (channelsLabelStartIdx > 0) {
        findChannelStart = channelsLabelStartIdx;
    } else if (planeLabelStartIdx > 0) {
        findChannelStart = planeLabelStartIdx;
    } else {
        findChannelStart = planeNameStartIdx;
    }

    const std::size_t foundChannelLen = std::strlen(kNatronOfxImageComponentsPlaneChannel);
    std::size_t foundChannel = comp.find(kNatronOfxImageComponentsPlaneChannel, findChannelStart);
    if (foundChannel == std::string::npos) {
        // There needs to be at least one channel.
        return false;
    }

    // Extract channels label
    if (channelsLabelStartIdx > 0) {
        *channelsLabel = comp.substr(channelsLabelStartIdx, foundChannel - channelsLabelStartIdx);
    }

    // Extract plane label
    if (planeLabelStartIdx > 0) {
        std::size_t endIndex = (foundChannelsLabel != std::string::npos) ? foundChannelsLabel : foundChannel;
        *layerLabel = comp.substr(planeLabelStartIdx, endIndex - planeLabelStartIdx);
    }

    // Extract plane name
    {
        std::size_t endIndex;
        if (foundPlaneLabel != std::string::npos) {
            // There's a plane label
            endIndex = foundPlaneLabel;
        } else if (foundChannelsLabel != std::string::npos) {
            // There's no plane label but a channels label
            endIndex = foundChannelsLabel;
        } else {
            // No plane label and no channels label
            endIndex = foundChannel;
        }
        *layerName = comp.substr(planeNameStartIdx, endIndex - planeNameStartIdx);
    }

    while (foundChannel != std::string::npos) {
        if (channels->size() >= 4) {
            // A plane must have between 1 and 4 channels.
            return false;
        }
        findChannelStart = foundChannel + foundChannelLen;
        std::size_t nextChannel = comp.find(kNatronOfxImageComponentsPlaneChannel, findChannelStart);
        std::string chan = comp.substr(findChannelStart, nextChannel - findChannelStart);
        channels->push_back(chan);
        foundChannel = nextChannel;
    }

    return true;
} // extractOFXEncodedCustomPlane

static ImagePlaneDesc
ofxCustomCompToNatronComp(const std::string& comp)
{
    std::string planeID, planeLabel, channelsLabel;
    std::vector<std::string> channels;
    if (!extractOFXEncodedCustomPlane(comp, &planeID, &planeLabel, &channelsLabel, &channels)) {
        return ImagePlaneDesc::getNoneComponents();
    }
    return ImagePlaneDesc(planeID, planeLabel, channelsLabel, channels);
}

ImagePlaneDesc
ImagePlaneDesc::mapOFXPlaneStringToPlane(const std::string& ofxPlane)
{
    assert(ofxPlane != kFnOfxImagePlaneColour);
    if (ofxPlane == kFnOfxImagePlaneBackwardMotionVector) {
        return ImagePlaneDesc::getBackwardMotionComponents();
    } else if (ofxPlane == kFnOfxImagePlaneForwardMotionVector) {
        return ImagePlaneDesc::getForwardMotionComponents();
    } else if (ofxPlane == kFnOfxImagePlaneStereoDisparityLeft) {
        return ImagePlaneDesc::getDisparityLeftComponents();
    } else if (ofxPlane == kFnOfxImagePlaneStereoDisparityRight) {
        return ImagePlaneDesc::getDisparityRightComponents();
    } else {
        return ofxCustomCompToNatronComp(ofxPlane);
    }
}

void
ImagePlaneDesc::mapOFXComponentsTypeStringToPlanes(const std::string& ofxComponents, ImagePlaneDesc* plane, ImagePlaneDesc* pairedPlane)
{
    if (ofxComponents ==  kOfxImageComponentRGBA) {
        *plane = ImagePlaneDesc::getRGBAComponents();
    } else if (ofxComponents == kOfxImageComponentAlpha) {
        *plane = ImagePlaneDesc::getAlphaComponents();
    } else if (ofxComponents == kOfxImageComponentRGB) {
        *plane = ImagePlaneDesc::getRGBComponents();
    }else if (ofxComponents == kNatronOfxImageComponentXY) {
        *plane = ImagePlaneDesc::getXYComponents();
    } else if (ofxComponents == kOfxImageComponentNone) {
        *plane = ImagePlaneDesc::getNoneComponents();
    } else if (ofxComponents == kFnOfxImageComponentMotionVectors) {
        *plane = ImagePlaneDesc::getBackwardMotionComponents();
        *pairedPlane = ImagePlaneDesc::getForwardMotionComponents();
    } else if (ofxComponents == kFnOfxImageComponentStereoDisparity) {
        *plane = ImagePlaneDesc::getDisparityLeftComponents();
        *pairedPlane = ImagePlaneDesc::getDisparityRightComponents();
    } else {
        *plane = ofxCustomCompToNatronComp(ofxComponents);
    }

} // mapOFXComponentsTypeStringToPlanes


static std::string
natronCustomCompToOfxComp(const ImagePlaneDesc &comp)
{
    std::stringstream ss;
    const std::vector<std::string>& channels = comp.getChannels();
    const std::string& planeID = comp.getPlaneID();
    const std::string& planeLabel = comp.getPlaneLabel();
    const std::string& channelsLabel = comp.getChannelsLabel();
    ss << kNatronOfxImageComponentsPlaneName << planeID;
    if (!planeLabel.empty()) {
        ss << kNatronOfxImageComponentsPlaneLabel << planeLabel;
    }
    if (!channelsLabel.empty()) {
        ss << kNatronOfxImageComponentsPlaneChannelsLabel << channelsLabel;
    }
    for (std::size_t i = 0; i < channels.size(); ++i) {
        ss << kNatronOfxImageComponentsPlaneChannel << channels[i];
    }

    return ss.str();
} // natronCustomCompToOfxComp


std::string
ImagePlaneDesc::mapPlaneToOFXPlaneString(const ImagePlaneDesc& plane)
{
    if (plane.isColorPlane()) {
        return kFnOfxImagePlaneColour;
    } else if ( plane == ImagePlaneDesc::getBackwardMotionComponents() ) {
        return kFnOfxImagePlaneBackwardMotionVector;
    } else if ( plane == ImagePlaneDesc::getForwardMotionComponents()) {
        return kFnOfxImagePlaneForwardMotionVector;
    } else if ( plane == ImagePlaneDesc::getDisparityLeftComponents()) {
        return kFnOfxImagePlaneStereoDisparityLeft;
    } else if ( plane == ImagePlaneDesc::getDisparityRightComponents() ) {
        return kFnOfxImagePlaneStereoDisparityRight;
    } else {
        return natronCustomCompToOfxComp(plane);
    }

}

std::string
ImagePlaneDesc::mapPlaneToOFXComponentsTypeString(const ImagePlaneDesc& plane)
{
    if ( plane == ImagePlaneDesc::getNoneComponents() ) {
        return kOfxImageComponentNone;
    } else if ( plane == ImagePlaneDesc::getAlphaComponents() ) {
        return kOfxImageComponentAlpha;
    } else if ( plane == ImagePlaneDesc::getRGBComponents() ) {
        return kOfxImageComponentRGB;
    } else if ( plane == ImagePlaneDesc::getRGBAComponents() ) {
        return kOfxImageComponentRGBA;
    } else if ( plane == ImagePlaneDesc::getXYComponents() ) {
        return kNatronOfxImageComponentXY;
    } else if ( plane == ImagePlaneDesc::getBackwardMotionComponents() ||
               plane == ImagePlaneDesc::getForwardMotionComponents()) {
        return kFnOfxImageComponentMotionVectors;
    } else if ( plane == ImagePlaneDesc::getDisparityLeftComponents() ||
               plane == ImagePlaneDesc::getDisparityRightComponents()) {
        return kFnOfxImageComponentStereoDisparity;
    } else {
        return natronCustomCompToOfxComp(plane);
    }
}


NATRON_NAMESPACE_EXIT
