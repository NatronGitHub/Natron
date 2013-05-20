//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include <cstdlib>
#include <QtGui/qrgb.h>
#include "Reader/Read.h"
#include "Reader/Reader.h"
#include "Gui/GLViewer.h"
#include "Core/lookUpTables.h"
#include "Reader/readerInfo.h"
#include "Superviser/controler.h"
#include "Core/model.h"
#include "Core/VideoEngine.h"
Read::Read(Reader* op,ViewerGL* ui_context):is_stereo(false), _autoCreateAlpha(false),_premult(false)
{
	_lut=NULL;
    this->ui_context = ui_context;
	this->op=op;
    _readInfo = new ReaderInfo;
    
}
Read::~Read(){ delete _lut; }


void Read::from_byte(float* r,float* g,float* b, const uchar* from, bool hasAlpha, int W, int delta ,float* a,bool qtbuf){
    if(!_lut->linear()){
        _lut->from_byte(r,g,b,from,hasAlpha,premultiplied(),autoAlpha(), W,delta,a,qtbuf);
    }
    else{
        Linear::from_byte(r,g,b,from,hasAlpha,premultiplied(),autoAlpha(),W,delta,a,qtbuf);
    }
	
}

void Read::from_float(float* r,float* g,float* b, const float* fromR,const float* fromG,
                      const float* fromB, int W, int delta ,const float* fromA,float* a){
    if(!_lut->linear()){
        _lut->from_float(r,g,b,fromR,fromG,fromB,premultiplied(),autoAlpha(), W,delta,fromA,a);
    }
    else{
        Linear::from_float(r,g,b,fromR,fromG,fromB,premultiplied(),autoAlpha(),W,delta,fromA,a);
    }
    
    
    
}



void Read::createKnobDynamically(){
    
}

void Read::setReaderInfo(Format dispW,
                   Box2D dataW,
                   QString file,
                   ChannelMask channels,
                   int Ydirection ,
                   bool rgb ){
	_readInfo->displayWindow(dispW.x(), dispW.y(), dispW.right(), dispW.top());
    _readInfo->setDisplayWindowName(dispW.name());
    _readInfo->pixelAspect(dispW.pixel_aspect());
    _readInfo->dataWindow(dataW.x(), dataW.y(), dataW.right(), dataW.top());
    _readInfo->channels(channels);
    _readInfo->Ydirection(Ydirection);
    _readInfo->rgbMode(rgb);
//	ui_context->getControler()->getModel()->getVideoEngine()->pushReaderInfo(_readInfo,op);
}