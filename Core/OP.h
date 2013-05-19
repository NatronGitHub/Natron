//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef OPERATOR_H
#define OPERATOR_H
#include "Core/node.h"
class Operator : public Node
{
public:

    Operator(Operator& ref):Node(ref){}
    Operator(Node* node);
    virtual ~Operator(){}
    virtual int inputs(){return 1;}


};

#endif // OPERATOR_H
