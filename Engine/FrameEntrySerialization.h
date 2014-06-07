//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */


#ifndef FRAMEENTRYSERIALIZATION_H
#define FRAMEENTRYSERIALIZATION_H

#include "Engine/FrameEntry.h"
#include "Engine/TextureRectSerialization.h"
#include <boost/serialization/version.hpp>
#define FRAME_KEY_INTRODUCES_INPUT_NAME 2
#define FRAME_KEY_VERSION FRAME_KEY_INTRODUCES_INPUT_NAME
template<class Archive>
void Natron::FrameKey::serialize(Archive & ar, const unsigned int version)
{
    ar & boost::serialization::make_nvp("Time", _time);
    ar & boost::serialization::make_nvp("TreeVersion", _treeVersion);
    ar & boost::serialization::make_nvp("Gain", _gain);
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
}

BOOST_CLASS_VERSION(Natron::FrameKey, FRAME_KEY_VERSION)

#endif // FRAMEENTRYSERIALIZATION_H
