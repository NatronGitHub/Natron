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

#ifndef FRAMEENTRYSERIALIZATION_H
#define FRAMEENTRYSERIALIZATION_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
#include <boost/serialization/version.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#endif

#include "Engine/FrameEntry.h"
#include "Engine/ImageParamsSerialization.h"
#include "Engine/TextureRectSerialization.h"
#include "Engine/EngineFwd.h"


#define FRAME_KEY_INTRODUCES_INPUT_NAME 2
#define FRAME_KEY_INTRODUCES_LAYERS 3
#define FRAME_KEY_INTRODUCES_GAMMA 4
#define FRAME_KEY_CHANGES_BITDEPTH_ENUM 5
#define FRAME_KEY_HANDLE_FP_CORRECTLY 6
#define FRAME_KEY_INTRODUCES_DRAFT 7
#define FRAME_KEY_INTRODUCES_CACHE_HOLDER_ID 8
#define FRAME_KEY_VERSION FRAME_KEY_INTRODUCES_CACHE_HOLDER_ID

template<class Archive>
void
Natron::FrameKey::serialize(Archive & ar,
                            const unsigned int version)
{
    ar & boost::serialization::make_nvp("Time", _time);
    ar & boost::serialization::make_nvp("TreeVersion", _treeVersion);
    ar & boost::serialization::make_nvp("Gain", _gain);
    if (version >= FRAME_KEY_INTRODUCES_GAMMA) {
        ar & boost::serialization::make_nvp("Gamma", _gamma);
    }
    ar & boost::serialization::make_nvp("Lut", _lut);
    ar & boost::serialization::make_nvp("BitDepth", _bitDepth);
    if (version < FRAME_KEY_CHANGES_BITDEPTH_ENUM) {
        _bitDepth += 1;
    }
    ar & boost::serialization::make_nvp("Channels", _channels);
    ar & boost::serialization::make_nvp("View", _view);
    ar & boost::serialization::make_nvp("TextureRect", _textureRect);
    ar & boost::serialization::make_nvp("ScaleX", _scale.x);
    ar & boost::serialization::make_nvp("ScaleY", _scale.y);

    if (version >= FRAME_KEY_VERSION) {
        ar & boost::serialization::make_nvp("InputName", _inputName);
    }
    
    if (version >= FRAME_KEY_INTRODUCES_LAYERS) {
        ar & boost::serialization::make_nvp("Layer", _layer);
        ar & boost::serialization::make_nvp("Alpha", _alphaChannelFullName);
    }
    if (version >= FRAME_KEY_HANDLE_FP_CORRECTLY) {
        ar & boost::serialization::make_nvp("UserShader", _useShaders);
    } else {
        _useShaders = false;
    }
    if (version >= FRAME_KEY_INTRODUCES_DRAFT) {
        ar & boost::serialization::make_nvp("Draft", _draftMode);
    } else {
        _draftMode = false;
    }
    
    if (version >= FRAME_KEY_INTRODUCES_CACHE_HOLDER_ID) {
        ar & boost::serialization::make_nvp("HolderID",_holderID);
    }
}

BOOST_CLASS_VERSION(Natron::FrameKey, FRAME_KEY_VERSION)
#endif // FRAMEENTRYSERIALIZATION_H
