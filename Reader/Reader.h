//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef READER_H
#define READER_H
#include "Core/inputnode.h"
#include <QtConcurrent/QtConcurrent>
/* Reader is the node associated to all image format readers. The reader creates the appropriate Read*
 to decode a certain image format.
*/
using namespace Powiter_Enums;

class FrameEntry;

/*special ReaderInfo deriving node Infos. This class add just a file name
 to a frame, it is used internally to find frames in the buffer.*/
class ReaderInfo : public Node::Info{
public:
    ReaderInfo():Node::Info(){}
    virtual ~ReaderInfo(){}
    
    void setCurrentFrameName(std::string str){_currentFrameName = str;}
    std::string getCurrentFrameName(){return _currentFrameName;}
    
    /*Returns a string with all infos*/
    std::string printOut();
    
    /*Constructs a new ReaderInfo from string*/
    static ReaderInfo* fromString(QString from);
    
    void operator=(const ReaderInfo& other){
        dynamic_cast<Node::Info&>(*this) = dynamic_cast<const Node::Info&>(other);
        _currentFrameName = other._currentFrameName;
    }
    
    
private:
    std::string _currentFrameName;
};


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
            
            const std::vector<int>& getRowsToRead(){return _rowsToRead;}
            
        private:
            std::map<int,int> _rows; //base rows
            std::vector<int> _rowsToRead; // rows that were added on top of the others and that need to be read
        };
        
        /*This class represents a frame residing in the buffer.*/
        class Descriptor{
        public:
            Descriptor(Read* readHandle,ReaderInfo* readInfo, std::string filename):
            _readHandle(readHandle),_readInfo(readInfo),
            _filename(filename){}
            
            Descriptor():_readHandle(0),_readInfo(0)
            ,_filename(""){}
            
            Descriptor(const Descriptor& other):
           _readHandle(other._readHandle),_readInfo(other._readInfo),
            _filename(other._filename){}
            
            virtual ~Descriptor(){}
            
            /*Returns true if this descriptor has to decode some data*/
            virtual bool hasToDecode()=0;
            
            /*Returns true if this descriptor supports scan-lines*/
            virtual bool supportsScanLines()=0;
            
            /*In case the decoded frame was not issued from the diskcache this is a pointer to the
             Read handle that operated/is operating the decoding.*/
            Read* _readHandle;
            
            /*info read from the Read* */
            ReaderInfo* _readInfo;
            
            /*The name of the frame in the buffer.*/
            std::string _filename;
            
        };
        
        class ScanLineDescriptor : public Reader::Buffer::Descriptor{
        public:
            ScanLineDescriptor(Read* readHandle,ReaderInfo* readInfo,
                       std::string filename,ScanLineContext *slContext):
            Reader::Buffer::Descriptor(readHandle,readInfo,filename),_slContext(slContext){}
            
            ScanLineDescriptor(): Reader::Buffer::Descriptor(),_slContext(0){}
            
            ScanLineDescriptor(const ScanLineDescriptor& other):Reader::Buffer::Descriptor(other),_slContext(other._slContext)
            {}
            
            virtual bool hasToDecode(){ return _slContext->getRowsToRead().size() > 0;}
            
            virtual bool supportsScanLines() {return true;}
            
            ScanLineContext* _slContext;
        };
        
        class FullFrameDescriptor : public Reader::Buffer::Descriptor{
        public:
            FullFrameDescriptor(Read* readHandle,ReaderInfo* readInfo,
                               std::string filename):
            Reader::Buffer::Descriptor(readHandle,readInfo,filename),_hasRead(false){}
            
            FullFrameDescriptor(): Reader::Buffer::Descriptor(),_hasRead(false){}
            
            FullFrameDescriptor(const FullFrameDescriptor& other):Reader::Buffer::Descriptor(other),
            _hasRead(other._hasRead)
            {}
            
            virtual bool hasToDecode(){ return _hasRead;}
            
            virtual bool supportsScanLines() {return false;}
            
            bool _hasRead;
        };
        
        typedef std::vector<Reader::Buffer::Descriptor*>::iterator DecodedFrameIterator;
        typedef std::vector<Reader::Buffer::Descriptor*>::reverse_iterator DecodedFrameReverseIterator;
        
        /*Enum used to know what kind of frame is enqueued in the buffer, it is used by
         isEnqueued(...) and this function is used by open(...) and openCachedFrame(...)*/
        enum SEARCH_TYPE{SCANLINE_FRAME = 1 ,FULL_FRAME = 2 ,ALL_FRAMES = 4};
    
        
        Buffer():_bufferSize(2){}
        ~Buffer(){clear();}
        void insert(Reader::Buffer::Descriptor* desc);
        void remove(std::string filename);
        bool decodeFinished(std::string filename);
        DecodedFrameIterator isEnqueued(std::string filename,SEARCH_TYPE searchMode);
        void clear();
        void setSize(int size){_bufferSize = size;}
        bool isFull(){return _buffer.size() == (U32)_bufferSize;}
        DecodedFrameIterator begin(){return _buffer.begin();}
        DecodedFrameIterator end(){return _buffer.end();}
        void erase(DecodedFrameIterator it);
        DecodedFrameIterator find(std::string filename);
        void debugBuffer();
    private:
        std::vector<  Reader::Buffer::Descriptor*  > _buffer; // decoding buffer
        int _bufferSize; // maximum size of the buffer
    };
    
    Reader(Node* node);
    Reader(Reader& ref);

    void showFilePreview();
    void getVideoSequenceFromFilesList();
	bool hasFrames(){return fileNameList.size()>0;}

    
    bool readCurrentHeader(int current_frame);
    
    void readCurrentData(int current_frame);
    
    int firstFrame();
	int lastFrame();
    
    int clampToRange(int f);
    
    std::string getRandomFrameName(int f);
	bool hasPreview(){return has_preview;}
	void hasPreview(bool b){has_preview=b;}
	void setPreview(QImage* img);
	QImage* getPreview(){return preview;}

    virtual ~Reader();
    virtual std::string className();
    virtual std::string description();

    
    virtual void _validate(bool forReal);
	
	virtual void engine(int y,int offset,int range,ChannelMask channels,Row* out);
    
    virtual bool cacheData(){return true;}
    
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
    bool makeCurrentDecodedFrame(bool forReal);
    
    void fitFrameToViewer(bool b){_fitFrameToViewer = b;}
    
protected:
	virtual void initKnobs(Knob_Callback *cb);
    
    virtual ChannelSet channelsNeeded(int inputNb){(void)inputNb;return Mask_None;}
private:
	QImage *preview;
	bool has_preview;
    QStringList fileNameList;
    bool video_sequence;
    /*useful when using readScanLine in the open(..) function, it determines
     how many scanlines we'd need*/
    bool _fitFrameToViewer;
	Read* readHandle;
    std::map<int,QString> files; // frames
    Buffer _buffer;
};




#endif // READER_H