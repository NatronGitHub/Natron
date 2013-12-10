//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "CurveSerialization.h"

// explicit template instantiations

template void KeyFrame::serialize<boost::archive::xml_iarchive>(
boost::archive::xml_iarchive & ar,
const unsigned int file_version
);
template void KeyFrame::serialize<boost::archive::xml_oarchive>(
boost::archive::xml_oarchive & ar,
const unsigned int file_version
);


template void Curve::serialize<boost::archive::xml_iarchive>(
boost::archive::xml_iarchive & ar,
const unsigned int file_version
);
template void Curve::serialize<boost::archive::xml_oarchive>(
boost::archive::xml_oarchive & ar,
const unsigned int file_version
);
