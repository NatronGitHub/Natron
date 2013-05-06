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
