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

#include "Engine/NonKeyParamsSerialization.h"


// explicit template instantiations


template void Natron::NonKeyParams::serialize<boost::archive::binary_iarchive>(boost::archive::binary_iarchive & ar,
                                                                               const unsigned int file_version);
template void Natron::NonKeyParams::serialize<boost::archive::binary_oarchive>(boost::archive::binary_oarchive & ar,
                                                                               const unsigned int file_version);

template void Natron::NonKeyParams::serialize<boost::archive::xml_iarchive>(   boost::archive::xml_iarchive & ar,
                                                                               const unsigned int file_version);
template void Natron::NonKeyParams::serialize<boost::archive::xml_oarchive>(   boost::archive::xml_oarchive & ar,
                                                                               const unsigned int file_version);
