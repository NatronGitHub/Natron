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
, _view(0)
, _treeVersion(0)
, _bitDepth(0)
, _textureRect()
, _useShaders(false)
, _draftMode(false)
{
}

FrameKey::FrameKey(SequenceTime time,
                   ViewIdx view,
                   U64 treeVersion,
                   int bitDepth,
                   const TextureRect & textureRect,
                   bool useShaders,
                   bool draftMode)
: KeyHelper<U64>(std::string())
, _time(time)
, _view(view)
, _treeVersion(treeVersion)
, _bitDepth(bitDepth)
, _textureRect(textureRect)
, _useShaders(useShaders)
, _draftMode(draftMode)
{
}

void
FrameKey::fillHash(Hash64* hash) const
{
    hash->append(_time);
    hash->append((int)_view);
    hash->append(_treeVersion);
    hash->append(_bitDepth);
    hash->append(_textureRect.x1);
    hash->append(_textureRect.y1);
    hash->append(_textureRect.x2);
    hash->append(_textureRect.y2);
    hash->append(_textureRect.closestPo2);
    hash->append(_useShaders);
    hash->append(_draftMode);
}

bool
FrameKey::operator==(const FrameKey & other) const
{
    return _time == other._time &&
           _treeVersion == other._treeVersion &&
           _useShaders == other._useShaders &&
           _bitDepth == other._bitDepth &&
           _view == other._view &&
           _textureRect == other._textureRect &&
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
    s->view = (int)_view;
    s->treeHash = _treeVersion;
    switch ((ImageBitDepthEnum)_bitDepth) {
        case eImageBitDepthByte:
            s->bitdepth = kBitDepthSerializationByte;
            break;
        case eImageBitDepthShort:
            s->bitdepth = kBitDepthSerializationShort;
            break;
        case eImageBitDepthHalf:
            s->bitdepth = kBitDepthSerializationHalf;
            break;
        case eImageBitDepthFloat:
            s->bitdepth = kBitDepthSerializationFloat;
            break;
        default:
            break;
    }
    _textureRect.toSerialization(&s->textureRect);
    s->draftMode = _draftMode;
    s->useShader = _useShaders;
}

void
FrameKey::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase & obj)
{
    const SERIALIZATION_NAMESPACE::FrameKeySerialization* s = dynamic_cast<const SERIALIZATION_NAMESPACE::FrameKeySerialization*>(&obj);
    if (!s) {
        return;
    }
    _time = s->frame;
    _view = ViewIdx(s->view);
    _treeVersion = s->treeHash;
    if (s->bitdepth == kBitDepthSerializationByte) {
        _bitDepth = (int)eImageBitDepthByte;
    } else if (s->bitdepth == kBitDepthSerializationShort) {
        _bitDepth = (int)eImageBitDepthShort;
    } else if (s->bitdepth == kBitDepthSerializationHalf) {
        _bitDepth = (int)eImageBitDepthHalf;
    } else if (s->bitdepth == kBitDepthSerializationFloat) {
        _bitDepth = (int)eImageBitDepthFloat;
    }
    _textureRect.fromSerialization(s->textureRect);
    _draftMode = s->draftMode;
    _useShaders = s->useShader;
}
NATRON_NAMESPACE_EXIT;
