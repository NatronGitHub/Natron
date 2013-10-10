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

#ifndef POWITER_READERS_READER_H_
#define POWITER_READERS_READER_H_
#include <QString>
#include <QStringList>
#include <QtCore/QMutex>
#include <QtGui/QImage> // FIXME: should be pimpled
#include <QtCore/QXmlStreamReader> // forward declaration does not work because of ugly #defines in qxmlstream.h
#include <QtCore/QXmlStreamWriter> // forward declaration does not work because of ugly #defines in qxmlstream.h

#include "Global/Macros.h"
#include "Engine/Node.h"

class FrameEntry;
class File_Knob;

/** @class special ReaderInfo deriving node Infos. This class add just a file name
 *to a frame, it is used internally to find frames in the buffer.
**/
class ReaderInfo : public Node::Info{

public:
    ReaderInfo():Node::Info(){}

    virtual ~ReaderInfo(){}
    

    void setCurrentFrameName(const std::string& str){_currentFrameName = str;}

    std::string getCurrentFrameName(){return _currentFrameName;}
    
    /**
     * @brief Returns a string with all infos
    **/
    void writeToXml(QXmlStreamWriter* writer);
    
    /**
     *@brief Constructs a new ReaderInfo from the xml reader
    **/
    static ReaderInfo* fromXml(QXmlStreamReader* reader);
    
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

/** @class Reader is the node associated to all image format readers. The reader creates the appropriate Read
 *to decode a certain image format.
**/
class Reader : public Node
{
    Q_OBJECT
    
public:
        
    
    /** @enum Internal enum used to determine whether we need to open both views from
     * files that support stereo( e.g : OpenEXR multiview files).
     * @warning This is depecrated and not used anymore.
    **/
    enum DecodeMode{DEFAULT_DECODE,STEREO_DECODE,DO_NOT_DECODE};
    
    /** @class This class manages the buffer of the reader, i.e:
     *a reader can have several frames stored in its buffer.
     *The size of the buffer is variable, see Reader::setMaxFrameBufferSize(...)
    **/
    class Buffer{
    public:
        
        /** @class This class represents the information needed to know how many scanlines were decoded for scan-line
         *readers. This class is used to determine whether 2 decoded scan-line frames
         *are equals and also as an optimization structure to read only needed scan-lines (for
         *zoom/panning purpose).
         *This class has "base" rows, that are describing the ScanLineContext when it was created.
         *Any extra row added to this context is passed into the _rowsToRead map before the merge()
         *function gets called.
        **/
        class ScanLineContext{

        public:

            typedef std::vector<int>::iterator ScanLineIterator;
            typedef std::vector<int>::const_iterator ScanLineConstIterator;
            typedef std::vector<int>::reverse_iterator ScanLineReverseIterator;
            
            ScanLineContext(){}

            /** @brief Construct a new ScanLineContext with the rows passed in parameters.
            **/
            ScanLineContext(const std::vector<int>& rows):_rows(rows) {}
            
            /** @brief Set the base scan-lines that represents the context*
             **/
            void setRows(const std::vector<int>& rows) { _rows=rows; }

            /**
             * @brief getRows
             * @return A non-const reference to the internal rows represented by the ScanLineContext
             */
            std::vector<int>& getRows(){return _rows;}
            
            /**
             * @brief hasScanLines
             * @return True if the internal scan-lines count is greater than 0.
             */
            bool hasScanLines(){return _rows.size() > 0;}
            
            /**
             * @brief  Adds to _rowsToRead the rows in others that are missing to _rows
            **/
            void computeIntersectionAndSetRowsToRead(const std::vector<int>& others);
            
            /**
             *@brief merges _rowsToRead and _rows and clears out _rowsToRead
            **/
            void merge();
            
            /**
             * @brief begin
             * @return Provides a begin iterator to the rows in the ScanLineContext
             */
            ScanLineIterator begin(){return _rows.begin();}

            /**
             * @brief end
             * @return Provides an end iterator to the rows in the ScanLineContext
             */
            ScanLineIterator end(){return _rows.end();}

            /**
             * @brief rbegin
             * @return Provides a reverse-begin iterator to the rows in the ScanLineContext
             */
            ScanLineReverseIterator rbegin(){return _rows.rbegin();}

            /**
             * @brief rend
             * @return Provides a reverse-end iterator to the rows in the ScanLineContext
             */
            ScanLineReverseIterator rend(){return _rows.rend();}
            
            /**
             * @brief getRowsToRead
             * @return A const reference to the rows that were added on top of the original ones.
             */
            const std::vector<int>& getRowsToRead(){return _rowsToRead;}
            
        private:
            std::vector<int> _rows; ///base rows
            std::vector<int> _rowsToRead; /// rows that were added on top of the others and that need to be read
        };
        
        /**
         *@class This is the base class of descriptors that represent a frame living in the buffer.
        **/
        class Descriptor{
        public:
            Descriptor(Read* readHandle_, const std::string& filename)
            : _readHandle(readHandle_)
            , _filename(filename){}
            
