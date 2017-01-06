/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#include "ImageKey.h"

#include <cassert>
#include <stdexcept>

#include "Engine/ViewIdx.h"
#include "Serialization/ImageKeySerialization.h"


NATRON_NAMESPACE_ENTER;

ImageKey::ImageKey()
    : KeyHelper<U64>()
    , _nodeHashKey(0)
    , _time(0)
    , _view(0)
    , _draftMode(false)
{
}

ImageKey::ImageKey(const std::string& pluginID,
                   U64 nodeHashKey,
                   double time,
                   //unsigned int mipMapLevel, //< Store different mipmapLevels under the same key
                   ViewIdx view,
                   bool draftMode)
    : KeyHelper<U64>(pluginID)
    , _nodeHashKey(nodeHashKey)
    , _time(time)
    , _view(view)
    , _draftMode(draftMode)
{
}

void
ImageKey::fillHash(Hash64* hash) const
{
    hash->append(_nodeHashKey);
    hash->append(_draftMode);
}

bool
ImageKey::operator==(const ImageKey & other) const
{
    // Do not compare view & time as they are already encoded into the hash
    return _nodeHashKey == other._nodeHashKey &&
    _draftMode == other._draftMode;

}

void
ImageKey::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase)
{
    SERIALIZATION_NAMESPACE::ImageKeySerialization* serialization = dynamic_cast<SERIALIZATION_NAMESPACE::ImageKeySerialization*>(serializationBase);
    if (!serialization) {
        return;
    }
    serialization->nodeHashKey = _nodeHashKey;
    serialization->time = _time;
    serialization->view = (int)_view;
    serialization->draft = _draftMode;
}

void
ImageKey::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase)
{
    const SERIALIZATION_NAMESPACE::ImageKeySerialization* serialization = dynamic_cast<const SERIALIZATION_NAMESPACE::ImageKeySerialization*>(&serializationBase);
    if (!serialization) {
        return;
    }
    _nodeHashKey = serialization->nodeHashKey;
    _view = ViewIdx(serialization->view);
    _draftMode = serialization->draft;
}

NATRON_NAMESPACE_EXIT;
