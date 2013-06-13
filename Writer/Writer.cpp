//
//  Writer.cpp
//  PowiterOsX
//
//  Created by Alexandre on 6/13/13.
//  Copyright (c) 2013 Alexandre. All rights reserved.
//

#include "Writer.h"
#include "Gui/knob_callback.h"
#include "Core/row.h"

Writer::Writer(Node* node):OutputNode(node){
    
}

Writer::Writer(Writer& ref):OutputNode(ref){
    
}

Writer::~Writer(){
    
}

std::string Writer::className(){
    return "Writer";
}

std::string Writer::description(){
    return "OutputNode";
}

void Writer::_validate(bool forReal){
    if(_parents.size()==1){
		copy_info(_parents[0],forReal);
	}
    setOutputChannels(Mask_All);
}

void Writer::engine(int y,int offset,int range,ChannelMask channels,Row* out){
    
}

void Writer::createKnobDynamically(){
    
}
void Writer::initKnobs(Knob_Callback *cb){
    
}
