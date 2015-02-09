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


#ifndef IMAGEKEY_H
#define IMAGEKEY_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Engine/KeyHelper.h"

namespace Natron {
class ImageKey
        :  public KeyHelper<U64>
{
public:

    U64 _nodeHashKey;
    bool _frameVaryingOrAnimated;
    SequenceTime _time;
    //unsigned int _mipMapLevel;
    int _view;
    double _pixelAspect;

    ImageKey();

    ImageKey(U64 nodeHashKey,
             bool frameVaryingOrAnimated,
             SequenceTime time,
             //unsigned int mipMapLevel, //< Store different mipmapLevels under the same key
             int view,
             double pixelAspect = 1.);

    void fillHash(Hash64* hash) const;

    U64 getTreeVersion() const
    {
        return _nodeHashKey;
    }

    bool operator==(const ImageKey & other) const;

    SequenceTime getTime() const
    {
        return _time;
    }
};




}

#endif // IMAGEKEY_H
