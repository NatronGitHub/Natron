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

 

 




#ifndef __PowiterOsX__ReadQt__
#define __PowiterOsX__ReadQt__
#include "Reader/Read.h"
#include <QtCore/QMutex>
#include <iostream>
#include <cstdio>
#include <QtGui/QImage>


class Row;
class ReadQt : public Read {

    QImage* _img;
    QString filename;
public:
    static Read* BuildRead(Reader* reader){return new ReadQt(reader);}
    
    ReadQt(Reader* op);
    virtual ~ReadQt();
    /*Should return the list of file types supported by the decoder: "png","jpg", etc..*/
    virtual std::vector<std::string> fileTypesDecoded(){
        std::vector<std::string> out;
        out.push_back("jpg");
        out.push_back("bmp");
        out.push_back("jpeg");
        out.push_back("png");
        out.push_back("gif");
        out.push_back("pbm");
        out.push_back("pgm");
        out.push_back("ppm");
        out.push_back("xbm");
        out.push_back("xpm");
        return out;
    };
    
    /*Should return the name of the reader : "ffmpeg", "OpenEXR" ...*/
    virtual std::string decoderName(){return "QImage (Qt)";}
    virtual void engine(int y,int offset,int range,ChannelSet channels,Row* out);
    virtual bool supports_stereo();
    virtual void readHeader(const QString filename,bool openBothViews);
    virtual void readAllData(bool openBothViews);
    virtual bool supportsScanLine(){return false;}
    virtual void make_preview();
    virtual void initializeColorSpace();
};

#endif /* defined(__PowiterOsX__ReadQt__) */
