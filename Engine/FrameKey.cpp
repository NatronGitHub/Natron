/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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
#include "Serialization/FrameKeySerialization.h"

NATRON_NAMESPACE_ENTER;

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

FrameKey::FrameKey(SequenceTime time,
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
                   const ImageComponents& layer,
                   const std::string& alphaChannelFullName,
                   bool useShaders,
                   bool draftMode)
    : KeyHelper<U64>(std::string())
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
    Hash64::appendQString(QString::fromUtf8( _layer.getLayerName().c_str() ), hash );
    const std::vector<std::string>& channels = _layer.getComponentsNames();
    for (std::size_t i = 0; i < channels.size(); ++i) {
        Hash64::appendQString(QString::fromUtf8( channels[i].c_str() ), hash );
    }
    if ( !_alphaChannelFullName.empty() ) {
        Hash64::appendQString(QString::fromUtf8( _alphaChannelFullName.c_str() ), hash );
    }

    Hash64::appendQString(QString::fromUtf8( _inputName.c_str() ), hash );
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

void
FrameKey::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj)
{
    SERIALIZATION_NAMESPACE::FrameKeySerialization* s = dynamic_cast<SERIALIZATION_NAMESPACE::FrameKeySerialization*>(obj);
    if (!s) {
        return;
    }
    s->frame = _time;
    s->view = _view;
    s->treeHash = _treeVersion;
    s->gain = _gain;
    s->gamma = _gamma;
    s->lut = _lut;
    s->bitdepth = _bitDepth;
    s->channels = _channels;
    _textureRect.toSerialization(&s->textureRect);
    s->inputName = _inputName;
    _layer.toSerialization(&s->layer);
    s->alphaChannelFullName = _alphaChannelFullName;
    s->draftMode = _draftMode;
}

void
FrameKey::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase & obj)
{
    const SERIALIZATION_NAMESPACE::FrameKeySerialization* s = dynamic_cast<const SERIALIZATION_NAMESPACE::FrameKeySerialization*>(&obj);
    if (!s) {
        return;
    }
    _time = s->frame;
    _view = s->view;
    _treeVersion = s->treeHash;
    _gain = s->gain;
    _gamma = s->gamma;
    _lut = s->lut;
    _bitDepth = s->bitdepth;
    _channels = s->channels;
    _textureRect.fromSerialization(s->textureRect);
    _inputName = s->inputName;
    _layer.fromSerialization(s->layer);
    _alphaChannelFullName = s->alphaChannelFullName;
    _draftMode = s->draftMode;
}
NATRON_NAMESPACE_EXIT;
