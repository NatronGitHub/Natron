//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef POWITER_ENGINE_HASH64_H_
#define POWITER_ENGINE_HASH64_H_

#include <vector>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"



class Node;
class QString;


/*The hash of a Node is the checksum of the vector of data containing:
    - the values of the current knob for this node + the name of the node
    - the hash values for the  tree upstream
*/

class Hash64 {
    
public:
    Hash64(){hash=0;}
    ~Hash64(){
        node_values.clear();
    }
    
    U64 value() const {return hash;}

    
    void computeHash();
    
    void reset();
    
    void append(U64 hashValue) {
        node_values.push_back(hashValue);
    }
    
    bool 	operator== (const Hash64& h) const {
        return this->hash==h.value();
    }
    bool 	operator!= (const Hash64& h) const {
        return this->hash==h.value();

    }
    
private:
    U64 hash;
    std::vector<U64> node_values;
};

void Hash64_appendQString(Hash64* hash, const QString& str);

#endif // POWITER_ENGINE_Hash64_H_

