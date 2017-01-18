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

#include "ImageComponents.h"

#include <ofxNatron.h>

#include <cassert>
#include <stdexcept>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Serialization/NodeSerialization.h"

NATRON_NAMESPACE_ENTER;

static const char* rgbaComps[4] = {"R", "G", "B", "A"};
static const char* rgbComps[3] = {"R", "G", "B"};
static const char* alphaComps[1] = {"Alpha"};
static const char* motionComps[2] = {"U", "V"};
static const char* disparityComps[2] = {"X", "Y"};
static const char* xyComps[2] = {"X", "Y"};


ImageComponents::ImageComponents()
    : _layerName("none")
    , _componentNames()
    , _globalComponentsName("none")
{
}

ImageComponents::ImageComponents(const std::string& layerName,
                                 const std::string& globalCompName,
                                 const std::vector<std::string>& componentsName)
    : _layerName(layerName)
    , _componentNames(componentsName)
    , _globalComponentsName(globalCompName)
{
    if ( _globalComponentsName.empty() ) {
        //Heuristic to give an appropriate name to components
        for (std::size_t i = 0; i < componentsName.size(); ++i) {
            _globalComponentsName.append(componentsName[i]);
        }
    }
}

ImageComponents::ImageComponents(const std::string& layerName,
                                 const std::string& globalCompName,
                                 const char** componentsName,
                                 int count)
    : _layerName(layerName)
    , _componentNames()
    , _globalComponentsName(globalCompName)
{
    _componentNames.resize(count);
    for (int i = 0; i < count; ++i) {
        _componentNames[i] = componentsName[i];
    }
}

ImageComponents::ImageComponents(const std::string& layerName,
                                 const std::string& pairedLayer,
                                 const std::string& globalCompName,
                                 const char** componentsName,
                                 int count)
    : _layerName(layerName)
    , _pairedLayer(pairedLayer)
    , _componentNames()
    , _globalComponentsName(globalCompName)
{
    _componentNames.resize(count);
    for (int i = 0; i < count; ++i) {
        _componentNames[i] = componentsName[i];
    }
}

ImageComponents::~ImageComponents()
{
}

bool
ImageComponents::isColorPlane(const std::string& layerName)
{
    return layerName == kNatronColorPlaneName;
}

bool
ImageComponents::isColorPlane() const
{
    return _layerName == kNatronColorPlaneName;
}

bool
ImageComponents::isEqualToPairedPlane(const ImageComponents& other,
                                      ImageComponents* pairedLayer) const
{
    assert( !other.isPairedComponents() );
    if ( !isPairedComponents() ) {
        return false;
    }
    if ( _componentNames.size() != other._componentNames.size() ) {
        return false;
    }

    for (std::size_t i = 0; i < _componentNames.size(); ++i) {
        if (_componentNames[i] != other._componentNames[i]) {
            return false;
        }
    }
    if (_layerName == other._layerName) {
        *pairedLayer = ImageComponents(_layerName, _globalComponentsName, _componentNames);

        return true;
    } else if (_pairedLayer == other._layerName) {
        *pairedLayer = ImageComponents(_pairedLayer, _globalComponentsName, _componentNames);

        return true;
    }

    return false;
}

bool
ImageComponents::getPlanesPair(ImageComponents* first,
                               ImageComponents* second) const
{
    if ( !isPairedComponents() ) {
        return false;
    }
    *first = ImageComponents(_layerName, _globalComponentsName, _componentNames);
    *second = ImageComponents(_pairedLayer, _globalComponentsName, _componentNames);

    return true;
}

bool
ImageComponents::isConvertibleTo(const ImageComponents& other) const
{
    if (*this == other) {
        return true;
    }
    if (_layerName != other._layerName) {
        return false;
    }
    if ( (_layerName == kNatronColorPlaneName) && (other._layerName == kNatronColorPlaneName) ) {
        return true;
    }

    return false;
}

bool
ImageComponents::operator==(const ImageComponents& other) const
{
    if ( _componentNames.size() != other._componentNames.size() ) {
        return false;
    }

    for (std::size_t i = 0; i < _componentNames.size(); ++i) {
        if (_componentNames[i] != other._componentNames[i]) {
            return false;
        }
    }

    return _layerName == other._layerName && _pairedLayer == other._pairedLayer;
}

bool
ImageComponents::operator<(const ImageComponents& other) const
{
    std::string hash, otherHash;

    hash.append(_layerName);
    for (std::size_t i = 0; i < _componentNames.size(); ++i) {
        hash.append(_componentNames[i]);
    }

    otherHash.append(other._layerName);
    for (std::size_t i = 0; i < other._componentNames.size(); ++i) {
        otherHash.append(other._componentNames[i]);
    }

    return hash < otherHash;
}

int
ImageComponents::getNumComponents() const
{
    return (int)_componentNames.size();
}

const std::string&
ImageComponents::getLayerName() const
{
    return _layerName;
}

const std::vector<std::string>&
ImageComponents::getComponentsNames() const
{
    return _componentNames;
}

const std::string&
ImageComponents::getComponentsGlobalName() const
{
    return _globalComponentsName;
}

bool
ImageComponents::isPairedComponents() const
{
    return !_pairedLayer.empty();
}

