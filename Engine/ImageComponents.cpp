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

#include "ImageComponents.h"
#include "ofxNatron.h"

NATRON_NAMESPACE_ENTER;

static const char* rgbaComps[4] = {"R","G","B","A"};
static const char* rgbComps[3] = {"R","G","B"};
static const char* alphaComps[1] = {"Alpha"};
static const char* motionComps[2] = {"U","V"};
static const char* disparityComps[2] = {"X","Y"};
static const char* xyComps[2] = {"X","Y"};


const char* ImageComponents::defaultComponents[][2] =
{
    {kNatronRGBAComponentsName, kNatronRGBAPlaneUserName},
    {kNatronRGBComponentsName, kNatronRGBPlaneUserName},
    {kNatronAlphaComponentsName, kNatronAlphaPlaneUserName},
    {kNatronDisparityLeftPlaneName, kNatronDisparityLeftPlaneUserName},
    {kNatronDisparityRightPlaneName, kNatronDisparityRightPlaneUserName},
    {kNatronBackwardMotionVectorsPlaneName, kNatronBackwardMotionVectorsPlaneUserName},
    {kNatronForwardMotionVectorsPlaneName, kNatronForwardMotionVectorsPlaneUserName},
    {0, 0}
};

std::string
ImageComponents::mapUserFriendlyPlaneNameToNatronInternalPlaneName(const std::string& userfriendlyPlaneName)
{
    int i = 0;
    while (defaultComponents[i][0] != 0) {
        if (userfriendlyPlaneName == std::string(defaultComponents[i][1])) {
            return std::string(defaultComponents[i][0]);
        }
        ++i;
    }
    return userfriendlyPlaneName;
}

std::string
ImageComponents::mapNatronInternalPlaneNameToUserFriendlyPlaneName(const std::string& planeName)
{
    int i = 0;
    while (defaultComponents[i][0] != 0) {
        if (planeName == std::string(defaultComponents[i][0])) {
            return std::string(defaultComponents[i][1]);
        }
        ++i;
    }
    return planeName;
}

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
    if (_globalComponentsName.empty()) {
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
ImageComponents::isConvertibleTo(const ImageComponents& other) const
{
    if (*this == other) {
        return true;
    }
    if (_layerName != other._layerName) {
        return false;
    }
    if (_layerName == kNatronColorPlaneName && other._layerName == kNatronColorPlaneName) {
        return true;
    }
    return false;
}

bool
ImageComponents::operator==(const ImageComponents& other) const
{
    if (_componentNames.size() != other._componentNames.size()) {
        return false;
    }
    
    for (std::size_t i = 0; i < _componentNames.size(); ++i) {
        if (_componentNames[i] != other._componentNames[i]) {
            return false;
        }
    }
    return _layerName == other._layerName;
}

bool
ImageComponents::operator<(const ImageComponents& other) const
{
    std::string hash,otherHash;
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

const ImageComponents&
ImageComponents::getNoneComponents()
{
    static const ImageComponents comp("none","none",std::vector<std::string>());
    return comp;
}

const ImageComponents&
ImageComponents::getRGBAComponents()
{
    static const ImageComponents comp(kNatronColorPlaneName,kNatronRGBAComponentsName,rgbaComps,4);
    return comp;
}

const ImageComponents&
ImageComponents::getRGBComponents()
{
    static const ImageComponents comp(kNatronColorPlaneName,kNatronRGBComponentsName,rgbComps,3);
    return comp;
}

const ImageComponents&
ImageComponents::getAlphaComponents()
{
    static const ImageComponents comp(kNatronColorPlaneName,kNatronAlphaComponentsName,alphaComps,1);
    return comp;
}

const ImageComponents&
ImageComponents::getBackwardMotionComponents()
{
    static const ImageComponents comp(kNatronBackwardMotionVectorsPlaneName,kNatronMotionComponentsName,motionComps,2);
    return comp;
}

const ImageComponents&
ImageComponents::getForwardMotionComponents()
{
    static const ImageComponents comp(kNatronForwardMotionVectorsPlaneName,kNatronMotionComponentsName,motionComps,2);
    return comp;
}

const ImageComponents&
ImageComponents::getDisparityLeftComponents()
{
    static const ImageComponents comp(kNatronDisparityLeftPlaneName,kNatronDisparityComponentsName,disparityComps,2);
    return comp;
}

const ImageComponents&
ImageComponents::getDisparityRightComponents()
{
    static const ImageComponents comp(kNatronDisparityRightPlaneName,kNatronDisparityComponentsName,disparityComps,2);
    return comp;
}

const ImageComponents&
ImageComponents::getXYComponents()
{
    static const ImageComponents comp("XY","xy",xyComps,2);
    return comp;
}

NATRON_NAMESPACE_EXIT;

