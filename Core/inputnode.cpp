#include "Core/inputnode.h"
InputNode::InputNode(Node* node ):Node(node){
//    _inputs();
}

InputNode::InputNode(InputNode& ref):Node((Node&)ref){}
InputNode::~InputNode(){}
bool InputNode::isInputNode(){return true;}
