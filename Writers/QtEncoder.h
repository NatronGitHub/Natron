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
#include "Writers/Encoder.h"

class QtEncoder :public Encoder{
    
    Box2D _rod;
    uchar* _buf;
    QString _filename;
    
public:
    
    static Encoder* BuildWrite(Writer* writer){return new QtEncoder(writer);}
    
    QtEncoder(Writer* writer);
    
    virtual ~QtEncoder();

    
    /*Should return the list of file types supported by the encoder: "png","jpg", etc..*/
    virtual std::vector<std::string> fileTypesEncoded() const OVERRIDE;
    
    /*Should return the name of the write handle : "ffmpeg", "OpenEXR" ...*/
    virtual std::string encoderName() const OVERRIDE;
    
    /*Must be implemented to tell whether this file type supports stereovision*/
	virtual bool supports_stereo() const OVERRIDE;
       
    /*Must implement it to initialize the appropriate colorspace  for
     the file type. You can initialize the _lut member by calling the
     function Powiter::Color::getLut(datatype) */
    virtual void initializeColorSpace() OVERRIDE;
    
    /*This must be implemented to do the output colorspace conversion*/
	virtual void renderRow(SequenceTime time,int left,int right,int y,const ChannelSet& channels) OVERRIDE;
    
    /*This function initialises the output file/output storage structure and put necessary info in it, like
     meta-data, channels, etc...This is called on the main thread so don't do any extra processing here,
     otherwise it would stall the GUI.*/
    virtual void setupFile(const QString& filename,const Box2D& rod) OVERRIDE;
    
    /*This function must fill the pre-allocated structure with the data calculated by engine.
     This function must close the file as writeAllData is the LAST function called before the
     destructor of Write.*/
    virtual void writeAllData() OVERRIDE;

    virtual void supportsChannelsForWriting(ChannelSet& channels) const OVERRIDE;
    
    };

#endif /* defined(POWITER_WRITERS_WRITEQT_H_) */
