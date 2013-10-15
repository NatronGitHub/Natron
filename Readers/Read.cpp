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

#include "Read.h"

#include <cstdlib>
#include <QtGui/qrgb.h>

#include "Readers/ReadExr.h"
#include "Gui/ViewerGL.h"
#include "Engine/Lut.h"
#include "Global/AppManager.h"
#include "Engine/Model.h"
#include "Engine/VideoEngine.h"
#include "Engine/Row.h"

using namespace std;
using namespace Powiter;

Read::Read(Reader* op_)
: is_stereo(false)
, _premult(false)
, _autoCreateAlpha(false)
, op(op_)
, _lut(0)
, _readerInfo(new ImageInfo)
{
}

Read::~Read(){
}

void Read::from_byte(Channel z, float* to, const uchar* from, const uchar* alpha, int W, int delta ){
    if( z <= 3 && !_lut->linear()){
        if(alpha && _premult){
            _lut->from_byte(to, from, alpha, W,delta);
        }else{
            _lut->from_byte(to, from, W,delta);
        }
    }else{
        Color::linear_from_byte(to, from, W,delta);
    }
}
void Read::from_byteQt(Channel z, float* to, const QRgb* from, int W, int delta){
    if( z <= 4 && !_lut->linear()){
        _lut->from_byteQt(to, from, z, _premult, W , delta);
    }
}
void Read::from_short(Channel z, float* to, const U16* from, const U16* alpha, int W, int, int delta ){
    if( z <= 3 && !_lut->linear()){
        if(alpha && _premult){
            _lut->from_short(to, from, alpha, W,delta);
        }else{
            _lut->from_short(to, from, W,delta);
        }
    }else{
        Color::linear_from_short(to, from, W,delta);
    }
}
void Read::from_float(Channel z, float* to, const float* from, const float* alpha, int W, int delta ){
    if( z <= 3 && !_lut->linear()){
        if(alpha && _premult){
            _lut->from_float(to, from, alpha, W,delta);
        }else{
            _lut->from_float(to, from, W,delta);
        }
    }else{
        Color::linear_from_float(to, from, W,delta);
    }
}


void Read::createKnobDynamically(){
    op->createKnobDynamically();
}

void Read::setReaderInfo(Format dispW,
	const Box2D& dataW,
	ChannelSet channels) {
    _readerInfo->setDisplayWindow(dispW);
    _readerInfo->setDataWindow(dataW);
    _readerInfo->setChannels(channels);
}

