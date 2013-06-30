//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef INPUTNODE_H
#define INPUTNODE_H
#include "Core/node.h"
class InputNode : public Node
{
public:

    InputNode(Node* node );
    InputNode(InputNode& ref);
    virtual ~InputNode();
    virtual int maximumInputs(){return 0;}
    virtual int minimumInputs(){return 0;}
    virtual int maximumSocketCount(){return 10000;}
    virtual bool cacheData()=0;
    bool isInputNode();
private:
  

};

#endif // INPUTNODE_H
