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

#ifndef FRAMEPARAMSSERIALIZATION_H
#define FRAMEPARAMSSERIALIZATION_H

#include "Global/Macros.h"

CLANG_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/binary_iarchive.hpp>
CLANG_DIAG_ON(unused-parameter)
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/base_object.hpp>

#include "Engine/FrameParams.h"

using namespace Natron;

template<class Archive>
void FrameParams::serialize(Archive & ar, const unsigned int /*version*/)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Natron::NonKeyParams);
    ar & boost::serialization::make_nvp("Rod",_rod);
}

#endif // FRAMEPARAMSSERIALIZATION_H
