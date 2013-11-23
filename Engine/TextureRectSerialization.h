//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_TEXTURERECTSERIALIZATION_H_
#define NATRON_ENGINE_TEXTURERECTSERIALIZATION_H_

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "Engine/TextureRect.h"

namespace boost {
    namespace serialization {

        template<class Archive>
        void serialize(Archive & ar, TextureRect &t, const unsigned int version)
        {
            (void)version;
            ar & t.x & t.y & t.r & t.t & t.w & t.h;
        }
    }
}

#endif // NATRON_ENGINE_TEXTURERECTSERIALIZATION_H_
