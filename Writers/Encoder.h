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

#ifndef POWITER_WRITERS_WRITE_H_
#define POWITER_WRITERS_WRITE_H_

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "Engine/ChannelSet.h"
#include "Engine/Box.h"
#include "Engine/Lut.h"

class Writer;
namespace Powiter{
    class Image;
    class Row;
}
class Separator_Knob;

/** @class This class is used by Writer to load the filetype-specific knobs.
* Due to the lifetime of Write objects, it's not possible to use the
* createKnobDynamically interface there. We must use an extra class.
* Class inheriting Write that wish to have special knobs,must also
* create a class inheriting WriteKnobs, and implement the virtual
* function createExtraKnobs() to return a valid object.
**/
class EncoderKnobs {
    
protected:
    Writer* _writer; ///pointer to the Writer
public:
    EncoderKnobs(Writer* op):_writer(op){}

    virtual ~EncoderKnobs(){}
    
    /** @brief Must initialise all filetype-specific knobs you want
    * in this function. You can register them to the callback
    * passed in parameters. Call the base class version of initKnobs
    * at the end of the function,
    * so they are properly added to the settings panel.
    **/
    virtual void initKnobs(const std::string& fileType);
    
    
    /** @brief Must be overloaded to tell the callback to
    * effectivly cleanup the knobs from the GUI and
    * from memory. This can be done by calling
    * knob->enqueueForDeletion(); Where knob
    * is the knob to cleanup.
    * This also means you need to store pointers to
    * your knobs to clean them up afterwards.
    **/
    virtual void cleanUpKnobs()=0;
    
    /** @brief Must return true if the content of all knobs is valid
     *to start rendering.
    **/
    virtual bool allValid()=0;

};



/**
 * @brief An Encoder is a light-weight object used to encode a single file in a sequence of image. For each image an object
 * is re-created.
 * This is used by the Writer when it needs to encode a particular file type. You can implement this class to make
 * a new encoder for a specific file type.
 **/
class Encoder {
    
protected:
    bool _premult; /// on if the user wants to write alpha pre-multiplied images
    const Powiter::Color::Lut* _lut; /// the lut used by the Write to apply colorspace conversion
    Writer* _writer; /// a pointer to the Writer
    EncoderKnobs* _optionalKnobs; /// a pointer to the object holding per-type specific Knobs.
    QString _filename;
public:
    
    /** @brief Constructors should initialize variables, but shouldn't do any heavy computations, as these objects
    * are oftenly re-created. To initialize the input color-space , you can do so by overloading
    * initializeColorSpace. This function is called after the constructor and before any
    * reading occurs.
    **/
    Encoder(Writer* writer);
    
    virtual ~Encoder();
    
    const QString& filename() const {return _filename;}
    
    /** @brief Set the pointer to the WriteKnobs. It will contain
     * all the values stored by extra knobs created by the derived WriteKnobs
     * class. You can then down-cast it to the per-encoder specific WriteKnobs
     * class to retrieve the values.
     * If the initSpecificKnobs() function returns NULL (default), then
     * the parameter passed here will always be NULL.*/
    void setOptionalKnobsPtr(EncoderKnobs* knobs){_optionalKnobs = knobs;}
    
    /**
     * @brief premultiplyByAlpha Enables alpha-premultiplication when writing out the frame.
     * @param premult Whether we want to enable alpha premultiplication:
     */
    void premultiplyByAlpha(bool premult){_premult = premult;}
    
    /**
     *@brief Should return the list of file types supported by the encoder: "png","jpg", etc..
    **/
    virtual std::vector<std::string> fileTypesEncoded() const = 0;
    
    /**
     *@brief Should return the name of the write handle : "ffmpeg", "OpenEXR" ...
     * It will serve to identify the encoder afterwards in the software.
    **/
    virtual std::string encoderName() const = 0;
    
    /** @brief Must be implemented to tell whether this file type supports stereovision
    **/
	virtual bool supports_stereo() const = 0;
    
    
    /** @brief This must be implemented to do the output colorspace conversion for the Row passed in parameters
    **/
	virtual void render(boost::shared_ptr<const Powiter::Image> inputImage,const Box2D& roi) = 0;
    
    /*@brief can be overloaded to finalize the file. This is called after the last call to render() was made for the frame*/
    virtual void finalizeFile(){}
    
    /** @brief Must implement it to initialize the appropriate colorspace  for
    * the file type. You can initialize the _lut member by calling the
    * function Powiter::Color::getLut(datatype)
    **/
    virtual void initializeColorSpace() = 0;
    
    Powiter::Status _setupFile(const QString& filename,const Box2D& rod){
        _filename = filename;
        return setupFile(filename,rod);
    }
    
    /** @brief Must return true if this encoder can encode all the channels in channels.
    * Otherwise must throw a descriptive exception
    * so it can be returned to the user.
    **/
    // FIXME: the above doc is obviously wrong.
    virtual void supportsChannelsForWriting(Powiter::ChannelSet& channels) const = 0;
    
    /** @return Returns the reader colorspace*/
    const Powiter::Color::Lut* lut() const {return _lut;}
    
    /** @brief Overload it if you need fileType specific knobs. See the
    * comment in WriteKnobs.
    **/
    virtual EncoderKnobs* initSpecificKnobs() {return NULL;}
    
    
    /**
     * @brief to_byte converts a buffer of float to a buffer of bytes to the output color-space.
     * @param z The channel identifier
     * @param to The output buffer
     * @param from The input buffer
     * @param alpha The optional alpha channel if premultiplyByAlpha() returns true
     * @param W The length of the buffer
     * @param delta How far apart the values in output should be spaced.
     */
    void to_byte(Powiter::Channel z, uchar* to, const float* from, const float* alpha, int W, int delta = 1);
    
    /** @brief Same as to_byte but converts to shorts output buffer.**/
    void to_short(Powiter::Channel z, U16* to, const float* from, const float* alpha, int W, int bits = 16, int delta = 1);
    
    /** @brief Same as to_byte but converts to float output buffer**/
    void to_float(Powiter::Channel z, float* to, const float* from, const float* alpha, int W, int delta = 1);
    
    /**
     * @brief to_byte converts a RGBA packed-buffer of float to a RGBA packed-buffer of bytes to the output color-space.
     */
    void to_byte_rect(uchar* to, const float* from,
                      const Box2D& rect,const Box2D& rod,
                      Powiter::Color::Lut::PackedPixelsFormat outputPacking = Powiter::Color::Lut::RGBA,int invertY = false);

    /** @brief Same as to_byte but converts to shorts output buffer.**/
    void to_short_rect(U16* to, const float* from,
                       const Box2D& rect,const Box2D& rod,
                       Powiter::Color::Lut::PackedPixelsFormat outputPacking = Powiter::Color::Lut::RGBA,int invertY = false);

    /** @brief Same as to_byte but converts to float output buffer**/
    void to_float_rect(float* to, const float* from,
                       const Box2D& rect,const Box2D& rod,
                       Powiter::Color::Lut::PackedPixelsFormat outputPacking = Powiter::Color::Lut::RGBA,int invertY = false);
    
protected:
    
    virtual Powiter::Status setupFile(const QString& filename,const Box2D& rod) = 0;
};

/** @typedef Classes deriving Write should implement a function named BuildWrite with the following signature:
 * static Write* BuildWrite(Writer*);
 **/
typedef Encoder* (*WriteBuilder)(void*);



#endif /* defined(POWITER_WRITERS_WRITE_H_) */
