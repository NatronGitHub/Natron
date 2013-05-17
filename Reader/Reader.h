#ifndef READER_H
#define READER_H
#include "Core/inputnode.h"
#include "Superviser/powiterFn.h"
#include "Gui/knob.h"
#include <QtGui/QImage>
#include <QtConcurrent/QtConcurrent>
#include "Core/diskcache.h"
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
class Reader : public InputNode
{
    
public:
    enum DecodeMode{DEFAULT_DECODE,STEREO_DECODE};
    
    /*This class manages the buffer of the read, i.e:
     *a reader can have several frames stored in its buffer*/
    class Buffer{
        
        
    public:
        typedef std::pair< QFuture<void>* ,std::pair<ReaderInfo*,Read*> > DecodedFrameDescriptor;
        typedef std::map<std::string,  DecodedFrameDescriptor  >::iterator DecodedFrameIterator;
        typedef std::map<std::string,  DecodedFrameDescriptor >::reverse_iterator DecodedFrameReverseIterator;
        
        Buffer():_bufferSize(2){}
        ~Buffer(){clear();}
        std::pair<Reader::Buffer::DecodedFrameIterator,bool> insert(
                    QString filename,QFuture<void> *future,ReaderInfo* info,Read* readHandle);
        void remove(std::string filename);
        QFuture<void>* getFuture(std::string filename);
        bool decodeFinished(std::string filename);
        DecodedFrameIterator isEnqueued(std::string filename);
        ReaderInfo* getInfos(std::string filename);
        Read* getReadHandle(std::string filename);
        DecodedFrameDescriptor getDescriptor(std::string filename);
        void clear();
        void setSize(int size){_bufferSize = size;}
        bool isFull(){return _buffer.size() == _bufferSize;}
        DecodedFrameIterator begin(){return _buffer.begin();}
        DecodedFrameIterator end(){return _buffer.end();}
    private:
        std::map<std::string,  DecodedFrameDescriptor  > _buffer; // decoding buffer
        int _bufferSize;
    };
    
    Reader(Node* node,ViewerGL* ui_context);
    Reader(Reader& ref);
	virtual void setFileExtension();
    void getVideoSequenceFromFilesList();
	bool hasFrames(){return fileNameList.size()>0;}
	void incrementCurrentFrameIndex();
    void decrementCurrentFrameIndex();
    void seekFirstFrame();
    void seekLastFrame();
    void randomFrame(int f);
    Reader::Buffer::DecodedFrameIterator open(QString filename,DecodeMode mode,bool startNewThread);
    Reader::Buffer::DecodedFrameIterator openCachedFrame(FramesIterator frame);
    
    std::vector<Reader::Buffer::DecodedFrameIterator>
    decodeFrames(DecodeMode mode,bool useCurrentThread,int otherThreadCount,bool forward);
    
    int firstFrame();
	int lastFrame();
    int currentFrame();
	int frame(){return current_frame;}
    std::string getCurrentFrameName();
    std::string getRandomFrameName(int f);
	//void setup_for_next_frame();
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
    
    bool isVideoSequence(){return video_sequence;}

    
    /*Set the number of frames that a single reader can store.
     Must be minimum 2. The appropriate way to call this
     function is in the constructor of your Read*. 
     You shouldn't need to call this function. The
     purpose of this function is to let the reader
     have more buffering space for certain reader
     that have a variable throughput (e.g: frameserver).*/
    void setMaxFrameBufferSize(int size){
        if(size < 2) return;
        _buffer.setSize(size);
    }
    
    /*Returns false if it couldn't find the current frame in the buffer or if
     * the frame is not finished*/
    bool makeCurrentDecodedFrame();
    
protected:
	virtual void initKnobs(Knob_Callback *cb);
private:
	QImage *preview;
	bool has_preview;
    QStringList fileNameList;
    bool video_sequence;
	Read* readHandle;
	File_Type filetype;
	ViewerGL* ui_context;
    
    std::map<int,QString> files; // frames
    int current_frame;
    Buffer _buffer;
};




#endif // READER_H