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

#ifndef POWITER_WRITERS_WRITEQT_H_
#define POWITER_WRITERS_WRITEQT_H_

#include "Global/Macros.h"
#include "Writers/Write.h"

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
     function Powiter::Color::getLut(datatype) */
    virtual void initializeColorSpace();
    
    /*This must be implemented to do the output colorspace conversion*/
	virtual void engine(int y,int offset,int range,ChannelSet channels,Row* out);
    
    /*This function initialises the output file/output storage structure and put necessary info in it, like
     meta-data, channels, etc...This is called on the main thread so don't do any extra processing here,
     otherwise it would stall the GUI.*/
    virtual void setupFile(const std::string& filename);
    
    /*This function must fill the pre-allocated structure with the data calculated by engine.
     This function must close the file as writeAllData is the LAST function called before the
     destructor of Write.*/
    virtual void writeAllData();

    virtual void supportsChannelsForWriting(ChannelSet& channels);
    
    WriteQt(Writer* writer);
    virtual ~WriteQt();
};

#endif /* defined(POWITER_WRITERS_WRITEQT_H_) */
