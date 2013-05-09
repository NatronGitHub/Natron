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
	Read(const QStringList& file_list,Reader* op,ViewerGL* ui_context);
	virtual ~Read();
	virtual void engine(int y,int offset,int range,ChannelMask channels,Row* out)=0;
	virtual void createKnobDynamically();
	virtual bool isVideoSequence(){return video_sequence;}
	virtual bool supports_stereo()=0;
    virtual void open()=0;
    void setup_for_next_frame(bool makePreview = false);
    virtual void make_preview()=0;
	virtual bool fileStereo(){return is_stereo;};
	const QStringList& fileNameList(){return file_name_list;};
	int firstFrame();
	int lastFrame();
	int frame(){return current_frame;}
    char* getCurrentFrameName(){
        QString str = files[current_frame];
        char* copy=QstringCpy(str);
        return copy;
    }
    char* getRandomFrameName(int f){
        QString str = files[f];
        char* copy=QstringCpy(str);
        return copy;
    }
	void incrementCurrentFrameIndex(){ current_frame<lastFrame() ? current_frame++ : current_frame;}
    void decrementCurrentFrameIndex(){current_frame>firstFrame() ? current_frame-- : current_frame;}
    void seekFirstFrame(){current_frame=firstFrame();}
    void seekLastFrame(){current_frame=lastFrame();}
    void randomFrame(int f){if(f>=firstFrame() && f<=lastFrame()) current_frame=f;}
	DisplayFormat& format();
    bool premultiplied(){return _premult;}
    bool autoAlpha(){return _autoCreateAlpha;}
    Lut* lut(){return _lut;}
	void setReaderInfo(DisplayFormat dispW,
                       IntegerBox dataW,
                       QString currentFrameName,
                       ChannelMask channels= Mask_RGB,
                       int Ydirection = -1,
                       bool rgb = true,
                       int currentFrame = 0,
                       int firstFrame=0,
                       int lastFrame=0);
    
protected:
    /*
     Convert bytes to floating point.
     
     from should point at a set of  W bytes, spaced delta apart.
     These are converted and placed into  to (1 apart).
     
     z is the channel number. If  z >= 3 then linear (divide by 255)
     conversion is done.
     
     Otherwise the lut() is called to do a normal conversion.
     
     If premult() is on and  alpha is not null, it should point at an
     array of  W bytes for an alpha channel, spaced  delta
     apart. The lut() is then called to do an unpremult-convert of the
     values.
     */
    //void from_byte(Channel z, float* to, const uchar* from, const uchar* alpha, int W, int delta = 1);
    void from_byte(float* r,float* g,float* b, const uchar* from, bool hasAlpha, int W, int delta = 1,float* a=NULL,bool qtbuf = true);
    /*
     Same as from_byte() but the source data is an array of shorts in the
     range 0 to (2<<bits)-1.
     */
    //void from_short(Channel z, float* to, const U16* from, const U16* alpha, int W, int bits, int delta = 1);
    void from_short(float* r,float* g,float* b, const U16* from, const U16* alpha, int W, int bits, int delta = 1,float* a=NULL);
    /*
     Same as from_byte() but the source is floating point data.
     Linear conversion will leave the numbers unchanged.
     */
    //void from_float(Channel z, float* to, const float* from, const float* alpha, int W, int delta = 1);
	void from_float(float* r,float* g,float* b, const float* fromR,const float* fromG,
                    const float* fromB, int W, int delta = 1,const float* fromA=NULL,float* a=NULL);
    
    
    void convertSubSampledBufferToFullSize(float* buf,int samplingFactor,int offset,int range);
    
	void determine_if_part_of_a_sequence();
	bool is_stereo;
	bool video_sequence;
    bool _premult; //if the file contains a premultiplied 4 channel image, this must be turned-on
    bool _autoCreateAlpha;
	const QStringList file_name_list;
	std::map<int,QString> files;
	int current_frame;
	DisplayFormat display_format;
	Reader* op;
    ViewerGL* ui_context;
    Lut* _lut;
	ReaderInfo* _readInfo;
    
};









#endif // __READ__H__