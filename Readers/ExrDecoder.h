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

#ifndef POWITER_READERS_READEXR_H_
#define POWITER_READERS_READEXR_H_

#include <string>
#include <vector>
#include <map>

#include <boost/scoped_ptr.hpp>

#include "Readers/Decoder.h"
#include "Global/Macros.h"


namespace Powiter{
class Row;
}

class ExrDecoder : public Decoder {
    
public:
    
    static Decoder* BuildRead(Reader* reader) {return new ExrDecoder(reader);}
    
    ExrDecoder(Reader* _reader);
    
    virtual ~ExrDecoder();
    
	
    /*Should return the list of file types supported by the decoder: "png","jpg", etc..*/
    virtual std::vector<std::string> fileTypesDecoded() const OVERRIDE {
        std::vector<std::string> out;
        out.push_back("exr");
        return out;
    }
    
    /*Should return the name of the reader : "ffmpeg", "OpenEXR" ...*/
    virtual std::string decoderName() const OVERRIDE {return "OpenEXR";}
    
    virtual void render(SequenceTime time,RenderScale scale,const Box2D& roi,boost::shared_ptr<Powiter::Image> output) OVERRIDE;
    
    virtual Powiter::Status readHeader(const QString& filename) OVERRIDE;
    
    virtual bool supports_stereo() const OVERRIDE {return true;}
        
    virtual void initializeColorSpace() OVERRIDE;
        
    typedef std::map<Powiter::Channel, std::string> ChannelsMap;
    
private:
    struct Implementation;
    boost::scoped_ptr<Implementation> _imp; // hide implementation details
};

#endif // POWITER_READERS_READEXR_H_
