//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_TEXTURERECTSERIALIZATION_H_
#define NATRON_ENGINE_TEXTURERECTSERIALIZATION_H_

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/version.hpp>

#include "Engine/TextureRect.h"

#define TEXTURE_RECT_SERIALIZATION_INTRODUCES_PAR 2
#define TEXTURE_RECT_VERSION TEXTURE_RECT_SERIALIZATION_INTRODUCES_PAR
namespace boost {
namespace serialization {
template<class Archive>
void
serialize(Archive & ar,
          TextureRect &t,
          const unsigned int version)
{

    ar & t.x1 & t.x2 & t.y1 & t.y2 & t.w & t.h & t.closestPo2;
    if (version >= TEXTURE_RECT_SERIALIZATION_INTRODUCES_PAR) {
        ar & t.par;
    }
}
}
}

BOOST_CLASS_VERSION(TextureRect, TEXTURE_RECT_VERSION);

#endif // NATRON_ENGINE_TEXTURERECTSERIALIZATION_H_
