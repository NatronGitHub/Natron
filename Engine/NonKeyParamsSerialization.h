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

#ifndef NONKEYPARAMSSERIALIZATION_H
#define NONKEYPARAMSSERIALIZATION_H

#endif // NONKEYPARAMSSERIALIZATION_H

#include "Engine/NonKeyParams.h"

#include "Global/Macros.h"

CLANG_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/binary_iarchive.hpp>
CLANG_DIAG_ON(unused-parameter)
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

using namespace Natron;

template<class Archive>
void NonKeyParams::serialize(Archive & ar,const unsigned int /*version*/)
{
    ar & boost::serialization::make_nvp("Cost",_cost);
    ar & boost::serialization::make_nvp("ElementsCount",_elementsCount);
}

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Natron::NonKeyParams);
