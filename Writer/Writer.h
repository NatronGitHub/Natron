//
//  Writer.h
//  PowiterOsX
//
//  Created by Alexandre on 6/13/13.
//  Copyright (c) 2013 Alexandre. All rights reserved.
//

#ifndef __PowiterOsX__Writer__
#define __PowiterOsX__Writer__

#include <iostream>
#include "Core/outputnode.h"

class Row;
class Knob_Callback;
class Writer: public OutputNode {
    
    
public:
    
    Writer(Node* node);
    
    Writer(Writer& ref);
    
    virtual ~Writer();
    
    virtual std::string className();
    
    virtual std::string description();
    
    virtual void _validate(bool forReal);
	
	virtual void engine(int y,int offset,int range,ChannelMask channels,Row* out);
        
	virtual void createKnobDynamically();
    
protected:
	virtual void initKnobs(Knob_Callback *cb);
};

#endif /* defined(__PowiterOsX__Writer__) */