            Descriptor()
            : _readHandle(0)
            , _filename("")
            {}
            
            Descriptor(const Descriptor& other)
            : _readHandle(other._readHandle)
            , _filename(other._filename)
            {}
            
            virtual ~Descriptor(){}
            
            /**
             *@return Returns true if this descriptor has to decode some data
            **/
            virtual bool hasToDecode()=0;
            
            /** @return Returns true if this descriptor supports scan-lines
             **/
            virtual bool supportsScanLines()=0;
            
            /*!< In case the decoded frame was not issued from the diskcache this is a pointer to the
             Read handle that operated/is operating the decoding.*/
            Read* _readHandle;
            
            /*!< The name of the frame in the buffer.*/
            std::string _filename;
            
        };
        
        /**
         * @brief The ScanLineDescriptor class is a descriptor for ScanLine-based frames living in the buffer.
         */
        class ScanLineDescriptor : public Reader::Buffer::Descriptor{
        public:
            ScanLineDescriptor(Read* readHandle_,
                       std::string filename,ScanLineContext *slContext):
            Reader::Buffer::Descriptor(readHandle_, filename),_slContext(slContext),_hasRead(false){}
            
            ScanLineDescriptor(): Reader::Buffer::Descriptor(),_slContext(0),_hasRead(false){}
            
            ScanLineDescriptor(const ScanLineDescriptor& other):Reader::Buffer::Descriptor(other),_slContext(other._slContext)
            ,_hasRead(other._hasRead){}
            
            /**
             * @brief hasToDecode
             * @return true if the _rows map is not empty or the _rowsToRead is not empty.
             */
            virtual bool hasToDecode(){ return !_hasRead || _slContext->getRowsToRead().size()>0;}
            
            virtual bool supportsScanLines() {return true;}
            
            ScanLineContext* _slContext;
            bool _hasRead;
        };
        
        class FullFrameDescriptor : public Reader::Buffer::Descriptor{
        public:
            FullFrameDescriptor(Read* readHandle_,
                               std::string filename):
            Reader::Buffer::Descriptor(readHandle_, filename),_hasRead(false){}
            
            FullFrameDescriptor(): Reader::Buffer::Descriptor(),_hasRead(false){}
            
            FullFrameDescriptor(const FullFrameDescriptor& other):Reader::Buffer::Descriptor(other),
            _hasRead(other._hasRead)
            {}
            
            /**
             * @brief hasToDecode
             * @return True if the flag _hasRead is true
             */
            virtual bool hasToDecode(){ return _hasRead;}
            
            virtual bool supportsScanLines() {return false;}
            
            bool _hasRead;
        };
        
        typedef std::vector<Reader::Buffer::Descriptor*>::iterator DecodedFrameIterator;
        typedef std::vector<Reader::Buffer::Descriptor*>::reverse_iterator DecodedFrameReverseIterator;
        
        /**
         *Enum used to know what kind of frame is enqueued in the buffer, it is used by
         *isEnqueued(...).
         **/
        enum SEARCH_TYPE{SCANLINE_FRAME = 1 ,FULL_FRAME = 2 ,ALL_FRAMES = 4};
    
        /**
         * @brief Construct an Empty buffer capable of containing 2 frames.
         */
        Buffer():_bufferSize(2){}

        ~Buffer(){clear();}

        /**
         * @brief Inserts a new frame into the buffer.
         * @param desc  The frame will be identified by this descriptor.
         */
        void insert(Reader::Buffer::Descriptor* desc);

        /**
         * @brief Removes a frame in the buffer
         * @param filename The name of the frame
         */
        void remove(const std::string& filename);

        /**
         * @brief decodeFinished can be used to query whether the Reader has finished to decode a frame.
         * @param filename The name of the frame.
         * @return True if the decoding is finished for that frame.
         */
        bool decodeFinished(const std::string& filename);

        /**
         * @brief isEnqueued is used to check whether a frame exists already into the buffer.
         * @param filename The name of the frame.
         * @param searchMode What kind of frame are we looking for.
         * @return An iterator pointing to a valid descriptor if it found it,otherwise it points to the end of the buffer.
         */
        DecodedFrameIterator isEnqueued(const std::string& filename, SEARCH_TYPE searchMode);

        /**
         * @brief Clears out the buffer.
         */
        void clear();

        /**
         * @brief SetSize will fix the maximum size of the buffer in frames.
         * @param size Size of the buffer in frames.
         */
        void setSize(int size){_bufferSize = size;}

        /**
         * @brief isFull
         * @return True if the buffer is full .
         */
        bool isFull(){return _buffer.size() == (U32)_bufferSize;}

        /**
         * @brief begin
         * @return An iterator pointing to the begin of the buffer.
         */
        DecodedFrameIterator begin(){return _buffer.begin();}

