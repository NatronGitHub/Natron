//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "ImageComponents.h"
#include "ofxNatron.h"

using namespace Natron;

static const char* rgbaComps[4] = {"R","G","B","A"};
static const char* rgbComps[3] = {"R","G","B"};
static const char* alphaComps[1] = {"Alpha"};
static const char* motionComps[2] = {"U","V"};
static const char* disparityComps[2] = {"X","Y"};
static const char* xyComps[2] = {"X","Y"};

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
    if (_layerName != other._layerName && isColorPlane()) {
        return true;
    }

    if (_layerName < other._layerName) {
        return true;
    }
    
    if (_componentNames.size() < other._componentNames.size()) {
        return true;
    } else if (_componentNames.size() > other._componentNames.size()) {
        return false;
    }
    
    for (std::size_t i = 0; i < _componentNames.size(); ++i) {
        if (_componentNames[i] < other._componentNames[i]) {
            return true;
        }
    }
    
    return false;
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

