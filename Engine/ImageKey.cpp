// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "ImageKey.h"

using namespace Natron;

ImageKey::ImageKey()
: KeyHelper<U64>()
, _nodeHashKey(0)
, _frameVaryingOrAnimated(false)
, _time(0)
//, _mipMapLevel(0)
, _view(0)
, _pixelAspect(1)
{
}

ImageKey::ImageKey(U64 nodeHashKey,
                   bool frameVaryingOrAnimated,
                   SequenceTime time,
                   //unsigned int mipMapLevel, //< Store different mipmapLevels under the same key
                   int view,
                   double pixelAspect)
: KeyHelper<U64>()
, _nodeHashKey(nodeHashKey)
, _frameVaryingOrAnimated(frameVaryingOrAnimated)
, _time(time)
//      , _mipMapLevel(mipMapLevel)
, _view(view)
, _pixelAspect(pixelAspect)
{
}

void
ImageKey::fillHash(Hash64* hash) const
{
    hash->append(_nodeHashKey);
    //hash->append(_mipMapLevel); //< Store different mipmapLevels under the same key
    if (_frameVaryingOrAnimated) {
        hash->append(_time);
    }
    hash->append(_view);
    hash->append(_pixelAspect);
}

bool
ImageKey::operator==(const ImageKey & other) const
{
    if (_frameVaryingOrAnimated) {
        return _nodeHashKey == other._nodeHashKey &&
        _time == other._time &&
        _view == other._view &&
        _pixelAspect == other._pixelAspect;
    } else {
        return _nodeHashKey == other._nodeHashKey &&
        _view == other._view &&
        _pixelAspect == other._pixelAspect;
    }
    
}
