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

#ifndef POWITER_READERS_READ_H_
#define POWITER_READERS_READ_H_

#include <QtCore/QString>
#include <QtGui/QRgb>
#include "Engine/ChannelSet.h"
#include "Engine/Format.h"
#include "Readers/Reader.h"

class ViewerGL;

namespace Powiter {
namespace Color {
    class Lut;
}
}

class Read{
    
public:
    /*Constructors should initialize variables, but shouldn't do any heavy computations, as these objects
     are oftenly re-created. To initialize the input color-space , you can do so by overloading
     initializeColorSpace. This function is called after the constructor and before any
     reading occurs.*/
	Read(Reader* op);
    
	virtual ~Read();
    
    /*Should return the list of file types supported by the decoder: "png","jpg", etc..*/
    virtual std::vector<std::string> fileTypesDecoded() const = 0;
    
    /*Should return the name of the reader : "ffmpeg", "OpenEXR" ...*/
    virtual std::string decoderName() const = 0;
    
    /*This must be implemented to do the to linear colorspace conversion*/
	virtual void engine(int y,int offset,int range,ChannelSet channels,Row* out) = 0;
    
    /*can be overloaded to add knobs dynamically to the reader depending on the file type*/
	virtual void createKnobDynamically();
    
    /*Must be implemented to tell whether this file type supports stereovision*/
	virtual bool supports_stereo() const = 0;
    
//    /*Must be implemented. This function opens the file specified by filename, reads
//     all the meta-data and extracts all the data. If openBothViews is on, this function
//     should read both views of the file.
//     More precise documentation coming soon.*/
//    virtual void open(const QString filename,bool openBothViews = false)=0;
    
    /*This function  calls readHeader and readAllData*/
    void readData(bool openBothViews = false);
    
    /*This function calls readScanLine for the requested scanLines. It does not call readHeader.
     If onlyExtaRows is true, this function reads only the content of the _rowsToCompute member
     of slContext. Otherwise, it will compute the rows contained in the _rows of slContext.*/
    void readScanLineData(Reader::Buffer::ScanLineContext* slContext);
    
    /*Should open the file and call set_readerInfo with the infos from the file.*/
    virtual void readHeader(const QString& filename, bool openBothViews) = 0;
    
    /*Must be implemented to know whether this Read* supports reading only
     scanlines. In the case it does,the engine can be must faster by reading
     only the lines that are needed.*/
    virtual bool supportsScanLine() const = 0;
    
    /*Must be overloaded by reader that supports scanline. It should return
     the current count of scanlines read by the Read* */
    virtual int scanLinesCount() const { return 0; }
    
    /*Should read the already opened file and extract all the data for either 1 view or
     2 views. By default does nothing, you should either overload this function or readScanLine.*/
    virtual void readAllData(bool) {}
    
    /*Must be implemented if supportsScanLine() returns true. It should
     read one scan line from the file.By default does nothing, you should
     either overload this function or readScanLine*/
    virtual void readScanLine(int) {}
    
    /*Must be overloaded: this function is used to create a little preview that
     can be seen on the GUI node*/
    virtual void make_preview() = 0;
    
    /*Returns true if the file is stereo*/
    bool fileStereo() const {return is_stereo;};
    
    /*Returns true if the file has alpha premultiplied data*/
    bool premultiplied() const {return _premult;}
    
    /*Returns true if the user checked autoAlpha:
     this feature creates automatically an empty alpha channel*/
    bool autoAlpha() const {return _autoCreateAlpha;}
    
    
    /*Must implement it to initialize the appropriate colorspace  for
     the file type. You can initialize the _lut member by calling the
     function getLut(datatype) */
    virtual void initializeColorSpace() = 0;
    
    /*Returns the reader colorspace*/
    const Powiter::Color::Lut* lut(){return _lut;}
    
    /*This function should be call at the end of open(...)
     It set all the reader infos necessary for the read frame.*/
	void set_readerInfo(Format dispW,
                       const Box2D& dataW,
                       const QString& file,
                       ChannelSet channels,
                       int Ydirection ,
                       bool rgb );
    
    /*Returns all the infos necessary for the current frame*/
    const ReaderInfo& readerInfo() const { return _readerInfo; }
    
protected:
    
    void from_byte(Powiter::Channel z, float* to, const uchar* from, const uchar* alpha, int W, int delta = 1);
    void from_byteQt(Powiter::Channel z, float* to, const QRgb* from, int W, int delta = 1);
    void from_short(Powiter::Channel z, float* to, const U16* from, const U16* alpha, int W, int bits, int delta = 1);
    void from_float(Powiter::Channel z, float* to, const float* from, const float* alpha, int W, int delta = 1);
   
        
	bool is_stereo;
    bool _premult; //if the file contains a premultiplied 4 channel image, this must be turned-on
    bool _autoCreateAlpha;
	Reader* op;
    const Powiter::Color::Lut* _lut;
    ReaderInfo _readerInfo;
    
};


typedef Read* (*ReadBuilder)(void*);
/*Classes deriving Read should implement a function named BuildRead with the following signature:
static Read* BuildRead(Reader*);
 */

#endif // POWITER_READERS_READ_H_
