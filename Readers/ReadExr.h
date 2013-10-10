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
#include "Readers/Read.h"
#include "Global/Macros.h"

namespace Imf {
    class StdIFStream;
    class InputFile;
}

namespace Powiter{
class Row;
}
class ReaderInfo;
class ExrChannelExctractor
{
public:
    ExrChannelExctractor(const char* name, const std::vector<std::string>& views) :
    _mappedChannel(Powiter::Channel_black),
    _valid(false)
    {     _valid = extractExrChannelName(name, views);  }
    
    ~ExrChannelExctractor() {}
    
    Powiter::Channel _mappedChannel;
    bool _valid;
    std::string _chan;
    std::string _layer;
    std::string _view;
    std::string exrName() const;
    bool isValid() const {return _valid;}
    
private:
    
    bool extractExrChannelName(const char* channelname,
                               const std::vector<std::string>& views);
};


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
    virtual void engine(Powiter::Row* out) OVERRIDE;
    virtual bool supportsScanLine() const OVERRIDE {return true;}
    virtual int scanLinesCount() const OVERRIDE {return (int)_img.size();}
    virtual void readHeader(const QString& filename, bool openBothViews) OVERRIDE;
    virtual void readScanLine(int y) OVERRIDE;
    virtual bool supports_stereo() const OVERRIDE {return true;}
    virtual void make_preview() OVERRIDE;
    virtual void initializeColorSpace() OVERRIDE;
    void debug();
    
private:
    Imf::InputFile* _inputfile;
    std::map<Powiter::Channel, const char*> _channel_map;
    std::vector<std::string> views;
    int _dataOffset;
    std::map< int, Powiter::Row* > _img;
#ifdef __POWITER_WIN32__
    // FIXME: this kind of system-dependent members should really be put in a PIMPL
    std::ifstream* inputStr;
    Imf::StdIFStream* inputStdStream;
#endif
	
    
};

#endif // POWITER_READERS_READEXR_H_
