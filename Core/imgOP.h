//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef IMGOPERATOR_H
#define IMGOPERATOR_H
#include "Core/OP.h"
class ImgOperator : public Operator
{
public:

    ImgOperator(Node* node);
    ImgOperator(ImgOperator& ref):Operator(ref){}


    virtual ~ImgOperator(){}
};

#endif // IMGOPERATOR_H
