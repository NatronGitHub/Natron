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

#ifndef FRAMEKEY_H
#define FRAMEKEY_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/KeyHelper.h"
#include "Engine/TextureRect.h"
#include "Engine/ImageComponents.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"

#include "Serialization/SerializationBase.h"

NATRON_NAMESPACE_ENTER;

class FrameKey
    : public KeyHelper<U64>
    , public SERIALIZATION_NAMESPACE::SerializableObjectBase
{
public:

    typedef SERIALIZATION_NAMESPACE::FrameKeySerialization SerializationType;

    FrameKey();

    FrameKey(SequenceTime time,
             ViewIdx view,
             U64 treeVersion,
             int bitDepth,
             const TextureRect & textureRect,
             bool useShaders,
             bool draftMode);

    virtual void fillHash(Hash64* hash) const OVERRIDE FINAL;

    bool operator==(const FrameKey & other) const;

    SequenceTime getTime() const WARN_UNUSED_RETURN
    {
        return _time;
    };

    int getBitDepth() const WARN_UNUSED_RETURN
    {
        return _bitDepth;
    };

    U64 getTreeVersion() const WARN_UNUSED_RETURN
    {
        return _treeVersion;
    }

    int getView() const WARN_UNUSED_RETURN
    {
        return _view;
    }



    const TextureRect& getTexRect() const WARN_UNUSED_RETURN
    {
        return _textureRect;
    }


    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj) OVERRIDE FINAL;

    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase & obj) OVERRIDE FINAL;

private:
    SequenceTime _time; // The frame in the sequence
    ViewIdx _view; // The view of the frame
    U64 _treeVersion; // The hash of the viewer process node + viewer node
    int _bitDepth;  // The bitdepth of the texture (i.e: 8bit or 32bit fp)

    TextureRect _textureRect;     // texture rectangle definition (bounds in the original image + width and height)
    bool _useShaders; // Whether GLSL shaders are active or not
    bool _draftMode; // Whether draft mode is enabled or not
};

NATRON_NAMESPACE_EXIT;

#endif // FRAMEKEY_H
