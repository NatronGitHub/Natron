/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#ifndef IMAGEKEY_H
#define IMAGEKEY_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/KeyHelper.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

class ImageKey
    :  public KeyHelper<U64>
{
public:

    U64 _nodeHashKey;
    double _time;
    double _pixelAspect;
    int /*ViewIdx*/ _view; // store it locally as an int for easier serialization
    bool _draftMode;
    bool _frameVaryingOrAnimated;

    //When true that means the image has been computed based on inputs using a mipmaplevel != 0
    //hence it is probably not very high quality, even though the mipmap level is 0
    bool _fullScaleWithDownscaleInputs;

    ImageKey();

    ImageKey(const CacheEntryHolder* holder,
             U64 nodeHashKey,
             bool frameVaryingOrAnimated,
             double time,
             ViewIdx view,
             double pixelAspect,
             bool draftMode,
             bool fullScaleWithDownscaleInputs);

    void fillHash(Hash64* hash) const;

    U64 getTreeVersion() const
    {
        return _nodeHashKey;
    }

    bool operator==(const ImageKey & other) const;

    double getTime() const
    {
        return _time;
    }

    ViewIdx getView() const
    {
        return ViewIdx(_view);
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

NATRON_NAMESPACE_EXIT

#endif // IMAGEKEY_H
