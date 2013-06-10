//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef IMGOPERATOR_H
#define IMGOPERATOR_H
#include "Core/OP.h"
class ImgOp : public Op
{
public:

    ImgOp(Node* node);
    ImgOp(ImgOp& ref):Op(ref){}

    virtual bool cacheData()=0;

    virtual ~ImgOp(){}
};

#endif // IMGOPERATOR_H
