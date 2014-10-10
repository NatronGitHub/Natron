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

#include "Engine/KeyHelper.h"

namespace Natron {
class ImageKey
        :  public KeyHelper<U64>
{
public:

    U64 _nodeHashKey;
    SequenceTime _time;
    //unsigned int _mipMapLevel;
    int _view;
    double _pixelAspect;

    ImageKey();

    ImageKey(U64 nodeHashKey,
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
