#ifndef IMAGEPARAMS_H
#define IMAGEPARAMS_H

#include "Global/GlobalDefines.h"

#include "Engine/NonKeyParams.h"
#include "Engine/Rect.h"



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
    
class ImageParams : public NonKeyParams
{
    
public:
    
    ImageParams()
    : NonKeyParams()
    , _rod()
    , _inputNbIdentity(-1)
    , _inputTimeIdentity(0)
    , _framesNeeded()
    , _components(Natron::ImageComponentRGBA)
    {
        
    }
    
    ImageParams(const ImageParams& other)
    : NonKeyParams(other)
    , _rod(other._rod)
    , _inputNbIdentity(other._inputNbIdentity)
    , _inputTimeIdentity(other._inputTimeIdentity)
    , _framesNeeded(other._framesNeeded)
    , _components(other._components)
    {
        
    }
    
    ImageParams(int cost,const RectI& rod,ImageComponents components,int inputNbIdentity,int inputTimeIdentity,
                const std::map<int, std::vector<RangeD> >& framesNeeded)
    : NonKeyParams(cost,rod.area() * getElementsCountForComponents(components))
    , _rod(rod)
    , _inputNbIdentity(inputNbIdentity)
    , _inputTimeIdentity(inputTimeIdentity)
    , _framesNeeded(framesNeeded)
    , _components(components)
    {
        
    }
    
    virtual ~ImageParams() {}
    
    const RectI& getRoD() const { return _rod; }
    
    int getInputNbIdentity() const { return _inputNbIdentity; }
    
    int getInputTimeIdentity() const { return _inputTimeIdentity; }
    
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
        && _components == imgParams._components;
    }
    
    RectI _rod;
    int _inputNbIdentity; // -1 if not an identity
    double _inputTimeIdentity;
    std::map<int, std::vector<RangeD> > _framesNeeded;
    ImageComponents _components;
};


}

#endif // IMAGEPARAMS_H
