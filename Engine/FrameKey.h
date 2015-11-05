/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include "Engine/KeyHelper.h"
#include "Engine/TextureRect.h"
#include "Engine/ImageComponents.h"
#include "Engine/EngineFwd.h"

namespace Natron {
class FrameKey
        : public KeyHelper<U64>
{
public:
    FrameKey();

    FrameKey(const CacheEntryHolder* holder,
             SequenceTime time,
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
             const std::string& alphaChannelFullName,
             bool useShaders,
             bool draftMode);

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

    double getGamma() const WARN_UNUSED_RETURN
    {
        return _gamma;
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

    const TextureRect& getTexRect() const WARN_UNUSED_RETURN
    {
    return _textureRect;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

private:
    SequenceTime _time;
    U64 _treeVersion;
    double _gain;
    double _gamma;
    int _lut;
    int _bitDepth;
    int _channels;
    int _view;
    TextureRect _textureRect;     // texture rectangle definition (bounds in the original image + width and height)
    RenderScale _scale;
    std::string _inputName;
    ImageComponents _layer;
    std::string _alphaChannelFullName; /// e.g: color.a , only used if _channels if A
    bool _useShaders;
    bool _draftMode;
};
}

#endif // FRAMEKEY_H
