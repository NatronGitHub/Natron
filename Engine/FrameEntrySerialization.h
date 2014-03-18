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

template<class Archive>
void Natron::FrameKey::serialize(Archive & ar, const unsigned int version)
{
    (void)version;
    ar & boost::serialization::make_nvp("Time", _time);
    ar & boost::serialization::make_nvp("TreeVersion", _treeVersion);
    ar & boost::serialization::make_nvp("Exposure", _exposure);
    ar & boost::serialization::make_nvp("Lut", _lut);
    ar & boost::serialization::make_nvp("BitDepth", _bitDepth);
    ar & boost::serialization::make_nvp("Channels", _channels);
    ar & boost::serialization::make_nvp("View", _view);
    ar & boost::serialization::make_nvp("TextureRect", _textureRect);
}



#endif // FRAMEENTRYSERIALIZATION_H
