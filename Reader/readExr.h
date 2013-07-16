//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//  contact: immarespond at gmail dot com
#ifndef __READ_EXR_H__
#define __READ_EXR_H__
#include "Reader/exrCommons.h"
#include <QtCore/QMutexLocker>
#include <QtCore/QMutex>
#include <fstream>
#include "Superviser/powiterFn.h"
#ifdef __POWITER_WIN32__
#include <ImfStdIO.h>
#endif
#include "Reader/Read.h"

using namespace Powiter_Enums;
class Row;
class ReaderInfo;
class ChannelName
{
public:
	ChannelName(const char* name, const std::vector<std::string>& views) { setname(name, views); }
	~ChannelName() {}
	void setname(const char* name, const std::vector<std::string>& views);
	std::string chan;
	std::string layer;
	std::string view;
	std::string name() const;
};


class ReadExr : public Read{

public:
    
    static Read* BuildRead(Reader* reader){return new ReadExr(reader);}

	ReadExr(Reader* op);
	virtual ~ReadExr();
    
	void lookupChannels(std::set<Channel>& channel, const char* name);
	bool getChannels(const ChannelName& channelName, int view, std::set<Channel>& channel);
	
    /*Should return the list of file types supported by the decoder: "png","jpg", etc..*/
    virtual std::vector<std::string> fileTypesDecoded(){
        std::vector<std::string> out;
        out.push_back("exr");
        return out;
    };
    
    /*Should return the name of the reader : "ffmpeg", "OpenEXR" ...*/
    virtual std::string decoderName(){return "OpenEXR";}
    virtual void engine(int y,int offset,int range,ChannelMask channels,Row* out);
    virtual bool supportsScanLine(){return true;}
    virtual int scanLinesCount(){return (int)_img.size();}
    virtual void readHeader(const QString filename,bool openBothViews);
    virtual void readScanLine(int y);
    virtual bool supports_stereo(){return true;}
    virtual void make_preview();
    virtual void initializeColorSpace();
    void debug();

private:
	Imf::InputFile* inputfile;
	std::map<Channel, const char*> channel_map;
	std::vector<std::string> views;
	std::string heroview;
	int dataOffset;
    std::map< int, Row* > _img;
	bool rgbMode;
#ifdef __POWITER_WIN32__
    std::ifstream *inputStr;
    Imf::StdIFStream* inputStdStream;
#endif
	

};

#endif // __READ_EXR_H__