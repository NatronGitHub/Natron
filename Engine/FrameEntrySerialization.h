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


#ifndef FRAMEENTRYSERIALIZATION_H
#define FRAMEENTRYSERIALIZATION_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Engine/FrameEntry.h"
#include "Engine/ImageParamsSerialization.h"
#include "Engine/TextureRectSerialization.h"
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/serialization/version.hpp>
#endif
#define FRAME_KEY_INTRODUCES_INPUT_NAME 2
#define FRAME_KEY_INTRODUCES_LAYERS 3
#define FRAME_KEY_INTRODUCES_GAMMA 4
#define FRAME_KEY_VERSION FRAME_KEY_INTRODUCES_GAMMA
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
}

BOOST_CLASS_VERSION(Natron::FrameKey, FRAME_KEY_VERSION)
#endif // FRAMEENTRYSERIALIZATION_H
