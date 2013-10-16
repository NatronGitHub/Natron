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
#ifdef __POWITER_WIN32__
#include <fstream>
#endif
#include <QtCore/QMutex>
#include "Readers/Read.h"
#include "Global/Macros.h"

namespace Imf {
    class StdIFStream;
    class InputFile;
}

namespace Powiter{
class Row;
}
class ReadExr : public Read{
    
public:
    
    static Read* BuildRead(Reader* reader){return new ReadExr(reader);}
    
	ReadExr(Reader* op);
    
	virtual ~ReadExr();
    
	
    /*Should return the list of file types supported by the decoder: "png","jpg", etc..*/
    virtual std::vector<std::string> fileTypesDecoded() const OVERRIDE {
        std::vector<std::string> out;
        out.push_back("exr");
        return out;
    }
    
    /*Should return the name of the reader : "ffmpeg", "OpenEXR" ...*/
    virtual std::string decoderName() const OVERRIDE {return "OpenEXR";}
    
    virtual void render(SequenceTime time,Powiter::Row* out) OVERRIDE;
    
    virtual Powiter::Status readHeader(const QString& filename) OVERRIDE;
    
    virtual bool supports_stereo() const OVERRIDE {return true;}
        
    virtual void initializeColorSpace() OVERRIDE;
        
    typedef std::map<Powiter::Channel, std::string> ChannelsMap;
    
private:
    
    
    
    Imf::InputFile* _inputfile;
    std::map<Powiter::Channel, std::string> _channel_map;
    std::vector<std::string> views;
    int _dataOffset;
#ifdef __POWITER_WIN32__
    // FIXME: this kind of system-dependent members should really be put in a PIMPL
    std::ifstream* inputStr;
    Imf::StdIFStream* inputStdStream;
#endif
	QMutex _lock;
    
};

#endif // POWITER_READERS_READEXR_H_
