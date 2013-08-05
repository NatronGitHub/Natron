//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 



#ifndef INPUTNODE_H
#define INPUTNODE_H
#include "Core/node.h"
class InputNode : public Node
{
public:

    InputNode();
    virtual ~InputNode();
    virtual int maximumInputs(){return 0;}
    virtual int minimumInputs(){return 0;}
    virtual bool cacheData()=0;
    bool isInputNode();
private:
  

};

#endif // INPUTNODE_H
