#ifndef FRAMEPARAMS_H
#define FRAMEPARAMS_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include "Engine/RectI.h"
#include "Engine/NonKeyParams.h"

namespace Natron {

class Image;
class FrameParams
        : public NonKeyParams
{
public:

    FrameParams()
        : NonKeyParams()
        , _tiles()
        , _rod()
    {
    }

    FrameParams(const FrameParams & other)
        : NonKeyParams(other)
        , _tiles(other._tiles)
        , _rod(other._rod)
    {
    }

    FrameParams(const RectI & rod,
                int bitDepth,
                int texW,
                int texH,
                const boost::shared_ptr<Natron::Image>& originalImage)
        : NonKeyParams(1,bitDepth != 0 ? texW * texH * 16 : texW * texH * 4)
        , _tiles()
        , _rod(rod)
    {
        if (originalImage) {
            _tiles.push_back(originalImage);
        }
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
    
    void getOriginalTiles(std::list<boost::shared_ptr<Natron::Image> >* ret) const
    {
        for (std::list<boost::weak_ptr<Natron::Image> >::const_iterator it = _tiles.begin(); it != _tiles.end(); ++it) {
            ret->push_back(it->lock());
        }
    }
    
    void addOriginalTile(const boost::shared_ptr<Natron::Image>& image)
    {
        _tiles.push_back(image);
    }
    
private:

    std::list<boost::weak_ptr<Natron::Image> > _tiles;
    RectI _rod;
};

}

#endif // FRAMEPARAMS_H

