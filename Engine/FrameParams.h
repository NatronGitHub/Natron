#ifndef FRAMEPARAMS_H
#define FRAMEPARAMS_H

#include "Engine/NonKeyParams.h"
#include "Engine/Rect.h"

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

private:

    virtual bool isEqualToVirtual(const NonKeyParams & other) const OVERRIDE FINAL
    {
        const FrameParams & imgParams = static_cast<const FrameParams &>(other);

        return _rod == imgParams._rod;
    }

    RectI _rod;
};
}

#endif // FRAMEPARAMS_H

