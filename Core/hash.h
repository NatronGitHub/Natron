//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef HASH_H
#define HASH_H


#include <cstdlib>
#include <boost/crc.hpp>
#include <vector>
#include <QtCore/qstring.h>
#include <QtCore/QChar>
#include "Superviser/powiterFn.h"



class Node;
class Knob;

/*The hash of a Node is the checksum of the vector of data containing:
    - the values of the current knob for this node + the name of the node
    - the hash values for the  tree upstream
*/

class Hash{
    
public:
    Hash(){hash=0;}
    ~Hash(){
        node_values.clear();
    }
    
    U64 getHashValue() const {return hash;}
    
    void computeHash();
    
    void reset();
    
    void appendNodeHashToHash(U64 hashValue);
    
    void appendQStringToHash(QString str);
    
    void appendKnobToHash(Knob* knob);
    
    bool 	operator== (const Hash &h) const {
        return this->hash==h.getHashValue();
    }
    bool 	operator!= (const Hash &h) const {
        return this->hash==h.getHashValue();
    }
    
private:
    U64 hash;
    std::vector<U64> node_values;
};
#endif // HASH_H
