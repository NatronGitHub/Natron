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

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#include <boost/serialization/version.hpp>
#endif
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
