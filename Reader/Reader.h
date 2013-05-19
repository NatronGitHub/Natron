#ifndef READER_H
#define READER_H
#include "Core/inputnode.h"
#include "Superviser/powiterFn.h"
#include "Gui/knob.h"
#include <QtGui/QImage>

/* Reader is the node associated to all image format readers. When the input file is input from the knob of the settings panel,
 setFileExtension() is called. From here the corresponding reader is created. Once it is created, it is able to know
 how many frames there will be if this is an image sequence, what is the first frame and what is the last one. 
 The file path must contain %d to indicate that this is an image sequence desired, otherwise Powiter will launch only
 one frame as a still image. For example Tree%d.exr will try and read all frames containing Tree in their name in the right order.
 Tree001.exr, however , will launch only this frame.

*/
using namespace Powiter_Enums;
class ViewerGL;
class Read;
class  Reader:public InputNode
{
public:

    Reader(Node* node,ViewerGL* ui_context);
    Reader(Reader& ref);
	virtual void createReadHandle();
	bool isImageSequence();	
	bool hasFrames(){return fileNameList.size()>0;}
	void incrementCurrentFrameIndex();
    void decrementCurrentFrameIndex();
    void seekFirstFrame();
    void seekLastFrame();
    void randomFrame(int f);
	void setup_for_next_frame();
    int currentFrame();
    char* getCurrentFrameName();
    char* getRandomFrameName(int f);
	bool hasPreview(){return has_preview;}
	void hasPreview(bool b){has_preview=b;}
	void setPreview(QImage* img){preview=img; hasPreview(true);}
	QImage* getPreview(){return preview;}

    virtual ~Reader();
    virtual const char* class_name();
    virtual const char* description();

	File_Type fileType(){return filetype;}
	
	virtual void engine(int y,int offset,int range,ChannelMask channels,Row* out);
	virtual void createKnobDynamically();
protected:
	virtual void initKnobs(Knob_Callback *cb);
private:
	QImage *preview;
	bool has_preview;
    QStringList fileNameList;
	bool is_image_sequence;
	Read* readHandle;
	File_Type filetype;
	ViewerGL* ui_context;
};




#endif // READER_H