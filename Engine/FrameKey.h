/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include "Engine/ImagePlaneDesc.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

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
             ViewIdx view,
             const TextureRect & textureRect,
             unsigned int mipMapLevel,
             const std::string & inputName,
             const ImagePlaneDesc& layer,
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

    unsigned int getMipMapLevel() const
    {
        return _mipMapLevel;
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
    SequenceTime _time; // The frame in the sequence
    U64 _treeVersion; // The hash of the viewer node
    double _gain; // The gain on the viewer (if we don't apply it through GLSL shaders)
    double _gamma;  // The gamma on the viewer (if we don't apply it through GLSL shaders)
    int _lut;  // The lut on the viewer (if we don't apply it through GLSL shaders)
    int _bitDepth;  // The bitdepth of the texture (i.e: 8bit or 32bit fp)
    int _channels; // The display channels, as requested by the user. Note that this will make a new cache entry whenever the user
                   // picks a new value in dropdown on the GUI
    int /*ViewIdx*/ _view; // The view of the frame, store it locally as an int for easier serialization
    TextureRect _textureRect;     // texture rectangle definition (bounds in the original image + width and height)
    unsigned int _mipMapLevel; // The scale of the image from which this texture was made
    std::string _inputName; // The name of the input node used (to not mix up input 1, 2, 3 etc...)
    ImagePlaneDesc _layer; // The Layer of the image
    std::string _alphaChannelFullName; /// e.g: color.a , only used if _channels if A
    bool _useShaders; // Whether GLSL shaders are active or not
    bool _draftMode; // Whether draft mode is enabled or not
};

NATRON_NAMESPACE_EXIT

#endif // FRAMEKEY_H
