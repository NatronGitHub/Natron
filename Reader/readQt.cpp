//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#include "Reader/readQt.h"
#include "Gui/GLViewer.h"
#include <QtGui/QImage>
#include <QtGui/QColor>
#include "Reader/Reader.h"
#include "Gui/node_ui.h"
#include "Core/lutclasses.h"
using namespace std;
ReadQt::ReadQt(Reader* op,ViewerGL* ui_context) : Read(op,ui_context){
    _lut=Lut::getLut(Lut::VIEWER);
    _lut->validate();
}

ReadQt::~ReadQt(){
    delete _img;
}
void ReadQt::engine(int y,int offset,int range,ChannelMask channels,Row* out){
    uchar* buffer;
    int h = op->getInfo()->getDisplayWindow().h();
    int Y = h - y - 1;
    buffer = _img->scanLine(Y);
    if(autoAlpha() && !_img->hasAlphaChannel()){
        out->turnOn(Channel_alpha);
    }
    const QRgb* from = reinterpret_cast<const QRgb*>(buffer);
    foreachChannels(z, channels){
        float* to = out->writable(z) + offset;
        if(to!=NULL){
            from_byteQt(z, to, from, (range-offset),1);
        }
    }     
}
bool ReadQt::supports_stereo(){
    return false;
}

void ReadQt::readHeader(const QString filename,bool openBothViews){
    this->filename = filename;
    /*load does actually loads the data too. And we must call it to read the header.
     That means in this case the readAllData function is useless*/
    _img= new QImage(filename);

    if(_img->format() == QImage::Format_Invalid){
        cout << "Couldn't load this image format" << endl;
        return ;
    }
    int width = _img->width();
    int height= _img->height();
    double aspect = _img->dotsPerMeterX()/ _img->dotsPerMeterY();
    
	bool rgb=true;
	int ydirection = -1;
	ChannelMask mask;    
    if(_autoCreateAlpha){
        
		mask = Mask_RGBA;
        
    }else{
        if(_img->format()==QImage::Format_ARGB32 || _img->format()==QImage::Format_ARGB32_Premultiplied
           || _img->format()==QImage::Format_ARGB4444_Premultiplied ||
           _img->format()==QImage::Format_ARGB6666_Premultiplied ||
           _img->format()==QImage::Format_ARGB8555_Premultiplied ||
           _img->format()==QImage::Format_ARGB8565_Premultiplied){
            
            if(_img->format()== QImage::Format_ARGB32_Premultiplied
               || _img->format()== QImage::Format_ARGB4444_Premultiplied
               || _img->format()== QImage::Format_ARGB6666_Premultiplied
               || _img->format()== QImage::Format_ARGB8555_Premultiplied
               || _img->format()== QImage::Format_ARGB8565_Premultiplied){
                _premult=true;
                
            }
            mask = Mask_RGBA;
        }
        else{
			mask = Mask_RGB;
            
        }
    }
    
    Format imageFormat(0,0,width,height,"",aspect);
    Box2D bbox(0,0,width,height);
    setReaderInfo(imageFormat, bbox, filename, mask, ydirection, rgb);
}
void ReadQt::readAllData(bool openBothViews){
// does nothing
}



void ReadQt::make_preview(const char* filename){
    QImage* img = new QImage(filename);
    op->setPreview(img);
}
