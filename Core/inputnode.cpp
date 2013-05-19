//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include "Core/inputnode.h"
InputNode::InputNode(Node* node ):Node(node){
//    _inputs();
}

InputNode::InputNode(InputNode& ref):Node((Node&)ref){}
InputNode::~InputNode(){}
bool InputNode::isInputNode(){return true;}
