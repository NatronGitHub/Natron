//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "FrameKey.h"

using namespace Natron;

FrameKey::FrameKey()
: KeyHelper<U64>()
, _time(0)
, _treeVersion(0)
, _gain(1)
, _gamma(1)
, _lut(0)
, _bitDepth(0)
, _channels(0)
, _view(0)
, _textureRect()
, _scale()
, _layer()
, _alphaChannelFullName()
{
    _scale.x = _scale.y = 0.;
}

FrameKey::FrameKey(SequenceTime time,
                   U64 treeVersion,
                   double gain,
                   double gamma,
                   int lut,
                   int bitDepth,
                   int channels,
                   int view,
                   const TextureRect & textureRect,
                   const RenderScale & scale,
                   const std::string & inputName,
                   const ImageComponents& layer,
                   const std::string& alphaChannelFullName)
: KeyHelper<U64>()
, _time(time)
, _treeVersion(treeVersion)
, _gain(gain)
, _gamma(gamma)
, _lut(lut)
, _bitDepth(bitDepth)
, _channels(channels)
, _view(view)
, _textureRect(textureRect)
, _scale(scale)
, _inputName(inputName)
, _layer(layer)
, _alphaChannelFullName(alphaChannelFullName)
{
}

void
FrameKey::fillHash(Hash64* hash) const
{
    hash->append(_time);
    hash->append(_treeVersion);
    hash->append(_gain);
    hash->append(_gamma);
    hash->append(_lut);
    hash->append(_bitDepth);
    hash->append(_channels);
    hash->append(_view);
    hash->append(_textureRect.x1);
    hash->append(_textureRect.y1);
    hash->append(_textureRect.x2);
    hash->append(_textureRect.y2);
    hash->append(_textureRect.w);
    hash->append(_textureRect.h);
    hash->append(_textureRect.closestPo2);
    hash->append(_scale.x);
    hash->append(_scale.y);
    Hash64_appendQString(hash,_layer.getLayerName().c_str());
    const std::vector<std::string>& channels = _layer.getComponentsNames();
    for (std::size_t i = 0; i < channels.size(); ++i) {
        Hash64_appendQString(hash,channels[i].c_str());
    }
    if (!_alphaChannelFullName.empty()) {
        Hash64_appendQString(hash,_alphaChannelFullName.c_str());
    }
    
    Hash64_appendQString(hash, _inputName.c_str());
 
}

bool
FrameKey::operator==(const FrameKey & other) const
{
    return _time == other._time &&
    _treeVersion == other._treeVersion &&
    _gain == other._gain &&
    _gamma == other._gamma && 
    _lut == other._lut &&
    _bitDepth == other._bitDepth &&
    _channels == other._channels &&
    _view == other._view &&
    _textureRect == other._textureRect &&
    _scale.x == other._scale.x &&
    _scale.y == other._scale.y &&
    _inputName == other._inputName &&
    _layer == other._layer &&
    _alphaChannelFullName == other._alphaChannelFullName;
}