const std::string&
ImageComponents::getPairedLayerName() const
{
    return _pairedLayer;
}

const ImageComponents&
ImageComponents::getNoneComponents()
{
    static const ImageComponents comp( "none", "none", std::vector<std::string>() );

    return comp;
}

const ImageComponents&
ImageComponents::getRGBAComponents()
{
    static const ImageComponents comp(kNatronColorPlaneName, "", rgbaComps, 4);

    return comp;
}

const ImageComponents&
ImageComponents::getRGBComponents()
{
    static const ImageComponents comp(kNatronColorPlaneName, "", rgbComps, 3);

    return comp;
}

const ImageComponents&
ImageComponents::getAlphaComponents()
{
    static const ImageComponents comp(kNatronColorPlaneName, "", alphaComps, 1);

    return comp;
}

const ImageComponents&
ImageComponents::getBackwardMotionComponents()
{
    static const ImageComponents comp(kNatronBackwardMotionVectorsPlaneName, kNatronMotionComponentsName, motionComps, 2);

    return comp;
}

const ImageComponents&
ImageComponents::getForwardMotionComponents()
{
    static const ImageComponents comp(kNatronForwardMotionVectorsPlaneName, kNatronMotionComponentsName, motionComps, 2);

    return comp;
}

const ImageComponents&
ImageComponents::getDisparityLeftComponents()
{
    static const ImageComponents comp(kNatronDisparityLeftPlaneName, kNatronDisparityComponentsName, disparityComps, 2);

    return comp;
}

const ImageComponents&
ImageComponents::getDisparityRightComponents()
{
    static const ImageComponents comp(kNatronDisparityRightPlaneName, kNatronDisparityComponentsName, disparityComps, 2);

    return comp;
}

const ImageComponents&
ImageComponents::getXYComponents()
{
    static const ImageComponents comp("XY", "xy", xyComps, 2);

    return comp;
}

const
ImageComponents&
ImageComponents::getPairedMotionVectors()
{
    //static const ImageComponents comp(kFnOfxImagePlaneForwardMotionVector,kFnOfxImagePlaneBackwardMotionVector,kFnOfxImageComponentMotionVectors,motionComps,2);
    static const ImageComponents comp(kNatronForwardMotionVectorsPlaneName, kNatronBackwardMotionVectorsPlaneName, kNatronMotionComponentsName, motionComps, 2);

    return comp;
}

const ImageComponents&
ImageComponents::getPairedStereoDisparity()
{
    //static const ImageComponents comp(kFnOfxImagePlaneStereoDisparityLeft,kFnOfxImagePlaneStereoDisparityRight,kFnOfxImageComponentStereoDisparity,xyComps,2);
    static const ImageComponents comp(kNatronDisparityLeftPlaneName, kNatronDisparityRightPlaneName, kNatronDisparityComponentsName, xyComps, 2);

    return comp;
}


void
ImageComponents::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj)
{
    SERIALIZATION_NAMESPACE::ImageComponentsSerialization* s = dynamic_cast<SERIALIZATION_NAMESPACE::ImageComponentsSerialization*>(obj);
    if (!s) {
        return;
    }
    s->layerName = _layerName;
    s->globalCompsName = _globalComponentsName;
    s->channelNames = _componentNames;
}

void
ImageComponents::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase & obj)
{
    const SERIALIZATION_NAMESPACE::ImageComponentsSerialization* s = dynamic_cast<const SERIALIZATION_NAMESPACE::ImageComponentsSerialization*>(&obj);
    if (!s) {
        return;
    }
    _layerName = s->layerName;
    _globalComponentsName = s->globalCompsName;
    _componentNames = s->channelNames;

}

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

ChoiceOption
ImageComponents::getChannelOption(int channelIndex) const
{
    if (channelIndex < 0 || channelIndex >= (int)_componentNames.size()) {
        assert(false);
        return ChoiceOption("","","");
    }
    std::string optionID, optionLabel;
    optionID += _layerName;
    if ( !_layerName.empty() ) {
        optionID += '.';
    }
    optionLabel = optionID;

    // For the option label, append the name of the channel
    optionLabel += _componentNames[channelIndex];

    // For the option ID, if this is the alpha channel we need to do something different:
    // The Color.RGBA and Color.Alpha planes both have alpha. To overcome this
    optionID += generateChannelID(_componentNames[channelIndex]);
    return ChoiceOption(optionID, optionLabel, "");
}

ChoiceOption
ImageComponents::getLayerOption() const
{
    std::string optionLabel = _layerName + "." + _globalComponentsName;

    // The option ID is always the name of the layer, this ensures for the Color plane that even if the components type changes, the choice stays
    // the same in the parameter.
    return ChoiceOption(_layerName, optionLabel, "");

}

const ImageComponents&
ImageComponents::getColorPlaneComponents(int nComps)
{
    switch (nComps) {
        case 1:
            return ImageComponents::getAlphaComponents();
        case 2:
            return ImageComponents::getXYComponents();
        case 3:
            return ImageComponents::getRGBComponents();
        case 4:
            return ImageComponents::getRGBAComponents();
        default:
            return ImageComponents::getNoneComponents();
    }
}

NATRON_NAMESPACE_EXIT;

