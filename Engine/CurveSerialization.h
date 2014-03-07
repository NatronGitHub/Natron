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



#ifndef NATRON_ENGINE_CURVESERIALIZATION_H_
#define NATRON_ENGINE_CURVESERIALIZATION_H_

#include "Global/Macros.h"
CLANG_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
CLANG_DIAG_ON(unused-parameter)
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/scoped_ptr.hpp>

#include "Engine/Curve.h"
#include "Engine/CurvePrivate.h"


namespace boost {
namespace serialization {


template<class Archive>
void serialize(Archive & ar,CurvePrivate& c, const unsigned int version)
{
    (void)version;
    QMutexLocker l(&c._lock);
    ar & boost::serialization::make_nvp("KeyFrameSet",c.keyFrames);
}


    
}
}


template<class Archive>
void Curve::serialize(Archive & ar, const unsigned int version)
{
    (void)version;
    ar & boost::serialization::make_nvp("PIMPL",_imp);
}




#endif // NATRON_ENGINE_CURVESERIALIZATION_H_
