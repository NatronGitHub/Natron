#ifndef __READ__H__
#define __READ__H__

#include <QtCore/QString>
#include <QtCore/QChar>
#include <map>
#include <QtCore/QStringList>
#include "Core/channels.h"
#include "Core/row.h"
#include "Core/displayFormat.h"
#include "Superviser/PwStrings.h"
class ViewerGL;
class Reader;
class Lut;
class ReaderInfo;
class Read{
    
public:
	Read(Reader* op,ViewerGL* ui_context);
	virtual ~Read();
	virtual void engine(int y,int offset,int range,ChannelMask channels,Row* out)=0;
	virtual void createKnobDynamically();
	virtual bool supports_stereo()=0;
    virtual void open(const QString filename,bool openBothViews = false)=0;
    virtual void make_preview(const char* filename)=0;
    bool fileStereo(){return is_stereo;};	
    bool premultiplied(){return _premult;}
    bool autoAlpha(){return _autoCreateAlpha;}
    Lut* lut(){return _lut;}
	void setReaderInfo(DisplayFormat dispW,
                       IntegerBox dataW,
                       QString file,
                       ChannelMask channels,
                       int Ydirection ,
                       bool rgb );
    ReaderInfo* getReaderInfo(){return _readInfo;}
    
protected:
    
    void from_byte(float* r,float* g,float* b, const uchar* from, bool hasAlpha, int W, int delta = 1,float* a=NULL,bool qtbuf = true);
   
    void from_short(float* r,float* g,float* b, const U16* from, const U16* alpha, int W, int bits, int delta = 1,float* a=NULL);
   
	void from_float(float* r,float* g,float* b, const float* fromR,const float* fromG,
                    const float* fromB, int W, int delta = 1,const float* fromA=NULL,float* a=NULL);
    
        
	bool is_stereo;
    bool _premult; //if the file contains a premultiplied 4 channel image, this must be turned-on
    bool _autoCreateAlpha;
	Reader* op;
    ViewerGL* ui_context;
    Lut* _lut;
	ReaderInfo* _readInfo;
    
};









#endif // __READ__H__