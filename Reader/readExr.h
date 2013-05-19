//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef __READ_EXR_H__
#define __READ_EXR_H__
#include <ImfPreviewImage.h>
#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <ImfStringAttribute.h>
#include <ImfIntAttribute.h>
#include <ImfFloatAttribute.h>
#include <ImfVecAttribute.h>
#include <ImfBoxAttribute.h>
#include <ImfStringVectorAttribute.h>
#include <ImfTimeCodeAttribute.h>
#include <ImfStandardAttributes.h>
#include <ImfMatrixAttribute.h>
#include <ImfRgba.h>
#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <QtCore/QMutexLocker>
#include <QtCore/QMutex>
#include <fstream>
#include "Superviser/powiterFn.h"
#ifdef __POWITER_WIN32__
#include <ImfStdIO.h>
#endif
#include "Reader/Read.h"

using namespace Powiter_Enums;

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
	

	ReadExr(Reader* op,ViewerGL* ui_context);
	virtual ~ReadExr();
    
	void lookupChannels(std::set<Channel>& channel, const char* name);
	bool getChannels(const ChannelName& channelName, int view, std::set<Channel>& channel);
	

	void normal_engine(const Imath::Box2i& datawin,
		const Imath::Box2i& dispwin,
		ChannelSet&   channels,
		int                 exrY,
		Row*                row,
		int                 x,
		int                 X,
		int                 r,
		int                 R);
    virtual void engine(int y,int offset,int range,ChannelMask channels,Row* out);
    virtual void createKnobDynamically();
    virtual void open(const QString filename,bool openBothViews = false);
    virtual bool supports_stereo(){return true;}
    virtual void make_preview(const char* filename);

private:
	Imf::InputFile* inputfile;
	std::map<Channel, const char*> channel_map;
	std::vector<std::string> views;
	std::string heroview;
	int dataOffset;
    std::vector< std::vector<float*> > _img;
    
   // Imath::V3f yWeights; // for luminance, not used anymore. Left out in comment in case we'd like to introduce it again

	bool rgbMode;
    
#ifdef __POWITER_WIN32__
    std::ifstream *inputStr;
    Imf::StdIFStream* inputStdStream;
#endif
	

};

#endif // __READ_EXR_H__