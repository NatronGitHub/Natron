//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com


#ifndef __PowiterOsX__Write__
#define __PowiterOsX__Write__

#include <iostream>
#include "Core/channels.h"
class Lut;
class Writer;
class Row;
class Write {
    
protected:
    bool _premult; // on if the user wants to write alpha pre-multiplied images
    Lut* _lut;
    Writer* op;

public:
    
    /*Constructors should initialize variables, but shouldn't do any heavy computations, as these objects
     are oftenly re-created. To initialize the input color-space , you can do so by overloading
     initializeColorSpace. This function is called after the constructor and before any
     reading occurs.*/
	Write(Writer* writer);
    
	virtual ~Write();
    
    void premultiplyByAlpha(bool premult){_premult = premult;}
    
    /*calls writeAllData and calls the destructor*/
    void writeAndDelete();
    
    /*Should return the list of file types supported by the encoder: "png","jpg", etc..*/
    virtual std::vector<std::string> fileTypesEncoded()=0;
    
    /*Should return the name of the write handle : "ffmpeg", "OpenEXR" ...*/
    virtual std::string encoderName()=0;
    
    /*Must be implemented to tell whether this file type supports stereovision*/
	virtual bool supports_stereo()=0;
    
    /*can be overloaded to add knobs dynamically to the writer depending on the file type.
     Subclasses should call this function to as the writer adds*/
	virtual void createKnobDynamically();
    
    /*This must be implemented to do the output colorspace conversion*/
	virtual void engine(int y,int offset,int range,ChannelMask channels,Row* out)=0;
    
    /*Must implement it to initialize the appropriate colorspace  for
     the file type. You can initialize the _lut member by calling the
     function Lut::getLut(datatype) */
    virtual void initializeColorSpace()=0;
    
    /*This function initialises the output file/output storage structure and put necessary info in it, like
     meta-data, channels, etc...This is called on the main thread so don't do any extra processing here,
     otherwise it would stall the GUI.*/
    virtual void setupFile(std::string filename)=0;
    
    /*This function must fill the pre-allocated structure with the data calculated by engine.
     This function must close the file as writeAllData is the LAST function called before the
     destructor of Write.*/
    virtual void writeAllData()=0;
    
    /*Must return true if this encoder can encode all the channels in channels.
     Otherwise must throw a descriptive exception
     so it can be returned to the user.*/
    virtual void supportsChannelsForWriting(ChannelSet& channels)=0;
    
    /*Returns the reader colorspace*/
    Lut* lut(){return _lut;}
    
    
    void to_byte(Channel z, uchar* to, const float* from, const float* alpha, int W, int delta = 1);
    void to_short(Channel z, U16* to, const float* from, const float* alpha, int W, int bits = 16, int delta = 1);
    void to_float(Channel z, float* to, const float* from, const float* alpha, int W, int delta = 1);
};


typedef Write* (*WriteBuilder)(void*);
/*Classes deriving Write should implement a function named BuildWrite with the following signature:
 static Write* BuildWrite(Writer*);
 */

#endif /* defined(__PowiterOsX__Write__) */
