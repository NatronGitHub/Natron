#ifndef FRAMEPARAMS_H
#define FRAMEPARAMS_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include "Engine/Rect.h"
#include "Engine/NonKeyParams.h"

namespace Natron {

class Image;
class FrameParams
        : public NonKeyParams
{
public:

    FrameParams()
        : NonKeyParams()
        , _originalImage()
        , _rod()
    {
    }

    FrameParams(const FrameParams & other)
        : NonKeyParams(other)
        , _originalImage(other._originalImage)
        , _rod(other._rod)
    {
    }

    FrameParams(const RectI & rod,
                int bitDepth,
                int texW,
                int texH,
                const boost::shared_ptr<Natron::Image>& originalImage)
        : NonKeyParams(1,bitDepth != 0 ? texW * texH * 16 : texW * texH * 4)
        , _originalImage(originalImage)
        , _rod(rod)
    {
    }

    virtual ~FrameParams()
    {
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    bool operator==(const FrameParams & other) const
    {
        return NonKeyParams::operator==(other) && _rod == other._rod;
    }
    
    bool operator!=(const FrameParams & other) const
    {
        return !(*this == other);
    }
    
    boost::shared_ptr<Natron::Image> getOriginalImage() const
    {
        return _originalImage.lock();
    }
    
    void setOriginalImage(const boost::shared_ptr<Natron::Image>& image)
    {
        _originalImage = image;
    }
    
private:

    boost::weak_ptr<Natron::Image> _originalImage;
    RectI _rod;
};

}

#endif // FRAMEPARAMS_H

