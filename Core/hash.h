#ifndef HASH_H
#define HASH_H


#include <cstdlib>
#include <boost/crc.hpp>
#include <vector>
#include <QtCore/qstring.h>
#include <QtCore/QChar>
#include "Superviser/powiterFn.h"

using namespace Powiter_Enums;

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
    
    U32 getHashValue() const {return hash;}
    
    void computeHash();
    
    void reset();
    
    void appendNodeHashToHash(U32 hashValue);
    
    void appendQStringToHash(QString str);
    
    void appendKnobToHash(Knob* knob);
    
    bool 	operator== (const Hash &h) const {
        return this->hash==h.getHashValue();
    }
    bool 	operator!= (const Hash &h) const {
        return this->hash==h.getHashValue();
    }
    
private:
    U32 hash;
    std::vector<U32> node_values;
};
#endif // HASH_H
