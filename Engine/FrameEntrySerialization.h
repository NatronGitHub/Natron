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

namespace boost {
    namespace serialization {
        
        template<class Archive>
        void serialize(Archive & ar, Natron::FrameKey & f, const unsigned int version)
        {
            (void)version;
            ar & boost::serialization::make_nvp("Time",f._time);
            ar & boost::serialization::make_nvp("TreeVersion",f._treeVersion);
            ar & boost::serialization::make_nvp("Exposure",f._exposure);
            ar & boost::serialization::make_nvp("Lut",f._lut);
            ar & boost::serialization::make_nvp("BitDepth",f._bitDepth);
            ar & boost::serialization::make_nvp("Channels",f._channels);
            ar & boost::serialization::make_nvp("View",f._view);
            ar & boost::serialization::make_nvp("TextureRect",f._textureRect);
        }
    }
}



#endif // FRAMEENTRYSERIALIZATION_H
