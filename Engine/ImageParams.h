#ifndef IMAGEPARAMS_H
#define IMAGEPARAMS_H

#include "Global/GlobalDefines.h"

#include "Engine/NonKeyParams.h"
#include "Engine/Format.h"


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
        , _inputNbIdentity(-1)
        , _inputTimeIdentity(0)
        , _framesNeeded()
        , _components(Natron::eImageComponentRGBA)
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
        , _inputNbIdentity(other._inputNbIdentity)
        , _inputTimeIdentity(other._inputTimeIdentity)
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
                ImageComponentsEnum components,
                int inputNbIdentity,
                int inputTimeIdentity,
                const std::map<int, std::vector<RangeD> > & framesNeeded)
        : NonKeyParams( cost,bounds.area() * getElementsCountForComponents(components) * getSizeOfForBitDepth(bitdepth) )
        , _rod(rod)
        , _bounds(bounds)
        , _isRoDProjectFormat(isRoDProjectFormat)
        , _inputNbIdentity(inputNbIdentity)
        , _inputTimeIdentity(inputTimeIdentity)
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

    const RectI & getBounds() const
    {
        return _bounds;
    }

    int getInputNbIdentity() const
    {
        return _inputNbIdentity;
    }

    int getInputTimeIdentity() const
    {
        return _inputTimeIdentity;
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
    
    const std::map<int, std::vector<RangeD> > & getFramesNeeded() const
    {
        return _framesNeeded;
    }

    ImageComponentsEnum getComponents() const
    {
        return _components;
    }

    void setComponents(ImageComponentsEnum comps)
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

private:

    virtual bool isEqualToVirtual(const NonKeyParams & other) const OVERRIDE FINAL
    {
        const ImageParams & imgParams = dynamic_cast<const ImageParams &>(other);

        if ( imgParams._framesNeeded.size() != _framesNeeded.size() ) {
            return false;
        }
        std::map<int,std::vector<RangeD> >::const_iterator it = _framesNeeded.begin();
        for (std::map<int,std::vector<RangeD> >::const_iterator itOther = imgParams._framesNeeded.begin();
             itOther != imgParams._framesNeeded.end(); ++itOther) {
            if ( it->second.size() != itOther->second.size() ) {
                return false;
            }
            for (U32 i = 0; i < it->second.size(); ++i) {
                if ( (it->second[i].min != itOther->second[i].min) || (it->second[i].max != itOther->second[i].max) ) {
                    return false;
                }
            }
            ++it;
        }

        return _rod == imgParams._rod
                && _inputNbIdentity == imgParams._inputNbIdentity
                && _inputTimeIdentity == imgParams._inputTimeIdentity
                && _components == imgParams._components
                && _bitdepth == imgParams._bitdepth
                && _mipMapLevel == imgParams._mipMapLevel;
    }

    RectD _rod;
    RectI _bounds;


    /// if true then when retrieving the associated image from cache
    /// the caller should update the rod to the current project format.
    /// This is because the project format might have changed since this image was cached.
    bool _isRoDProjectFormat;
    int _inputNbIdentity; // -1 if not an identity
    double _inputTimeIdentity;
    std::map<int, std::vector<RangeD> > _framesNeeded;
    Natron::ImageComponentsEnum _components;
    Natron::ImageBitDepthEnum _bitdepth;
    unsigned int _mipMapLevel;
    double _par;
};
}

#endif // IMAGEPARAMS_H
