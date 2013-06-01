//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef READER_H
#define READER_H
#include "Core/inputnode.h"
#include "Superviser/powiterFn.h"
#include "Gui/knob.h"
#include <QtGui/QImage>
#include <QtConcurrent/QtConcurrent>
#include "Core/viewercache.h"
/* Reader is the node associated to all image format readers. The reader creates the appropriate Read*
 to decode a certain image format.
*/
using namespace Powiter_Enums;
class ViewerGL;
class Read;
class ViewerCache;
class Reader : public InputNode
{
    
public:
    /*Internal enum used to determine whether we need to open both views from
     * files that support stereo( e.g : OpenEXR multiview files)*/
    enum DecodeMode{DEFAULT_DECODE,STEREO_DECODE,DO_NOT_DECODE};
    
    /*This class manages the buffer of the reader, i.e:
     *a reader can have several frames stored in its buffer.
     The size of the buffer is variable, see Reader::setMaxFrameBufferSize(...)*/
    class Buffer{
    public:
        
        /*This class represent the information needed to know how many scanlines were decoded for scan-line
         readers. This class is used to determine whether 2 decoded scan-line frames
         are equals and also as an optimization structure to read only needed scan-lines (for
         zoom/panning purpose)*/
        class ScanLineContext{
        public:
            typedef std::map<int,int>::iterator ScanLineIterator;
            typedef std::map<int,int>::reverse_iterator ScanLineReverseIterator;
            
            ScanLineContext(){}
            ScanLineContext(std::map<int,int> rows):_rows(rows){}
            
            /*set the base scan-lines that represents the context*/
            void setRows(std::map<int,int> rows){_rows=rows;}
            std::map<int,int>& getRows(){return _rows;}
            
            bool hasScanLines(){return _rows.size() > 0;}
            
            /*Adds to _rowsToRead the rows in others that are missing to _rows*/
            void computeIntersectionAndSetRowsToRead(std::map<int,int>& others);
            
            /*merges _rowsToRead and _rows and clears out _rowsToRead*/
            void merge();
            
            ScanLineIterator begin(){return _rows.begin();}
            ScanLineIterator end(){return _rows.end();}
            ScanLineReverseIterator rbegin(){return _rows.rbegin();}
            ScanLineReverseIterator rend(){return _rows.rend();}
            
            std::vector<int>& getRowsToRead(){return _rowsToRead;}
            
        private:
            std::map<int,int> _rows; //base rows
            std::vector<int> _rowsToRead; // rows that were added on top of the others and that need to be read
        };
        
        /*This class represents a frame residing in the buffer.*/
        class DecodedFrameDescriptor{
        public:
            DecodedFrameDescriptor(QFuture<void>* asynchTask,Read* readHandle,
                                   ReaderInfo* frameInfo,const char* cachedFrame,
                                   QFutureWatcher<const char*>* cacheWatcher,std::string filename,
                                   ScanLineContext *slContext = 0):
            _asynchTask(asynchTask),_readHandle(readHandle),_frameInfo(frameInfo),_cachedFrame(cachedFrame),
            _cacheWatcher(cacheWatcher),_filename(filename),_slContext(slContext)
            {}
            DecodedFrameDescriptor():_asynchTask(0),_readHandle(0),_frameInfo(0),_cachedFrame(0),_cacheWatcher(0)
            ,_filename(""),_slContext(0){}
            DecodedFrameDescriptor(const DecodedFrameDescriptor& other):
            _asynchTask(other._asynchTask),_readHandle(other._readHandle),_frameInfo(other._frameInfo),
            _cachedFrame(other._cachedFrame),_cacheWatcher(other._cacheWatcher),_filename(other._filename),
            _slContext(other._slContext)
            {}
            /*This member is not 0 if the decoding for the frame was done in an asynchronous manner
             It is used to query whether it has finish or not.*/
            QFuture<void>* _asynchTask;
            
            /*In case the decoded frame was not issued from the diskcache this is a pointer to the
             Read handle that operated/is operating the decoding.*/
            Read* _readHandle;
            
            /*The info of the frame (i.e: dataWindow,displayWindow,channels, etc...).Maybe 0
             if the decoding is asynchronous and the task is not finished yet.*/
            ReaderInfo* _frameInfo;
            
            /*A pointer to the frame data if it comes from the diskCache. Otherwise 0.*/
            const char* _cachedFrame;
            
            /*This member is not 0 if the frame comes from the diskCache AND it was
             *retrieved by another thread.*/
            QFutureWatcher<const char*>* _cacheWatcher;
            
            /*The name of the frame in the buffer.*/
            std::string _filename;
            
            ScanLineContext* _slContext;
        };
        
        typedef std::vector<DecodedFrameDescriptor>::iterator DecodedFrameIterator;
        typedef std::vector<DecodedFrameDescriptor>::reverse_iterator DecodedFrameReverseIterator;        
        
