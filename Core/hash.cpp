//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include "Core/hash.h"
#include "Core/node.h"
#include "Gui/knob.h"
void Hash::computeHash(){


    boost::crc_32_type result;
    U32* data=(U32*)malloc(sizeof(U32)*node_values.size());

    for(int i=0;i<node_values.size();i++){

        data[i]=node_values[i];
    }
    result.process_bytes(data,node_values.size()*sizeof(U32));
    hash=result.checksum();
    free(data);
}
void Hash::reset(){
    node_values.clear();
    hash=0;
}


void Hash::appendNodeHashToHash(U32 hashValue){
        node_values.push_back(hashValue);
}

void Hash::appendKnobToHash(Knob* knob){
    std::vector<U32> values=knob->getValues();
    for(int i=0;i<values.size();i++){
        node_values.push_back(values[i]);
    }
}

void Hash::appendQStringToHash(QString str){
    for(int i =0 ; i< str.size();i++){
        node_values.push_back((U32)str.at(i).unicode());
    }
}


