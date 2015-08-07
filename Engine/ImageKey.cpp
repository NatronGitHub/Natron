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
, _draftMode(false)
{
}

ImageKey::ImageKey(U64 nodeHashKey,
                   bool frameVaryingOrAnimated,
                   double time,
                   //unsigned int mipMapLevel, //< Store different mipmapLevels under the same key
                   int view,
                   double pixelAspect,
                   bool draftMode)
: KeyHelper<U64>()
, _nodeHashKey(nodeHashKey)
, _frameVaryingOrAnimated(frameVaryingOrAnimated)
, _time(time)
, _view(view)
, _pixelAspect(pixelAspect)
, _draftMode(draftMode)
{
}

void
ImageKey::fillHash(Hash64* hash) const
{
    hash->append(_nodeHashKey);
    if (_frameVaryingOrAnimated) {
        hash->append(_time);
    }
    hash->append(_view);
    hash->append(_pixelAspect);
    hash->append(_draftMode);
}

bool
ImageKey::operator==(const ImageKey & other) const
{
    if (_frameVaryingOrAnimated) {
        return _nodeHashKey == other._nodeHashKey &&
        _time == other._time &&
        _view == other._view &&
        _pixelAspect == other._pixelAspect &&
        _draftMode == other._draftMode;
    } else {
        return _nodeHashKey == other._nodeHashKey &&
        _view == other._view &&
        _pixelAspect == other._pixelAspect &&
        _draftMode == other._draftMode;
    }
    
}
