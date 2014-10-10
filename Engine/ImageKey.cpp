#include "ImageKey.h"

using namespace Natron;

ImageKey::ImageKey()
: KeyHelper<U64>()
, _nodeHashKey(0)
, _time(0)
//, _mipMapLevel(0)
, _view(0)
, _pixelAspect(1)
{
}

ImageKey::ImageKey(U64 nodeHashKey,
                   SequenceTime time,
                   //unsigned int mipMapLevel, //< Store different mipmapLevels under the same key
                   int view,
                   double pixelAspect)
: KeyHelper<U64>()
, _nodeHashKey(nodeHashKey)
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
    hash->append(_time);
    hash->append(_view);
    hash->append(_pixelAspect);
}

bool
ImageKey::operator==(const ImageKey & other) const
{
    return _nodeHashKey == other._nodeHashKey &&
    //_mipMapLevel == other._mipMapLevel && //< Store different mipmapLevels under the same key
    _time == other._time &&
    _view == other._view &&
    _pixelAspect == other._pixelAspect;
}
