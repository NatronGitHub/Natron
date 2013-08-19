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

 

 




#include "Write.h"
#include "Core/Row.h"
#include "Writer/Writer.h"
#include "Core/Lut.h"
#include "Gui/knob.h"
using namespace Powiter;
/*Constructors should initialize variables, but shouldn't do any heavy computations, as these objects
 are oftenly re-created. To initialize the input color-space , you can do so by overloading
 initializeColorSpace. This function is called after the constructor and before any
 reading occurs.*/
Write::Write(Writer* writer):_premult(false),_lut(0),op(writer),_optionalKnobs(0){
    
}

Write::~Write(){
    
}


void Write::writeAndDelete(){
    writeAllData();
    delete this;
}

void Write::to_byte(Channel z, uchar* to, const float* from, const float* alpha, int W, int delta ){
    if( z <= 3 && !_lut->linear()){
        if(alpha && _premult){
            _lut->to_byte(to, from, alpha, W,delta);
        }else{
            _lut->to_byte(to, from, W,delta);
        }
    }else{
        Linear::to_byte(to, from, W,delta);
    }
}
void Write::to_short(Channel z, U16* to, const float* from, const float* alpha, int W, int , int delta){
    if( z <= 3 && !_lut->linear()){
        if(alpha && _premult){
            _lut->to_short(to, from, alpha, W,delta);
        }else{
            _lut->to_short(to, from, W,delta);
        }
    }else{
        Linear::to_short(to, from, W,delta);
    }
}
void Write::to_float(Channel z, float* to, const float* from, const float* alpha, int W, int delta ){
    if( z <= 3 && !_lut->linear()){
        if(alpha && _premult){
            _lut->to_float(to, from, alpha, W,delta);
        }else{
            _lut->to_float(to, from, W,delta);
        }
    }else{
        Linear::to_float(to, from, W,delta);
    }
}

void WriteKnobs::initKnobs(Knob_Callback*,std::string&){
    
    _op->createKnobDynamically();
}

