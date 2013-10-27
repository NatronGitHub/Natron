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

#ifndef POWITER_READERS_READQT_H_
#define POWITER_READERS_READQT_H_

#include <vector>
#include <string>
#include "Readers/Decoder.h"

class QImage;
namespace Powiter{
class Row;
}
class QtDecoder : public Decoder {

    QImage* _img;

public:
    static Decoder* BuildRead(Reader* reader) {return new QtDecoder(reader);}
    
    QtDecoder(Reader* _reader);
    
    virtual ~QtDecoder();
    
    /*Should return the list of file types supported by the decoder: "png","jpg", etc..*/
    virtual std::vector<std::string> fileTypesDecoded() const OVERRIDE;
    
    /*Should return the name of the reader : "ffmpeg", "OpenEXR" ...*/
    virtual std::string decoderName() const OVERRIDE {return "QImage (Qt)";}
    
    virtual void render(SequenceTime time,RenderScale scale,const Box2D& roi,boost::shared_ptr<Powiter::Image> output) OVERRIDE;
    
    virtual bool supports_stereo() const OVERRIDE {return false;}
    
    virtual Powiter::Status readHeader(const QString& filename) OVERRIDE;
        
    virtual void initializeColorSpace() OVERRIDE;
};

#endif /* defined(POWITER_READERS_READQT_H_) */
