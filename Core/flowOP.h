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

 

 




#ifndef FLOWOPERATOR_H
#define FLOWOPERATOR_H
#include "Core/OP.h"
class FlowOperator : public Op
{
public:

    FlowOperator();
    
    virtual bool cacheData(){return false;}

    virtual ~FlowOperator(){}
};

#endif // FLOWOPERATOR_H
