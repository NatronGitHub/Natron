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
