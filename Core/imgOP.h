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

 

 



#ifndef IMGOPERATOR_H
#define IMGOPERATOR_H
#include "Core/OP.h"
class ImgOp : public Op
{
public:

    ImgOp();

    virtual bool cacheData()=0;

    virtual ~ImgOp(){}
};

#endif // IMGOPERATOR_H
