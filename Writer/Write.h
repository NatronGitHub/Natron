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

 

 





#ifndef __PowiterOsX__Write__
#define __PowiterOsX__Write__

#include <iostream>
#include "Core/channels.h"


class Lut;
class Writer;
class Row;
class Separator_Knob;
class Knob_Callback;
/*This class is used by Writer to load the filetype-specific knobs.
 Due to the lifetime of Write objects, it's not possible to use the
 createKnobDynamically interface there. We must use an extra class.
 Class inheriting Write that wish to have special knobs,must also 
 create a class inheriting WriteKnobs, and implement the virtual
 function createExtraKnobs() to return a valid object.*/
class WriteKnobs {
    
protected:
    Writer* _op;
public:
    WriteKnobs(Writer* op):_op(op){}
    virtual ~WriteKnobs(){}
    
    /*Must initialise all filetype-specific knobs you want
     in this function. You can register them to the callback
     passed in parameters. Call the base class version of initKnobs
     at the end of the function,
     so they are properly added to the settings panel.*/
    virtual void initKnobs(Knob_Callback* callback,std::string& fileType);
    
    
    /*Must be overloaded to tell the callback to 
     effectivly cleanup the knobs from the GUI and
     from memory. This can be done by calling 
     knob->enqueueForDeletion(); Where knob
     is the knob to cleanup.
     This also means you need to store pointers to
     your knobs to clean them up afterwards.*/
    virtual void cleanUpKnobs()=0;
    
    /*Must return true if the content of all knobs is valid
     to start rendering.*/
    virtual bool allValid()=0;

};



class Write {
    
protected:
    bool _premult; // on if the user wants to write alpha pre-multiplied images
    Lut* _lut;
    Writer* op;
    WriteKnobs* _optionalKnobs;
public:
    
    /*Constructors should initialize variables, but shouldn't do any heavy computations, as these objects
     are oftenly re-created. To initialize the input color-space , you can do so by overloading
     initializeColorSpace. This function is called after the constructor and before any
     reading occurs.*/
	Write(Writer* writer);
    
	virtual ~Write();
    
    /*Set the pointer to the WriteKnobs. It will contain
     all the values stored by extra knobs created by the derived WriteKnobs
     class. You can then down-cast it to the per-encoder specific WriteKnobs
     class to retrieve the values.
     If the initSpecificKnobs() function returns NULL (default), then
     the parameter passed here will always be NULL.*/
    void setOptionalKnobsPtr(WriteKnobs* knobs){_optionalKnobs = knobs;}
    
    void premultiplyByAlpha(bool premult){_premult = premult;}
    
    /*calls writeAllData and calls the destructor*/
    void writeAndDelete();
    
    /*Should return the list of file types supported by the encoder: "png","jpg", etc..*/
    virtual std::vector<std::string> fileTypesEncoded()=0;
    
    /*Should return the name of the write handle : "ffmpeg", "OpenEXR" ...*/
    virtual std::string encoderName()=0;
    
    /*Must be implemented to tell whether this file type supports stereovision*/
	virtual bool supports_stereo()=0;
    
    
    /*This must be implemented to do the output colorspace conversion*/
	virtual void engine(int y,int offset,int range,ChannelSet channels,Row* out)=0;
    
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
    
    /*Overload it if you need fileType specific knobs. See the
     comment in WriteKnobs.*/
    virtual WriteKnobs* initSpecificKnobs(){return NULL;}
    
    void to_byte(Powiter::Channel z, uchar* to, const float* from, const float* alpha, int W, int delta = 1);
    void to_short(Powiter::Channel z, U16* to, const float* from, const float* alpha, int W, int bits = 16, int delta = 1);
    void to_float(Powiter::Channel z, float* to, const float* from, const float* alpha, int W, int delta = 1);
};


typedef Write* (*WriteBuilder)(void*);
/*Classes deriving Write should implement a function named BuildWrite with the following signature:
 static Write* BuildWrite(Writer*);
 */


#endif /* defined(__PowiterOsX__Write__) */
