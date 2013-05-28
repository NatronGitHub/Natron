//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include <cstdlib>
#include <QtGui/qrgb.h>
#include "Reader/Read.h"
#include "Reader/Reader.h"
#include "Reader/readExr.h"
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
Read::~Read(){
    if(_lut)
        delete _lut;
}


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


void Read::open(const QString filename,bool fitFrameToviewer,bool openBothViews){
    readHeader(filename,openBothViews);
    if(supportsScanLine()){
        
        float h = (float)(_readInfo->displayWindow().h());
        std::pair<int,int> incr;
        float zoomFactor = (float)ui_context->height()/h -0.05;
        float builtInZoom;
        if(fitFrameToviewer){
            builtInZoom = ui_context->getBuiltinZooms().closestBuiltinZoom(zoomFactor);
            incr = ui_context->getBuiltinZooms()[builtInZoom];
            ui_context->initViewer(_readInfo->displayWindow());
        }else{
            builtInZoom = ui_context->getCurrentBuiltinZoom();
            incr = ui_context->getZoomIncrement();
        }
        _readInfo->setBuiltInZoom(builtInZoom);
        pair<int,int> rowSpan = ui_context->getRowSpan(_readInfo->displayWindow(), builtInZoom);
        //cout << "read rowspan: " << rowSpan.first << " " << rowSpan.second << endl;
        float incrementNew = incr.first;
        float incrementFullsize = incr.second;
        int Ydirection = _readInfo->Ydirection();
        int startY=0,endY=0;
        int y= 0; 
        int rowY = 0;
        bool mainCondition;
        if(Ydirection < 0){// Ydirection < 0 means we cycle from top to bottom
            rowY = rowSpan.first;
            startY = rowSpan.first;
            endY = rowSpan.second-1;
        }else{
            rowY = rowSpan.second;
            startY = rowSpan.second;
            endY = rowSpan.first+1;
        }
        y = startY;
        Ydirection < 0 ? mainCondition = y > endY : mainCondition = y < endY;
        while (mainCondition){
            int k = y;
            bool condition;
            if(Ydirection < 0){
                condition = k > (y -incrementNew) && (rowY >= 0) && (k > endY);
            }else{
                condition = k < (incrementNew + y);
            }
            while(condition){
                readScanLine(k);
                Ydirection < 0 ? rowY-- : rowY++ ;
                Ydirection < 0 ?  k-- : k++ ;
                if(Ydirection < 0){
                    condition = k > (y -incrementNew) && (rowY >= 0) && (k > endY);
                }else{
                    condition = k < (incrementNew + y);
                }
            }
            Ydirection < 0 ? y-=incrementFullsize : y+=incrementFullsize;
            Ydirection < 0 ? mainCondition = y > endY : mainCondition = y < endY;
        }
    }else{
        readAllData(openBothViews);
        _readInfo->setBuiltInZoom(ui_context->getCurrentBuiltinZoom());
    }
}