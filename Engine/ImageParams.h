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

#ifndef IMAGEPARAMS_H
#define IMAGEPARAMS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Global/GlobalDefines.h"

#include "Engine/Format.h"
#include "Engine/ImageComponents.h"
#include "Engine/NonKeyParams.h"
#include "Engine/RectD.h"
#include "Engine/RectI.h"
#include "Engine/EngineFwd.h"

#include "Serialization/RectDSerialization.h"
#include "Serialization/ImageParamsSerialization.h"

NATRON_NAMESPACE_ENTER;

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

inline bool
isColorComponents(ImageComponentsEnum comp)
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

class ImageParams
    : public NonKeyParams
{
public:

    typedef SERIALIZATION_NAMESPACE::ImageParamsSerialization SerializationType;

    ImageParams()
        : NonKeyParams()
        , _rod()
        , _par(1.)
        , _components( ImageComponents::getRGBAComponents() )
        , _bitdepth(eImageBitDepthFloat)
        , _fielding(eImageFieldingOrderNone)
        , _premult(eImagePremultiplicationPremultiplied)
        , _mipMapLevel(0)
    {
    }

    ImageParams(const ImageParams & other)
        : NonKeyParams(other)
        , _rod(other._rod)
        , _par(other._par)
        , _components(other._components)
        , _bitdepth(other._bitdepth)
        , _fielding(other._fielding)
        , _premult(other._premult)
        , _mipMapLevel(other._mipMapLevel)
    {
    }

    ImageParams(const RectD & rod,
                const double par,
                const unsigned int mipMapLevel,
                const RectI & bounds,
                ImageBitDepthEnum bitdepth,
                ImageFieldingOrderEnum fielding,
                ImagePremultiplicationEnum premult,
                const ImageComponents& components,
                const OSGLContextPtr& context,
                StorageModeEnum storageMode,
                U32 textureTarget)
        : NonKeyParams()
        , _rod(rod)
        , _par(par)
        , _components(components)
        , _bitdepth(bitdepth)
        , _fielding(fielding)
        , _premult(premult)
        , _mipMapLevel(mipMapLevel)
    {
        CacheEntryStorageInfo& info = getStorageInfo();

        info.mode = storageMode;
        info.bounds = bounds;
        info.dataTypeSize = getSizeOfForBitDepth(bitdepth);
        info.numComponents = (std::size_t)components.getNumComponents();
        info.textureTarget = textureTarget;
        info.glContext = context;
        info.isGPUTexture = true;
    }

    virtual ~ImageParams()
    {
    }

    // return the RoD in canonical coordinates
    const RectD & getRoD() const
    {
        return _rod;
    }

    void setRoD(const RectD& rod)
    {
        _rod = rod;
    }

    const RectI & getBounds() const
    {
        return getStorageInfo().bounds;
    }

    void setBounds(const RectI& bounds)
    {
        getStorageInfo().bounds = bounds;
    }

    ImageBitDepthEnum getBitDepth() const
    {
        return _bitdepth;
    }

    void setBitDepth(ImageBitDepthEnum bitdepth)
    {
        _bitdepth = bitdepth;
    }

    const ImageComponents& getComponents() const
    {
        return _components;
    }

    ImageFieldingOrderEnum getFieldingOrder() const
    {
        return _fielding;
    }

    ImagePremultiplicationEnum getPremultiplication() const
    {
        return _premult;
    }

    void setPremultiplication(ImagePremultiplicationEnum premult)
    {
        _premult = premult;
    }

    double getPixelAspectRatio() const
    {
        return _par;
    }

    void setPixelAspectRatio(double par)
    {
        _par = par;
    }

    unsigned int getMipMapLevel() const
    {
        return _mipMapLevel;
    }

    void setMipMapLevel(unsigned int mmlvl)
    {
        _mipMapLevel = mmlvl;
    }


    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase) OVERRIDE FINAL
    {
        SERIALIZATION_NAMESPACE::ImageParamsSerialization* serialization = dynamic_cast<SERIALIZATION_NAMESPACE::ImageParamsSerialization*>(serializationBase);
        if (!serialization) {
            return;
        }
        NonKeyParams::toSerialization(serializationBase);
        _rod.toSerialization(&serialization->rod);
        serialization->par = _par;
        serialization->fielding = (int)_fielding;
        serialization->premult = (int)_premult;
        serialization->bitdepth = (int)_bitdepth;
        serialization->mipMapLevel = _mipMapLevel;
        _components.toSerialization(&serialization->components);
    }

    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase) OVERRIDE FINAL
    {
        const SERIALIZATION_NAMESPACE::ImageParamsSerialization* serialization = dynamic_cast<const SERIALIZATION_NAMESPACE::ImageParamsSerialization*>(&serializationBase);
        if (!serialization) {
            return;
        }
        NonKeyParams::fromSerialization(serializationBase);
        _rod.fromSerialization(serialization->rod);
        _par = serialization->par;
        _fielding = (ImageFieldingOrderEnum)serialization->fielding;
        _bitdepth = (ImageBitDepthEnum)serialization->bitdepth;
        _premult = (ImagePremultiplicationEnum)serialization->premult;
        _mipMapLevel = serialization->mipMapLevel;
        _components.fromSerialization(serialization->components);
    }

    bool operator==(const ImageParams & other) const
    {
        if ( NonKeyParams::operator!=(other) ) {
            return false;
        }

        return _rod == other._rod
               && _components == other._components
               && _bitdepth == other._bitdepth
               && _mipMapLevel == other._mipMapLevel
               && _premult == other._premult
               && _fielding == other._fielding;
    }

    bool operator!=(const ImageParams & other) const
    {
        return !(*this == other);
    }

private:

    RectD _rod;
    double _par;
    ImageComponents _components;
    ImageBitDepthEnum _bitdepth;
    ImageFieldingOrderEnum _fielding;
    ImagePremultiplicationEnum _premult;
    unsigned int _mipMapLevel;

};

NATRON_NAMESPACE_EXIT;


#endif // IMAGEPARAMS_H
