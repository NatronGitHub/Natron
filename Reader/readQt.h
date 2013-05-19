//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

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