        /*Enum used to know what kind of frame is enqueued in the buffer, it is used by
         isEnqueued(...) and this function is used by open(...) and openCachedFrame(...)*/
        enum SEARCH_TYPE{CACHED_FRAME = 1,SCANLINE_FRAME = 2 ,FULL_FRAME = 4 , NOT_CACHED_FRAME = 8,ALL_FRAMES = 16};
    
        
        Buffer():_bufferSize(2){}
        ~Buffer(){clear();}
        Reader::Buffer::DecodedFrameDescriptor insert(QString filename,
                                                      QFuture<void> *future,
                                                      ReaderInfo* info,
                                                      Read* readHandle,
                                                      const char* cachedFrame,
                                                      ScanLineContext *slContext = 0,
                                                      QFutureWatcher<const char*>* cacheWatcher=NULL);
        void remove(std::string filename);
        QFuture<void>* getFuture(std::string filename);
        bool decodeFinished(std::string filename);
        DecodedFrameIterator isEnqueued(std::string filename,SEARCH_TYPE searchMode);
        void clear();
        void setSize(int size){_bufferSize = size;}
        bool isFull(){return _buffer.size() == _bufferSize;}
        DecodedFrameIterator begin(){return _buffer.begin();}
        DecodedFrameIterator end(){return _buffer.end();}
        void erase(DecodedFrameIterator it);
        DecodedFrameIterator find(std::string filename);
        void debugBuffer();
        void removeAllCachedFrames();
    private:
        std::vector<  DecodedFrameDescriptor  > _buffer; // decoding buffer
        int _bufferSize; // maximum size of the buffer
    };
    
    Reader(Node* node,ViewerGL* ui_context,ViewerCache* cache);
    Reader(Reader& ref);

    void showFilePreview();
    void getVideoSequenceFromFilesList();
	bool hasFrames(){return fileNameList.size()>0;}
	void incrementCurrentFrameIndex();
    void decrementCurrentFrameIndex();
    void seekFirstFrame();
    void seekLastFrame();
    void randomFrame(int f);
    
    /*Chooses the appropriate Read* to open the file named by filename.
     *If mode is stereo, it will try to open frames as stereo if the Read* supports stereo.
     *If startNewThread is on, it will actually open the file in a separate thread.
     *If the frame is already opened in the reader's internal buffer, this function returns instantly.
     *Otherwise it inserts a new DecodedFrameResult to the internal buffer.
     *Note that this function is not called if the frame is already in the diskcache. See openCachedFrame(...)
     **/
    Reader::Buffer::DecodedFrameDescriptor open(QString filename,DecodeMode mode,bool startNewThread);
    
    /*Open from the diskcache the frame represented by the FramesIterator.
     *FrameNB is the index of the frame, it is used to update the cache on the timeline.
     * This function allocates OpenGL the pbo, map it and fill it with the cached frame.
     *It doesn't unmap the PBO as it is done in GLViewer::copyPBOtoTexture(...).
     *It will insert a new DecodedFrameResult in the internal buffer. The buffer offer a way
     *to know it is a frame taken from the diskcache, either with _cacheFrame!=NULL or
     * _cacheWatcher!=NULL.
     *if startNewThread is on,the retrieval from the cache and the copy to the PBO will occur in a separate thread.
     **/
    Reader::Buffer::DecodedFrameDescriptor openCachedFrame(ViewerCache::FramesIterator frame ,int frameNb,bool startNewThread);
    
    /*This function is used internally by openCachedFrame(...) in case startNewThread is true.
     *It opens the frame identified by the FramesIterator in the diskcache and copies it to the
     * dst buffer(OpenGL mapped PBO).*/
    const char* retrieveCachedFrame(ViewerCache::FramesIterator frame,int frameNb,void* dst,size_t dataSize);
    
    /*This function will decode one or more frames depending on its parameters.
     *It will not be called for a cached frame.
     * TODO : should call only this function in VideoEngine::startReading(...) instead of having
     * a separate call for openCachedFrame(...) and decodeFrames(...)*/
    std::vector<Reader::Buffer::DecodedFrameDescriptor>
    decodeFrames(DecodeMode mode,bool useCurrentThread,bool useOtherThread,bool forward);
    
    int firstFrame();
	int lastFrame();
    int currentFrame();
	int frame(){return current_frame;}
    std::string getCurrentFrameName();
    std::string getRandomFrameName(int f);
	bool hasPreview(){return has_preview;}
	void hasPreview(bool b){has_preview=b;}
	void setPreview(QImage* img);
	QImage* getPreview(){return preview;}

    virtual ~Reader();
    virtual const char* className();
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
    
    void removeCurrentFrameFromBuffer();
    
    void removeCachedFramesFromBuffer();
    
    void fitFrameToViewer(bool b){_fitFrameToViewer = b;}
    
protected:
	virtual void initKnobs(Knob_Callback *cb);
private:
	QImage *preview;
	bool has_preview;
    QStringList fileNameList;
    bool video_sequence;
    /*useful when using readScanLine in the open(..) function, it determines
     how many scanlines we'd need*/
    bool _fitFrameToViewer;
	Read* readHandle;
	File_Type filetype;
	ViewerGL* ui_context;
    ViewerCache* _cache;
    int _pboIndex;
    std::map<int,QString> files; // frames
    int current_frame;
    Buffer _buffer;
};




#endif // READER_H