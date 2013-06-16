//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com


#ifndef __PowiterOsX__writeQt__
#define __PowiterOsX__writeQt__

#include <iostream>
#include "Superviser/powiterFn.h"
#include "Writer/Write.h"
#include <QtGui/QImage>

class WriteQt :public Write{
    
    uchar* _buf;
    std::string _filename;
    
public:
    
    static Write* BuildWrite(Writer* writer){return new WriteQt(writer);}
    
    /*Should return the list of file types supported by the encoder: "png","jpg", etc..*/
    virtual std::vector<std::string> fileTypesEncoded();
    
    /*Should return the name of the write handle : "ffmpeg", "OpenEXR" ...*/
    virtual std::string encoderName();
    
    /*Must be implemented to tell whether this file type supports stereovision*/
	virtual bool supports_stereo();
       
    /*Must implement it to initialize the appropriate colorspace  for
     the file type. You can initialize the _lut member by calling the
     function Lut::getLut(datatype) */
    virtual void initializeColorSpace();
    
    /*This must be implemented to do the output colorspace conversion*/
	virtual void engine(int y,int offset,int range,ChannelMask channels,Row* out);
    
    /*This function initialises the output file/output storage structure and put necessary info in it, like
     meta-data, channels, etc...This is called on the main thread so don't do any extra processing here,
     otherwise it would stall the GUI.*/
    virtual void setupFile(std::string filename);
    
    /*This function must fill the pre-allocated structure with the data calculated by engine.
     This function must close the file as writeAllData is the LAST function called before the
     destructor of Write.*/
    virtual void writeAllData();

    virtual void supportsChannelsForWriting(ChannelSet& channels);
    
    WriteQt(Writer* writer);
    virtual ~WriteQt();
};

#endif /* defined(__PowiterOsX__writeQt__) */
