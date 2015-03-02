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

using namespace Natron;

ImageComponents::ImageComponents()
: _layerName("none")
, _componentsName("none")
, _count(0)
{
    
}

ImageComponents::ImageComponents(const std::string& layerName,const std::string& componentsName,int count)
: _layerName(layerName)
, _componentsName(componentsName)
, _count(count)
{
    
}

ImageComponents::~ImageComponents()
{
    
}

bool
ImageComponents::isColorPlane() const
{
    return _componentsName == kNatronColorPlaneName;
}

bool
ImageComponents::isConvertibleTo(const ImageComponents& other) const
{
    if (_layerName != other._layerName) {
        return false;
    }
    
    if (_componentsName == kNatronColorPlaneName && other._componentsName == kNatronColorPlaneName) {
        return true;
    }
    return false;
}

bool
ImageComponents::operator==(const ImageComponents& other) const
{
    return _layerName == other._layerName &&
    _componentsName == other._componentsName &&
    _count == other._count;
}

bool
ImageComponents::operator<(const ImageComponents& other) const
{
    if (_count < other._count) {
        return true;
    }
    if (_layerName < other._layerName) {
        return true;
    }
    
    if (_componentsName < other._componentsName) {
        return true;
    }
    
    return false;
}

int
ImageComponents::getNumComponents() const
{
    return _count;
}

const std::string&
ImageComponents::getLayerName() const
{
    return _layerName;
}

const std::string&
ImageComponents::getComponentsName() const
{
    return _componentsName;
}


const ImageComponents&
ImageComponents::getNoneComponents()
{
    static const ImageComponents comp("none","none",0);
    return comp;
}

const ImageComponents&
ImageComponents::getRGBAComponents()
{
    static const ImageComponents comp(kNatronColorPlaneName,kNatronRGBAComponentsName,4);
    return comp;
}

const ImageComponents&
ImageComponents::getRGBComponents()
{
    static const ImageComponents comp(kNatronColorPlaneName,kNatronRGBComponentsName,3);
    return comp;
}

const ImageComponents&
ImageComponents::getAlphaComponents()
{
    static const ImageComponents comp(kNatronColorPlaneName,kNatronAlphaComponentsName,1);
    return comp;
}

const ImageComponents&
ImageComponents::getBackwardMotionComponents()
{
    static const ImageComponents comp(kNatronBackwardMotionVectorsPlaneName,kNatronMotionComponentsName,2);
    return comp;
}

const ImageComponents&
ImageComponents::getForwardMotionComponents()
{
    static const ImageComponents comp(kNatronForwardMotionVectorsPlaneName,kNatronMotionComponentsName,2);
    return comp;
}

const ImageComponents&
ImageComponents::getDisparityLeftComponents()
{
    static const ImageComponents comp(kNatronDisparityLeftPlaneName,kNatronDisparityComponentsName,2);
    return comp;
}

const ImageComponents&
ImageComponents::getDisparityRightComponents()
{
    static const ImageComponents comp(kNatronDisparityRightPlaneName,kNatronDisparityComponentsName,2);
    return comp;
}

