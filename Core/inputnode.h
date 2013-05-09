#ifndef INPUTNODE_H
#define INPUTNODE_H
#include "Core/node.h"
class InputNode : public Node
{
public:

    InputNode(Node* node );
    InputNode(InputNode& ref);
    virtual ~InputNode();
    virtual int inputs(){return 0;}
   
    bool isInputNode();
private:
  

};

#endif // INPUTNODE_H
