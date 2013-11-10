//  Natron
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
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include "Engine/ChannelSet.h"
#include "Engine/Format.h"
#include "Engine/Lut.h"

class Reader;
class ViewerGL;
class ImageInfo;
namespace Natron {

    class Image;
    class Row;
}

/**
 * @brief A Decoder is a light-weight object used to decode a single file in a sequence of image. For each image an object 
 * is re-created.
 * This is used by the Reader when it needs to decode a particular file type. You can implement this class to make
 * a new decoder for a specific file type.
 **/
class Decoder{
    
public:
    /*Constructors should initialize variables, but shouldn't do any heavy computations, as these objects
     are oftenly re-created. To initialize the input color-space , you can do so by overloading
     initializeColorSpace. This function is called after the constructor and before any
     reading occurs.*/
    Decoder(Reader* _reader);
    
    virtual ~Decoder();
    
    /*Should return the list of file types supported by the decoder: "png","jpg", etc..*/
    virtual std::vector<std::string> fileTypesDecoded() const = 0;
    
    /*Should return the name of the reader : "ffmpeg", "OpenEXR" ...*/
    virtual std::string decoderName() const = 0;
    
    /*must be implemented to decode the data for the given roi, and store it in the output image
     at the given scale.*/
    virtual Natron::Status render(SequenceTime time,RenderScale scale,const RectI& roi,boost::shared_ptr<Natron::Image> output) = 0;
    
    
    Natron::Status _readHeader(const QString& filename){
        _filename = filename;
        return readHeader(filename);
    }
    
    /*can be overloaded to add knobs dynamically to the reader depending on the file type*/
	virtual void createKnobDynamically();
    
    /*Must be implemented to tell whether this file type supports stereovision*/
	virtual bool supports_stereo() const = 0;
    
    
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
    const Natron::Color::Lut* lut(){return _lut;}
    
    /*This function should be call at the end of open(...)
     It set all the reader infos necessary for the read frame.*/
	void setReaderInfo(Format dispW,
                       const RectI& dataW,
                       Natron::ChannelSet channels);
    
    /*Returns all the infos necessary for the current frame*/
    const ImageInfo& readerInfo() const { return *_readerInfo; }
    
    const QString& filename() const {return _filename;}
protected:
    
    
    /*Should open the file and call setReaderInfo with the infos from the file.
     In case of failure, return StatFailed and post an appropriate message, otherwise return StatOK.
     */
    virtual Natron::Status readHeader(const QString& filename) = 0;
    
    void from_byte(Natron::Channel z, float* to, const uchar* from, const uchar* alpha, int W, int delta = 1);
    void from_byteQt(Natron::Channel z, float* to, const QRgb* from, int W, int delta = 1);
    void from_short(Natron::Channel z, float* to, const U16* from, const U16* alpha, int W, int bits, int delta = 1);
    void from_float(Natron::Channel z, float* to, const float* from, const float* alpha, int W, int delta = 1);
    
    
    void from_byte_rect(float* to,const uchar* from,
                        const RectI& rect,const RectI& rod,
                        Natron::Color::Lut::PackedPixelsFormat inputPacking = Natron::Color::Lut::RGBA,bool invertY = false);
 
    void from_short_rect(float* to,const U16* from,
                         const RectI& rect,const RectI& rod,
                         Natron::Color::Lut::PackedPixelsFormat inputPacking = Natron::Color::Lut::RGBA,bool invertY = false);
    void from_float_rect(float* to,const float* from,
                         const RectI& rect,const RectI& rod,
                         Natron::Color::Lut::PackedPixelsFormat inputPacking = Natron::Color::Lut::RGBA,bool invertY = false);
   
    bool _premult; //if the file contains a premultiplied 4 channel image, this must be turned-on
    bool _autoCreateAlpha;
    Reader* _reader;
    const Natron::Color::Lut* _lut;
    boost::scoped_ptr<ImageInfo> _readerInfo;
    QString _filename;
    
};


typedef Decoder* (*ReadBuilder)(void*);
/*Classes deriving Read should implement a function named BuildRead with the following signature:
static Read* BuildRead(Reader*);
 */

#endif // POWITER_READERS_READ_H_
