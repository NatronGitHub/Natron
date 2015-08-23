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

#ifndef FRAMEPARAMSSERIALIZATION_H
#define FRAMEPARAMSSERIALIZATION_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_OFF_48(unused-local-typedefs) //-Wunused-local-typedefs
CLANG_DIAG_OFF(unused-local-typedefs) //-Wunused-local-typedefs
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
GCC_DIAG_ON(unused-parameter)
#include <boost/serialization/base_object.hpp>
GCC_DIAG_ON_48(unused-local-typedefs)
CLANG_DIAG_ON(unused-local-typedefs) //-Wunused-local-typedefs
#endif
#include "Engine/FrameParams.h"

using namespace Natron;

template<class Archive>
void
FrameParams::serialize(Archive & ar,
                       const unsigned int /*version*/)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Natron::NonKeyParams);
    ar & boost::serialization::make_nvp("Rod",_rod);
}

#endif // FRAMEPARAMSSERIALIZATION_H
