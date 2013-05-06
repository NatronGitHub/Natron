//
//  readJpeg.h
//  PowiterOsX
//
//  Created by Alexandre on 1/15/13.
//  Copyright (c) 2013 Alexandre. All rights reserved.
//

#ifndef __PowiterOsX__readJpeg__
#define __PowiterOsX__readJpeg__
#include "Reader/Read.h"
#include <QtCore/QMutex>
#include <iostream>
#include <cstdio>
#include <QtGui/QImage>



class ReadJPEG : public Read {

    QImage* _img;
    
public:
    ReadJPEG(const QStringList& file_list,Reader* op,ViewerGL* ui_context);
    virtual ~ReadJPEG();
    virtual void engine(int y,int offset,int range,ChannelMask channels,Row* out);
    virtual bool supports_stereo();
    virtual void open();
    virtual void make_preview();
};

#endif /* defined(__PowiterOsX__readJpeg__) */
