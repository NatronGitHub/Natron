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

#include "Hash64.h"

#include <algorithm>  // for std::for_each
#ifndef Q_MOC_RUN
#include <boost/crc.hpp>
#endif
#include <QtCore/QString>

#include "Engine/Node.h"

using namespace Natron;

void Hash64::computeHash() {

    if (node_values.empty() ) {
        return;
    }

    const unsigned char* data = reinterpret_cast<const unsigned char*>(node_values.data());
    boost::crc_optimal<64,0x42F0E1EBA9EA3693ULL,0,0,false,false> crc_64;
    crc_64 = std::for_each( data, data+node_values.size()*sizeof(node_values[0]), crc_64 );
    hash = crc_64();
}

void Hash64::reset(){
    node_values.clear();
    hash=0;
}


void Hash64_appendQString(Hash64* hash, const QString& str) {
    for(int i =0 ; i< str.size();++i) {
        hash->append(str.at(i).unicode());
    }
}