        /**
         * @brief end
         * @return An iterator pointing to the end of the buffer.
         */
        DecodedFrameIterator end(){return _buffer.end();}

        /**
         * @brief Erases a specific frame from the buffer.
         * @param it An iterator pointing to that frame's descriptor.
         */
        void erase(DecodedFrameIterator it);

        /**
         * @brief  find
         * @param filename The name of the frame.
         * @return Returns an iterator pointing to a valid frame in the buffer if it could find it. Otherwise points to end()
         */
        DecodedFrameIterator find(const std::string& filename);

        /**
         * @brief debugBuffer
         */
        void debugBuffer();
    private:
        std::vector<  Reader::Buffer::Descriptor*  > _buffer; /// decoding buffer
        int _bufferSize; /// maximum size of the buffer
    };
    
    Reader(Model* model);

    virtual ~Reader();
    
    /**
     * @brief hasFrames
     * @return True if the files list contains at list a file.
     */
	bool hasFrames() const;

    /**
     * @brief Reads the header of the file associated to the current frame.
     * @param current_frame The index of the frame to read.
     * @return True if it could decode the header.
     */
    bool readCurrentHeader(int current_frame) WARN_UNUSED_RETURN;
    
    /**
     * @brief Reads the data of the file associated to the current frame.
     * @warning readCurrentHeader with the same frame number must have been called beforehand.
     * @param current_frame The index of the frame to decode.
     */
    void readCurrentData(int current_frame);
    
    /**
     * @brief firstFrame
     * @return Returns the index of the first frame in the sequence held by this Reader.
     */
    int firstFrame();

    /**
     * @brief lastFrame
     * @return Returns the index of the last frame in the sequence held by this Reader.
     */
	int lastFrame();
    
    /**
     * @brief nearestFrame
     * @return Returns the index of frame clamped to the Range [ firstFrame() - lastFrame( ].
     * @param f The index of the frame to clamp.
     */
    int nearestFrame(int f);
    
    /**
     * @brief getRandomFrameName
     * @param f The index of the frame.
     * @return The file name associated to the frame index. Returns an empty string if it couldn't find it.
     */
    virtual const QString getRandomFrameName(int /*frame*/) const;

    /**
     * @brief hasPreview
     * @return True if the preview image is valid.
     */
	bool hasPreview() { return _has_preview; }

    /**
     * @brief setPreview will set the preview image for this Reader.
     * @param img The preview image
     */
	void setPreview(const QImage& img);

    /**
     * @brief getPreview
     * @return A pointer to the preview image.
     */
	const QImage& getPreview() { return _preview; }


    /**
     * @brief className
     * @return A string containing "Reader".
     */
    virtual std::string className() const OVERRIDE;

    /**
     * @brief description
     * @return A string containing "InputNode".
     */
    virtual std::string description() const OVERRIDE;

    /**
     * @brief Not documented yet as it will be revisited soon.
     */
    virtual bool _validate(bool doFullWork) OVERRIDE;
	
    /**
     * @brief Calls Read::engine(int,int,int,ChannelSet,Row*)
     */
	virtual void engine(int y,int offset,int range,ChannelSet channels,Row* out) OVERRIDE;
    
    /**
     * @brief cacheData
     * @return true, indicating that the reader caches data.
     */
    virtual bool cacheData() const OVERRIDE {return true;}
    
    /**
     * @brief Calls the base class version.
     */
	virtual void createKnobDynamically() OVERRIDE;
    
    /** @brief Set the number of frames that a single reader can store.
    * Must be minimum 2. The appropriate way to call this
    * function is in the constructor of your Read*.
    * You shouldn't need to call this function. The
    * purpose of this function is to let the reader
    * have more buffering space for certain reader
    * that have a variable throughput (e.g: frameserver).
    **/
    void setMaxFrameBufferSize(int size){
        if(size < 2) return;
        _buffer.setSize(size);
    }
    
    
    void fitFrameToViewer(bool b){_fitFrameToViewer = b;}
    
    virtual int maximumInputs() const OVERRIDE {return 0;}

    virtual int minimumInputs() const OVERRIDE {return 0;}
    
    virtual bool isInputNode() const OVERRIDE {return true;}
    
public slots:
    /**
     * @brief showFilePreview This will effectivly show a preview of the 1st frame in the sequence recently loaded.
     *The preview will appear on the Reader's node GUI.
     */
    void showFilePreview();
    
    void onFrameRangeChanged(int first,int last);

protected:

	virtual void initKnobs() OVERRIDE;
    
    virtual ChannelSet supportedComponents() OVERRIDE { return ChannelSet(Powiter::Mask_All); }
private:
	QImage _preview;
	bool _has_preview;
    /*useful when using readScanLine in the open(..) function, it determines
     how many scanlines we'd need*/
    bool _fitFrameToViewer;
	Read* _readHandle;
    Buffer _buffer;
    File_Knob* _fileKnob;
    QMutex _readMutex;
};




#endif // POWITER_READERS_READER_H_
