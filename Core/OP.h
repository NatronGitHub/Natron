//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef OPERATOR_H
#define OPERATOR_H
#include "Core/node.h"
class Op : public Node
{
public:

    Op(Op& ref):Node(ref){}
    Op(Node* node);
    virtual ~Op(){}


};

#endif // OPERATOR_H
