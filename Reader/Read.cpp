//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//  contact: immarespond at gmail dot com
#include <cstdlib>
#include <QtGui/qrgb.h>
#include "Reader/Read.h"
#include "Reader/readExr.h"
#include "Gui/GLViewer.h"
#include "Core/lookUpTables.h"
#include "Superviser/controler.h"
#include "Core/model.h"
#include "Core/VideoEngine.h"
#include "Core/row.h"

Read::Read(Reader* op):is_stereo(false),_premult(false), _autoCreateAlpha(false),op(op),_lut(0)
{
	
	_readInfo = new ReaderInfo; // deleted by the reader which manages this read handle

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
        Linear::from_byte(to, from, W,delta);
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
        Linear::from_short(to, from, W,delta);
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
        Linear::from_float(to, from, W,delta);
    }
}


void Read::createKnobDynamically(){
    op->createKnobDynamically();
}

void Read::setReaderInfo(Format dispW,
	Box2D dataW,
	QString file,
	ChannelMask channels,
	int Ydirection ,
	bool rgb ){
    _readInfo->setDisplayWindow(dispW);
    _readInfo->set(dataW);
    _readInfo->setChannels(channels);
    _readInfo->setYdirection(Ydirection);
    _readInfo->rgbMode(rgb);
    _readInfo->setCurrentFrameName(file.toStdString());
}

void Read::readScanLineData(Reader::Buffer::ScanLineContext* slContext){
    if(slContext->getRowsToRead().size() == 0){
        const std::map<int,int>& rows = slContext->getRows();
        if(_readInfo->getYdirection() < 0){
            //top to bottom
            map<int,int>::const_reverse_iterator it  = rows.rbegin();
            for(; it!=rows.rend() ; it++){
                readScanLine(it->first);
            }
        }else{
            //bottom to top
            map<int,int>::const_iterator it = rows.begin();
            for(; it!=rows.end() ; it++){
                readScanLine(it->first);
            }
        }
    }else{
        const std::vector<int>& rows = slContext->getRowsToRead();
        for(U32 i = 0; i <rows.size();i++){
            readScanLine(rows[i]);
        }
        slContext->merge();
    }
}
void Read::readData(bool openBothViews){
    readAllData(openBothViews);
}
