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

#ifndef NONKEYPARAMS_H
#define NONKEYPARAMS_H

#include <cstddef>

#include "Global/GlobalDefines.h"

namespace boost {
    namespace serialization {
        class access;
    }
}

namespace Natron {
    


class NonKeyParams {
    
public:
    
    NonKeyParams();
    
    virtual ~NonKeyParams(){}
    
    NonKeyParams(int cost,U64 elementsCount);
    
    NonKeyParams(const NonKeyParams& other);
    
    ///the number of elements the associated cache entry should allocate the first time (relative to the datatype of the entry)
    U64 getElementsCount() const;
    
    int getCost() const;
    
    bool operator==(const NonKeyParams& other) const;
    
    bool operator!=(const NonKeyParams& other) const ;
    
    template<class Archive>
    void serialize(Archive & ar,const unsigned int /*version*/);

    
protected:
    
    virtual bool isEqualToVirtual(const NonKeyParams& /*other*/) const { return true; }
    
private:
    
    
    int _cost; //< the cost of the element associated to this key
    size_t _elementsCount; //< the number of elements the associated cache entry should allocate (relative to the datatype of the entry)
};

}

#endif // NONKEYPARAMS_H
