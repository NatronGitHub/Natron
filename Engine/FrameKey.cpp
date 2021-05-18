/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "FrameKey.h"

#include <cassert>
#include <stdexcept>

#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER

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
    , _mipMapLevel(0)
    , _layer()
    , _alphaChannelFullName()
    , _useShaders(false)
    , _draftMode(false)
{
}

FrameKey::FrameKey(const CacheEntryHolder* holder,
                   SequenceTime time,
                   U64 treeVersion,
                   double gain,
                   double gamma,
                   int lut,
                   int bitDepth,
                   int channels,
                   ViewIdx view,
                   const TextureRect & textureRect,
                   unsigned int mipMapLevel,
                   const std::string & inputName,
                   const ImagePlaneDesc& layer,
                   const std::string& alphaChannelFullName,
                   bool useShaders,
                   bool draftMode)
    : KeyHelper<U64>(holder)
    , _time(time)
    , _treeVersion(treeVersion)
    , _gain(gain)
    , _gamma(gamma)
    , _lut(lut)
    , _bitDepth(bitDepth)
    , _channels(channels)
    , _view(view)
    , _textureRect(textureRect)
    , _mipMapLevel(mipMapLevel)
    , _inputName(inputName)
    , _layer(layer)
    , _alphaChannelFullName(alphaChannelFullName)
    , _useShaders(useShaders)
    , _draftMode(draftMode)
{
}

void
FrameKey::fillHash(Hash64* hash) const
{
    hash->append(_time);
    hash->append(_treeVersion);
    if (!_useShaders) {
        hash->append(_gain);
        hash->append(_gamma);
        hash->append(_lut);
    }
    hash->append(_bitDepth);
    hash->append(_channels);
    hash->append(_view);
    hash->append(_textureRect.x1);
    hash->append(_textureRect.y1);
    hash->append(_textureRect.x2);
    hash->append(_textureRect.y2);
    hash->append(_textureRect.closestPo2);
    hash->append(_mipMapLevel);
    Hash64_appendQString( hash, QString::fromUtf8( _layer.getPlaneID().c_str() ) );
    const std::vector<std::string>& channels = _layer.getChannels();
    for (std::size_t i = 0; i < channels.size(); ++i) {
        Hash64_appendQString( hash, QString::fromUtf8( channels[i].c_str() ) );
    }
    if ( !_alphaChannelFullName.empty() ) {
        Hash64_appendQString( hash, QString::fromUtf8( _alphaChannelFullName.c_str() ) );
    }

    Hash64_appendQString( hash, QString::fromUtf8( _inputName.c_str() ) );
    hash->append(_draftMode);
}

bool
FrameKey::operator==(const FrameKey & other) const
{
    return _time == other._time &&
           _treeVersion == other._treeVersion &&
           ( (_gain == other._gain &&
              _gamma == other._gamma &&
              _lut == other._lut) || (_useShaders && other._useShaders) ) &&
           _bitDepth == other._bitDepth &&
           _channels == other._channels &&
           _view == other._view &&
           _textureRect == other._textureRect &&
           _mipMapLevel == other._mipMapLevel &&
           _inputName == other._inputName &&
           _layer == other._layer &&
           _alphaChannelFullName == other._alphaChannelFullName &&
           _draftMode == other._draftMode;
}

NATRON_NAMESPACE_EXIT
