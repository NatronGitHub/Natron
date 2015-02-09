#ifndef FRAMEPARAMS_H
#define FRAMEPARAMS_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Engine/Rect.h"
#include "Engine/NonKeyParams.h"

namespace Natron {

class FrameParams
        : public NonKeyParams
{
public:

    FrameParams()
        : NonKeyParams()
        , _rod()
    {
    }

    FrameParams(const FrameParams & other)
        : NonKeyParams(other)
        , _rod(other._rod)
    {
    }

    FrameParams(const RectI & rod,
                int bitDepth,
                int texW,
                int texH)
        : NonKeyParams(1,bitDepth != 0 ? texW * texH * 16 : texW * texH * 4)
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
private:


    RectI _rod;
};

}

#endif // FRAMEPARAMS_H

