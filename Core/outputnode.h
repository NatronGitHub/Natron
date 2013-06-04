//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef OUTPUTNODE_H
#define OUTPUTNODE_H
#include "Core/node.h"


class Model;
class OutputNode : public Node
{
public:

    OutputNode(OutputNode& ref):Node(ref){_currentFrame=-1;}
    OutputNode(Node* node);
   
    virtual ~OutputNode(){}
    
    virtual bool hasOutput(){return false;}
    virtual bool isOutputNode(){return true;}
    virtual void setOutputNb(){_freeOutputCount=0;}
    virtual int totalInputsCount(){return 1;}
   
	int currentFrame(){return _currentFrame;}
	void currentFrame(int c){_currentFrame=c;}

protected:
    int _currentFrame;
private:


    


};


#endif // OUTPUTNODE_H
