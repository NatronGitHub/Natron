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


#ifndef FRAMEKEY_H
#define FRAMEKEY_H

#include "Engine/KeyHelper.h"
#include "Engine/TextureRect.h"

namespace Natron {
class FrameKey
        : public KeyHelper<U64>
{
    friend class boost::serialization::access;

public:
    FrameKey();

    FrameKey(SequenceTime time,
             U64 treeVersion,
             double gain,
             int lut,
             int bitDepth,
             int channels,
             int view,
             const TextureRect & textureRect,
             const RenderScale & scale,
             const std::string & inputName);

    void fillHash(Hash64* hash) const;

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

    double getGain() const WARN_UNUSED_RETURN
    {
        return _gain;
    }

    int getLut() const WARN_UNUSED_RETURN
    {
        return _lut;
    }

    int getChannels() const WARN_UNUSED_RETURN
    {
        return _channels;
    }

    int getView() const WARN_UNUSED_RETURN
    {
        return _view;
    }

    const RenderScale & getScale() const WARN_UNUSED_RETURN
    {
        return _scale;
    }

    const std::string & getInputName() const WARN_UNUSED_RETURN
    {
        return _inputName;
    }

private:
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
    SequenceTime _time;
    U64 _treeVersion;
    double _gain;
    int _lut;
    int _bitDepth;
    int _channels;
    int _view;
    TextureRect _textureRect;     // texture rectangle definition (bounds in the original image + width and height)
    RenderScale _scale;
    std::string _inputName;
};
}

#endif // FRAMEKEY_H
