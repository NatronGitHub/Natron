#ifndef IMAGEPARAMS_H
#define IMAGEPARAMS_H

#include "Global/GlobalDefines.h"

#include "Engine/NonKeyParams.h"
#include "Engine/Format.h"


namespace Natron {
    
    inline int getElementsCountForComponents(ImageComponents comp) {
        switch (comp) {
            case ImageComponentNone:
                return 0;
            case ImageComponentAlpha:
                return 1;
            case ImageComponentRGB:
                return 3;
            case ImageComponentRGBA:
                return 4;
            default:
                ///unsupported components
                assert(false);
                break;
        }
    }
    
    inline int getSizeOfForBitDepth(Natron::ImageBitDepth bitdepth)
    {
        switch (bitdepth) {
            case Natron::IMAGE_BYTE:
                return sizeof(unsigned char);
            case Natron::IMAGE_SHORT:
                return sizeof(unsigned short);
            case Natron::IMAGE_FLOAT:
                return sizeof(float);
            case Natron::IMAGE_NONE:
                assert(false);
                break;
        }
    }
    
class ImageParams : public NonKeyParams
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
    , _components(Natron::ImageComponentRGBA)
    {
        
    }
    
    ImageParams(const ImageParams& other)
    : NonKeyParams(other)
    , _rod(other._rod)
    , _bounds(other._bounds)
    , _isRoDProjectFormat(other._isRoDProjectFormat)
    , _inputNbIdentity(other._inputNbIdentity)
    , _inputTimeIdentity(other._inputTimeIdentity)
    , _framesNeeded(other._framesNeeded)
    , _components(other._components)
    {
        
    }
    
    ImageParams(int cost,
                const RectD& rod,
                const RectI& bounds,
                Natron::ImageBitDepth bitdepth,
                bool isRoDProjectFormat,
                ImageComponents components,
                int inputNbIdentity,
                int inputTimeIdentity,
                const std::map<int, std::vector<RangeD> >& framesNeeded)
    : NonKeyParams(cost,bounds.area() * getElementsCountForComponents(components) * getSizeOfForBitDepth(bitdepth))
    , _rod(rod)
    , _bounds(bounds)
    , _isRoDProjectFormat(isRoDProjectFormat)
    , _inputNbIdentity(inputNbIdentity)
    , _inputTimeIdentity(inputTimeIdentity)
    , _framesNeeded(framesNeeded)
    , _components(components)
    , _bitdepth(bitdepth)
    {
        
    }
    
    virtual ~ImageParams() {}

    // return the RoD in canonical coordinates
    const RectD& getRoD() const { return _rod; }
    
    const RectI& getBounds() const { return _bounds; }
    
    int getInputNbIdentity() const { return _inputNbIdentity; }
    
    int getInputTimeIdentity() const { return _inputTimeIdentity; }
    
    bool isRodProjectFormat() const { return _isRoDProjectFormat; }
    
    Natron::ImageBitDepth getBitDepth() const { return _bitdepth; }
    
    const std::map<int, std::vector<RangeD> >& getFramesNeeded() const { return _framesNeeded; }
    
    ImageComponents getComponents() const { return _components; }
    
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
    
    
private:
        
    virtual bool isEqualToVirtual(const NonKeyParams& other) const OVERRIDE FINAL {
        const ImageParams& imgParams = dynamic_cast<const ImageParams&>(other);
        if (imgParams._framesNeeded.size() != _framesNeeded.size()) {
            return false;
        }
        std::map<int,std::vector<RangeD> >::const_iterator it = _framesNeeded.begin();
        for (std::map<int,std::vector<RangeD> >::const_iterator itOther = imgParams._framesNeeded.begin();
             itOther != imgParams._framesNeeded.end(); ++itOther) {
            if (it->second.size() != itOther->second.size() ) {
                return false;
            }
            for (U32 i = 0; i < it->second.size() ; ++i) {
                if (it->second[i].min != itOther->second[i].min || it->second[i].max != itOther->second[i].max) {
                    return false;
                }
            }
            ++it;
        }
        
        return _rod == imgParams._rod
        && _inputNbIdentity == imgParams._inputNbIdentity
        && _inputTimeIdentity == imgParams._inputTimeIdentity
        && _components == imgParams._components
        && _bitdepth == imgParams._bitdepth;
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
    Natron::ImageComponents _components;
    Natron::ImageBitDepth _bitdepth;
};


}

#endif // IMAGEPARAMS_H
