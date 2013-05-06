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
