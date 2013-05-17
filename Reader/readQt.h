//
//  ReadQt.h
//  PowiterOsX
//
//  Created by Alexandre on 1/15/13.
//  Copyright (c) 2013 Alexandre. All rights reserved.
//

#ifndef __PowiterOsX__ReadQt__
#define __PowiterOsX__ReadQt__
#include "Reader/Read.h"
#include <QtCore/QMutex>
#include <iostream>
#include <cstdio>
#include <QtGui/QImage>



class ReadQt : public Read {

    QImage* _img;
    QString filename;
public:
    ReadQt(Reader* op,ViewerGL* ui_context);
    virtual ~ReadQt();
    virtual void engine(int y,int offset,int range,ChannelMask channels,Row* out);
    virtual bool supports_stereo();
    virtual void open(const QString filename,bool openBothViews = false);
    virtual void make_preview(const char* filename);
};

#endif /* defined(__PowiterOsX__ReadQt__) */
