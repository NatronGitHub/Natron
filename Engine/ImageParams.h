/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef IMAGEPARAMS_H
#define IMAGEPARAMS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/GlobalDefines.h"

#include "Engine/Format.h"
#include "Engine/ImageComponents.h"
#include "Engine/NonKeyParams.h"
#include "Engine/RectD.h"
#include "Engine/RectI.h"


namespace Natron {
    
inline int
        getElementsCountForComponents(ImageComponentsEnum comp)
{
    switch (comp) {
    case eImageComponentNone:

        return 0;
    case eImageComponentAlpha:

        return 1;
    case eImageComponentRGB:

        return 3;
    case eImageComponentRGBA:

        return 4;
    case eImageComponentXY:
            return 2;
    }
    assert(false);

    return 0;
}
    
inline bool isColorComponents(Natron::ImageComponentsEnum comp)
{
    switch (comp) {
        case eImageComponentNone:
            
            return false;
        case eImageComponentAlpha:
            
            return true;
        case eImageComponentRGB:
            
            return true;
        case eImageComponentRGBA:
            
            return true;
        case eImageComponentXY:
            return false;
    }
    assert(false);
    
    return 0;
  
}

inline int
        getSizeOfForBitDepth(Natron::ImageBitDepthEnum bitdepth)
{
    switch (bitdepth) {
    case Natron::eImageBitDepthByte:

        return sizeof(unsigned char);
    case Natron::eImageBitDepthShort:

        return sizeof(unsigned short);
    case Natron::eImageBitDepthFloat:

        return sizeof(float);
    case Natron::eImageBitDepthNone:
        break;
    }
    return 0;
}

class ImageParams
        : public NonKeyParams
{
public:

    ImageParams()
        : NonKeyParams()
        , _rod()
        , _bounds()
        , _isRoDProjectFormat(false)
        , _framesNeeded()
        , _components(ImageComponents::getRGBAComponents())
        , _bitdepth(Natron::eImageBitDepthFloat)
        , _mipMapLevel(0)
        , _par(1.)
    {
    }

    ImageParams(const ImageParams & other)
        : NonKeyParams(other)
        , _rod(other._rod)
        , _bounds(other._bounds)
        , _isRoDProjectFormat(other._isRoDProjectFormat)
        , _framesNeeded(other._framesNeeded)
        , _components(other._components)
        , _bitdepth(other._bitdepth)
        , _mipMapLevel(other._mipMapLevel)
        , _par(other._par)
    {
    }

    ImageParams(int cost,
                const RectD & rod,
                const double par,
                const unsigned int mipMapLevel,
                const RectI & bounds,
                Natron::ImageBitDepthEnum bitdepth,
                bool isRoDProjectFormat,
                const ImageComponents& components,
                const std::map<int, std::map<int,std::vector<RangeD> > > & framesNeeded)
        : NonKeyParams( cost,bounds.area() * components.getNumComponents() * getSizeOfForBitDepth(bitdepth) )
        , _rod(rod)
        , _bounds(bounds)
        , _isRoDProjectFormat(isRoDProjectFormat)
        , _framesNeeded(framesNeeded)
        , _components(components)
        , _bitdepth(bitdepth)
        , _mipMapLevel(mipMapLevel)
        , _par(par)
    {
    }

    virtual ~ImageParams()
    {
    }

    // return the RoD in canonical coordinates
    const RectD & getRoD() const
    {
        return _rod;
    }
    
    void setRoD(const RectD& rod) {
        _rod = rod;
    }

    const RectI & getBounds() const
    {
        return _bounds;
    }
    
    void setBounds(const RectI& bounds)
    {
        _bounds = bounds;
        setElementsCount(bounds.area() * _components.getNumComponents() * getSizeOfForBitDepth(_bitdepth));
    }

    bool isRodProjectFormat() const
    {
        return _isRoDProjectFormat;
    }

    Natron::ImageBitDepthEnum getBitDepth() const
    {
        return _bitdepth;
    }
    
    void setBitDepth(ImageBitDepthEnum bitdepth)
    {
        _bitdepth = bitdepth;
    }
    
    const std::map<int, std::map<int,std::vector<RangeD> > > & getFramesNeeded() const
    {
        return _framesNeeded;
    }

    const ImageComponents& getComponents() const
    {
        return _components;
    }

    void setComponents(const ImageComponents& comps)
    {
        _components = comps;
    }
    
    double getPixelAspectRatio() const  {
        return _par;
    }

    unsigned int getMipMapLevel() const {
        return _mipMapLevel;
    }

    void setMipMapLevel(unsigned int mmlvl)
    {
        _mipMapLevel = mmlvl;
    }
    
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    bool operator==(const ImageParams & other) const
    {
        if (NonKeyParams::operator!=(other)) {
            return false;
        }
        if ( other._framesNeeded.size() != _framesNeeded.size() ) {
            return false;
        }
        std::map<int, std::map<int,std::vector<RangeD> > >::const_iterator it = _framesNeeded.begin();
        for (std::map<int, std::map<int,std::vector<RangeD> > >::const_iterator itOther = other._framesNeeded.begin();
             itOther != other._framesNeeded.end(); ++itOther, ++it) {
            if ( it->second.size() != itOther->second.size() ) {
                return false;
            }
            
            std::map<int,std::vector<RangeD> > ::const_iterator it2Other = itOther->second.begin();
            for (std::map<int,std::vector<RangeD> > ::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2, ++it2Other) {
                for (U32 i = 0; i < it2->second.size(); ++i) {
                    if ( (it2->second[i].min != it2Other->second[i].min) || (it2->second[i].max != it2Other->second[i].max) ) {
                        return false;
                    }
                }
            }
           
        }
        
        return _rod == other._rod
        && _components == other._components
        && _bitdepth == other._bitdepth
        && _mipMapLevel == other._mipMapLevel;
    }
    
    bool operator!=(const ImageParams & other) const
    {
        return !(*this == other);
    }
    
private:


    RectD _rod;
    RectI _bounds;


    /// if true then when retrieving the associated image from cache
    /// the caller should update the rod to the current project format.
    /// This is because the project format might have changed since this image was cached.
    bool _isRoDProjectFormat;
    std::map<int, std::map<int,std::vector<RangeD> > > _framesNeeded;
    Natron::ImageComponents _components;
    Natron::ImageBitDepthEnum _bitdepth;
    unsigned int _mipMapLevel;
    double _par;
};
}

#endif // IMAGEPARAMS_H
