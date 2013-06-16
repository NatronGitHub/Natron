//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com


#ifndef __PowiterOsX__writeExr__
#define __PowiterOsX__writeExr__

#include <iostream>
#include "Writer/Write.h"
#include <vector>
class ComboBox_Knob;
class Separator_Knob;
class Knob_Callback;
class Row;
class Writer;

/*This class is used by Writer to load the filetype-specific knobs.
 Due to the lifetime of Write objects, it's not possible to use the
 createKnobDynamically interface there. We must use an extra class.
 Class inheriting*/
class ExrWriteKnobs : public WriteKnobs{
    Separator_Knob* sepKnob;
    ComboBox_Knob *compressionCBKnob;
    ComboBox_Knob *depthCBKnob ;
    
public:
    
    std::string _dataType;
    std::string _compression;
    
    ExrWriteKnobs(Writer* op):WriteKnobs(op){}
    virtual ~ExrWriteKnobs(){}
    
    virtual void initKnobs(Knob_Callback* callback,std::string& fileType);
    
    virtual void cleanUpKnobs();
};


class WriteExr :public Write{
    
   
    std::string _filename;
    std::vector<Row*> _img;
    
public:
    
    static Write* BuildWrite(Writer* writer){return new WriteExr(writer);}
    
    WriteExr(Writer* writer);
    virtual ~WriteExr();
    
    virtual WriteKnobs* initSpecificKnobs(){return new ExrWriteKnobs(op);}
    
    /*Should return the name of the write handle : "ffmpeg", "OpenEXR" ...*/
    virtual std::string encoderName(){return "OpenEXR";}
    
    /*Should return the list of file types supported by the encoder: "png","jpg", etc..*/
    virtual std::vector<std::string> fileTypesEncoded();
    
    /*Must be implemented to tell whether this file type supports stereovision*/
	virtual bool supports_stereo(){return true;}
    
    
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
    
    /*Doesn't throw any exception since OpenEXR can write all channels*/
    virtual void supportsChannelsForWriting(ChannelSet& channels){}
    
    
};

#endif /* defined(__PowiterOsX__writeExr__) */
