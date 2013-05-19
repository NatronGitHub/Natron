//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#ifndef FLOWOPERATOR_H
#define FLOWOPERATOR_H
#include "Core/OP.h"
class FlowOperator : public Operator
{
public:

    FlowOperator(FlowOperator& ref):Operator(ref){}
    FlowOperator(Node* node);


    virtual ~FlowOperator(){}
};

#endif // FLOWOPERATOR_H
