//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#include "Decoder.h"

#include <cstdlib>
#include <QtGui/qrgb.h>

#include "Global/AppManager.h"

#include "Engine/Row.h"
#include "Engine/ImageInfo.h"

#include "Readers/Reader.h"
#include "Readers/ExrDecoder.h"

using namespace Natron;

Decoder::Decoder(Reader* op_):
_premult(false)
, _autoCreateAlpha(false)
, _reader(op_)
, _lut(0)
, _readerInfo(new ImageInfo)
, _filename()
{
}

Decoder::~Decoder(){
}

void Decoder::from_byte(Channel z, float* to, const uchar* from, const uchar* alpha, int W, int delta ){
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
void Decoder::from_byteQt(Channel z, float* to, const QRgb* from, int W, int delta){
    if( z <= 4 && !_lut->linear()){
        _lut->from_byteQt(to, from, z, _premult, W , delta);
    }
}
void Decoder::from_short(Channel z, float* to, const U16* from, const U16* alpha, int W, int, int delta ){
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
void Decoder::from_float(Channel z, float* to, const float* from, const float* alpha, int W, int delta ){
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

void Decoder::from_byte_rect(float* to,const uchar* from,
                             const RectI& rect,const RectI& rod,
                             Natron::Color::Lut::PackedPixelsFormat inputPacking ,bool invertY ){
    if(!_lut->linear()){
        _lut->from_byte_rect(to,from,rect,rod,invertY,_premult,inputPacking);
    }else{
        Color::linear_from_byte_rect(to,from,rect,rod,invertY,inputPacking);
    }
}

void Decoder::from_short_rect(float* to,const U16* from,
                              const RectI& rect,const RectI& rod,
                              Natron::Color::Lut::PackedPixelsFormat inputPacking ,bool invertY ){
    if(!_lut->linear()){
        _lut->from_short_rect(to,from,rect,rod,invertY,_premult,inputPacking);
    }else{
        Color::linear_from_short_rect(to,from,rect,rod,invertY,inputPacking);
    }
}
void Decoder::from_float_rect(float* to,const float* from,
                              const RectI& rect,const RectI& rod,
                              Natron::Color::Lut::PackedPixelsFormat inputPacking ,bool invertY ){
    if(!_lut->linear()){
        _lut->from_float_rect(to,from,rect,rod,invertY,_premult,inputPacking);
    }else{
        Color::linear_from_float_rect(to,from,rect,rod,invertY,inputPacking);
    }
}

void Decoder::createKnobDynamically(){
    _reader->createKnobDynamically();
}

void Decoder::setReaderInfo(Format dispW,
	const RectI& dataW,
    Natron::ChannelSet channels) {
    _readerInfo->setDisplayWindow(dispW);
    _readerInfo->setRoD(dataW);
    _readerInfo->setChannels(channels);
}

