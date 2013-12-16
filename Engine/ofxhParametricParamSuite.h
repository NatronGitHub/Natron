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

#ifndef OFXHPARAMETRICPARAMSUITE_H
#define OFXHPARAMETRICPARAMSUITE_H

#include <ofxhParam.h>

namespace OFX {

namespace Host {

namespace ParametricParam {
    
    /// the Descriptor of a plugin parametric parameter
    ///it overloads the base param version to add properties
    ///specific to parametric parameters
    class ParametricDescriptor : public Param::Descriptor
    {
    public:
        
        /// make a parametric parameter, with the given type and name
        ParametricDescriptor(const std::string &type, const std::string &name);
        
    };
    


/// fetch the parametric params suite
void *GetSuite(int version);

} //namespace ParametricParam

} //namespace Host

} //namespace OFX

#endif // OFXHPARAMETRICPARAMSUITE_H
