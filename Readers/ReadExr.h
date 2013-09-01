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

namespace Imf {
    class StdIFStream;
    class InputFile;
}

class Row;
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
    virtual std::vector<std::string> fileTypesDecoded(){
        std::vector<std::string> out;
        out.push_back("exr");
        return out;
    }
    
    /*Should return the name of the reader : "ffmpeg", "OpenEXR" ...*/
    virtual std::string decoderName(){return "OpenEXR";}
    virtual void engine(int y,int offset,int range,ChannelSet channels,Row* out);
    virtual bool supportsScanLine(){return true;}
    virtual int scanLinesCount(){return (int)_img.size();}
    virtual void readHeader(const QString& filename, bool openBothViews);
    virtual void readScanLine(int y);
    virtual bool supports_stereo(){return true;}
    virtual void make_preview();
    virtual void initializeColorSpace();
    void debug();
    
private:
	Imf::InputFile* inputfile;
	std::map<Powiter::Channel, const char*> channel_map;
	std::vector<std::string> views;
	std::string heroview;
	int dataOffset;
    std::map< int, Row* > _img;
#ifdef __POWITER_WIN32__
    // FIXME: this kind of system-dependent members should really be put in a PIMPL
    std::ifstream* inputStr;
    Imf::StdIFStream* inputStdStream;
#endif
	
    
};

#endif // POWITER_READERS_READEXR_H_
